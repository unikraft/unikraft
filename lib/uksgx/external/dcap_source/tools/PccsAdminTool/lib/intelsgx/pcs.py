import urllib
import requests
import json
import binascii
from urllib import parse
from OpenSSL import crypto
from pypac import PACSession
from platform import system
from lib.intelsgx.credential import Credentials
from requests.adapters import HTTPAdapter
from urllib3.util import Retry

certBegin= '-----BEGIN CERTIFICATE-----'
certEnd= '-----END CERTIFICATE-----'
certEndOffset= len(certEnd)

class PCS:
    BaseUrl= ''
    ApiVersion= 3
    Key= ''
    HttpError= 200
    Errors= []
    Verbose= True

    # HTTP header name
    HDR_TCB_INFO_ISSUER_CHAIN = 'SGX-TCB-Info-Issuer-Chain'
    HDR_PCK_Certificate_Issuer_Chain = 'SGX-PCK-Certificate-Issuer-Chain'
    HDR_Enclave_Identity_Issuer_Chain = 'SGX-Enclave-Identity-Issuer-Chain'

    def __init__(self, url, apiVersion,key):
        self.BaseUrl = url
        self.ApiVersion = apiVersion
        self.Key= key
        if apiVersion >= 4:
            PCS.HDR_TCB_INFO_ISSUER_CHAIN = 'TCB-Info-Issuer-Chain'

    def api_version(self):
        return self.ApiVersion

    def _geturl(self, func, type='sgx'):
        if type == 'sgx':
            return urllib.parse.urljoin(self.BaseUrl, func)
        elif type == 'tdx':
            return urllib.parse.urljoin(self.BaseUrl.replace('/sgx/', '/tdx/'), func)
        else:
            raise Exception('Internal error!')

    def _get_request(self, url, needKey):
        if self.Verbose:
            print(url)

        headers = {}
        if needKey :
            headers['Ocp-Apim-Subscription-Key'] = self.Key
        
        PARAMS = {}
        https = requests.Session()
        https.mount("https://", HTTPAdapter(max_retries=Retry(method_whitelist=["HEAD", "GET", "PUT", "POST", "DELETE", "OPTIONS", "TRACE"])))
        r = https.get(url = url, headers=headers, params = PARAMS, verify=True)

        return r

    def _post_request(self, url, data, needKey):
        if self.Verbose:
            print(url)

        headers = {'Content-type': 'application/json', 'Accept': 'text/plain'}
        if needKey :
            headers['Ocp-Apim-Subscription-Key'] = self.Key
        
        PARAMS = {}
        https = requests.Session()
        https.mount("https://", HTTPAdapter(max_retries=Retry(method_whitelist=["HEAD", "GET", "PUT", "POST", "DELETE", "OPTIONS", "TRACE"])))
        r = https.post(url = url, headers=headers, params = PARAMS, data=json.dumps(data), verify=True)

        return r

