/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef SGX_DCAP_PARSERS_H_
#define SGX_DCAP_PARSERS_H_

#include <vector>
#include <set>
#include <string>
#include <ctime>
#include <stdexcept>
#include <rapidjson/fwd.h>
#include <openssl/x509.h>

namespace intel { namespace sgx { namespace dcap { namespace parser
{

    namespace json
    {
        class JsonParser;

        class TcbInfo;
        class TcbLevel;

        class TcbComponent
        {
        public:
            uint8_t getSvn() const;
            const std::string &getCategory() const;
            const std::string &getType() const;
            TcbComponent(const uint8_t& svn, const std::string& category, const std::string& type)
                : _svn(svn), _category(category), _type(type) {}
            TcbComponent(const uint8_t& svn): _svn(svn) {}

            bool operator>(const TcbComponent& other) const;
            bool operator<(const TcbComponent& other) const;
        private:
            uint8_t _svn = 0;
            std::string _category;
            std::string _type;
            explicit TcbComponent(const ::rapidjson::Value& tcbComponent);
            friend class TcbLevel;
        };

        /**
         * Class representing properties of Intel’s TDX SEAM module.
         */
        class TdxModule
        {
        public:
            TdxModule() = default;
            virtual ~TdxModule() = default;
            TdxModule(const std::vector<uint8_t>& mrsigner, const std::vector<uint8_t>& attributes,
                      const std::vector<uint8_t>& attributesMask): _mrsigner(mrsigner), _attributes(attributes),
                      _attributesMask(attributesMask) {}

            /**
             * Get MRSIGNER
             * @return vector of bytes representing the measurement of a TDX SEAM module’s signer
             *          (48 bytes with SHA384 hash, value: 0’ed for Intel SEAM module).
             */
            virtual const std::vector<uint8_t>& getMrSigner() const;

            /**
             * Get attributes
             * @return vector of bytes representing attributes "golden" value (upon applying mask) for TDX SEAM module
             *          (value: 8 bytes set to 0x00).
             */
            virtual const std::vector<uint8_t>& getAttributes() const;

            /**
             * Get attributes mask
             * @return vector of bytes representing representing mask to be applied to TDX SEAM module’s attributes
             *          value retrieved from the platform (value: 8 bytes set to 0xFF).
             */
            virtual const std::vector<uint8_t>& getAttributesMask() const;
        private:
            std::vector<uint8_t> _mrsigner;
            std::vector<uint8_t> _attributes;
            std::vector<uint8_t> _attributesMask;
            explicit TdxModule(const ::rapidjson::Value& tdxModule);
            friend class TcbInfo;
        };

        /**
         * Class representing a TCB information structure which holds information about TCB Levels for specific FMSPC
         */
        class TcbInfo
        {
        public:
            /**
             * Enum describing version of TCB Level structure
             */
            enum class Version : uint32_t
            {
                V2 = 2,
                V3 = 3
            };

            /// Identifier of the SGX TCB Info
            static const std::string SGX_ID;
            /// Identifier of the TDX TCB Info
            static const std::string TDX_ID;

            TcbInfo() = default;
            virtual ~TcbInfo() = default;

            /**
             * Get identifier of TCB Info structure
             * @return string with identifier
             */
            virtual std::string getId() const;

            /**
             * Get version of TCB Info structure
             * @return unsigned integer with version number
             */
            virtual uint32_t getVersion() const;

            /**
             * Get date and time when TCB information was created in UTC
             * @return std::time_t object with issue date
             */
            virtual std::time_t getIssueDate() const;

            /**
             * Get date and time by which next TCB information will be issued in UTC
             * @return std::time_t object with next update date
             */
            virtual std::time_t getNextUpdate() const;

            /**
             * Get FMSPC (Family-Model-Stepping-Platform-CustomSKU)
             * @return vector of bytes representing FMSPC (6 bytes)
             */
            virtual const std::vector<uint8_t>& getFmspc() const;

            /**
             * Get PCE Identifier
             * @return vector of bytes representing PCE identifier
             */
            virtual const std::vector<uint8_t>& getPceId() const;

