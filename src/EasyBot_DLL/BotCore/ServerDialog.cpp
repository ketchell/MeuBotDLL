#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif
#include "ServerDialog.h"
#include "ServerProfile.h"
#include <windows.h>
#include <cstring>
#include <cstdio>

// ---- Control IDs ----
#define IDC_LIST        101
#define IDC_EDIT_NAME   102
#define IDC_BTN_OK      103
#define IDC_BTN_ADD     104
#define IDC_BTN_UPDATE  105
#define IDC_LABEL_NEW   106

// ---- Dialog state (passed via GWLP_USERDATA) ----
struct DialogState {
    ServerDialogOutput* out;
    bool addMode;   // true when the user clicked "Add New Server"
};

static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    DialogState* state = reinterpret_cast<DialogState*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_INITDIALOG: {
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, lp);
        state = reinterpret_cast<DialogState*>(lp);

        // Populate listbox with saved server names
        char names[32][64] = {};
        int count = 0;
        ListServerProfiles(names, &count, 32);
        HWND list = GetDlgItem(hwnd, IDC_LIST);
        for (int i = 0; i < count; i++)
            SendMessageA(list, LB_ADDSTRING, 0, (LPARAM)names[i]);
        if (count > 0)
            SendMessageA(list, LB_SETCURSEL, 0, 0);

        // Hide the new-name edit and its label initially
        ShowWindow(GetDlgItem(hwnd, IDC_EDIT_NAME), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDC_LABEL_NEW), SW_HIDE);

        // Center on screen
        RECT rc; GetWindowRect(hwnd, &rc);
        int w = rc.right  - rc.left;
        int h = rc.bottom - rc.top;
        int sw = GetSystemMetrics(SM_CXSCREEN);
        int sh = GetSystemMetrics(SM_CYSCREEN);
        SetWindowPos(hwnd, HWND_TOPMOST, (sw - w) / 2, (sh - h) / 2, 0, 0,
                     SWP_NOSIZE);
        return TRUE;
    }

    case WM_COMMAND: {
        int id = LOWORD(wp);

        if (id == IDC_BTN_ADD) {
            // Toggle into "add new" mode — show text input
            state->addMode = true;
            ShowWindow(GetDlgItem(hwnd, IDC_EDIT_NAME), SW_SHOW);
            ShowWindow(GetDlgItem(hwnd, IDC_LABEL_NEW), SW_SHOW);
            SetFocus(GetDlgItem(hwnd, IDC_EDIT_NAME));
            // Disable Update since we're in add mode
            EnableWindow(GetDlgItem(hwnd, IDC_BTN_UPDATE), FALSE);
            return TRUE;
        }

        if (id == IDC_BTN_UPDATE) {
            HWND list = GetDlgItem(hwnd, IDC_LIST);
            int sel = (int)SendMessageA(list, LB_GETCURSEL, 0, 0);
            if (sel == LB_ERR) {
                MessageBoxA(hwnd, "Please select a server first.", "EasyBot", MB_OK | MB_ICONWARNING);
                return TRUE;
            }
            char name[64] = {};
            SendMessageA(list, LB_GETTEXT, sel, (LPARAM)name);
            strncpy_s(state->out->serverName, name, 63);
            state->out->result = DIALOG_UPDATE;
            EndDialog(hwnd, DIALOG_UPDATE);
            return TRUE;
        }

        if (id == IDC_BTN_OK || id == IDOK) {
            if (state->addMode) {
                // Read from text box
                char name[64] = {};
                GetDlgItemTextA(hwnd, IDC_EDIT_NAME, name, sizeof(name));
                if (name[0] == '\0') {
                    MessageBoxA(hwnd, "Please enter a server name.", "EasyBot", MB_OK | MB_ICONWARNING);
                    return TRUE;
                }
                strncpy_s(state->out->serverName, name, 63);
                state->out->result = DIALOG_ADD_NEW;
                EndDialog(hwnd, DIALOG_ADD_NEW);
            } else {
                HWND list = GetDlgItem(hwnd, IDC_LIST);
                int sel = (int)SendMessageA(list, LB_GETCURSEL, 0, 0);
                if (sel == LB_ERR) {
                    // No saved servers — treat as add new
                    MessageBoxA(hwnd, "No saved servers found.\nEnter a name to add a new server.", "EasyBot", MB_OK | MB_ICONINFORMATION);
                    state->addMode = true;
                    ShowWindow(GetDlgItem(hwnd, IDC_EDIT_NAME), SW_SHOW);
                    ShowWindow(GetDlgItem(hwnd, IDC_LABEL_NEW), SW_SHOW);
                    SetFocus(GetDlgItem(hwnd, IDC_EDIT_NAME));
                    EnableWindow(GetDlgItem(hwnd, IDC_BTN_UPDATE), FALSE);
                    return TRUE;
                }
                char name[64] = {};
                SendMessageA(list, LB_GETTEXT, sel, (LPARAM)name);
                strncpy_s(state->out->serverName, name, 63);
                state->out->result = DIALOG_OK;
                EndDialog(hwnd, DIALOG_OK);
            }
            return TRUE;
        }

        if (id == IDCANCEL) {
            state->out->result = DIALOG_CANCEL;
            state->out->serverName[0] = '\0';
            EndDialog(hwnd, DIALOG_CANCEL);
            return TRUE;
        }
        break;
    }

    case WM_CLOSE:
        state->out->result = DIALOG_CANCEL;
        state->out->serverName[0] = '\0';
        EndDialog(hwnd, DIALOG_CANCEL);
        return TRUE;
    }

    return FALSE;
}

