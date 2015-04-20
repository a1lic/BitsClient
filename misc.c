#include "misc.h"
#include <tchar.h>
#include <stdarg.h>
#include <stdio.h>

void Debug(const TCHAR *fmt, ...)
{
	va_list ap;
	TCHAR debug_msg[1024];

	if(IsDebuggerPresent())
	{
		va_start(ap,fmt);
		_vstprintf_s(debug_msg,1024,fmt,ap);
		va_end(ap);

		OutputDebugString(debug_msg);
	}
}

void EnableWindows(HWND dlg,int ctl,BOOLEAN enable)
{
	HWND hwnd;
	DWORD style;
	unsigned int count;

	hwnd = GetDlgItem(dlg,ctl);
	/* 次にWS_GROUPスタイルを持つウィンドウに遭遇するまで、Zオーダー順に有効/無効を一括設定する */
	for(count = 0; hwnd; count++)
	{
		style = (DWORD)GetWindowLongPtr(hwnd,GWL_STYLE);
		if(count && ((style & (WS_CHILD | WS_GROUP)) == (WS_CHILD | WS_GROUP)))
			break;

		EnableWindow(hwnd,(BOOL)enable);
		hwnd = GetNextWindow(hwnd,GW_HWNDNEXT);
	}
}

void FileTimeToLocalizedString(const FILETIME *ft,PTSTR buf,size_t buf_chars)
{
	SYSTEMTIME sys_time;
	FILETIME l_ft;
	PTSTR tptr;

	if(!ft->dwLowDateTime && !ft->dwHighDateTime)
	{
		_tcscpy_s(buf,buf_chars,_T("なし"));
		return;
	}

	FileTimeToLocalFileTime(ft,&l_ft);
	FileTimeToSystemTime(&l_ft,&sys_time);

	GetDateFormat(LOCALE_USER_DEFAULT,DATE_LONGDATE,&sys_time,NULL,buf,(int)buf_chars);
	_tcscat_s(buf,buf_chars,_T(" "));
	tptr = buf + _tcslen(buf);
	GetTimeFormat(LOCALE_USER_DEFAULT,0,&sys_time,NULL,tptr,(int)(tptr - buf));
}

BOOLEAN ValidateOSVersion(BYTE major,BYTE minor)
{
	OSVERSIONINFO version;

	version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
#pragma warning(push)
#pragma warning(disable:4996)
	GetVersionEx(&version);
#pragma warning(pop)

	if(version.dwMajorVersion > major)
		return TRUE;
	else if((version.dwMajorVersion == major) && (version.dwMinorVersion >= minor))
		return TRUE;

	return FALSE;
}

void CenteringWindow(HWND window,const POINT *pos)
{
	/* ウィンドウの中心点がposで指定した位置に来るように移動する */
	HMONITOR monitor;
	POINT p;
	RECT dlg_rect;
	MONITORINFO mon_info;

	p = *pos;
	/* 指定した点のあるモニタ */
	if(ValidateOSVersion(4,10))
	{
		monitor = MonitorFromPoint(p,MONITOR_DEFAULTTONEAREST);
	}
	else
		monitor = NULL;

	if(monitor)
	{
		/* そのモニタに映っている作業領域の大きさを調べる */
		mon_info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor,&mon_info);
	}
	else
	{
		/* モニタのハンドルが取れなかった場合はプライマリモニタと仮定 */
		SystemParametersInfo(SPI_GETWORKAREA,0,&mon_info.rcWork,0);
	}

	/* ダイアログの大きさ */
	GetWindowRect(window,&dlg_rect);
	OffsetRect(&dlg_rect,-dlg_rect.left,-dlg_rect.top);

	/* 大きさの半分を座標から減ずる */
	p.x -= dlg_rect.right / 2;
	p.y -= dlg_rect.bottom / 2;

	/* はみ出る場合は収まるようにする */
	if((p.x + dlg_rect.right) > mon_info.rcWork.right)
		p.x = mon_info.rcWork.right - dlg_rect.right;
	if((p.y + dlg_rect.bottom) > mon_info.rcWork.bottom)
		p.y = mon_info.rcWork.bottom - dlg_rect.bottom;

	if(p.x < mon_info.rcWork.left)
		p.x = mon_info.rcWork.left;
	if(p.y < mon_info.rcWork.top)
		p.y = mon_info.rcWork.top;

	SetWindowPos(window,NULL,p.x,p.y,0,0,SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
}

void CenteringWindowToCursor(HWND window)
{
	POINT p;

	GetCursorPos(&p);
	CenteringWindow(window,&p);
}

void CenteringWindowToParent(HWND window,HWND parent)
{
	RECT parent_rect;
	POINT p;

	if(GetWindowRect(parent,&parent_rect))
	{
		p.x = parent_rect.left + (parent_rect.right - parent_rect.left) / 2;
		p.y = parent_rect.top + (parent_rect.bottom - parent_rect.top) / 2;
		CenteringWindow(window,&p);
	}
}
