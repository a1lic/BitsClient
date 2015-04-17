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
	int show_window;
	JobList * job_list;
	JobStatus * job_status;
	HMENU menu;
	SIZE current_size;
public:
	MainWindow(HINSTANCE, int);
	~MainWindow();
	inline bool IsValidInstance() { return (window != nullptr); }
	int EnterMessageLoop();
private:
	bool create(const CREATESTRUCT*);
	void destroy();
	void size(WORD, WORD);
	void set_focus();
	void get_min_max_info(MINMAXINFO*);
	void draw_item(const DRAWITEMSTRUCT*);
	void measure_item(const MEASUREITEMSTRUCT*);
	void notify(const NMHDR*);
	void command(WORD, WORD, HWND);
	void timer(UINT_PTR);
	void enter_menu_loop();
	static LRESULT CALLBACK main_window_proc(HWND, UINT, WPARAM, LPARAM);
public:
	static const WNDCLASSEX main_window_class_t;
};
