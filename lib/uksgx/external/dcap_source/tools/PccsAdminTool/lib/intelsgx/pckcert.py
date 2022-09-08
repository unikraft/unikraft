from cryptography import x509
from cryptography.x509.oid import ObjectIdentifier
from cryptography.hazmat.backends import default_backend
import asn1
import struct

# This is a very simplistic ASN1 parser. Production code should use
# something like ans1c to build a parser from the ASN1 spec file so
# that it can check and enforce data validity.

class SgxPckCertificateExtensions:
	id_ce_sGXExtensions = '1.2.840.113741.1.13.1'
	id_ce_sGXExtensions_tCB= id_ce_sGXExtensions+".2"
	id_ce_sGXExtensions_configuration= id_ce_sGXExtensions+".7"
	id_cdp_extension = '2.5.29.31'
	decoder= asn1.Decoder()
	_data= {}
	ca= ''
	oids= {
		id_ce_sGXExtensions: 'sGXExtensions',
		id_ce_sGXExtensions+".1": 'pPID',
		id_ce_sGXExtensions_tCB: 'tCB',
		id_ce_sGXExtensions_tCB+".1": 'tCB-sGXTCBComp01SVN',
		id_ce_sGXExtensions_tCB+".2": 'tCB-sGXTCBComp02SVN',
		id_ce_sGXExtensions_tCB+".3": 'tCB-sGXTCBComp03SVN',
		id_ce_sGXExtensions_tCB+".4": 'tCB-sGXTCBComp04SVN',
		id_ce_sGXExtensions_tCB+".5": 'tCB-sGXTCBComp05SVN',
		id_ce_sGXExtensions_tCB+".6": 'tCB-sGXTCBComp06SVN',
		id_ce_sGXExtensions_tCB+".7": 'tCB-sGXTCBComp07SVN',
		id_ce_sGXExtensions_tCB+".8": 'tCB-sGXTCBComp08SVN',
		id_ce_sGXExtensions_tCB+".9": 'tCB-sGXTCBComp09SVN',
		id_ce_sGXExtensions_tCB+".10": 'tCB-sGXTCBComp10SVN',
		id_ce_sGXExtensions_tCB+".11": 'tCB-sGXTCBComp11SVN',
		id_ce_sGXExtensions_tCB+".12": 'tCB-sGXTCBComp12SVN',
		id_ce_sGXExtensions_tCB+".13": 'tCB-sGXTCBComp13SVN',
		id_ce_sGXExtensions_tCB+".14": 'tCB-sGXTCBComp14SVN',
		id_ce_sGXExtensions_tCB+".15": 'tCB-sGXTCBComp15SVN',
		id_ce_sGXExtensions_tCB+".16": 'tCB-sGXTCBComp16SVN',
		id_ce_sGXExtensions_tCB+".17": 'tCB-pCESVN',
		id_ce_sGXExtensions_tCB+".18": 'tCB-cPUSVN',
		id_ce_sGXExtensions+".3": 'pCE-ID',
		id_ce_sGXExtensions+".4": 'fMSPC',
		id_ce_sGXExtensions+".5": 'sGXType',
		id_ce_sGXExtensions+".6": 'platformInstanceID',
		id_ce_sGXExtensions_configuration: 'configuration',
		id_ce_sGXExtensions_configuration+".1": 'dynamicPlatform',
		id_ce_sGXExtensions_configuration+".2": 'cachedKeys',
		id_ce_sGXExtensions_configuration+".3": 'sMTEnabled'
	}

	def _parse_asn1(self, d, oid, lnr=asn1.Numbers.ObjectIdentifier):
		tag= self.decoder.peek()
		while tag:
			if tag.typ == asn1.Types.Constructed:
				self.decoder.enter()
				if ( lnr == asn1.Numbers.ObjectIdentifier ):
					d[self.oids[oid]]= {}
					self._parse_asn1(d[self.oids[oid]], oid, tag.nr)
				else:
					self._parse_asn1(d, oid, tag.nr)
				self.decoder.leave()
			elif tag.typ == asn1.Types.Primitive:
				tag, value= self.decoder.read()
				if ( tag.nr == asn1.Numbers.ObjectIdentifier ):
					oid= value
				else:
					d[self.oids[oid]]= value
			lnr= tag.nr
			tag= self.decoder.peek()
		return

	def parse_pem_certificate(self, pem):
		self._data= {}
		cert= x509.load_pem_x509_certificate(pem, default_backend())
		issuerCN = cert.issuer.rfc4514_string()
		if (issuerCN.find('Processor') != -1) :
			self.ca = 'PROCESSOR'
		elif (issuerCN.find('Platform') != -1) :
			self.ca = 'PLATFORM'
		else :
			self.ca = None
		
		sgxext= cert.extensions.get_extension_for_oid(
			ObjectIdentifier(self.id_ce_sGXExtensions)
		)

		self.decoder.start(sgxext.value.value)
		self._parse_asn1(self._data, self.id_ce_sGXExtensions)

	def get_root_ca_crl(self, pem):
		self._data= {}
		cert= x509.load_pem_x509_certificate(pem, default_backend())
		cdpext= cert.extensions.get_extension_for_oid(
			ObjectIdentifier(self.id_cdp_extension)
		)

		return getattr(getattr(cdpext.value[0], "_full_name")[0], "value")

	def data(self, field=None):
		if 'sGXExtensions' not in self._data:
			return None

		d= self._data['sGXExtensions']

		if field:
			if field in d:
				return d[field]
			return None

		return d

	def _hex_data(self, field):
		val= self.data(field)
		if val is None:
			return None
		return val.hex()

	# Commonly-needed data fields
	#------------------------------

	def get_fmspc(self):
		return self._hex_data('fMSPC')

	def get_ca(self):
		return self.ca

	def get_tcbm(self):
		tcb= self.data('tCB')
		if tcb is None:
			return None
		return tcb['tCB-cPUSVN'].hex() + self.get_pcesvn()

	def get_pceid(self):
		return self._hex_data('pCE-ID')

	def get_ppid(self):
		return self._hex_data('pPID')

	def get_pcesvn(self):
		tcb= self.data('tCB')
		# pCESVN should be packed little-endian
		pcesvn= struct.pack('<H', tcb['tCB-pCESVN'])
		return pcesvn.hex()
