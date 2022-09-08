
#include "QuoteStructures.h"
#include "QuoteParsers.h"
#include <algorithm>
#include <iterator>

namespace intel { namespace sgx { namespace dcap { namespace quote {
using namespace constants;

bool Header::insert(std::vector<uint8_t>::const_iterator& from, const std::vector<uint8_t>::const_iterator& end)
{
    if (!copyAndAdvance(version, from, end)) { return false; }
    if (!copyAndAdvance(attestationKeyType, from, end)) { return false; }
    if (!copyAndAdvance(teeType, from, end)) { return false; }
    if (!copyAndAdvance(qeSvn, from, end)) { return false; }
    if (!copyAndAdvance(pceSvn, from, end)) { return false; }
    if (!copyAndAdvance(qeVendorId, from, end)) { return false; }
    if (!copyAndAdvance(userData, from, end)) { return false; }
    return true;
}

bool EnclaveReport::insert(std::vector<uint8_t>::const_iterator& from, const std::vector<uint8_t>::const_iterator& end)
{
    if (!copyAndAdvance(cpuSvn, from, end)) { return false; }
    if (!copyAndAdvance(miscSelect, from, end)) { return false; }
    if (!copyAndAdvance(reserved1, from, end)) { return false; }
    if (!copyAndAdvance(attributes, from, end)) { return false; }
    if (!copyAndAdvance(mrEnclave, from, end)) { return false; }
    if (!copyAndAdvance(reserved2, from, end)) { return false; }
    if (!copyAndAdvance(mrSigner, from, end)) { return false; }
    if (!copyAndAdvance(reserved3, from, end)) { return false; }
    if (!copyAndAdvance(isvProdID, from, end)) { return false; }
    if (!copyAndAdvance(isvSvn, from, end)) { return false; }
    if (!copyAndAdvance(reserved4, from, end)) { return false; }
    if (!copyAndAdvance(reportData, from, end)) { return false; }
    return true;
}

std::array<uint8_t,ENCLAVE_REPORT_BYTE_LEN> EnclaveReport::rawBlob() const
{
    std::array<uint8_t, ENCLAVE_REPORT_BYTE_LEN> ret{};
    auto to = ret.begin();
    std::copy(cpuSvn.begin(), cpuSvn.end(), to);
    std::advance(to, (unsigned) cpuSvn.size());

    const auto arrMiscSelect = toArray(swapBytes(miscSelect));
    std::copy(arrMiscSelect.begin(), arrMiscSelect.end(), to);
    std::advance(to, arrMiscSelect.size());

    std::copy(reserved1.begin(), reserved1.end(), to);
    std::advance(to, (unsigned) reserved1.size());

    std::copy(attributes.begin(), attributes.end(), to);
    std::advance(to, (unsigned) attributes.size());

    std::copy(mrEnclave.begin(), mrEnclave.end(), to);
    std::advance(to, (unsigned) mrEnclave.size());

    std::copy(reserved2.begin(), reserved2.end(), to);
    std::advance(to, (unsigned) reserved2.size());

    std::copy(mrSigner.begin(), mrSigner.end(), to);
    std::advance(to, (unsigned) mrSigner.size());

    std::copy(reserved3.begin(), reserved3.end(), to);
    std::advance(to, (unsigned) reserved3.size());

    const auto arrIsvProdId = toArray(swapBytes(isvProdID));
    std::copy(arrIsvProdId.begin(), arrIsvProdId.end(), to);
    std::advance(to, arrIsvProdId.size());

    const auto arrIsvSvn = toArray(swapBytes(isvSvn));
    std::copy(arrIsvSvn.begin(), arrIsvSvn.end(), to);
    std::advance(to, arrIsvSvn.size());

    std::copy(reserved4.begin(), reserved4.end(), to);
    std::advance(to, (unsigned) reserved4.size());

    std::copy(reportData.begin(), reportData.end(), to);
    std::advance(to, (unsigned) reportData.size());

    return ret;
}

bool TDReport::insert(std::vector<uint8_t>::const_iterator& from, const std::vector<uint8_t>::const_iterator& end)
{
    if (!copyAndAdvance(teeTcbSvn, from, end)) { return false; }
    if (!copyAndAdvance(mrSeam, from, end)) { return false; }
    if (!copyAndAdvance(mrSignerSeam, from, end)) { return false; }
    if (!copyAndAdvance(seamAttributes, from, end)) { return false; }
    if (!copyAndAdvance(tdAttributes, from, end)) { return false; }
    if (!copyAndAdvance(xFAM, from, end)) { return false; }
    if (!copyAndAdvance(mrTd, from, end)) { return false; }
    if (!copyAndAdvance(mrConfigId, from, end)) { return false; }
    if (!copyAndAdvance(mrOwner, from, end)) { return false; }
    if (!copyAndAdvance(mrOwnerConfig, from, end)) { return false; }
    if (!copyAndAdvance(rtMr0, from, end)) { return false; }
    if (!copyAndAdvance(rtMr1, from, end)) { return false; }
    if (!copyAndAdvance(rtMr2, from, end)) { return false; }
    if (!copyAndAdvance(rtMr3, from, end)) { return false; }
    if (!copyAndAdvance(reportData, from, end)) { return false; }
    return true;
}

std::array<uint8_t,TD_REPORT_BYTE_LEN> TDReport::rawBlob() const
{
    std::array<uint8_t, TD_REPORT_BYTE_LEN> ret{};
    auto to = ret.begin();

    std::copy(teeTcbSvn.begin(), teeTcbSvn.end(), to);
    std::advance(to, (unsigned) teeTcbSvn.size());

    std::copy(mrSeam.begin(), mrSeam.end(), to);
    std::advance(to, (unsigned) mrSeam.size());

    std::copy(mrSignerSeam.begin(), mrSignerSeam.end(), to);
    std::advance(to, (unsigned) mrSignerSeam.size());

    std::copy(seamAttributes.begin(), seamAttributes.end(), to);
    std::advance(to, (unsigned) seamAttributes.size());

    std::copy(tdAttributes.begin(), tdAttributes.end(), to);
    std::advance(to, (unsigned) tdAttributes.size());

    std::copy(xFAM.begin(), xFAM.end(), to);
    std::advance(to, (unsigned) xFAM.size());

    std::copy(mrTd.begin(), mrTd.end(), to);
    std::advance(to, (unsigned) mrTd.size());

    std::copy(mrConfigId.begin(), mrConfigId.end(), to);
    std::advance(to, (unsigned) mrConfigId.size());

    std::copy(mrOwner.begin(), mrOwner.end(), to);
    std::advance(to, (unsigned) mrOwner.size());

    std::copy(mrOwnerConfig.begin(), mrOwnerConfig.end(), to);
    std::advance(to, (unsigned) mrOwnerConfig.size());

    std::copy(rtMr0.begin(), rtMr0.end(), to);
    std::advance(to, (unsigned) rtMr0.size());

    std::copy(rtMr1.begin(), rtMr1.end(), to);
    std::advance(to, (unsigned) rtMr1.size());

    std::copy(rtMr2.begin(), rtMr2.end(), to);
    std::advance(to, (unsigned) rtMr2.size());

    std::copy(rtMr3.begin(), rtMr3.end(), to);
    std::advance(to, (unsigned) rtMr3.size());

    std::copy(reportData.begin(), reportData.end(), to);
    std::advance(to, (unsigned) reportData.size());

    return ret;
}

uint32_t TDReport::getSeamSvn() const
{
    return teeTcbSvn[0];
}

bool Ecdsa256BitSignature::insert(std::vector<uint8_t>::const_iterator& from, const std::vector<uint8_t>::const_iterator& end)
{
    return copyAndAdvance(signature, from, end);
}

bool Ecdsa256BitPubkey::insert(std::vector<uint8_t>::const_iterator& from, const std::vector<uint8_t>::const_iterator& end)
{
    return copyAndAdvance(pubKey, from, end);
}

bool QeAuthData::insert(std::vector<uint8_t>::const_iterator& from, const std::vector<uint8_t>::const_iterator& end)
{
    const auto amount = static_cast<size_t>(std::distance(from, end));
    if(from > end || amount < QE_AUTH_DATA_SIZE_BYTE_LEN)
    {
        return false;
    }

    this->data.clear();
    if (!copyAndAdvance(parsedDataSize, from, end))
    {
        return false;
    }

    if(parsedDataSize != amount - QE_AUTH_DATA_SIZE_BYTE_LEN)
    {
        // invalid format
        // moving back pointer
        from = std::prev(from, sizeof(decltype(parsedDataSize)));
        return false;
    }

    if(parsedDataSize == 0)
    {
        // all good, parsed size is zero
        // data are cleared and from is moved
        return true;
    }

    data.reserve(parsedDataSize);
    std::copy_n(from, parsedDataSize, std::back_inserter(data));
    std::advance(from, parsedDataSize);
    return true;
}

bool CertificationData::insert(std::vector<uint8_t>::const_iterator& from, const std::vector<uint8_t>::const_iterator& end)
{
    const auto minLen = CERTIFICATION_DATA_SIZE_BYTE_LEN + CERTIFICATION_DATA_TYPE_BYTE_LEN;
    const auto amount = static_cast<size_t>(std::distance(from, end));
    if(from > end || amount < minLen)
    {
        return false;
    }

    data.clear();
    if (!copyAndAdvance(type, from, end)) { return false; }
    if (!copyAndAdvance(parsedDataSize, from, end)) { return false; }
    if(parsedDataSize != amount - minLen)
    {
        // invalid format, moving back pointer
        from = std::prev(from, sizeof(decltype(type)) + sizeof(decltype(parsedDataSize)));
        return false;
    }

    if(parsedDataSize == 0)
    {
        // all good, parsed size is 0
        // data cleared and pointer moved
        return true;
    }

    data.reserve(parsedDataSize);
    std::copy_n(from, parsedDataSize, std::back_inserter(data));
    std::advance(from, parsedDataSize);
    return true;
}

bool QEReportCertificationData::insert(std::vector<uint8_t>::const_iterator& from, const std::vector<uint8_t>::const_iterator& end)
{
    if (!copyAndAdvance(qeReport, from, ENCLAVE_REPORT_BYTE_LEN, end)) { return false; }
    if (!copyAndAdvance(qeReportSignature, from, ECDSA_SIGNATURE_BYTE_LEN, end)) { return false; }

    uint16_t authSize = 0;
    if (!copyAndAdvance(authSize, from, end))
    {
        return false;
    }
    from = std::prev(from, sizeof(uint16_t));
    if (!copyAndAdvance(qeAuthData, from, authSize + sizeof(uint16_t), end))
    {
        return false;
    }

    uint32_t qeCertSize = 0;
    const auto available = std::distance(from, end);
    if (available < 0 || (unsigned) available < sizeof(uint16_t))
    {
        return false;
    }
    std::advance(from, sizeof(uint16_t)); // skip type
    if (!copyAndAdvance(qeCertSize, from, end))
    {
        return false;
    }
    from = std::prev(from, sizeof(uint32_t) + sizeof(uint16_t)); // go back to beg of struct data
    if (!copyAndAdvance(certificationData, from, qeCertSize + sizeof(uint16_t) + sizeof(uint32_t), end))
    {
        return false;
    }

    if (from != end)
    {
        return false; // Inconsistent structure
    }
    return true;
}

bool Ecdsa256BitQuoteV3AuthData::insert(std::vector<uint8_t>::const_iterator& from, const std::vector<uint8_t>::const_iterator& end)
{
    if (!copyAndAdvance(ecdsa256BitSignature, from, ECDSA_SIGNATURE_BYTE_LEN, end)) { return false; }
    if (!copyAndAdvance(ecdsaAttestationKey, from, ECDSA_PUBKEY_BYTE_LEN, end)) { return false; }
    if (!copyAndAdvance(qeReport, from, ENCLAVE_REPORT_BYTE_LEN, end)) { return false; }
    if (!copyAndAdvance(qeReportSignature, from, ECDSA_SIGNATURE_BYTE_LEN, end)) { return false; }

    uint16_t authSize = 0;
    if (!copyAndAdvance(authSize, from, end))
    {
        return false;
    }
    from = std::prev(from, sizeof(uint16_t));
    if (!copyAndAdvance(qeAuthData, from, authSize + sizeof(uint16_t), end))
    {
        return false;
    }

    uint32_t qeCertSize = 0;
    const auto available = std::distance(from, end);
    if (available < 0 || (unsigned) available < sizeof(uint16_t))
    {
        return false;
    }
    std::advance(from, sizeof(uint16_t)); // skip type
    if (!copyAndAdvance(qeCertSize, from, end))
    {
        return false;
    }
    from = std::prev(from, sizeof(uint32_t) + sizeof(uint16_t)); // go back to beg of struct data
    if (!copyAndAdvance(certificationData, from, qeCertSize + sizeof(uint16_t) + sizeof(uint32_t), end))
    {
        return false;
    }
    return true;
}

bool Ecdsa256BitQuoteV4AuthData::insert(std::vector<uint8_t>::const_iterator& from, const std::vector<uint8_t>::const_iterator& end)
{
    if (!copyAndAdvance(ecdsa256BitSignature, from, ECDSA_SIGNATURE_BYTE_LEN, end)) { return false; }
    if (!copyAndAdvance(ecdsaAttestationKey, from, ECDSA_PUBKEY_BYTE_LEN, end)) { return false; }

    uint32_t qeCertSize = 0;
    const auto available = std::distance(from, end);
    if (available < 0 || (unsigned) available < sizeof(uint16_t))
    {
        return false;
    }
    std::advance(from, sizeof(uint16_t)); // skip type
    if (!copyAndAdvance(qeCertSize, from, end))
    {
        return false;
    }
    from = std::prev(from, sizeof(uint32_t) + sizeof(uint16_t)); // go back to beg of struct data
    if (!copyAndAdvance(certificationData, from, qeCertSize + sizeof(uint16_t) + sizeof(uint32_t), end))
    {
        return false;
    }
    return true;
}

}}}}
