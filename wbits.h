#pragma once
#include <windows.h>
#include "JobList.h"

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
	JobList *JobListWindow;
	HMENU Menu;
};
typedef struct MAINWINDOWSTRUCT MAINWINDOWSTRUCT;
