#include "stdafx.h"
#include "GameOGL.h"
using std::function;

#define SET_ERROR_AND_RETURN_FALSE(EC, EMF, ...) \
    do { m_pErrorHandler->SetError(EC, _T(EMF), __VA_ARGS__); return false; } while(false)

#define DESTROY_SET_ERROR_RETURN_FALSE(EC, EMF, ...) \
    DestroyWnd(); \
    SET_ERROR_AND_RETURN_FALSE(EC, EMF, __VA_ARGS__)

GameOGL::GameOGL(ErrorHandler * pErrorHandler)
    : m_bQuit(false),
    m_bActive(true),
    m_GameState(GS_STOP),
    m_dTimeGame(0),
    m_pErrorHandler(pErrorHandler) {

    // Define function pointer to GetDeltaTime
    long long lPerformanceFrequency;
    if (QueryPerformanceFrequency((LARGE_INTEGER*)&lPerformanceFrequency)) {
        QueryPerformanceCounter((LARGE_INTEGER*)&m_lPrevTime);
        m_dTimeScaleFactor = 1.0 / lPerformanceFrequency;
        GetDeltaTime = std::bind(&GameOGL::GetTimePerformanceHigh, this);
    } else {
        m_lPrevTime = timeGetTime();
        m_dTimeScaleFactor = 0.001;
        GetDeltaTime = std::bind(&GameOGL::GetTimePerformanceRegular, this);
    }
}

GameOGL::~GameOGL() {
    DestroyWnd();
}

