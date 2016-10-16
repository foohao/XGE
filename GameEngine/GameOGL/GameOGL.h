#ifndef GAMEOGL_H
#define GAMEOGL_H

#include "Core.h"
#include <functional>

struct ColorOGL {
    float fRed;
    float fGreen;
    float fBlue;

    void Set(float Red, float Green, float Blue) {
        fRed = Red;
        fGreen = Green;
        fBlue = Blue;
    }
};

class GameOGL {
public:
    GameOGL(ErrorHandler * pErrorHandler);
    virtual ~GameOGL();

    bool Create(const WndParam *pWndParam);

    int StartMsgLoop();

protected:

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
    virtual bool MsgHandler(UINT iMsg, WPARAM wParam, LPARAM lParam);

    float ConvertColor(int iColor);

    void Initialize();
    void CleanUp();
    void DestroyWnd();

    void Frame();
    bool UpdateGame(float fDeltaTime);

    void RenderPre();
    bool RenderPost();

    void WndFocusReceived();
    void WndFocusLost();

    std::function<double(void)> GetDeltaTime;
    double GetTimePerformanceHigh();
    double GetTimePerformanceRegular();

    void CountFPS(double dDeltaTime);
    bool DisplayFPSinTitle();

    bool m_bQuit;
    bool m_bActive;
    GameState m_GameState;

    double m_dTimeScaleFactor;
    long long m_lPrevTime;
    long long m_lCurrentTime;
    float m_fFPS;
    double m_dTimeGame;
    double m_dTimeDelta;

    // Window parameters
    PTCH m_pszTitle;
    INT m_iWidth;
    INT m_iHeight;
    bool m_bIsFullScreen;
    INT m_iRefreshRate;
    bool m_bWindowed;
    bool m_b32Bit;
    bool m_bZBuffer;

    HWND      m_hWnd;
    HDC       m_hDC;
    HINSTANCE m_hInst;

    WNDCLASSEX m_WndClassEx;
    MSG m_Msg;
    HGLRC m_hGLRC;
    ColorOGL m_BGColor;

    ErrorHandler *m_pErrorHandler;
};

#endif // !GAMEOGL_H