// Build the dialog template in memory — no .rc file needed.
// Layout (all coords in dialog units):
//   [10,10] Label "Saved servers:"
//   [10,22] Listbox 180x80
//   [10,108] Label "New server name:"  (hidden initially)
//   [10,120] Edit 180x14              (hidden initially)
//   [10,140] [Add New Server] [Update] [OK]
static HGLOBAL BuildDialogTemplate() {
    // We use a DLGTEMPLATE with DLGITEMTEMPLATE entries in a raw buffer.
    // Each control entry must be DWORD-aligned.
    struct Buf {
        char data[2048];
        int  pos;
        void align4() { pos = (pos + 3) & ~3; }
        void write(const void* p, int n) { memcpy(data + pos, p, n); pos += n; }
        void write16(uint16_t v) { write(&v, 2); }
        void write32(uint32_t v) { write(&v, 4); }
        void writeStr(const wchar_t* s) {
            // null-terminated wide string
            int n = (int)(wcslen(s) + 1) * 2;
            write(s, n);
        }
        void writeClass(uint16_t cls) {
            write16(0xFFFF); write16(cls);
        }
    } buf{};

    // DLGTEMPLATE
    DLGTEMPLATE dt{};
    dt.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER | DS_SETFONT;
    dt.cdit  = 7;  // number of controls
    dt.x = 0; dt.y = 0; dt.cx = 200; dt.cy = 165;
    buf.write(&dt, sizeof(dt));
    buf.write16(0); // menu: none
    buf.write16(0); // class: predefined
    buf.writeStr(L"EasyBot - Server Select");  // title
    // Font (required by DS_SETFONT)
    buf.write16(8);          // point size
    buf.writeStr(L"MS Shell Dlg");

    auto addCtrl = [&](int x, int y, int cx, int cy, int id,
                        DWORD style, uint16_t cls, const wchar_t* text) {
        buf.align4();
        DLGITEMTEMPLATE it{};
        it.style = WS_CHILD | WS_VISIBLE | style;
        it.x = (short)x; it.y = (short)y;
        it.cx = (short)cx; it.cy = (short)cy;
        it.id = (WORD)id;
        buf.write(&it, sizeof(it));
        buf.writeClass(cls);
        buf.writeStr(text);
        buf.write16(0); // no extra data
    };

    // Static label "Saved servers:"
    addCtrl(10, 10,  180, 8,  IDC_STATIC,    SS_LEFT,             0x0082, L"Saved servers:");
    // Listbox
    addCtrl(10, 20,  180, 78, IDC_LIST,      LBS_NOTIFY | WS_BORDER | WS_VSCROLL, 0x0083, L"");
    // Static "New server name:" (hidden — WS_VISIBLE removed at init)
    addCtrl(10, 103, 180, 8,  IDC_LABEL_NEW, SS_LEFT,             0x0082, L"New server name:");
    // Edit box (hidden initially)
    addCtrl(10, 113, 180, 14, IDC_EDIT_NAME, WS_BORDER | ES_AUTOHSCROLL, 0x0081, L"");
    // Buttons
    addCtrl(10,  133, 58, 14, IDC_BTN_ADD,    BS_PUSHBUTTON, 0x0080, L"Add New Server");
    addCtrl(72,  133, 58, 14, IDC_BTN_UPDATE, BS_PUSHBUTTON, 0x0080, L"Update Server");
    addCtrl(134, 133, 56, 14, IDC_BTN_OK,     BS_DEFPUSHBUTTON, 0x0080, L"OK");

    HGLOBAL h = GlobalAlloc(GMEM_ZEROINIT, buf.pos);
    memcpy(GlobalLock(h), buf.data, buf.pos);
    GlobalUnlock(h);
    return h;
}

ServerDialogOutput ShowServerDialog() {
    ServerDialogOutput out{};
    out.result = DIALOG_CANCEL;

    DialogState state{ &out, false };

    HGLOBAL tmpl = BuildDialogTemplate();
    DialogBoxIndirectParamA(
        GetModuleHandleA(NULL),
        reinterpret_cast<LPCDLGTEMPLATE>(GlobalLock(tmpl)),
        NULL,
        DialogProc,
        reinterpret_cast<LPARAM>(&state));
    GlobalUnlock(tmpl);
    GlobalFree(tmpl);

    return out;
}