#----------------------------------------------------------------------------
# Certificate verification
#----------------------------------------------------------------------------

    def init_cert_store(self, pychain):
        store= crypto.X509Store()
        
        for tcert in pychain:
            store.add_cert(tcert)

        return store

    def verify_crl_trust(self, pychain_in, pycrl):
        # Copy our list so we don't modify the original
        pychain= pychain_in[:]

        # PyOpenSSL doesn't have methods for verifying a CRL issuer,
        # so we need to translate from it to cryptography.

        crl= pycrl.to_cryptography()

        # The chain_pem is our CRL issuer and the CA for the issuer.
        # Verify that first.

        pycert= pychain.pop()
        if not self.verify_cert_trust(pychain, [pycert]):
            self.error("Could not verify CRL signing certificate")
            return False

        # Now verify the CRL signature

        signer_key= pycert.get_pubkey().to_cryptography_key()

        if not crl.is_signature_valid(signer_key):
            self.error("Could not verify CRL signature")
            return False

        # Check the crl issuer

        if pycrl.get_issuer() != pycert.get_subject():
            self.error("CRL issuer doesn't match issuer chain")
            return False

        return True

    def verify_cert_trust(self, pychain, pycerts):
        store= self.init_cert_store(pychain)

        for pycert in pycerts:
            store_ctx= crypto.X509StoreContext(store, pycert)
            try:
                store_ctx.verify_certificate()
            except crypto.X509StoreContextError:
                return False

        return True

    def verify_signature(self, pycert, signature, msg):
        # OpenSSL expects the signature to be ASN1-encoded.

        # r and s must be positive ints, so if the high bit is set in 
        # the first byte, prepend 0x00 to indicate an unsigned value.

        r= signature[0:32]
        if r[0] & 0x80:
            r= bytes([0])+r

        s= signature[32:]
        if s[0] & 0x80:
            s= bytes([0])+s

        # ASN1 encode as a sequnce of two integers:
        # 0x30 || len(following) || 0x02 || len(r) || r || 0x02 || len(s) || s

        sig= bytes([0x30,len(r)+len(s)+4,2,len(r)]) + r + bytes([2,len(s)]) + s

        try:
            crypto.verify(pycert, sig, msg, "sha256")
        except crypto.Error as e:
            self.error('Signature verification failed: {:s}'.format(str(e)))
            return False

        return True

    def pem_to_pycert(self, cert_pem):
        return crypto.load_certificate(crypto.FILETYPE_PEM, cert_pem)

    def pems_to_pycerts(self, certs_pem):
        pycerts= []
        for cert_pem in certs_pem:
            pycerts.append(
                crypto.load_certificate(crypto.FILETYPE_PEM, cert_pem)
            )
        return pycerts

    def parse_chain_pem(self, chain_pem):
        certs_pem= []
        l= len(chain_pem)
        spos= chain_pem.find(certBegin)
        while spos > -1:
            epos= chain_pem.find(certEnd, spos)
            if epos == -1:
                return None
            epos+= certEndOffset+1
            certs_pem.append( chain_pem[spos:epos] )
            if epos >= l:
                break
            spos= chain_pem.find(certBegin, epos)

        return certs_pem

    # Sort certificate chain
    #----------------------------------------

    def sort_pycert_chain(self, chain_in):
        lchain= len(chain_in)

        # Special cases

        if lchain == 0 or lchain == 1:
            return chain_in
        elif lchain == 2:
            cert0= chain_in[0]
            cert1= chain_in[1]

            if cert0.get_subject() == cert1.get_issuer():
                return chain_in
            elif cert1.get_subject() == cert0.get_issuer():
                chain_in.reverse()
                return chain_in
            else:
                self.error('multiple certs with no issuer in chain')
                return None

        # First, are they already sorted?

        sorted= True
        for i in range(1, len(chain_in)):
            cert= chain_in[i]
            pcert= chain_in[i-1]
            if cert.get_issuer() != pcert.get_subject():
                sorted= False
                break

        if sorted:
            return chain_in

        chain= chain_in[:]
        sorted_chain= []

        # Look for any self-signed certs. There should be 0 or 1.

        cert_subjects= {}
        rootidx= -1
        for i in range(0, len(chain)):
            cert= chain[i]
            subject= cert.get_subject()
            issuer= cert.get_issuer()
            cert_subjects[subject.CN]= cert
            print("cert: {:s} <- {:s}" . format(subject.CN, issuer.CN))

            if subject == issuer:
                if len(sorted_chain) > 0:
                    self.error('multiple root certificates in chain')
                    return None

                sorted_chain.append(cert)
                rootidx= i

        if rootidx > -1:
            chain.pop(rootidx)

        # Map issuers to subjects

        issued_by= {}
        issuer_to= {}

        for cert in chain:
            issuer= cert.get_issuer().CN
            subject= cert.get_subject().CN

            if issuer in issued_by:
                self.error('multiple certs issued by same cert in chain')
                return None

            if issuer in issuer_to:
                self.error('multiple certs issued by same cert in chain')
                return None

            issued_by[issuer]= subject
            issuer_to[subject]= issuer

        # Find the top of the chain (if necessary)

        if len(sorted_chain) > 0:
            for cert in chain:
                issuer= cert.get_issuer().CN
                if issuer not in issued_by:
                    if len(sorted_chain) > 0:
                        self.error('multiple certs with no issuer')
                        return None
                    sorted_chain.append(cert)

        if not len(sorted_chain):
            self.error('circular issuer chain')
            return None

        # Build the sorted chain

        cert= sorted_chain[0]

        while len(sorted_chain) < lchain:
            issuer_subject= cert.get_subject().der()

            if issuer_subject not in issuer_to:
                self.error('cert in chain with no issuer')
                return None

            child_subject= issuer_to[issuer_subject]
            ncert= cert_subjects[child_subject]
            cert= ncert

        return sorted_chain