bool GameOGL::Create(const WndParam * pWndParam) {

    if (pWndParam->iHeight <= 0 || pWndParam->iWidth <= 0 || pWndParam->iRefreshRate <= 0) {
        SET_ERROR_AND_RETURN_FALSE(EC_Error, "Invalid parameters\n"
                                   "Width        = %d\n"
                                   "Height       = %d\n"
                                   "Refresh Rate = %d\n", pWndParam->iWidth, pWndParam->iHeight, pWndParam->iRefreshRate);
    }


    m_pszTitle = pWndParam->pszTitle;
    m_iWidth = pWndParam->iWidth;
    m_iHeight = pWndParam->iHeight;
    m_bIsFullScreen = pWndParam->bIsFullScreen;
    m_iRefreshRate = pWndParam->iRefreshRate;
    m_bWindowed = pWndParam->bWindowed;
    m_b32Bit = pWndParam->b32Bit;
    m_bZBuffer = pWndParam->bZBuffer;

    m_hInst = pWndParam->hInst;

    m_fFPS = float(pWndParam->iRefreshRate);

    // Register window class
    m_WndClassEx.cbSize = sizeof(WNDCLASSEX);
    m_WndClassEx.style = CS_HREDRAW | CS_VREDRAW | CS_CLASSDC | CS_DBLCLKS;
    m_WndClassEx.lpfnWndProc = WndProc;
    m_WndClassEx.cbClsExtra = 0;
    m_WndClassEx.cbWndExtra = 0;
    m_WndClassEx.hInstance = m_hInst;
    m_WndClassEx.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    m_WndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
    m_WndClassEx.hbrBackground = NULL;
    m_WndClassEx.lpszMenuName = NULL;
    m_WndClassEx.lpszClassName = m_pszTitle;
    m_WndClassEx.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    if (!RegisterClassEx(&m_WndClassEx)) {
        SET_ERROR_AND_RETURN_FALSE(EC_Windows, "Cannot register %s with Windows", m_WndClassEx.lpszClassName);
    }

    DWORD dwStyle, dwExStyle;
    if (m_bIsFullScreen) {
        dwStyle = WS_POPUP | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX;
        dwExStyle = WS_EX_APPWINDOW;
    } else {
        dwStyle = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    }

    RECT rcWnd;
    rcWnd.left = 0;
    rcWnd.right = m_iWidth;
    rcWnd.top = 0;
    rcWnd.bottom = m_iHeight;
    AdjustWindowRectEx(&rcWnd, dwStyle, FALSE, dwExStyle);

    m_hWnd = CreateWindowEx(dwExStyle,
                                     m_WndClassEx.lpszClassName,
                                     m_WndClassEx.lpszClassName,
                                     dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                     0, 0,
                                     rcWnd.right - rcWnd.left,
                                     rcWnd.bottom - rcWnd.top,
                                     NULL,
                                     NULL,
                                     m_hInst,
                                     (LPVOID)this);

    if (m_hWnd == NULL) {
        DESTROY_SET_ERROR_RETURN_FALSE(EC_Windows, "Cannot create window.");
    }

    m_hDC = GetDC(m_hWnd);

    if (m_hDC == NULL) {
        DESTROY_SET_ERROR_RETURN_FALSE(EC_Windows, "Could not get window's DC.");
    }

    // Center window onscreen if it is a window
    if (m_bWindowed) {
        int cx = GetSystemMetrics(SM_CXSCREEN);
        int cy = GetSystemMetrics(SM_CYSCREEN);
        int px = 0;
        int py = 0;

        if (cx > m_iWidth) {
            px = int((cx - m_iWidth) / 2);
        }
        if (cy > m_iHeight) {
            py = int((cy - m_iHeight) / 2);
        }
        MoveWindow(m_hWnd, px, py, m_iWidth, m_iHeight, FALSE);
    }

    // Define pixel format
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = m_b32Bit ? 32 : 16;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int iPixelFormat = ChoosePixelFormat(m_hDC, &pfd);
    if (!iPixelFormat) {
        DESTROY_SET_ERROR_RETURN_FALSE(EC_OpenGL, "Could not find suitable OpenGL pixel format to use.");
    }

    if (!SetPixelFormat(m_hDC, iPixelFormat, &pfd)) {
        DESTROY_SET_ERROR_RETURN_FALSE(EC_OpenGL, "Could not set OpenGL pixel format.");
    }

    m_hGLRC = wglCreateContext(m_hDC);
    if (m_hGLRC = NULL) {
        DESTROY_SET_ERROR_RETURN_FALSE(EC_OpenGL, "Could not create OpenGL context.");
    }
    if (!wglMakeCurrent(m_hDC, m_hGLRC)) {
        DESTROY_SET_ERROR_RETURN_FALSE(EC_OpenGL, "Could not make current OpenGL context.");
    }

    // Obtain detailed information about the device context's pixel format
    if (!DescribePixelFormat(m_hDC, iPixelFormat, sizeof(pfd), &pfd)) {
        DESTROY_SET_ERROR_RETURN_FALSE(EC_OpenGL, "Could not get information about selected pixel format.");
    }

    m_b32Bit = pfd.cColorBits == 32;
    m_bZBuffer = pfd.cDepthBits > 0;

    // Create appropriate window
    if (m_bIsFullScreen) {
        DEVMODE dmScreenSettings;
        memset(&dmScreenSettings, 0, sizeof(DEVMODE));

        dmScreenSettings.dmSize = sizeof(DEVMODE);
        dmScreenSettings.dmPelsWidth = m_iWidth;
        dmScreenSettings.dmPelsHeight = m_iHeight;
        dmScreenSettings.dmBitsPerPel = pfd.cColorBits;
        dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

        if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
            DESTROY_SET_ERROR_RETURN_FALSE(EC_Windows, "Failed to switch to %dx%d fullscreen mode.", m_iWidth, m_iHeight);
        }
    }

    ShowWindow(m_hWnd, SW_SHOW);
    SetForegroundWindow(m_hWnd);
    SetFocus(m_hWnd);
    UpdateWindow(m_hWnd);

    return true;
}

int GameOGL::StartMsgLoop() {

    Initialize();

    while (!m_bQuit) {
        // Get all messages before rendering a frame
        if (PeekMessage(&m_Msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&m_Msg);
            DispatchMessage(&m_Msg);
        } else {
            Frame();
        }
    }

    CleanUp();

    return m_Msg.wParam;
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

void GameOGL::CountFPS(double dDeltaTime) {

    static double dTotalTime = 0;
    static int loops = 0;

    dTotalTime += dDeltaTime;
    if (dTotalTime > 0.5) {
        m_fFPS = float(loops / dTotalTime);

        // reset
        loops = 0;
        dTotalTime = 0;
    }
    ++loops;
}

// Display the FPS on the title bar until the font engine is built
bool GameOGL::DisplayFPSinTitle() {
    
    static TCHAR szFPS[20];
    _stprintf_s(szFPS, 20, _T("%2.0f FPS"), m_fFPS);

    static size_t uTitleLen = _tcslen(m_pszTitle) + 22;
    static PTCH pszTitle = new TCHAR[uTitleLen];
    _stprintf_s(pszTitle, uTitleLen, _T("%s %s"), m_pszTitle, szFPS);
    SetWindowText(m_hWnd, pszTitle);

    return true;
}

LRESULT GameOGL::WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    static GameOGL *pWnd = NULL;
    static bool bProcessed = false;
    switch (iMsg) {
    case WM_NCCREATE:
    {
        pWnd = reinterpret_cast<GameOGL*>(lParam);
        break;
    }
    default:
        if (NULL != pWnd) {
            bProcessed = pWnd->MsgHandler(iMsg, wParam, lParam);
        }
    }

    if (!bProcessed) {
        return DefWindowProc(hWnd, iMsg, wParam, lParam);
    }
    return 0;
}