            /**
             * Get sorted list of SGX TCB levels for given FMSPC
             * @return array of TCB Level objects
             */
            virtual const std::set<TcbLevel, std::greater<TcbLevel>>& getTcbLevels() const;

            /**
             * Get signature over tcbInfo body (without whitespaces) using TCB Signing Key
             * @return vector of bytes representing signature
             */
            virtual const std::vector<uint8_t>& getSignature() const;

            /**
             * Get raw tcb info body data
             * @return vector of bytes with raw data
             */
            virtual const std::vector<uint8_t>& getInfoBody() const;

            /**
             * Get type of TCB level composition that determines TCB level comparison logic
             * @return integer representing TCB Type
             *
             * @throws intel::sgx::dcap::parser::FormatException in case of TCBInfo version equal 1
             */
            virtual int getTcbType() const;

            /**
             * Get a monotically increasing sequence number changed then Intel updates the content of the TCB evaluation
             * data set: TCB Info, QE Identity and QVE Identity. Idenity and QVE Identity. The tcbEvaluationDataNumber
             * update is synchronized across TCB Info for all flavors of SGX CPUs (Family-Model-Stepping-Platform-CustomSKU)
             * and QE/QVE Identity. This sequence number allows users to easily determine when a particular
             * TCB Info/QE Idenity/QVE Identiy superseedes another TCB Info/QE Identity/QVE Identity.
             *
             * @return unsigned integer representing TCB Evaluation Data Number
             *
             * @throws intel::sgx::dcap::parser::FormatException in case of TCBInfo version equal 1
             */
            virtual uint32_t getTcbEvaluationDataNumber() const;

            /**
             * Get properties of Intel’s TDX SEAM module
             * This field is optional:
                -	In case of TCB Info for SGX it is not present.
                -	In case of TCB Info for TDX it must be present.
             * @return TdxModule object representing properties of Intel's TDX SEAM Module
             * @throw FormatException if TdxModule is not present
             */
            virtual const TdxModule& getTdxModule() const;

            /**
             * Staic function that parses JSON text from a string into TCB Info object
             * @param string with text in JSON Format
             * @return TcbInfo instance
             *
             * @throws intel::sgx::dcap::parser::FormatException in case of parsing error
             */
            static TcbInfo parse(const std::string& json);

        private:
            std::string _id;
            Version _version = Version::V2;
            std::time_t _issueDate = 0;
            std::time_t _nextUpdate = 0;
            std::vector<uint8_t> _fmspc;
            std::vector<uint8_t> _pceId;
            std::set<TcbLevel, std::greater<TcbLevel>> _tcbLevels;
            std::vector<uint8_t> _signature;
            std::vector<uint8_t> _infoBody;
            TdxModule _tdxModule;
            int _tcbType{};
            uint32_t _tcbEvaluationDataNumber{};

            void parsePartV2(const ::rapidjson::Value &tcbInfo, JsonParser& jsonParser);
            void parsePartV3(const ::rapidjson::Value &tcbInfo);
            explicit TcbInfo(const std::string& jsonString);
        };

        /**
         * Class representing a single TCB Level
         */
        class TcbLevel
        {
        public:
            /**
             * Creates TCB Level object with provided data
             * @param cpuSvnComponents - Vector of bytes representing CPU SVN value (16 bytes)
             * @param pceSvn - unsigned integer for PCE SVN
             * @param status - TCB Level status which is a string with one of those values:
             *          - UpToDate
             *          - ConfigurationNeeded
             *          - OutOfDate
             *          - OutOfDateConfigurationNeeded
             *          - Revoked
             * @return the value for given component
             *
             */
            TcbLevel(const std::vector<uint8_t>& cpuSvnComponents,
                     uint32_t pceSvn,
                     const std::string& status);