#----------------------------------------------------------------------------
# Error messages
#----------------------------------------------------------------------------

    def clear_errors(self):
        self.Errors= []

    def error(self, msg):
        self.Errors.append(msg)

    def perror(self, prefix):
        for msg in self.Errors:
            print("{:s}: {:s}".format(prefix, msg))

    def strerror(self):
        return "\n".join(self.Errors)

#----------------------------------------------------------------------------
# PCS: Get PCK Certificate(s)
#----------------------------------------------------------------------------

    def get_pck_cert(self, eppid, pceid, cpusvn, pcesvn, dec=None):
        self.clear_errors()
        url= self._geturl('pckcert')
        url+= "?encrypted_ppid={:s}&pceid={:s}&cpusvn={:s}&pcesvn={:s}".format(
            eppid, pceid, cpusvn, pcesvn)

        response= self._get_request(url, True)
        if response.status_code != 200:
            print(str(response.content, 'utf-8'))
            if response.status_code == 401:
                Credentials().set_pcs_api_key('')   #reset ApiKey
            return None

        # Verify expected headers

        if not response.headers['Request-ID']:
            self.error("Response missing Request-ID header")
            return None
        if not response.headers[PCS.HDR_PCK_Certificate_Issuer_Chain]:
            self.error("Response missing SGX-PCK-Certificate-Issuer-Chain header")
            return None

        if response.headers['Content-Type'] != 'application/x-pem-file':
            self.error("Content-Type should be application/x-pem-file")
            return None

        # Validate certificate with signer

        chain= parse.unquote(
            response.headers[PCS.HDR_PCK_Certificate_Issuer_Chain]
        )

        chain_pems= self.parse_chain_pem(chain)
        pychain= self.pems_to_pycerts(chain_pems)
        pychain= self.sort_pycert_chain(pychain)
        if pychain is None:
            return None

        cert_pem= response.content
        pycert= self.pem_to_pycert(cert_pem)

        if not self.verify_cert_trust(pychain, [pycert]):
            self.error("Could not validate certificate using trust chain")
            return None

        if dec is not None:
            cert_pem = str(cert_pem, dec)

        return [cert_pem, response.getheader(PCS.HDR_PCK_Certificate_Issuer_Chain)]

    def get_cpusvn_from_tcb(self, tcb):
        return ("%0.2X" % tcb['sgxtcbcomp01svn'] + 
            "%0.2X" % tcb['sgxtcbcomp02svn'] + 
            "%0.2X" % tcb['sgxtcbcomp03svn'] + 
            "%0.2X" % tcb['sgxtcbcomp04svn'] + 
            "%0.2X" % tcb['sgxtcbcomp05svn'] + 
            "%0.2X" % tcb['sgxtcbcomp06svn'] + 
            "%0.2X" % tcb['sgxtcbcomp07svn'] + 
            "%0.2X" % tcb['sgxtcbcomp08svn'] + 
            "%0.2X" % tcb['sgxtcbcomp09svn'] + 
            "%0.2X" % tcb['sgxtcbcomp10svn'] + 
            "%0.2X" % tcb['sgxtcbcomp11svn'] + 
            "%0.2X" % tcb['sgxtcbcomp12svn'] + 
            "%0.2X" % tcb['sgxtcbcomp13svn'] + 
            "%0.2X" % tcb['sgxtcbcomp14svn'] + 
            "%0.2X" % tcb['sgxtcbcomp15svn'] + 
            "%0.2X" % tcb['sgxtcbcomp16svn'])

    def get_pck_certs(self, eppid, pceid, platform_manifest, dec=None):
        self.clear_errors()
        certs_pem= []
        url= self._geturl('pckcerts')

        if self.ApiVersion >= 3 and len(platform_manifest) > 0 :
            data = {}
            data["pceid"] = pceid
            data["platformManifest"] = platform_manifest
            response= self._post_request(url, data, True)
        else:
            url+= "?encrypted_ppid={:s}&pceid={:s}".format(eppid, pceid)
            response= self._get_request(url, True)

        if response.status_code != 200:
            print(str(response.content, 'utf-8'))
            if response.status_code == 401:
                Credentials().set_pcs_api_key('')   #reset ApiKey
            return None

        # Verify expected headers
        if not response.headers['Request-ID']:
            self.error("Response missing Request-ID header")
            return None
        if not response.headers[PCS.HDR_PCK_Certificate_Issuer_Chain]:
            self.error("Response missing SGX-PCK-Certificate-Issuer-Chain header")
            return None

        if response.headers['Content-Type'] != 'application/json':
            self.error("Content-Type should be application/json")
            return None
    
        # Validate the certificates with signer

        chain= parse.unquote(
            response.headers[PCS.HDR_PCK_Certificate_Issuer_Chain]
        )
        chain_pems= self.parse_chain_pem(chain)
        pychain= self.pems_to_pycerts(chain_pems)
        pychain= self.sort_pycert_chain(pychain)
        if pychain is None:
            return None

        certs_available = []
        certs_not_available = []
        data= response.content
        tcbcerts = json.loads(data)
        tcbcerts_na = list(filter(lambda x: x["cert"] == 'Not available', tcbcerts))
        tcbcerts_valid = list(filter(lambda x: x["cert"] != 'Not available', tcbcerts))
        for tcbcert in tcbcerts_na:
            certs_not_available.append({
                "enc_ppid":eppid,
                "platform_manifest":platform_manifest,
                "qe_id":"",
                "pce_id":pceid,
                "cpu_svn":self.get_cpusvn_from_tcb(tcbcert['tcb']),
                "pce_svn":str(tcbcert['tcb']['pcesvn'])
            })

        for tcbcert in tcbcerts_valid:
            certs_available.append(tcbcert)
            cert_pem= parse.unquote(tcbcert['cert'])
            certs_pem.append(cert_pem)

        pycerts= self.pems_to_pycerts(certs_pem)
        if not self.verify_cert_trust(pychain, pycerts):
            self.error("Could not validate certificate using trust chain")
            return None

        return [certs_available, certs_not_available, response.headers[PCS.HDR_PCK_Certificate_Issuer_Chain]]


