// Hasher.h — хэширование паролей SHA-256
#pragma once
#include <string>
#include <windows.h>
#include <wincrypt.h>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "advapi32.lib")

namespace Utils {
    class Hasher {
    public:
        static std::wstring hashSha256(const std::wstring& password,
            const std::wstring& salt) {
            std::wstring data = password + salt;

            HCRYPTPROV hProv = 0;
            HCRYPTHASH hHash = 0;

            if (!CryptAcquireContext(&hProv, nullptr, nullptr,
                PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
                return L"";
            }

            if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
                CryptReleaseContext(hProv, 0);
                return L"";
            }

            std::string dataStr(data.begin(), data.end());
            if (!CryptHashData(hHash, (BYTE*)dataStr.c_str(),
                (DWORD)dataStr.length(), 0)) {
                CryptDestroyHash(hHash);
                CryptReleaseContext(hProv, 0);
                return L"";
            }

            DWORD hashLen = 32;
            BYTE hash[32];
            if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
                CryptDestroyHash(hHash);
                CryptReleaseContext(hProv, 0);
                return L"";
            }

            std::wstringstream ss;
            for (DWORD i = 0; i < hashLen; i++) {
                ss << std::hex << std::setw(2) << std::setfill(L'0')
                    << (int)hash[i];
            }

            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);

            return ss.str();
        }

        static std::wstring generateSalt() {
            HCRYPTPROV hProv = 0;
            if (!CryptAcquireContext(&hProv, nullptr, nullptr,
                PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
                return L"default_salt";
            }

            BYTE salt[16];
            CryptGenRandom(hProv, 16, salt);
            CryptReleaseContext(hProv, 0);

            std::wstringstream ss;
            for (int i = 0; i < 16; i++) {
                ss << std::hex << std::setw(2) << std::setfill(L'0')
                    << (int)salt[i];
            }
            return ss.str();
        }
    };
}
