#pragma once
#include <windows.h>
#include <commctrl.h>

class JobStatus
{
	HWND status_bar;
	HWND parent;
	HINSTANCE instance;
	UINT id;
public:
	JobStatus(HWND, UINT);
	~JobStatus();
	void SendResizeMessage(WORD, WORD);
	unsigned int GetHeight();
};
