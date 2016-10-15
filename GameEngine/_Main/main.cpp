#include "stdafx.h"
#include "ErrorHandler.h"
#include "Core.h"
#include "GameOGL.h"

int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPTSTR lpCmdLine, int nShowCmd) {

    TCHAR szGameName[] = TEXT("Name of Game");

    // prevent multiple start ups
    HANDLE hMutex;
    CoInitialize(NULL);
    hMutex = CreateMutex(NULL, FALSE, szGameName);
    if (hMutex == NULL || (GetLastError() == ERROR_ALREADY_EXISTS)) {
        CloseHandle(hMutex);
        MessageBox(NULL, TEXT("Application has already been\n"
            "started in a different window."),
            TEXT("Application Already Running"), MB_ICONWARNING | MB_OK);
        return 1;
    }

    // game code
    int iReturn = 0;

    // Error handler
    ErrorHandler errorHandler(_T("Log.txt"));

    // Get all window parameters to use;
    WndParam wndParam(szGameName, hInst);
    wndParam.iWidth = 800;
    wndParam.iHeight = 600;

    // Main game window interface
    GameOGL myGame(&errorHandler);

    // Unique start of this game
    if (myGame.Create(&wndParam)) {
        // Start the game
        iReturn = myGame.StartMessageLoop();
    } else {
        errorHandler.ShowErrorMessage();
        iReturn = -1;
    }

    // close this unique program to allow it to restart again.
    CloseHandle(hMutex);
    CoUninitialize();
    return iReturn;
} // WinMain
