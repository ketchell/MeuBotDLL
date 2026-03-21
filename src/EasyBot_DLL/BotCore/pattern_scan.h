#ifndef PATTERN_SCAN_H
#define PATTERN_SCAN_H
#include <Windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <psapi.h>

uintptr_t FindPattern(const BYTE* lpPattern, LPCSTR szMask);
std::vector<BYTE> HexToBytes(const std::string& hex);



#endif //PATTERN_SCAN_H
