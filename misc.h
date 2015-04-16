#include <windows.h>

#ifndef EXTERN
#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN extern
#endif
#endif

EXTERN void Debug(const TCHAR*, ...);
EXTERN void EnableWindows(HWND,int,BOOLEAN);
EXTERN void FileTimeToLocalizedString(const FILETIME*,PTSTR,size_t);
EXTERN BOOLEAN ValidateOSVersion(BYTE,BYTE);
EXTERN void CenteringWindow(HWND,const POINT*);
EXTERN void CenteringWindowToCursor(HWND);
EXTERN void CenteringWindowToParent(HWND,HWND);
EXTERN UINT64 QueryCounter();
