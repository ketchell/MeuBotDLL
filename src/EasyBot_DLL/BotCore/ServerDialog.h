#pragma once
#include <cstdint>

enum ServerDialogResult {
    DIALOG_OK,        // user selected an existing server and clicked OK
    DIALOG_ADD_NEW,   // user typed a new name and clicked Add New Server
    DIALOG_UPDATE,    // user selected an existing server and clicked Update
    DIALOG_CANCEL     // user closed the dialog
};

struct ServerDialogOutput {
    ServerDialogResult result;
    char serverName[64];  // selected or entered name (empty on CANCEL)
};

// Shows a modal Win32 dialog.  Blocks until the user makes a choice.
// Must be called from a thread that can run a message loop (any non-GUI thread
// is fine — we create the window ourselves).
ServerDialogOutput ShowServerDialog();
