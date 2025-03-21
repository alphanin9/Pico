#include <Shared/EnvironmentIntegrity/SignatureVerifier.hpp>

namespace Detail
{
/**
 * \brief Utility function to close Wintrust data for driver verification properly.
 *
 * \param aData A pointer to the WINTRUST_DATA structure.
 */
void CloseWintrustDataDriver(WINTRUST_DATA* aData)
{
    static GUID policy = DRIVER_ACTION_VERIFY;

    aData->dwStateAction = WTD_STATEACTION_CLOSE;

    WinVerifyTrust((HWND)(INVALID_HANDLE_VALUE), &policy, aData);
}

/**
 * \brief Utility function to setup the WINTRUST_DATA structure.
 *
 * \param aData A reference to a WINTRUST_DATA structure.
 * \param aIsDriver Whether or not this call will verify a driver.
 * \param aIsCatalog Whether this call will verify a catalog or a file.
 */
void SetupGeneralWintrustStructure(WINTRUST_DATA& aData, pico::Bool aIsDriver = false, pico::Bool aIsCatalog = false)
{
    aData.cbStruct = sizeof(WINTRUST_DATA);
    aData.dwUIChoice = WTD_UI_NONE;
    aData.fdwRevocationChecks = 0u;
    aData.dwStateAction = WTD_STATEACTION_VERIFY;
    aData.dwUIContext = 0u;
    aData.dwUnionChoice = aIsCatalog ? WTD_CHOICE_CATALOG : WTD_CHOICE_FILE;
    aData.dwUIContext = WTD_UICONTEXT_EXECUTE;

    // Make things a little faster, don't verify EVERYTHING with web
    aData.dwProvFlags |= WTD_CACHE_ONLY_URL_RETRIEVAL;

    if (aIsDriver)
    {
        aData.dwProvFlags |= WTD_CODE_INTEGRITY_DRIVER_MODE;
    }
}

using WintrustDriverData =
    wil::unique_struct<WINTRUST_DATA, decltype(&CloseWintrustDataDriver), CloseWintrustDataDriver>;

static GUID s_driverPolicy = DRIVER_ACTION_VERIFY;
static GUID s_defaultPolicy = WINTRUST_ACTION_GENERIC_VERIFY_V2;
} // namespace Detail

pico::Bool pico::shared::EnvironmentIntegrity::VerifyFileTrustFromCatalog(pico::UnicodeStringView aPath,
                                                                          EFileType aType)
{
    auto& policy = aType == EFileType::Other ? Detail::s_defaultPolicy : Detail::s_driverPolicy;

    wil::shared_hcatadmin catalogAdmin{};

    if (!CryptCATAdminAcquireContext2(catalogAdmin.addressof(), &policy, nullptr, nullptr, 0u))
    {
        return false;
    }

    auto [fileHandle, errorCode] = wil::try_open_file(aPath.data());

    if (!fileHandle.is_valid())
    {
        return false;
    }

    pico::Uint32 hashSize{};

    CryptCATAdminCalcHashFromFileHandle2(catalogAdmin.get(), fileHandle.get(), (DWORD*)(&hashSize),
                                         nullptr, 0u);

    pico::Vector<pico::Uint8> hashBuffer(hashSize, {});

    if (!CryptCATAdminCalcHashFromFileHandle2(catalogAdmin.get(), fileHandle.get(), (DWORD*)(&hashSize),
                                              hashBuffer.data(), 0u))
    {
        return false;
    }

    auto hadMatchingCatalogs = false;

    for (auto info :
         wil::make_crypt_catalog_enumerator(catalogAdmin, hashBuffer.data(), (DWORD)(hashBuffer.size())))
    {
        auto catalogRef = info.move_from_unique_hcatinfo();

        if (catalogRef)
        {
            CATALOG_INFO catInfo{};

            if (CryptCATCatalogInfoFromContext(catalogRef.get(), &catInfo, 0u))
            {
                hadMatchingCatalogs = true;
                WINTRUST_CATALOG_INFO catalogTrustInfo{};

                catalogTrustInfo.cbStruct = sizeof(WINTRUST_CATALOG_INFO);
                catalogTrustInfo.pcwszCatalogFilePath = catInfo.wszCatalogFile;
                catalogTrustInfo.pcwszMemberFilePath = aPath.data();
                catalogTrustInfo.hMemberFile = fileHandle.get();
                catalogTrustInfo.pbCalculatedFileHash = hashBuffer.data();
                catalogTrustInfo.cbCalculatedFileHash = (DWORD)(hashBuffer.size());
                catalogTrustInfo.hCatAdmin = catalogAdmin.get();

                switch (aType)
                {
                case EFileType::Other:
                {
                    wil::unique_wintrust_data data{};

                    Detail::SetupGeneralWintrustStructure(*data.addressof(), false, true);

                    data.pCatalog = &catalogTrustInfo;

                    if (WinVerifyTrust((HWND)(INVALID_HANDLE_VALUE), &policy, data.addressof()) != 0)
                    {
                        return false;
                    }

                    break;
                }
                case EFileType::Driver:
                {
                    Detail::WintrustDriverData data{};

                    Detail::SetupGeneralWintrustStructure(*data.addressof(), true, true);

                    data.pCatalog = &catalogTrustInfo;

                    if (WinVerifyTrust((HWND)(INVALID_HANDLE_VALUE), &policy, data.addressof()) != 0)
                    {
                        return false;
                    }

                    break;
                }
                }
            }
        }
    }

    return hadMatchingCatalogs;
}

pico::Bool pico::shared::EnvironmentIntegrity::VerifyFileTrust(pico::UnicodeStringView aPath, EFileType aType)
{
    WINTRUST_FILE_INFO fileInfo{};

    fileInfo.cbStruct = sizeof(fileInfo);
    fileInfo.pcwszFilePath = aPath.data();

    switch (aType)
    {
    case EFileType::Other:
    {
        wil::unique_wintrust_data data{};

        Detail::SetupGeneralWintrustStructure(*data.addressof());

        data.pFile = &fileInfo;

        const auto trustStatus =
            WinVerifyTrust((HWND)(INVALID_HANDLE_VALUE), &Detail::s_defaultPolicy, data.addressof());

        // Maybe a catalog has the signature?
        return trustStatus == 0u || VerifyFileTrustFromCatalog(aPath, aType);
    }
    case EFileType::Driver:
    {
        Detail::WintrustDriverData data{};

        Detail::SetupGeneralWintrustStructure(*data.addressof(), true);

        data.pFile = &fileInfo;

        const auto trustStatus =
            WinVerifyTrust((HWND)(INVALID_HANDLE_VALUE), &Detail::s_driverPolicy, data.addressof());

        // Maybe a catalog has the signature?
        return trustStatus == 0u || VerifyFileTrustFromCatalog(aPath, aType);
    }
    }

    return false;
}
