#pragma once
#include <windows.h>
#include <commctrl.h>
#include <bits.h>
#include "JobItem.h"

class JobList
{
	HWND listview;
	HWND parent;
	HINSTANCE instance;
	unsigned short item_width;
	unsigned short item_height;
	UINT64 last_update_time;
	IBackgroundCopyManager *bits;
	HANDLE list_semaphore;
	bool non_super_user;
public:
	JobList(HWND,UINT,const RECT*);
	~JobList();
	void Resize(const RECT*);
	HWND GetWindow() { return listview; }
	void UpdateList();
	void CompleteSelectedJobs();
	void AbortSelectedJobs();
	void PauseSelectedJobs();
	void ResumeSelectedJobs();
	unsigned int GetSelectedCount() { return ListView_GetSelectedCount(listview); }
	void ShowProperty();
	static void DrawListItem(class JobList*,const DRAWITEMSTRUCT*);
	static void MeasureListItem(MEASUREITEMSTRUCT*);
	static void GetDispInfo(NMLVDISPINFO*);
#ifdef UNICODE
	static const LVCOLUMN columns[6];
	static const LVCOLUMN prop_filelist[2];
#else
	static LVCOLUMN columns[6];
	static LVCOLUMN prop_filelist[2];
#endif
	static const TCHAR zero_length_string[1];
private:
	void refresh_item(class JobItem*);
	bool additem(class JobItem*);
	void cleanup_unused_items();
	void cleanup_all_items();
	int find_next_selected_item(int,class JobItem**);
	class JobItem * find_item_by_id(const GUID*,int*);
	void lock_list(bool);
	//static void guid_to_str(const GUID*,TCHAR*,size_t);
	static void set_filetime_to_editbox(HWND,int,const FILETIME*);
	static void set_dialog_item_unicode_text(HWND,int,PCWSTR);
	static PTSTR convert_string(PWSTR);
	static void free_convert_string(PTSTR);
	static INT_PTR CALLBACK job_property_main_proc(HWND,UINT,WPARAM,LPARAM);
	static INT_PTR CALLBACK job_property_proxy_proc(HWND,UINT,WPARAM,LPARAM);
	static INT_PTR CALLBACK job_property_cred_proc(HWND,UINT,WPARAM,LPARAM);
	static INT_PTR CALLBACK job_property_credentials_proc(HWND,UINT,WPARAM,LPARAM);
	static const PROPSHEETHEADER job_property_sheet_t;
	static const PROPSHEETPAGE job_property_main_t;
	static const PROPSHEETPAGE job_property_proxy_t;
	static const PROPSHEETPAGE job_property_credentials_t;
};