bool GameOGL::MsgHandler(UINT iMsg, WPARAM wParam, LPARAM lParam) {

    switch (iMsg) {
    case WM_QUIT:
    {
        m_bQuit = true;
        return true;
    }
    case WM_CLOSE:
    {
        DestroyWindow(m_hWnd);
        return true;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return true;
    }
    case WM_ACTIVATE:
    {
        if (LOWORD(wParam) == WA_INACTIVE) {
            m_bActive = false;
            WndFocusLost();
        } else {
            m_bActive = true;
            WndFocusReceived();
        }
        return true;
    }
    case WM_SYSCOMMAND:
    {
        switch (wParam) {
        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
        {
            return true;
        }
        default:
            return false;
        }
    }
    default:
        return false;
    }
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

void GameOGL::Initialize() {

    // Setup projection martix
    glViewport(0, 0, m_iWidth, m_iHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.f, float(m_iWidth) / m_iHeight, 0.1f, 1000.f);

    glShadeModel(GL_SMOOTH);

    // Define background color of window
    m_BGColor.Set(1.f, 1.f, 1.f);

    // Background & depth clear color
    glClearColor(m_BGColor.fRed, m_BGColor.fGreen, m_BGColor.fBlue);
    glClearDepth(1.f);

    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Make perspective look good if processor time is available
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_PERSPECTIVE_CORRECTION_HINT);

    // Enable backface culling
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    m_GameState = GS_PLAY;

    return;
}

void GameOGL::CleanUp() {

    if (m_pErrorHandler->GetErrorCode != EC_NoError) {
        m_pErrorHandler->ShowErrorMessage();
    }
}

void GameOGL::DestroyWnd() {

    // Reset screen mode back to desktop settings
    if (m_bIsFullScreen) {
        ChangeDisplaySettings(NULL, 0);
    }

    // Deselect OpenGL rendering context
    if (m_hDC) {
        wglMakeCurrent(m_hDC, NULL);
    }

    // Delete the OpenGL context
    if (m_hGLRC) {
        wglDeleteContext(m_hGLRC);
        m_hGLRC = NULL;
    }

    // Free memory for DC
    if (m_hWnd && m_hDC) {
        ReleaseDC(m_hWnd, m_hDC);
        m_hDC = NULL;
    }

    if (m_hWnd) {
        DestroyWindow(m_hWnd);
        m_hWnd = NULL;
    }

    if (m_hInst) {
        UnregisterClass(m_pszTitle, m_hInst);
        m_hInst = NULL;
    }
}

void GameOGL::Frame() {

    // If window is in focus, run the game
    if (m_GameState == GS_PLAY) {
        // Get time elapsed since last frame update
        m_dTimeDelta = GetDeltaTime();

        if (m_dTimeDelta > 0.25) {
            m_dTimeDelta = 0.25;
        }

        m_dTimeGame += m_dTimeDelta;

        if (!UpdateGame(float(m_dTimeDelta))) {
            m_bQuit = true;
            return;
        } 
    }

    CountFPS(m_dTimeDelta);
    DisplayFPSinTitle();

    Sleep(1);
}

bool GameOGL::UpdateGame(float fDeltaTime) {
    RenderPre();

    // Render stuff here

    return RenderPost();
}

void GameOGL::RenderPre() {

    if (m_bZBuffer) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.f, 0.f, -4.f);
}

bool GameOGL::RenderPost() {

    glFlush();

    return SwapBuffers(m_hDC) ? true : false;
}

void GameOGL::WndFocusReceived() {

    if (m_GameState == GS_PAUSE) {
        m_GameState = GS_PLAY;
    }
}

void GameOGL::WndFocusLost() {
    m_GameState = GS_PAUSE;
}
