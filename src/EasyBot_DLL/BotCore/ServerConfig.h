#pragma once
#include <Windows.h>

struct ServerConfig {
    const char* name;
    uintptr_t classFunctionOffset;
    uintptr_t singletonFunctionOffset;
    uintptr_t onTextMessageOffset;
    uintptr_t onTalkOffset;
    uintptr_t lightHackOffset;
};

extern ServerConfig g_serverConfig;

void ShowServerSelectionDialog();