            /**
             * Creates TCB Level object with provided data
             * @param cpuSvnComponents - Vector of bytes representing CPU SVN value (16 bytes)
             * @param pceSvn - unsigned integer for PCE SVN
             * @param status - TCB Level status which is a string with one of those values:
             *          - UpToDate
             *          - ConfigurationNeeded
             *          - OutOfDate
             *          - OutOfDateConfigurationNeeded
             *          - Revoked
             * @param tcbDate - Date and time when the TCB level was certified not to be vulnerable to any issues described in SAs that were published on or prior to this date.
             * @param advisoryIDs - Vector of strings representing security advisory IDs describing vulnerabilities that this TCB level is vulnerable to, e.g. [ "INTEL-SA-00079", "INTEL-SA-00076" ]
             * @return the value for given component
             *
             */
            TcbLevel(const std::vector<uint8_t>& cpuSvnComponents,
                     const uint32_t pceSvn,
                     const std::string& status,
                     const std::time_t tcbDate,
                     std::vector<std::string> advisoryIDs);

            /**
             * Creates TCB Level V3 object with provided data
             * @param id - Identifier of TEE TYPE - SGX or TDX
             * @param sgxTcbComponents - Vector of TCB Components representing SGX TCB Components value (16 entries)
             * @param tdxTcbComponents - Vector of TCB Components representing TDX TCB Components value (16 entries)
             * @param pceSvn - unsigned integer for PCE SVN
             * @param status - TCB Level status which is a string with one of those values:
             *          - UpToDate
             *          - ConfigurationNeeded
             *          - OutOfDate
             *          - OutOfDateConfigurationNeeded
             *          - Revoked
             * @return the value for given component
             *
             */
            TcbLevel(const std::string& id,
                     const std::vector<TcbComponent>& sgxTcbComponents,
                     const std::vector<TcbComponent>& tdxTcbComponents,
                     const uint32_t pceSvn,
                     const std::string& status);

            virtual ~TcbLevel() = default;
            virtual bool operator>(const TcbLevel& other) const;

            /**
             * Get SVN component value at given position
             * @param componentNumber
             * @return the value for given component
             *
             * @throws intel::sgx::dcap::parser::FormatException when out of range
             */
            virtual uint32_t getSgxTcbComponentSvn(uint32_t componentNumber) const;

            /**
             * Get CPU SVN
             * @return Vector of bytes representing CPU SVN (16 bytes)
             *
             */
            virtual const std::vector<uint8_t>& getCpuSvn() const;

            /**
             * Get SGX TCB Component
             * @return Vector of TcbComponents representing (16 entries)
             *
             */
            virtual const std::vector<TcbComponent>& getSgxTcbComponents() const;

            /**
             * Get SGX SVN component value at given position
             * @param componentNumber
             * @return the value for given component
             *
             * @throws intel::sgx::dcap::parser::FormatException when out of range
             */
            virtual const TcbComponent& getSgxTcbComponent(uint32_t componentNumber) const;

            /**
             * Get TDX TCB Component
             * @return Vector of TcbComponents representing (16 entries)
             *
             */
            virtual const std::vector<TcbComponent>& getTdxTcbComponents() const;

            /**
             * Get TDX SVN component value at given position
             * @param componentNumber
             * @return the value for given component
             *
             * @throws intel::sgx::dcap::parser::FormatException when out of range
             */
            virtual const TcbComponent& getTdxTcbComponent(uint32_t componentNumber) const;

            /**
             * Get PCE SVN
             * @return unsigned integer value representing PCE SVN
             *
             */
            virtual uint32_t getPceSvn() const;

            /**
             * Get status of this tcb level
             * @return string with one of those values:
             *          - UpToDate
             *          - ConfigurationNeeded
             *          - OutOfDate
             *          - OutOfDateConfigurationNeeded
             *          - Revoked
             *
             */
            virtual const std::string& getStatus() const;

            /**
             * Get date and time when the TCB level was certified not to be vulnerable to any issues described in SAs that were published on or prior to this date.
             * @return std::time_t structure representing TCB Date
             *
             */
            virtual const std::time_t& getTcbDate() const;

            /**
             * Get array of Advisory IDs describing vulnerabilities that this TCB level is vulnerable to, e.g. [ "INTEL-SA-00079", "INTEL-SA-00076" ]
             * @return array of strings
             *
             */
            virtual const std::vector<std::string>& getAdvisoryIDs() const;