#----------------------------------------------------------------------------
# PCS: Get Revocation List
#----------------------------------------------------------------------------

    def get_pck_crl(self, target, dec=None):
        self.clear_errors()
        if ( target != 'processor' and target != 'platform' ):
            self.error('Invalid argument')
            return None

        url= self._geturl('pckcrl')
        if self.ApiVersion<3:
            url+= "?ca={:s}".format(target)
        else:
            url+= "?ca={:s}&encoding=der".format(target)

        response= self._get_request(url, False)
        if response.status_code != 200:
            self.error(response.status_code)
            return None

        # Verify expected headers
        if not response.headers['Request-ID']:
            self.error("Response missing Request-ID header")
            return None
        if not response.headers['SGX-PCK-CRL-Issuer-Chain']:
            self.error("Response missing SGX-PCK-CRL-Issuer-Chain header")
            return None

        # Validate the CRL 

        chain= parse.unquote(
            response.headers['SGX-PCK-CRL-Issuer-Chain']
        )
        chain_pems= self.parse_chain_pem(chain)
        pychain= self.pems_to_pycerts(chain_pems)
        pychain= self.sort_pycert_chain(pychain)
        if pychain is None:
            return None

        crl= response.content
        if self.ApiVersion<3:
            crl_str= str(crl, dec)
            pycrl= crypto.load_crl(crypto.FILETYPE_PEM, crl)
        else:
            crl_str= binascii.hexlify(crl).decode(dec)
            pycrl= crypto.load_crl(crypto.FILETYPE_ASN1, crl)

        if not self.verify_crl_trust(pychain, pycrl):
            self.error("Could not validate certificate using trust chain")
            return None

        return [crl_str, response.headers['SGX-PCK-CRL-Issuer-Chain']]

