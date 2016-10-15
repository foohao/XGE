#include "stdafx.h"
#include "GameOGL.h"

GameOGL::GameOGL(ErrorHandler * pErrorHandler)
    : m_bQuit(false),
    m_bActive(true),
    m_GameState(GS_STOP),

    m_pWndParam(NULL),

    m_fFPS(60),
    m_dTimeGame(0),

    m_hGLRC(NULL),

    m_pErrorHandler(pErrorHandler) {

    // Define function pointer to GetDeltaTime
    long long lPerformanceFrequency;
    if (QueryPerformanceFrequency((LARGE_INTEGER*)&lPerformanceFrequency)) {
        QueryPerformanceCounter((LARGE_INTEGER*)&m_lPrevTime);
        m_dTimeScaleFactor = 1.0 / lPerformanceFrequency;
        GetDeltaTime = GameOGL::GetTimePerformanceHigh;
    } else {
        m_lPrevTime = timeGetTime();
        m_dTimeScaleFactor = 0.001;
        GetDeltaTime = GameOGL::GetTimePerformanceRegular;
    }
}

GameOGL::~GameOGL() {
    DestroyWnd();
}

bool GameOGL::Create(WndParam * pWndParam) {

    if (pWndParam->iHeight <= 0 || pWndParam->iWidth <= 0 || pWndParam->iRefreshRate <= 0) {
        m_pErrorHandler->SetError(EC_Error, _T("Invalid parameters\n"
            "Width        = %d\n"
            "Height       = %d\n"
            "Refresh Rate = %d\n"), pWndParam->iWidth, pWndParam->iHeight, pWndParam->iRefreshRate);
        return false;
    }

    m_pWndParam = pWndParam;
    m_fFPS = pWndParam->iRefreshRate;

    // Register window class
    m_WndClassEx.cbSize = sizeof(WNDCLASSEX);
    m_WndClassEx.style = CS_HREDRAW | CS_VREDRAW | CS_CLASSDC | CS_DBLCLKS;
    m_WndClassEx.lpfnWndProc = WndProc;
    m_WndClassEx.cbClsExtra = 0;
    m_WndClassEx.cbWndExtra = 0;
    m_WndClassEx.hInstance = pWndParam->hInst;
    m_WndClassEx.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    m_WndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
    m_WndClassEx.hbrBackground = NULL;
    m_WndClassEx.lpszMenuName = NULL;
    m_WndClassEx.lpszClassName = pWndParam->pszTitle;
    m_WndClassEx.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    if (!RegisterClassEx(&m_WndClassEx)) {
        pWndParam->hInst = NULL;
        m_pErrorHandler->SetError(EC_Windows, _T("Cannot register %s with Windows"), m_WndClassEx.lpszClassName);
        return false;
    }

    DWORD dwStyle, dwExStyle;
    if (pWndParam->bIsFullScreen) {
        dwStyle = WS_POPUP | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX;
        dwExStyle = WS_EX_APPWINDOW;
    } else {
        dwStyle = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    }

    RECT rcWnd;
    rcWnd.left = 0;
    rcWnd.right = pWndParam->iWidth;
    rcWnd.top = 0;
    rcWnd.bottom = pWndParam->iHeight;
    AdjustWindowRectEx(&rcWnd, dwStyle, FALSE, dwExStyle);

    pWndParam->hWnd = CreateWindowEx(dwExStyle,
        m_WndClassEx.lpszClassName,
        m_WndClassEx.lpszClassName,
        dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0, 0,
        rcWnd.right - rcWnd.left,
        rcWnd.bottom - rcWnd.top,
        NULL,
        NULL,
        pWndParam->hInst,
        (LPVOID)this);

    if (pWndParam->hWnd == NULL) {
        DestroyWnd();
        m_pErrorHandler->SetError(EC_Windows, _T("Cannot create window."));
        return false;
    }

    pWndParam->hDC = GetDC(pWndParam->hWnd);

    if (pWndParam->hDC == NULL) {
        DestroyWnd();
        m_pErrorHandler->SetError(EC_Windows, _T("Could not get window's DC."));
        return false;
    }

    // Center window onscreen if it is a window
    if (pWndParam->bWindowed) {
        int cx = GetSystemMetrics(SM_CXSCREEN);
        int cy = GetSystemMetrics(SM_CYSCREEN);
        int px = 0;
        int py = 0;

        if (cx > pWndParam->iWidth) {
            px = int((cx - pWndParam->iWidth) / 2);
        }
        if (cy > pWndParam->iHeight) {
            py = int((cy - pWndParam->iHeight) / 2);
        }
        MoveWindow(pWndParam->hWnd, px, py, pWndParam->iWidth, pWndParam->iHeight, FALSE);
    }

    // Define pixel format
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = pWndParam->b32Bit ? 32 : 16;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int iPixelFormat = ChoosePixelFormat(pWndParam->hDC, &pfd);
    if (!iPixelFormat) {
        DestroyWnd();
        m_pErrorHandler->SetError(EC_OpenGL, _T("Could not find suitable OpenGL pixel format to use."));
        return false;
    }

    if (!SetPixelFormat(pWndParam->hDC, iPixelFormat, &pfd)) {

    }

    return true;
}

double GameOGL::GetTimePerformanceHigh(void) {
    long long lCurrentTime;
    double dDeltaTime;

    QueryPerformanceCounter((LARGE_INTEGER*)&lCurrentTime);
    dDeltaTime = (lCurrentTime - m_lPrevTime) * m_dTimeScaleFactor;
    m_lPrevTime = lCurrentTime;

    return dDeltaTime;
}

double GameOGL::GetTimePerformanceRegular(void) {
    long long lCurrnetTime;
    double dDeltaTime;

    lCurrnetTime = timeGetTime();
    dDeltaTime = (lCurrnetTime - m_lPrevTime) * m_dTimeScaleFactor;
    m_lPrevTime = lCurrnetTime;

    return dDeltaTime;
}