        private:
            std::string _id;
            TcbInfo::Version _version = TcbInfo::Version::V2;
            std::vector<uint8_t> _cpuSvnComponents;
            std::vector<TcbComponent> _sgxTcbComponents;
            std::vector<TcbComponent> _tdxTcbComponents;
            uint32_t _pceSvn;
            std::string _status;
            std::time_t _tcbDate;
            std::vector<std::string> _advisoryIDs{};

            void setCpuSvn(const ::rapidjson::Value& tcb, JsonParser& jsonParser);
            void setTcbComponents(const ::rapidjson::Value& tcb);
            void parseSvns(const ::rapidjson::Value& tcbLevel, JsonParser& jsonParser);
            void parseStatus(const ::rapidjson::Value &tcbLevel,
                             const std::vector<std::string> &validStatuses,
                             const std::string &filedName);
            void parseTcbLevelV2(const ::rapidjson::Value& tcbLevel, JsonParser& jsonParser);
            void parseTcbLevelV3(const ::rapidjson::Value &tcbLevel, JsonParser& jsonParser);
            void parseTcbLevelCommon(const ::rapidjson::Value& tcbLevel, JsonParser& jsonParser);
            explicit TcbLevel(const ::rapidjson::Value& tcbLevel, const uint32_t version);
            explicit TcbLevel(const ::rapidjson::Value& tcbLevel, const uint32_t version, const std::string& id);
            friend class TcbInfo;
        };

    }

    namespace x509
    {
        class Certificate;

        /**
         * Class that represents x509 distinguished name
         */
        class DistinguishedName
        {
        public:
            DistinguishedName() = default;
            /**
             * Create instance of DistinguishedName class
             * @param raw - string with raw distinguished name
             * @param commonName - common name component of distinguished name
             * @param countryName - country component of distinguished name
             * @param organizationName - organization component of distinguished name
             * @param locationName - location component of distinguished name
             * @param stateName - state component of distinguished name
             */
            DistinguishedName(const std::string& raw,
                              const std::string& commonName,
                              const std::string& countryName,
                              const std::string& organizationName,
                              const std::string& locationName,
                              const std::string& stateName);
            virtual ~DistinguishedName() = default;

            virtual bool operator==(const DistinguishedName& other) const;
            virtual bool operator!=(const DistinguishedName& other) const;

            /**
             * Get string with raw name
             * @return string with raw name
             */
            virtual const std::string& getRaw() const;
            /**
             * Get common name component of Distinguished Name
             * @return string with common name
             */
            virtual const std::string& getCommonName() const;
            /**
             * Get country name component of Distinguished Name
             * @return string with country name
             */
            virtual const std::string& getCountryName() const;
            /**
             * Get organization name component of Distinguished Name
             * @return string with organization name
             */
            virtual const std::string& getOrganizationName() const;
            /**
             * Get location name component of Distinguished Name
             * @return string with location name
             */
            virtual const std::string& getLocationName() const;
            /**
             * Get state name component of Distinguished Name
             * @return string with state name
             */
            virtual const std::string& getStateName() const;

        private:
            std::string _raw;
            std::string _commonName;
            std::string _countryName;
            std::string _organizationName;
            std::string _locationName;
            std::string _stateName;

            explicit DistinguishedName(X509_name_st *x509Name);

            friend class Certificate;
        };

        /**
         * Class that represents certificate's validity
         */
        class Validity
        {
        public:
            Validity() = default;
            /**
             * Creates instance of Validity object
             * @param notBeforeTime - date when validity starts
             * @param notAfterTime - date when validity ends
             */
            Validity(std::time_t notBeforeTime, std::time_t notAfterTime);
            virtual ~Validity() = default;

            /**
             * Check if validity objects are equal
             * @param other - second validity
             * @return true if objects are equal
             */
            virtual bool operator==(const Validity& other) const;

            /**
             * Get notBeforeTime component of validity
             * @return notBeforeTime
             */
            virtual std::time_t getNotBeforeTime() const;
            /**
             * Get notAfterTime of validity
             * @return notAfterTime
             */
            virtual std::time_t getNotAfterTime() const;

