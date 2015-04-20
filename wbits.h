#pragma once
#include <windows.h>
#include "JobList.h"
#include "JobStatus.h"

#ifdef UNICODE
#define MAINWINDOWCLASS L"░BITS Tool Class"
#define MAINWINDOWTITLE L"░BITS Tool"
#else
#define MAINWINDOWCLASS "BITS Tool Class"
#define MAINWINDOWTITLE "BITS Tool"
#endif

struct MAINWINDOWSTRUCT
{
	HINSTANCE Instance;
	int ShowWindow;
};
typedef struct MAINWINDOWSTRUCT MAINWINDOWSTRUCT;

class MainWindow
{
	HWND window;
	HINSTANCE instance;
#if defined(THREADED)
	HANDLE thread;
#endif
	int show_window;
	JobList * job_list;
	JobStatus * job_status;
	HMENU menu;
	SIZE current_size;
public:
	MainWindow(HINSTANCE, int);
	~MainWindow();
	inline HWND GetParent() { return this->window; }
	inline JobStatus * GetStatusBar() { return this->job_status; }
	inline bool IsValidInstance() { return (this->window != nullptr); }
	void Start();
	int WaitForClose();
private:
	bool create(const CREATESTRUCT*);
	void destroy();
	void size(WORD, WORD);
	void set_focus();
	void system_color_change();
	void get_min_max_info(MINMAXINFO*);
	void draw_item(const DRAWITEMSTRUCT*);
	void measure_item(const MEASUREITEMSTRUCT*);
	void notify(const NMHDR*);
	void command(WORD, WORD, HWND);
	void timer(UINT_PTR);
	void enter_menu_loop();
	void theme_changed();
	static LRESULT CALLBACK main_window_proc(HWND, UINT, WPARAM, LPARAM);
	static unsigned int __stdcall thread_proc(void*);
	static int do_message_loop();
public:
	static const WNDCLASSEX main_window_class_t;
};