#----------------------------------------------------------------------------
# PCS: Get TCB Info
#----------------------------------------------------------------------------

    def get_tcb_info(self, fmspc, type, dec=None):
        self.clear_errors()
        url= self._geturl('tcb', type)
        url+= "?fmspc={:s}".format(fmspc)

        response= self._get_request(url, False)
        if response.status_code != 200:
            print(str(response.content, 'utf-8'))
            return None

        # Verify required headers

        if not response.headers['Request-ID']:
            self.error("Response missing Request-ID header")
            return None
        if not response.headers[PCS.HDR_TCB_INFO_ISSUER_CHAIN]:
            self.error("Response missing TCB_INFO_ISSUER_CHAIN header")
            return None

        if response.headers['Content-Type'] != 'application/json':
            self.error("Content-Type should be application/json")
            return None

        # Extract the certificate chain
        chain= parse.unquote(
            response.headers[PCS.HDR_TCB_INFO_ISSUER_CHAIN]
        )

        chain_pems= self.parse_chain_pem(chain)
        pychain= self.pems_to_pycerts(chain_pems)
        pychain= self.sort_pycert_chain(pychain)
        if pychain is None:
            return None

        # Verify the signing cert
        signcert= pychain.pop()
        if not self.verify_cert_trust(pychain, [signcert]):
            self.error("Could not validate certificate using trust chain")
            return None

        # Verify the ECDSA signature. This is calculated over the JSON
        # string of the tcbInfo parameter, which is itself in the
        # JSON of the output. To do this, extract the substring for
        # the tcbInfo parameter from the string and discard the rest.

        data= response.content
        datastr= str(data, 'ascii')

        spos= datastr.find('"tcbInfo":{')
        if spos == -1:
            self.error("Could not extract tcbInfo from JSON")
            return None
        spos+= len('"tcbInfo":')
        epos= datastr.find('},"signature":')
        msg= bytes(datastr[spos:epos+1], 'ascii')

        # Now get the signature. Just parse the response as JSON and
        # extract it.

        tcbinfo= json.loads(datastr)
        signature_hex= tcbinfo['signature']
        signature= bytes.fromhex(signature_hex)
        
        # Now verify the signature

        if not self.verify_signature(signcert, signature, msg):
            return None

        if dec is not None:
            data = str(data, dec)
        
        return [data, response.headers[PCS.HDR_TCB_INFO_ISSUER_CHAIN]]

#----------------------------------------------------------------------------
# PCS: Get QE/QVE/TD_QE Identity
#----------------------------------------------------------------------------

    def get_enclave_identity(self, name, dec=None):
        self.clear_errors()

        if name == 'tdqe':
            url= self._geturl('qe/identity', 'tdx')
        else:
            url= self._geturl(name + '/identity', 'sgx')

        response= self._get_request(url, False)
        if response.status_code != 200:
            print(str(response.content, 'utf-8'))
            return None

        # Verify required headers

        if not response.headers['Request-ID']:
            self.error("Response missing Request-ID header")
            return None
        if not response.headers[PCS.HDR_Enclave_Identity_Issuer_Chain]:
            self.error("Response missing SGX-Enclave-Identity-Issuer-Chain header")
            return None

        if response.headers['Content-Type'] != 'application/json':
            self.error("Content-Type should be application/json")
            return None

        # Extract the certificate chain

        chain= parse.unquote(
            response.headers[PCS.HDR_Enclave_Identity_Issuer_Chain]
        )
        chain_pems= self.parse_chain_pem(chain)
        pychain= self.pems_to_pycerts(chain_pems)
        pychain= self.sort_pycert_chain(pychain)
        if pychain is None:
            return None

        # Verify the signing cert
        signcert= pychain.pop()
        if not self.verify_cert_trust(pychain, [signcert]):
            self.error("Could not validate certificate using trust chain")
            return None

        # Verify the ECDSA signature. This is calculated over the JSON
        # string of the tcbInfo parameter, which is itself in the
        # JSON of the output. To do this, extract the substring for
        # the tcbInfo parameter from the string and discard the rest.

        data= response.content
        datastr= str(data, 'ascii')

        spos= datastr.find('"enclaveIdentity":{')
        if spos == -1:
            self.error("Could not extract enclave identity from JSON")
            return None
        spos+= len('"enclaveIdentity":')
        epos= datastr.find('},"signature":')
        msg= bytes(datastr[spos:epos+1], 'ascii')

        # Now get the signature. Just parse the response as JSON and
        # extract it.

        qeid= json.loads(datastr)
        signature_hex= qeid['signature']
        signature= bytes.fromhex(signature_hex)
        
        # Now verify the signature

        if not self.verify_signature(signcert, signature, msg):
            return None

        if dec is not None:
            data = str(data, dec)

        return [data, response.headers[PCS.HDR_Enclave_Identity_Issuer_Chain]]

    def getFileFromUrl(self, url):
        self.clear_errors()

        os = system()
        if os == 'Windows':
            session = PACSession()
            r = session.get(url)
        else:
            r = requests.get(url, proxies=urllib.request.getproxies())

        if self.ApiVersion<3:
            return str(r.content, 'utf-8')
        else:
            return binascii.hexlify(r.content).decode('utf-8')
