#include "stdafx.h"
#include "ErrorHandler.h"

ErrorHandler::ErrorHandler(PCTCH filename, bool bON)
    : m_bON(bON)
{
    SetError(EC_NoError, _T("No Error"));

    // use default file name
    if (filename == NULL) {
        _stprintf_s(m_szFilename, FILENAME_MAX, TEXT("errorLog.txt"));
    }
    else {
        // determine if .txt extension is needed
        if ((_tcslen(filename) > 4) && (0 == _tcscmp(filename+_tcslen(filename)-4, _T(".txt")))) {
            _stprintf_s(m_szFilename, FILENAME_MAX, _T("%s"), filename);
        }
        else {
            _stprintf_s(m_szFilename, FILENAME_MAX, _T("%s.txt"), filename);
        }
    }

    // disable error handling file
    if (!bON) {
        return;
    }

    FILE *fp;
    _tfopen_s(&fp, m_szFilename, _T("wt"));
    if (fp != NULL) {
        TCHAR tmpbuf[BUFSIZ];
        _tstrdate_s(tmpbuf, BUFSIZ);
        _ftprintf_s(fp, _T("Date(M/D/Y): %s "), tmpbuf);
        _tstrtime_s(tmpbuf, BUFSIZ);
        _ftprintf_s(fp, _T("%s\n\n"), tmpbuf);
        fclose(fp);
    }
} // ErrorHandler

ErrorHandler::~ErrorHandler() {

}

void ErrorHandler::SetError(ErrorCode EC, PCTCH szMessage, ...)
{
    va_list ap;
    va_start(ap, szMessage);
    _vstprintf_s(m_szErrorMsg, ERROR_MSG_MAX, szMessage, ap);
    va_end(ap);

    m_EC_ID = EC;

    if (!m_bON) {
        return;
    }

    FILE *fp;
    _tfopen_s(&fp, m_szFilename, _T("at"));
    if (fp != NULL) {
        _ftprintf(fp, _T("%s\n"), m_szErrorMsg);
        fclose(fp);
    }
}

ErrorCode ErrorHandler::GetErrorCode()
{
    return m_EC_ID;
}

PCTCH ErrorHandler::GetErrorMessage()
{
    return m_szErrorMsg;
}

void ErrorHandler::ShowErrorMessage()
{
    switch (m_EC_ID)
    {
    case EC_NoError:
        MessageBox(GetDesktopWindow(), m_szErrorMsg, TEXT("All Okay"), MB_ICONINFORMATION | MB_OK);
        break;
    case EC_Windows:
        MessageBox(GetDesktopWindow(), m_szErrorMsg, TEXT("Windows Error"), MB_ICONERROR | MB_OK);
        break;
    case EC_Unknow:
    case EC_Error:
    case EC_NotEnoughMemory:
        MessageBox(GetDesktopWindow(), m_szErrorMsg, TEXT("Memory Error"), MB_ICONERROR | MB_OK);
        break;
    case EC_DirectX:
        MessageBox(GetDesktopWindow(), m_szErrorMsg, TEXT("DirectX 9 Error"), MB_ICONERROR | MB_OK);
        break;
    case EC_GameError:
        MessageBox(GetDesktopWindow(), m_szErrorMsg, TEXT("Game Error"), MB_ICONERROR | MB_OK);
        break;
    case EC_OpenGL:
        MessageBox(GetDesktopWindow(), m_szErrorMsg, TEXT("OpenGL Error"), MB_ICONERROR | MB_OK);
        break;
    default:
        MessageBox(GetDesktopWindow(), TEXT("Unrecognized Error Message"), TEXT("Unknow Message"), MB_ICONQUESTION | MB_OK);
        break;
    }
} // Show Error Message
