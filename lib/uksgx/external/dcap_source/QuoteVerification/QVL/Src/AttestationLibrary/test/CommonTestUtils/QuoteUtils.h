#ifndef SGXECDSAATTESTATION_TEST_QUOTE_UTILS_H_
#define SGXECDSAATTESTATION_TEST_QUOTE_UTILS_H_

#include <OpensslHelpers/Bytes.h>

namespace intel { namespace sgx { namespace dcap { namespace test {

static constexpr size_t QUOTE_HEADER_SIZE = 48;
static constexpr size_t ENCLAVE_REPORT_SIGNATURE_SIZE = 64;
static constexpr size_t ECDSA_PUBLIC_KEY_SIZE = 64;
static constexpr size_t ENCLAVE_REPORT_SIZE = 384;
static constexpr size_t BODY_SIZE = ENCLAVE_REPORT_SIZE;

static constexpr size_t QE_CERT_DATA_MIN_SIZE = 6;
static constexpr size_t QE_AUTH_DATA_MIN_SIZE = 2;
static constexpr size_t QE_AUTH_SIZE_BYTE_LEN = 2;
static constexpr size_t QUOTE_AUTH_DATA_SIZE_FIELD_SIZE = 4;

static constexpr size_t QUOTE_V3_AUTH_DATA_MIN_SIZE =
        ENCLAVE_REPORT_SIGNATURE_SIZE + // quote signature
        ECDSA_PUBLIC_KEY_SIZE +
        ENCLAVE_REPORT_SIZE + //qeReport
        ENCLAVE_REPORT_SIGNATURE_SIZE + //qeReportSignate
        QE_AUTH_DATA_MIN_SIZE +
        QE_CERT_DATA_MIN_SIZE;

static constexpr size_t QUOTE_V3_MINIMAL_SIZE =
        QUOTE_HEADER_SIZE +
        ENCLAVE_REPORT_SIZE +
        QUOTE_AUTH_DATA_SIZE_FIELD_SIZE +
        QUOTE_V3_AUTH_DATA_MIN_SIZE;

static constexpr size_t QUOTE_V4_AUTH_DATA_MIN_SIZE =
        ENCLAVE_REPORT_SIGNATURE_SIZE + // quote signature
        ECDSA_PUBLIC_KEY_SIZE +
        QE_CERT_DATA_MIN_SIZE +
        ENCLAVE_REPORT_SIZE +
        ENCLAVE_REPORT_SIGNATURE_SIZE +
        QE_AUTH_SIZE_BYTE_LEN +
        QE_CERT_DATA_MIN_SIZE;

static constexpr size_t QUOTE_V4_MINIMAL_SIZE =
        QUOTE_HEADER_SIZE +
        ENCLAVE_REPORT_SIZE +
        QUOTE_AUTH_DATA_SIZE_FIELD_SIZE +
        QUOTE_V4_AUTH_DATA_MIN_SIZE;

template<class DataType>
Bytes toBytes(DataType &data) {
    Bytes retVal;
    auto bytes = reinterpret_cast<uint8_t *>(const_cast<typename std::remove_cv<DataType>::type *>(&data));
    retVal.insert(retVal.end(), bytes, bytes + sizeof(DataType));
    return retVal;
}

}}}}

#endif //SGXECDSAATTESTATION_TEST_QUOTE_UTILS_H_