        private:
            std::time_t _notBeforeTime{};
            std::time_t _notAfterTime{};
        };

        /**
         * Class that represents SGX specific X509 certificate extensions
         */
        class Extension
        {
        public:
            /**
             * Enum representing SGX extension type
             */
            enum class Type : int
            {
                NONE = -1,
                PPID = 0,
                CPUSVN,
                PCESVN,
                PCEID,
                FMSPC,
                SGX_TYPE,
                DYNAMIC_PLATFORM,
                CACHED_KEYS,
                TCB,
                SGX_TCB_COMP01_SVN,
                SGX_TCB_COMP02_SVN,
                SGX_TCB_COMP03_SVN,
                SGX_TCB_COMP04_SVN,
                SGX_TCB_COMP05_SVN,
                SGX_TCB_COMP06_SVN,
                SGX_TCB_COMP07_SVN,
                SGX_TCB_COMP08_SVN,
                SGX_TCB_COMP09_SVN,
                SGX_TCB_COMP10_SVN,
                SGX_TCB_COMP11_SVN,
                SGX_TCB_COMP12_SVN,
                SGX_TCB_COMP13_SVN,
                SGX_TCB_COMP14_SVN,
                SGX_TCB_COMP15_SVN,
                SGX_TCB_COMP16_SVN,
                PLATFORM_INSTANCE_ID,
                CONFIGURATION,
                SMT_ENABLED
            };

            Extension();

            /**
             * Creates an instance of Extension class
             * @param nid
             * @param name - extension name
             * @param value - extension value
             */
            Extension(int nid,
                      const std::string& name,
                      const std::vector<uint8_t>& value) noexcept;
            virtual ~Extension() = default;

            /**
             * Check if extensions are equal
             * @return true if equal
             */
            virtual bool operator==(const Extension&) const;

            /**
             * Check if extensions are not equal
             * @return true if not equal
             */
            virtual bool operator!=(const Extension&) const;

            /**
             * Get NID
             * @return nid
             */
            virtual int getNid() const;

            /**
             * Get name
             * @return name
             */
            virtual const std::string& getName() const;

            /**
             * Get value
             * @return vector of bytes representing extension's value
             */
            virtual const std::vector<uint8_t>& getValue() const;

        private:
            int _nid;
            std::string _name;
            std::vector<uint8_t> _value;

            explicit Extension(X509_EXTENSION *ext);

            friend class Certificate;
            friend class UnitTests;
        };

        /**
         * Class that represents ECDSA signature
         */
        class Signature
        {
        public:
            Signature();

            /**
             * Creates instance of signature
             * @param rawDer - vector of bytes representing raw signature in DER format
             * @param r - r component of signature
             * @param s - s component of signature
             */
            Signature(const std::vector<uint8_t>& rawDer,
                      const std::vector<uint8_t>& r,
                      const std::vector<uint8_t>& s);
            virtual ~Signature() = default;

            /**
             * Check if signatures are equal
             * @param other
             * @return true if are equal
             */
            virtual bool operator==(const Signature& other) const;

            /**
             * Get raw signature in DER format
             * @return vector of bytes with signature
             */
            virtual const std::vector<uint8_t>& getRawDer() const;

            /**
             * Get R component of ECDSA signature
             * @return vector of bytes with R component
             */
            virtual const std::vector<uint8_t>& getR() const;

            /**
             * Get S component of ECDSA signature
             * @return vector of bytes with S component
             */
            virtual const std::vector<uint8_t>& getS() const;

        private:
            std::vector<uint8_t> _rawDer;
            std::vector<uint8_t> _r;
            std::vector<uint8_t> _s;

            explicit Signature(const ASN1_BIT_STRING* pSig);

            friend class Certificate;
        };

        /**
         * Class that represents X509 Certificate
         */
        class Certificate
        {
        public:
            Certificate();
            Certificate(const Certificate &) = default;
            Certificate(Certificate &&) = default;
            virtual ~Certificate() = default;

