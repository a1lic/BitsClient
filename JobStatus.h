#pragma once
#include <windows.h>
#include <commctrl.h>

class JobStatus
{
	HWND status_bar;
	class MainWindow * parent;
	HINSTANCE instance;
	UINT id;
public:
	JobStatus(class MainWindow*, UINT);
	~JobStatus();
	void SendResizeMessage(WORD, WORD);
	unsigned int GetHeight();
	void SetText(const TCHAR*);
};
