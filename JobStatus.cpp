#include "JobStatus.h"

JobStatus::JobStatus(HWND parent, UINT id)
{
	this->parent = parent;
	this->id = id;

	this->instance = reinterpret_cast<HINSTANCE>(::GetWindowLongPtr(parent, GWLP_HINSTANCE));

	this->status_bar = ::CreateWindowEx(0, STATUSCLASSNAME, NULL, WS_CHILD | WS_TABSTOP | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 64, 64, parent, (HMENU)id, this->instance, nullptr);
	if(!this->status_bar)
	{
		throw STATUS_INVALID_HANDLE;
	}
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