            Certificate& operator=(const Certificate &) = delete;
            Certificate& operator=(Certificate &&) = default;

            /**
             * Check if certificate objects are equal
             * @param other certificate
             * @return true if equal
             */
            virtual bool operator==(const Certificate& other) const;

            /**
             * Get version of X509 certificate
             * @return unsigned integer with version
             */
            virtual uint32_t getVersion() const;

            /**
             * Get serial number of X509 certificate
             * @return vector of bytes with serial number
             */
            virtual const std::vector<uint8_t>& getSerialNumber() const;

            /**
             * Get subject of X509 certificate
             * @return subject as DistinguishedName object
             */
            virtual const DistinguishedName& getSubject() const;

            /**
             * Get issuer of X509 certificate
             * @return issuer as DistinguishedName object
             */
            virtual const DistinguishedName& getIssuer() const;

            /**
             * Get validity of X509 certificate (period when certificate is valid)
             * @return validity as Validity object
             */
            virtual const Validity& getValidity() const;

            /**
             * Get extensions of X509 certificate
             * @return vector of Extension objects
             */
            virtual const std::vector<Extension>& getExtensions() const;

            /**
             * Get raw PEM data of X509 certificate
             * @return string - PEM format
             */
            virtual const std::string& getPem() const;

            /**
             * Get Certificate info (TBS) binary value that should be verifiable
             * using signature and public key with SHA256 algorithm
             * @return Vector of bytes representing certificate info
             */
            virtual const std::vector<uint8_t>& getInfo() const;

            /**
             * Get Certificate's signature
             * @return signature as Signature object
             */
            virtual const Signature& getSignature() const;

            /**
             * Get public key bytes <header[1B]><x[32B]><y[32B]>
             * @return Vector of bytes
             */
            virtual const std::vector<uint8_t>& getPubKey() const;

            /**
             * Parse PEM encoded X.509 certificate
             * @param pem PEM encoded X.509 certificate
             * @return Certificate instance
             *
             * @throws intel::sgx::dcap::parser::FormatException in case of parsing error
             */
            static Certificate parse(const std::string& pem);

        protected:
            uint32_t _version;
            DistinguishedName _subject;
            DistinguishedName _issuer;
            Validity _validity;
            std::vector<Extension> _extensions;
            Signature _signature;
            std::vector<uint8_t> _serialNumber;
            std::vector<uint8_t> _pubKey;
            std::vector<uint8_t> _info;
            std::string _pem;

            explicit Certificate(const std::string& pem);

        private:
            void setInfo(X509* x509);
            void setVersion(const X509* x509);
            void setSerialNumber(const X509* x509);
            void setSubject(const X509* x509);
            void setIssuer(const X509* x509);
            void setValidity(const X509* x509);
            void setExtensions(const X509* x509);
            void setSignature(const X509* x509);
            void setPublicKey(const X509* x509);
        };

        enum SgxType
        {
            Standard,
            Scalable,
            ScalableWithIntegrity
        };

        class PckCertificate;

        /**
         * Class that represents Intel SGX Trusted Computing Base information stored in Provisioning Certification Key Certificate
         */
        class Tcb
        {
        public:
            Tcb() = default;
            /**
             * Creates instance of TCB class
             * @param cpusvn vector of bytes representing CPU SVN
             * @param cpusvnComponents vector of bytes representing CPU SVN components
             * @param pcesvn unsigned integer with PCE SVN value
             */
            Tcb(const std::vector<uint8_t>& cpusvn,
                const std::vector<uint8_t>& cpusvnComponents,
                uint32_t pcesvn);
            virtual ~Tcb() = default;

            /**
             * Check if TCB objects are equal
             * @param other TCB
             * @return true if equal
             */
            virtual bool operator==(const Tcb& other) const;

            /**
             * Get SVN component value at given position
             * @param componentNumber
             * @return the value for given component
             *
             * @throws intel::sgx::dcap::parser::FormatException when out of range
             */
            virtual uint32_t getSgxTcbComponentSvn(uint32_t componentNumber) const;
            /**
             * Get all CPU SVN components
             * @return vector of bytes with cpu svn components
             */
            virtual const std::vector<uint8_t>& getSgxTcbComponents () const;

