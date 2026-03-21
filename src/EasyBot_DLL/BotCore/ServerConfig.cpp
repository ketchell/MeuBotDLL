#include "ServerConfig.h"

ServerConfig g_serverConfig;

static const ServerConfig g_servers[] = {
    // name        classFn  singletonFn  onTextMsg  onTalk  lightHack
    { "Miracle",  0x50,    0x10,        0x14,      0x10,   0xAC },
    { "Realera",  0x0C,    0x10,        0x14,      0x10,   0xAC },
    { "Dbwots",   0x50,    0x10,        0x14,      0x10,   0xAC },
    { "Outcast",  0x50,    0x10,        0x14,      0x10,   0xAC },
    { "Thirus",   0x50,    0x10,        0x14,      0x10,   0xAC },
    { "Exordion", 0x58,    0x10,        0x14,      0x10,   0xB0 },
    { "Yurevo",   0x50,    0x10,        0x14,      0x10,   0xAC },
    { "Dbl",      0x50,    0x10,        0x14,      0x10,   0xAC },
    { "Treasura", 0x50,    0x10,        0x14,      0x10,   0xAC },
};

static const int g_serverCount = _countof(g_servers);
static int s_selected = 0;

static LRESULT CALLBACK ServerSelectProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hList = NULL;
    switch (msg) {
    case WM_CREATE:
        hList = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            8, 8, 234, 190, hwnd, (HMENU)100, GetModuleHandleA(NULL), NULL);
        for (int i = 0; i < g_serverCount; i++)
            SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)g_servers[i].name);
        SendMessage(hList, LB_SETCURSEL, 0, 0);
        CreateWindowExA(0, "BUTTON", "OK",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            80, 208, 90, 24, hwnd, (HMENU)IDOK, GetModuleHandleA(NULL), NULL);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK ||
            (LOWORD(wParam) == 100 && HIWORD(wParam) == LBN_DBLCLK)) {
            int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
            s_selected = (sel == LB_ERR) ? 0 : sel;
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void ShowServerSelectionDialog() {
    WNDCLASSEXA wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = ServerSelectProc;
    wc.hInstance     = GetModuleHandleA(NULL);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "EasyBotServerSel";
    RegisterClassExA(&wc);

    HWND hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_DLGMODALFRAME,
        "EasyBotServerSel",
        "EasyBot - Select Server",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        0, 0, 252, 248,
        NULL, NULL, GetModuleHandleA(NULL), NULL);

    // Center on screen
    RECT rc;
    GetWindowRect(hwnd, &rc);
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(hwnd, NULL,
        (sw - (rc.right - rc.left)) / 2,
        (sh - (rc.bottom - rc.top)) / 2,
        0, 0, SWP_NOZORDER | SWP_NOSIZE);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    g_serverConfig = g_servers[s_selected];
    UnregisterClassA("EasyBotServerSel", GetModuleHandleA(NULL));
}
