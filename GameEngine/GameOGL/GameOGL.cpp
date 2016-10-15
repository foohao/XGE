#include "stdafx.h"
#include "GameOGL.h"

#define SET_ERROR_AND_RETURN_FALSE(EC, EMF, ...) \
    do { m_pErrorHandler->SetError(EC, _T(EMF), __VA_ARGS__); return false; } while(false)

#define DESTROY_SET_ERROR_RETURN_FALSE(EC, EMF, ...) \
    DestroyWnd(); \
    SET_ERROR_AND_RETURN_FALSE(EC, EMF, __VA_ARGS__)

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
        SET_ERROR_AND_RETURN_FALSE(EC_Error, "Invalid parameters\n"
            "Width        = %d\n"
            "Height       = %d\n"
            "Refresh Rate = %d\n", pWndParam->iWidth, pWndParam->iHeight, pWndParam->iRefreshRate);
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
        SET_ERROR_AND_RETURN_FALSE(EC_Windows, "Cannot register %s with Windows", m_WndClassEx.lpszClassName);
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
        DESTROY_SET_ERROR_RETURN_FALSE(EC_Windows, "Cannot create window.");
    }

    pWndParam->hDC = GetDC(pWndParam->hWnd);

    if (pWndParam->hDC == NULL) {
        DESTROY_SET_ERROR_RETURN_FALSE(EC_Windows, "Could not get window's DC.");
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
        DESTROY_SET_ERROR_RETURN_FALSE(EC_OpenGL, "Could not find suitable OpenGL pixel format to use.");
    }

    if (!SetPixelFormat(pWndParam->hDC, iPixelFormat, &pfd)) {
        DESTROY_SET_ERROR_RETURN_FALSE(EC_OpenGL, "Could not set OpenGL pixel format.");
    }

    m_hGLRC = wglCreateContext(pWndParam->hDC);
    if (m_hGLRC = NULL) {
        DESTROY_SET_ERROR_RETURN_FALSE(EC_OpenGL, "Could not create OpenGL context.");
    }
    if (!wglMakeCurrent(pWndParam->hDC, m_hGLRC)) {
        DESTROY_SET_ERROR_RETURN_FALSE(EC_OpenGL, "Could not make current OpenGL context.");
    }

    // Obtain detailed information about the device context's pixel format
    if (!DescribePixelFormat(pWndParam->hDC, iPixelFormat, sizeof(pfd), &pfd)) {
        DESTROY_SET_ERROR_RETURN_FALSE(EC_OpenGL, "Could not get information about selected pixel format.");
    }

    pWndParam->b32Bit = pfd.cColorBits == 32;
    pWndParam->bZBuffer = pfd.cDepthBits > 0;

    // Create appropriate window
    if (pWndParam->bIsFullScreen) {
        DEVMODE dmScreenSettings;
        memset(&dmScreenSettings, 0, sizeof(DEVMODE));

        dmScreenSettings.dmSize = sizeof(DEVMODE);
        dmScreenSettings.dmPelsWidth = pWndParam->iWidth;
        dmScreenSettings.dmPelsHeight = pWndParam->iHeight;
        dmScreenSettings.dmBitsPerPel = pfd.cColorBits;
        dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

        if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
            DESTROY_SET_ERROR_RETURN_FALSE(EC_Windows, "Failed to switch to %dx%d fullscreen mode.", pWndParam->iWidth, pWndParam->iHeight);
        }
    }

    ShowWindow(pWndParam->hWnd, SW_SHOW);
    SetForegroundWindow(pWndParam->hWnd);
    SetFocus(pWndParam->hWnd);
    UpdateWindow(pWndParam->hWnd);

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

// Convert a number range from 0 to 255 to a number range from 0 to 1
float GameOGL::ConvertColor(int iColor) {
    if (iColor >= 255) {
        return 1.f;
    } else if (iColor <= 0) {
        return 0.f;
    } else {
        return float(iColor) / 255.f;
    }
    return 0.0f;
}