            /**
             * Get PCE SVN
             * @return unsigned integer representing PCE SVN
             */
            virtual uint32_t getPceSvn() const;

            /**
             * Get CPU SVN
             * @return vector of bytes representing CPU SVN (16 bytes)
             */
            virtual const std::vector<uint8_t>& getCpuSvn() const;

        private:
            std::vector<uint8_t> _cpuSvn;
            std::vector<uint8_t> _cpuSvnComponents;
            uint32_t _pceSvn{};

            explicit Tcb(const ASN1_TYPE *);

            friend class PckCertificate;
        };

        /**
         * Optional sequence of additional configuration settings.
         * This data is only relevant to PCK Certificates issued by PCK Platform CA
         */
        class Configuration {
        public:
            Configuration() = default;
            /**
             * Creates instance of Configuration class.
             * @param dynamicPlatform Optional flag describing whether platform can be extended with additional packages
             * @param cachedKeys Optional flag describing whether platform root keys are cached by SGX Registration Backend
             * @param smtEnabled Optional flag describing whether platform has SMT
             */
            Configuration(
                    bool dynamicPlatform,
                    bool cachedKeys,
                    bool smtEnabled);
            virtual ~Configuration() = default;

            /**
             * Check if Configuration objects are equal
             * @param other Configuration
             * @return true if equal
             */
            virtual bool operator==(const Configuration& other) const;

            /**
             * Optional flag describing whether platform can be extended with additional packages
             * @return true if platform can be extended with additional packages
             */
            virtual bool isDynamicPlatform() const;

            /**
             * Optional flag describing whether platform root keys are cached by SGX Registration Backend
             * @return true if platform root keys are cached by SGX Registration Backend
             */
            virtual bool isCachedKeys() const;

            /**
             * Optional flag describing whether platform has SMT
             * @return true if platform has SMT
             */
            virtual bool isSmtEnabled() const;

        private:
            bool _dynamicPlatform = true;
            bool _cachedKeys = true;
            bool _smtEnabled = true;

            explicit Configuration(const ASN1_TYPE *configurationSeq);

            friend class PckCertificate;
            friend class ProcessorPckCertificate;
            friend class PlatformPckCertificate;
        };

        /**
         * Class that represents Provisioning Certification Key Certificate
         */
        class PckCertificate : public Certificate
        {
        public:
            PckCertificate() = default;
            PckCertificate(const PckCertificate &) = delete;
            PckCertificate(PckCertificate &&) = default;
            virtual ~PckCertificate() = default;

            /**
             * Upcast Certificate to PCK certificate
             * @param certificate
             *
             * @throws intel::sgx::dcap::parser::FormatException in case of upcasting error
             */
            explicit PckCertificate(const Certificate& certificate);

            PckCertificate& operator=(const PckCertificate &) = delete;
            PckCertificate& operator=(PckCertificate &&) = default;

            /**
             * Check if PCK certificates are equal
             * @param other PCK certificate
             * @return true if equal
             */
            virtual bool operator==(const PckCertificate& other) const;

            /**
             * Get PPID
             * @return vector of bytes representing PPID (16 bytes)
             */
            virtual const std::vector<uint8_t>& getPpid() const;

            /**
             * Get PCE identifier
             * @return vector of bytes representing PCE ID (2 bytes)
             */
            virtual const std::vector<uint8_t>& getPceId() const;

            /**
             * Get FMSPC
             * @return vector of bytes representing FMSPC (6 bytes)
             */
            virtual const std::vector<uint8_t>& getFmspc() const;

            /**
             * Get SGX Type
             * @return Enum representing SGX Type
             */
            virtual SgxType getSgxType() const;

            /**
             * Get sequence of TCB components.
             * @return Structure of TCB components
             */
            virtual const Tcb& getTcb() const;

