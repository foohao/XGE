#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

// ==== Includes above ====|====Structs below ====

enum ErrorCode {
    EC_NoError = 0,
    EC_Unknow,  // General
    EC_Error,
    EC_NotEnoughMemory,
    EC_Windows,
    EC_DirectX, // DirectX9
    EC_OpenGL, // OpenGL
    EC_GameError // Game Logic
};

// ==== Structs above ====|==== Function prototypes below ====



class ErrorHandler {
public:
    ErrorHandler(PCTCH filename, bool bON = true);
    virtual ~ErrorHandler();

    void SetError(ErrorCode EC, PCTCH szMessage, ...);
    ErrorCode GetErrorCode();
    LPCTCH GetErrorMessage();

    void ShowErrorMessage();

protected:

    // --------- functions above --------------|--------- variables below ----------

private:
    TCHAR m_szFilename[FILENAME_MAX];
    TCHAR m_szErrorMsg[ERROR_MSG_MAX];
    ErrorCode m_EC_ID;

    bool m_bON;
};


#endif // !ERRORHANDLER_H
