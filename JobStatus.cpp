#include "JobStatus.h"
#include "wbits.h"

JobStatus::JobStatus(MainWindow * parent, UINT id)
{
	this->parent = parent;
	this->id = id;

	auto parent_window = parent->GetParent();
	this->instance = reinterpret_cast<HINSTANCE>(::GetWindowLongPtr(parent_window, GWLP_HINSTANCE));

	this->status_bar = ::CreateWindow(STATUSCLASSNAME, NULL, WS_CHILD | WS_TABSTOP | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 64, 64, parent_window, (HMENU)id, this->instance, nullptr);
	if(!this->status_bar)
	{
		throw STATUS_INVALID_HANDLE;
	}
	::SendMessage(this->status_bar, SB_SIMPLE, static_cast<WPARAM>(TRUE), 0);
}

JobStatus::~JobStatus()
{
	::SendMessage(this->status_bar, WM_CLOSE, 0, 0);
}

void JobStatus::SendResizeMessage(WORD width, WORD height)
{
	::SendMessage(this->status_bar, WM_SIZE, 0, MAKELPARAM(width, height));
}

unsigned int JobStatus::GetHeight()
{
	RECT r;

	::GetWindowRect(this->status_bar, &r);
	return static_cast<unsigned int>(r.bottom - r.top);
}

void JobStatus::SetText(const TCHAR * text)
{
	::SendMessage(this->status_bar, SB_SETTEXT, static_cast<WPARAM>(SB_SIMPLEID | SBT_NOTABPARSING), reinterpret_cast<LPARAM>(text));
}