            /**
             * Parse PEM encoded X.509 PCK certificate
             * @param pem PEM encoded X.509 certificate
             * @return PCK certificate instance
             *
             * @throws intel::sgx::dcap::parser::FormatException in case of parsing error
             */
            static PckCertificate parse(const std::string& pem);

        private:
            std::vector<uint8_t> _ppid;
            std::vector<uint8_t> _pceId;
            std::vector<uint8_t> _fmspc;
            Tcb _tcb;
            SgxType _sgxType;

            uint8_t PROCESSOR_CA_EXTENSION_COUNT = 5;
            uint8_t PLATFORM_CA_EXTENSION_COUNT = 7;

            stack_st_ASN1_TYPE* getSgxExtensions();
            void setMembers(stack_st_ASN1_TYPE *sgxExtensions);

            explicit PckCertificate(const std::string& pem);

            friend class ProcessorPckCertificate;
            friend class PlatformPckCertificate;
        };

        /**
         * Class that represents Processor Provisioning Certification Key Certificate
         */
        class ProcessorPckCertificate : public PckCertificate
        {
            using PckCertificate::PckCertificate;
        public:
            /**
             * Upcast Certificate to ProcessorPckCertificate PCK certificate
             * @param certificate
             *
             * @throws intel::sgx::dcap::parser::FormatException in case of upcasting error
             */
            explicit ProcessorPckCertificate(const Certificate& certificate);

            /**
             * Parse PEM encoded X.509 PCK certificate
             * @param pem PEM encoded X.509 certificate
             * @return PCK certificate instance
             *
             * @throws intel::sgx::dcap::parser::FormatException in case of parsing error
             */
            static ProcessorPckCertificate parse(const std::string& pem);
        private:
            explicit ProcessorPckCertificate(const std::string& pem);

            void setMembers(stack_st_ASN1_TYPE *sgxExtensions);
        };

        /**
         * Class that represents Platform Certification Key Certificate
         */
        class PlatformPckCertificate : public PckCertificate {
        public:

            PlatformPckCertificate() = default;
            PlatformPckCertificate(const PlatformPckCertificate &) = delete;
            PlatformPckCertificate(PlatformPckCertificate &&) = default;
            virtual ~PlatformPckCertificate() = default;

            /**
             * Upcast Certificate to PlatformPckCertificate PCK certificate
             * @param certificate
             *
             * @throws intel::sgx::dcap::parser::FormatException in case of upcasting error
             */
            explicit PlatformPckCertificate(const Certificate& certificate);

            PlatformPckCertificate& operator=(const PlatformPckCertificate &) = delete;
            PlatformPckCertificate& operator=(PlatformPckCertificate &&) = default;

            /**
             * Check if Platform PCK certificates are equal
             * @param other PCK certificate
             * @return true if equal
             */
            virtual bool operator==(const PlatformPckCertificate& other) const;

            /**
             * Get value of Platform Instance ID.
             * @return vector of bytes representing Platform Instance ID (16 bytes)
             */
            virtual const std::vector<uint8_t>& getPlatformInstanceId() const;

            /**
             * Get sequence of additional configuration settings.
             * @return Configuration instance
             */
            virtual const Configuration& getConfiguration() const;

            /**
             * Parse PEM encoded X.509 PCK certificate
             * @param pem PEM encoded X.509 certificate
             * @return PCK certificate instance
             *
             * @throws intel::sgx::dcap::parser::FormatException in case of parsing error
             */
            static PlatformPckCertificate parse(const std::string& pem);
        private:
            explicit PlatformPckCertificate(const std::string& pem);

            void setMembers(stack_st_ASN1_TYPE *sgxExtensions);

            std::vector<uint8_t> _platformInstanceId;
            Configuration _configuration;
        };
    }

    /**
     * Exception when parsing fails due to incorrect format of the structure
     */
    class FormatException : public std::logic_error
    {
    public:
        using std::logic_error::logic_error;
    };

    /**
     * Exception when invalid extension is found in certificate
     */
    class InvalidExtensionException : public std::logic_error
    {
    public:
        using std::logic_error::logic_error;
    };

}}}}

#endif // SGX_DCAP_PARSERS_H_