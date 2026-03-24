#include "ServerProfile.h"
#include <windows.h>
#include <cstdio>
#include <cstring>

static const uint32_t MAGIC   = SERVER_PROFILE_MAGIC;
static const uint32_t VERSION = SERVER_PROFILE_VERSION;

void GetProfileDir(char* out, int outSize) {
    snprintf(out, outSize, "C:\\EasyBot\\profiles");
    CreateDirectoryA("C:\\EasyBot", NULL);
    CreateDirectoryA("C:\\EasyBot\\profiles", NULL);
}

static void ProfilePath(const char* serverName, char* out, int outSize) {
    char dir[MAX_PATH];
    GetProfileDir(dir, sizeof(dir));
    snprintf(out, outSize, "%s\\%s.dat", dir, serverName);
}

bool LoadServerProfile(const char* serverName, ServerProfile* out) {
    char path[MAX_PATH];
    ProfilePath(serverName, path, sizeof(path));

    FILE* f = fopen(path, "rb");
    if (!f) return false;

    ServerProfile tmp{};
    bool ok = (fread(&tmp, sizeof(tmp), 1, f) == 1);
    fclose(f);

    if (!ok) return false;
    if (tmp.magic   != MAGIC)   return false;
    if (tmp.version != VERSION) return false;

    *out = tmp;
    return true;
}

bool SaveServerProfile(const ServerProfile* profile) {
    // Ensure dir exists
    char dir[MAX_PATH];
    GetProfileDir(dir, sizeof(dir));

    char path[MAX_PATH];
    ProfilePath(profile->serverName, path, sizeof(path));

    // Write to a temp file first, then rename — avoids corrupt profiles on crash
    char tmp[MAX_PATH];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    FILE* f = fopen(tmp, "wb");
    if (!f) return false;

    // Stamp magic/version in case caller forgot
    ServerProfile copy = *profile;
    copy.magic   = MAGIC;
    copy.version = VERSION;

    bool ok = (fwrite(&copy, sizeof(copy), 1, f) == 1);
    fclose(f);

    if (!ok) { DeleteFileA(tmp); return false; }

    // Atomic replace
    DeleteFileA(path);
    return MoveFileA(tmp, path) != 0;
}

void ListServerProfiles(char names[][64], int* count, int maxCount) {
    *count = 0;
    char dir[MAX_PATH];
    GetProfileDir(dir, sizeof(dir));

    char pattern[MAX_PATH];
    snprintf(pattern, sizeof(pattern), "%s\\*.dat", dir);

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;

    do {
        if (*count >= maxCount) break;
        // Strip the .dat extension
        char name[64];
        strncpy_s(name, fd.cFileName, sizeof(name) - 1);
        char* dot = strrchr(name, '.');
        if (dot) *dot = '\0';
        strncpy_s(names[*count], name, 63);
        (*count)++;
    } while (FindNextFileA(h, &fd));

    FindClose(h);
}
