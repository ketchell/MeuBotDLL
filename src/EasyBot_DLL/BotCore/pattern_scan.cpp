#include "Pattern_Scan.h"

uintptr_t FindPattern(const BYTE* lpPattern, LPCSTR szMask)
{
    HMODULE hModule = GetModuleHandle(NULL);
    MODULEINFO moduleInfo = { 0 };
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo)))
        return NULL;

    BYTE* image_base = (BYTE*)moduleInfo.lpBaseOfDll;
    DWORD image_size = moduleInfo.SizeOfImage;
    size_t pattern_length = strlen(szMask);

    BYTE* image_end = image_base + image_size - pattern_length;

    for (BYTE* addr = image_base; addr < image_end; ++addr)
    {
        size_t i = 0;
        for (; i < pattern_length; ++i)
        {
            if (szMask[i] != '?' && addr[i] != lpPattern[i])
                break;
        }
        if (i == pattern_length)
            return reinterpret_cast<uintptr_t>(addr);
    }
    return NULL;
}


std::vector<BYTE> HexToBytes(const std::string& hex)
{
    std::vector<BYTE> bytes;
    for (size_t i = 0; i < hex.length(); )
    {
        while (i < hex.length() && isspace(hex[i])) {
            i++;
        }
        if (i >= hex.length()) {
            break;
        }
        try {
            std::string byteString = hex.substr(i, 2);
            if (byteString.length() != 2 || !isxdigit(byteString[0]) || !isxdigit(byteString[1])) {
                i++;
                continue;
            }
            BYTE b = static_cast<BYTE>(std::stoul(byteString, nullptr, 16));
            bytes.push_back(b);
            i += 2;
        }
        catch (const std::out_of_range&) {
            break;
        }
    }
    return bytes;
}

