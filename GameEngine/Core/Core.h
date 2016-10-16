#ifndef CORE_H
#define CORE_H

#include "ErrorHandler.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) { delete (p); (p) = NULL; }
#endif

struct WndParam {
    PTCH pszTitle;
    INT iWidth;
    INT iHeight;
    bool bIsFullScreen;
    INT iRefreshRate;
    bool bWindowed;
    bool b32Bit;
    bool bZBuffer;

    HINSTANCE hInst;

    WndParam(PTCH szTitle, HINSTANCE hInst)
        : pszTitle(szTitle),
        iWidth(0),
        iHeight(0),
        bIsFullScreen(false),
        iRefreshRate(60),
        bWindowed(true),
        b32Bit(true),
        bZBuffer(true),
        hInst(hInst) {

    }
};

enum GameState {
    GS_STARTING = 0,
    GS_MENU,
    GS_LOADING,
    GS_PLAY,
    GS_PAUSE,
    GS_STOP
};

#endif // !CORE_H
