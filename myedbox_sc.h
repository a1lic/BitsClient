#pragma once

#ifndef EXTERN
#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN extern
#endif
#endif

#define WINUSER_WNDCLASS_EDIT TEXT("Edit")
#define SUBCLASSED_EDIT TEXT("EditButton")

#define REFERER_BMP_WIDTH 10
#define REFERER_BMP_HEIGHT 2

struct EDITBOXDESC
{
	HANDLE Heap;
	WNDPROC DefaultWndProc;
	LONG_PTR OldUserData;
	struct DCBMP
	{
		HDC DC;
		HBITMAP Bitmap;
		HBITMAP DefaultBitmap;
	} ButtonLabel;
	UINT Id;
	RECT ButtonRect;
	BOOLEAN Pressed;
	BOOLEAN Mouse;
	BOOLEAN ToolTipInit;
};
typedef struct EDITBOXDESC EDITBOXDESC;

EXTERN unsigned short GetEditBoxWindowHeight(HWND,HFONT,unsigned int);
EXTERN ATOM prepare_editbox(HINSTANCE);
