#include "JobList.h"
#include <assert.h>
#include "resource.h"
#include <tchar.h>
#include <windowsx.h>
#include "misc.h"
#include "CredentialsDialog.h"

#ifdef UNICODE
const LVCOLUMN JobList::columns[6] = {
#else
LVCOLUMN JobList::columns[6] = {
#endif
	/* mask,fmt,cx,pszText,cchTextMax,iSubItem,iImage,iOrder */
	{LVCF_TEXT | LVCF_WIDTH,0,128,TEXT("ジョブ名"),0,0,0,0},
	{LVCF_TEXT | LVCF_WIDTH,0,128,TEXT("GUID"),0,0,0,0},
	{LVCF_TEXT | LVCF_WIDTH,0,128,TEXT("状態"),0,0,0,0},
	{LVCF_TEXT | LVCF_WIDTH,0,128,TEXT("モード"),0,0,0,0},
	{LVCF_TEXT | LVCF_WIDTH,0,128,TEXT("ファイルサイズ"),0,0,0,0},
	{LVCF_TEXT | LVCF_WIDTH,0,128,TEXT("転送済みサイズ"),0,0,0,0}};

#ifdef UNICODE
const LVCOLUMN JobList::prop_filelist[2] = {
#else
LVCOLUMN JobList::prop_filelist[2] = {
#endif
	{LVCF_TEXT | LVCF_WIDTH,0,128,TEXT("ファイル名"),0,0,0,0},
	{LVCF_TEXT | LVCF_WIDTH,0,128,TEXT("URL"),0,0,0,0}};

const PROPSHEETHEADER JobList::job_property_sheet_t = {
	/* dwSize      */ sizeof(PROPSHEETHEADER),
	/* dwFlags     */ PSH_PROPSHEETPAGE | PSH_PROPTITLE,
	/* hwndParent  */ nullptr,
	/* hInstance   */ nullptr,
	/* hIcon       */ nullptr,
	/* pszCaption  */ nullptr,
	/* nPages      */ 3,
	/* nStartPage  */ 0,
	/* ppsp        */ nullptr,
	/* pfnCallback */ nullptr };

const PROPSHEETPAGE JobList::job_property_main_t = {
	/* dwSize            */ sizeof(PROPSHEETPAGE),
	/* dwFlags           */ 0,
	/* hInstance         */ nullptr,
	/* pszTemplate       */ MAKEINTRESOURCE(IDD_JOBINFO),
	/* hIcon             */ nullptr,
	/* pszTitle          */ nullptr,
	/* pfnDlgProc        */ job_property_main_proc,
	/* lParam            */ 0,
	/* pfnCallback       */ nullptr,
	/* pcRefParent       */ nullptr,
	/* pszHeaderTitle    */ nullptr,
	/* pszHeaderSubTitle */ nullptr };

const PROPSHEETPAGE JobList::job_property_proxy_t = {
	/* dwSize            */ sizeof(PROPSHEETPAGE),
	/* dwFlags           */ 0,
	/* hInstance         */ nullptr,
	/* pszTemplate       */ MAKEINTRESOURCE(IDD_JOBPROXY),
	/* hIcon             */ nullptr,
	/* pszTitle          */ nullptr,
	/* pfnDlgProc        */ job_property_proxy_proc,
	/* lParam            */ 0,
	/* pfnCallback       */ nullptr,
	/* pcRefParent       */ nullptr,
	/* pszHeaderTitle    */ nullptr,
	/* pszHeaderSubTitle */ nullptr };

const PROPSHEETPAGE JobList::job_property_credentials_t = {
	/* dwSize            */ sizeof(PROPSHEETPAGE),
	/* dwFlags           */ 0,
	/* hInstance         */ nullptr,
	/* pszTemplate       */ MAKEINTRESOURCE(IDD_CREDENTIALS),
	/* hIcon             */ nullptr,
	/* pszTitle          */ nullptr,
	/* pfnDlgProc        */ job_property_credentials_proc,
	/* lParam            */ 0,
	/* pfnCallback       */ nullptr,
	/* pcRefParent       */ nullptr,
	/* pszHeaderTitle    */ nullptr,
	/* pszHeaderSubTitle */ nullptr };

const TCHAR JobList::zero_length_string[1] = TEXT("");

JobList::JobList(HWND parent, UINT id, const RECT * rect)
{
	unsigned int cx,cy;
	unsigned char i;

	this->instance = (HINSTANCE)::GetWindowLongPtr(parent, GWLP_HINSTANCE);

	this->list_semaphore = ::CreateSemaphore(nullptr, 1, 1, nullptr);
	if(this->list_semaphore == nullptr)
	{
		throw STATUS_INVALID_HANDLE;
	}

	this->bits = nullptr;
	::CoCreateInstance(CLSID_BackgroundCopyManager, nullptr, CLSCTX_LOCAL_SERVER, IID_IBackgroundCopyManager, (void**)&this->bits);
	if(this->bits == nullptr)
	{
		::CloseHandle(this->list_semaphore);
		throw STATUS_INVALID_HANDLE;
	}

	cx = rect->right - rect->left;
	cy = rect->bottom - rect->top;

	this->parent = parent;

	this->item_width = (unsigned short)::GetSystemMetrics(SM_CXSMICON);
	this->item_height = (unsigned short)::GetSystemMetrics(SM_CYSMICON);

	this->listview = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL, WS_CHILD | WS_TABSTOP | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER | LVS_OWNERDRAWFIXED, rect->left, rect->top, cx, cy, parent, (HMENU)id, this->instance, nullptr);
	if(this->listview == nullptr)
	{
		::CloseHandle(this->list_semaphore);
		this->bits->Release();
		throw STATUS_INVALID_HANDLE;
	}

	ListView_SetExtendedListViewStyleEx(listview, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	for(i = 0; i < 6; i++)
	{
		ListView_InsertColumn(listview, i, &columns[i]);
	}

	this->non_super_user = false;
}

JobList::~JobList()
{
	// 各アイテムごとに保持しているバッファを解放する処理
	this->cleanup_all_items();

	::CloseHandle(this->list_semaphore);

	// ListViewを破棄する(しなくても良いが)
	::SendMessage(this->listview, WM_CLOSE, 0, 0);

	this->bits->Release();
}

void JobList::Resize(const RECT *rect)
{
	::SetWindowPos(this->listview, nullptr, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOACTIVATE);
}

void JobList::UpdateList()
{
	GUID gid;
	IEnumBackgroundCopyJobs *jobs;
	IBackgroundCopyJob *job;
	JobItem *item;
	ULONG i,count;
	HRESULT Result;

	::QueryPerformanceCounter((LARGE_INTEGER*)&last_update_time);

	this->lock_list(true);
	jobs = nullptr;
	Result = this->bits->EnumJobs(this->non_super_user ? 0 : BG_JOB_ENUM_ALL_USERS, &jobs);
	if(jobs)
	{
		jobs->GetCount(&count);
		for(i = 0; i < count; i++)
		{
			job = nullptr;
			jobs->Next(1, &job, nullptr);
			if(job)
			{
				job->GetId(&gid);
				item = this->find_item_by_id(&gid, nullptr);
				if(!item)
				{
					try
					{
						item = new JobItem(bits, &gid);
					}
					catch(HRESULT _e)
					{
						Debug(TEXT("%hs::%hs(0x%p) %hs => 0x%08X(%u)\n"), "JobList", "UpdateList", this, "JobItem failed.", _e, _e);
						item = nullptr;
					}
					if(item)
					{
						this->additem(item);
					}
				}

				item->UpdateStatus();
				this->refresh_item(item);
				job->Release();
			}
		}
		jobs->Release();
	}
	else
	{
		Debug(TEXT("%hs::%hs(0x%p) %hs => 0x%08X(%u)\n"), "JobList", "UpdateList", this, "EnumJobs failed.", Result, Result);

		if(Result == E_ACCESSDENIED)
		{
			this->non_super_user = true;
		}
	}

	this->lock_list(false);
	this->cleanup_unused_items();
}

void JobList::CompleteSelectedJobs()
{
	int i;
	JobItem *item;

	::WaitForSingleObject(list_semaphore, INFINITE);
	for(i = -1;;)
	{
		i = this->find_next_selected_item(i, &item);
		if(i == -1)
		{
			break;
		}

		item->Complete();
	}
	::ReleaseSemaphore(list_semaphore, 1, nullptr);
}

void JobList::AbortSelectedJobs()
{
	int i;
	JobItem *item;

	::WaitForSingleObject(list_semaphore, INFINITE);
	for(i = -1;;)
	{
		i = this->find_next_selected_item(i, &item);
		if(i == -1)
		{
			break;
		}

		item->Abort();
	}
	::ReleaseSemaphore(list_semaphore, 1, nullptr);
}

void JobList::PauseSelectedJobs()
{
	int i;
	JobItem *item;

	::WaitForSingleObject(list_semaphore, INFINITE);
	for(i = -1;;)
	{
		i = this->find_next_selected_item(i,&item);
		if(i == -1)
		{
			break;
		}

		item->Pause();
	}
	::ReleaseSemaphore(list_semaphore, 1, nullptr);
}

void JobList::ResumeSelectedJobs()
{
	int i;
	JobItem *item;

	::WaitForSingleObject(list_semaphore, INFINITE);
	for(i = -1;;)
	{
		i = this->find_next_selected_item(i, &item);
		if(i == -1)
		{
			break;
		}

		item->Resume();
	}
	::ReleaseSemaphore(list_semaphore, 1, nullptr);
}

void JobList::ShowProperty()
{
	PROPSHEETHEADER psh;
	PROPSHEETPAGE ps[3];
	JobItem *item;

	if(this->find_next_selected_item(-1, &item) != -1)
	{
		//::memcpy(&ps[0], &job_property_main_t, sizeof(PROPSHEETPAGE));
		ps[0] = JobList::job_property_main_t;
		ps[0].lParam = (LPARAM)item;
		//::memcpy(&ps[1], &job_property_proxy_t, sizeof(PROPSHEETPAGE));
		ps[1] = JobList::job_property_proxy_t;
		ps[1].lParam = (LPARAM)item;
		//::memcpy(&ps[2], &job_property_credentials_t, sizeof(PROPSHEETPAGE));
		ps[2] = JobList::job_property_credentials_t;
		ps[2].lParam = (LPARAM)item;

		//::memcpy(&psh,&job_property_sheet_t,sizeof(PROPSHEETHEADER));
		psh = JobList::job_property_sheet_t;
		psh.hwndParent = parent;
		psh.pszCaption = TEXT("ジョブ");
		psh.ppsp = ps;

		::PropertySheet(&psh);
	}
}

void JobList::DrawListItem(JobList *xthis, const DRAWITEMSTRUCT *draw_info)
{
	((JobItem*)draw_info->itemData)->DrawListItem(xthis, draw_info);
}

void JobList::MeasureListItem(MEASUREITEMSTRUCT *item)
{
	item->itemHeight = 2 + ::GetSystemMetrics(SM_CYSMICON);
}

void JobList::GetDispInfo(NMLVDISPINFO *info)
{
	((JobItem*)info->item.lParam)->GetDispInfo(info);
}

void JobList::refresh_item(JobItem *item)
{
	LVITEM litem;
	JobItem *i;
	GUID id;
	bool r;
	int index;

	r = false;
	item->GetJobID(&id);
	i = this->find_item_by_id(&id, &index);
	if(i)
	{
		litem.mask = LVIF_TEXT;
		litem.pszText = LPSTR_TEXTCALLBACK;

		for(litem.iSubItem = 1; litem.iSubItem < 6; litem.iSubItem++)
		{
			r |= item->IsInfoUpdated(litem.iSubItem);
		}
	}

	if(r)
	{
		ListView_Update(listview, index);
	}
}

bool JobList::additem(JobItem *item)
{
	LVITEM litem;
	//JobItem *i;
	//GUID id;

	::memset(&litem, 0, sizeof(LVITEM));

	//item->GetJobID(&id);
	//i = find_item_by_id(&id);

	//if(i)
	//	return false;

	litem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
	litem.pszText = LPSTR_TEXTCALLBACK;
	litem.lParam = (LPARAM)item;
	litem.iImage = 0;

	ListView_InsertItem(listview, &litem);

	litem.mask = LVIF_TEXT;

	for(litem.iSubItem = 1; litem.iSubItem < 6; litem.iSubItem++)
	{
		ListView_SetItem(listview, &litem);
	}

	Debug(TEXT("JobList::additem: Interface version: %u\n"), item->GetJobInterfaceVersion());

	return true;
}

void JobList::cleanup_unused_items()
{
	LVITEM item;
	bool deleted;
	int i,count;

	this->lock_list(true);
	count = ListView_GetItemCount(listview);
	for(i = 0; i < count; i++)
	{
/*
		if(!retry)
		{
			item.iItem = ListView_GetNextItem(listview,item.iItem,LVNI_ALL);
			if(item.iItem == -1)
				break;
		}
		else
			retry = false;
*/

		item.iItem = i;
		item.mask = LVIF_PARAM;
		item.iSubItem = 0;
		ListView_GetItem(listview,&item);

		if(item.lParam)
		{
			if(((JobItem*)item.lParam)->GetUpdateTime() < this->last_update_time)
			{
				delete (JobItem*)item.lParam;
				item.lParam = 0;
				ListView_SetItem(listview,&item);

				deleted = (ListView_DeleteItem(listview,item.iItem) != FALSE);
			}
			else
			{
				deleted = false;
			}
		}
		else
		{
			deleted = (ListView_DeleteItem(listview, item.iItem) != FALSE);
		}

		if(deleted)
		{
			i--;
			count--;
		}
	}
	this->lock_list(false);
}

void JobList::cleanup_all_items()
{
	LVITEM item;
	int i,count;

	this->lock_list(true);
	count = ListView_GetItemCount(listview);
	for(i = 0; i < count; i++)
	{
		item.iItem = i;
		item.mask = LVIF_PARAM;
		item.iSubItem = 0;
		ListView_GetItem(listview,&item);

		if(item.lParam)
		{
			delete (JobItem*)item.lParam;
			item.lParam = 0;
			ListView_SetItem(listview,&item);
		}
	}

	ListView_DeleteAllItems(listview);
	this->lock_list(false);
}

int JobList::find_next_selected_item(int sindex, JobItem **pitem)
{
	LVITEM item;
	int i;

	i = sindex;
	for(;;)
	{
		i = ListView_GetNextItem(listview, i, LVNI_SELECTED);
		if(i == -1)
		{
			break;
		}

		item.iItem = i;
		item.mask = LVIF_PARAM;
		item.iSubItem = 0;
		if(ListView_GetItem(listview, &item))
		{
			if(item.lParam)
			{
				*pitem = (JobItem*)item.lParam;
				break;
			}
		}
	}

	return i;
}

JobItem * JobList::find_item_by_id(const GUID *id, int *index)
{
	LVITEM item;
	int i,count;
	GUID lid;

	count = ListView_GetItemCount(listview);
	for(i = 0; i < count; i++)
	{
		item.iItem = i;
		item.mask = LVIF_PARAM;
		item.iSubItem = 0;
		ListView_GetItem(listview, &item);

		if(item.lParam)
		{
			((JobItem*)item.lParam)->GetJobID(&lid);
			if(!memcmp(&lid, id, sizeof(GUID)))
			{
				if(index)
				{
					*index = item.iItem;
				}
				break;
			}
		}
	}

	return (i < count) ? (JobItem*)item.lParam : NULL;
}

void JobList::lock_list(bool lock)
{
	if(lock)
	{
		::WaitForSingleObject(this->list_semaphore, INFINITE);
	}
	else
	{
		::ReleaseSemaphore(this->list_semaphore, 1, nullptr);
	}
}

void JobList::set_filetime_to_editbox(HWND dlg, int id, const FILETIME *ft)
{
	TCHAR str[128];

	::FileTimeToLocalizedString(ft, str, 128);
	::SetDlgItemText(dlg, id, str);
}

void JobList::set_dialog_item_unicode_text(HWND dlg, int id, PCWSTR text)
{
#ifndef UNICODE
	int bytes;
	PSTR mtext;
#endif

	if(text && ::wcslen(text))
	{
#ifdef UNICODE
		::SetDlgItemTextW(dlg, id, text);
#else
		if(bytes = ::WideCharToMultiByte(CP_ACP, 0, text, -1, NULL, 0, NULL, NULL))
		{
			mtext = new char[bytes];
			::WideCharToMultiByte(CP_ACP, 0, text, -1, mtext, bytes, NULL, NULL);
			::SetDlgItemTextA(dlg, id, mtext);
			delete [] name;
		}
#endif
	}
	else
	{
		::SetDlgItemText(dlg, id, JobList::zero_length_string);
	}
}

PTSTR JobList::convert_string(PWSTR text)
{
	PTSTR r;
#ifndef UNICODE
	int bytes;
#endif

	if(text && ::wcslen(text))
	{
#ifdef UNICODE
		r = text;
#else
		if(bytes = ::WideCharToMultiByte(CP_ACP, 0, text, -1, NULL, 0, NULL, NULL))
		{
			r = new char[bytes];
			::WideCharToMultiByte(CP_ACP, 0, text, -1, r, bytes, NULL, NULL);
		}
		else
		{
			r = (PSTR)JobList::zero_length_string;
		}
#endif
	}
	else
	{
		r = (PTSTR)JobList::zero_length_string;
	}

	return r;
}

void JobList::free_convert_string(PTSTR text)
{
#ifndef UNICODE
	if(text && (text != zero_length_string))
	{
		delete[] text;
	}
#endif
}

INT_PTR CALLBACK JobList::job_property_main_proc(HWND dlg, UINT msg, WPARAM w, LPARAM l)
{
	INT_PTR r;
	JobItem *item;
	BG_JOB_TIMES times;
	HWND ditem;
	int idx;
	BG_JOB_PRIORITY priority;
	ULONG i,num;
	LVITEM litem;
#ifndef UNICODE
	PSTR name;
	PSTR arg;
#endif
	PWSTR wname;
	PWSTR warg;
	IEnumBackgroundCopyFiles *files;
	IBackgroundCopyFile *file;

	switch(msg)
	{
	case WM_DESTROY:
		::RemoveProp(dlg, TEXT("JobItem"));
		Debug(TEXT("job_property_main_proc: WM_DESTROY\n"));
		r = TRUE;
		break;

	case WM_COMMAND:
		switch(LOWORD(w))
		{
		case IDC_DESC:
		case IDC_PRIORITY:
		case IDC_RETRY:
		case IDC_TIMEOUT:
		case IDC_FIN_EXEC:
		case IDC_FIN_ARG:
			if((HIWORD(w) == EN_CHANGE) || (HIWORD(w) == CBN_SELCHANGE))
			{
				if(::GetProp(dlg, TEXT("JobItem")))
				{
					PropSheet_Changed(::GetParent(dlg), dlg);
				}
			}
			break;
		}
		break;

	case WM_INITDIALOG:
		item = (JobItem*)((const PROPSHEETPAGE*)l)->lParam;
		::SetProp(dlg, TEXT("JobItem"), (HANDLE)0);

		item->JobGetDescription(&wname);
		JobList::set_dialog_item_unicode_text(dlg, IDC_DESC, wname);
		::CoTaskMemFree(wname);

		item->JobGetTimes(&times);
		JobList::set_filetime_to_editbox(dlg, IDC_CREATE, &times.CreationTime);
		JobList::set_filetime_to_editbox(dlg, IDC_MOD, &times.ModificationTime);
		JobList::set_filetime_to_editbox(dlg, IDC_FIN, &times.TransferCompletionTime);

		ditem = ::GetDlgItem(dlg, IDC_FILELIST);
		//ListView_SetExtendedListViewStyleEx(ditem,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
		for(i = 0; i < 2; i++)
		{
			ListView_InsertColumn(ditem, i, &prop_filelist[i]);
		}

		// ジョブのファイルを一覧に追加
		if(item->JobEnumFiles(&files))
		{
			if(files->GetCount(&num) == S_OK)
			{
				::memset(&litem, 0, sizeof(LVITEM));
				for(i = 0; i < num; i++)
				{
					files->Next(1,&file,NULL);

					file->GetLocalName(&wname);

					litem.mask = LVIF_TEXT;
					litem.pszText = JobList::convert_string(wname);
					ListView_InsertItem(ditem,&litem);
					JobList::free_convert_string(litem.pszText);

					::CoTaskMemFree(wname);
					file->GetRemoteName(&wname);

					litem.mask = LVIF_TEXT;
					litem.iSubItem = 1;
					litem.pszText = JobList::convert_string(wname);
					ListView_SetItem(ditem,&litem);
					JobList::free_convert_string(litem.pszText);

					::CoTaskMemFree(wname);
					file->Release();
				}
			}
			files->Release();
		}

		ditem = ::GetDlgItem(dlg, IDC_PRIORITY);
		priority = item->JobGetPriority();
		idx = ComboBox_AddString(ditem,TEXT("最高"));
		if(priority == BG_JOB_PRIORITY_FOREGROUND)
		{
			ComboBox_SetCurSel(ditem, idx);
		}
		idx = ComboBox_AddString(ditem,TEXT("高"));
		if(priority == BG_JOB_PRIORITY_HIGH)
		{
			ComboBox_SetCurSel(ditem, idx);
		}
		idx = ComboBox_AddString(ditem,TEXT("通常"));
		if(priority == BG_JOB_PRIORITY_NORMAL)
		{
			ComboBox_SetCurSel(ditem, idx);
		}
		idx = ComboBox_AddString(ditem,TEXT("低"));
		if(priority == BG_JOB_PRIORITY_LOW)
		{
			ComboBox_SetCurSel(ditem, idx);
		}

		num = item->JobGetMinimumRetryDelay();
		::SendDlgItemMessage(dlg, IDC_RETRY_UD, UDM_SETRANGE32, 1, INT_MAX);
		::SetDlgItemInt(dlg, IDC_RETRY, (UINT)num, FALSE);
		num = item->JobGetNoProgressTimeout();
		::SendDlgItemMessage(dlg, IDC_TIMEOUT_UD, UDM_SETRANGE32, 1, INT_MAX);
		::SetDlgItemInt(dlg, IDC_TIMEOUT, (UINT)num, FALSE);

		if(item->GetJobInterfaceVersion() < 2)
		{
			::EnableWindows(dlg, IDC_FIN_EXEC_LABEL, FALSE);
		}
		else
		{
			if(item->JobGetNotifyCommandLine(&wname, &warg))
			{
				JobList::set_dialog_item_unicode_text(dlg, IDC_FIN_EXEC, wname);
				JobList::set_dialog_item_unicode_text(dlg, IDC_FIN_ARG, warg);
				::CoTaskMemFree(warg);
				::CoTaskMemFree(wname);
			}
		}

		::SetProp(dlg, TEXT("JobItem"), (HANDLE)item);
		r = TRUE;
		break;

	default:
		r = FALSE;
		break;
	}

	return r;
}

INT_PTR CALLBACK JobList::job_property_proxy_proc(HWND dlg, UINT msg, WPARAM w, LPARAM l)
{
	INT_PTR r;
	JobItem *item;
	BG_JOB_PROXY_USAGE proxy;
	PWSTR list,blist;
	int id;

	switch(msg)
	{
	case WM_DESTROY:
		::RemoveProp(dlg, TEXT("JobItem"));
		Debug(TEXT("job_property_proxy_proc: WM_DESTROY\n"));
		r = TRUE;
		break;

	case WM_COMMAND:
		switch(LOWORD(w))
		{
		case IDC_USE_INETCFG:
		case IDC_NO_PROXY:
		case IDC_CUSTOM_PROXY:
		case IDC_AUTODETECT:
			if(HIWORD(w) == BN_CLICKED)
			{
				item = (JobItem*)::GetProp(dlg, TEXT("JobItem"));
				id = LOWORD(w);
				::EnableWindows(dlg, IDC_PROXYLIST_LABEL, (id == IDC_CUSTOM_PROXY) ? TRUE : FALSE);
				Debug(TEXT("job_property_proxy_proc: item=%p\n"),item);
				if(item)
				{
					PropSheet_Changed(::GetParent(dlg), dlg);
				}
			}
			break;
		}
		r = TRUE;
		break;

	case WM_INITDIALOG:
		item = (JobItem*)((const PROPSHEETPAGE*)l)->lParam;
		::SetProp(dlg, TEXT("JobItem"), (HANDLE)0);

		item->JobGetProxySettings(&proxy, &list, &blist);
		switch(proxy)
		{
		case BG_JOB_PROXY_USAGE_PRECONFIG:
			id = IDC_USE_INETCFG;
			break;
		case BG_JOB_PROXY_USAGE_NO_PROXY:
			id = IDC_NO_PROXY;
			break;
		case BG_JOB_PROXY_USAGE_OVERRIDE:
			id = IDC_CUSTOM_PROXY;
			break;
		case BG_JOB_PROXY_USAGE_AUTODETECT:
			id = IDC_AUTODETECT;
			break;
		default:
			id = 0;
		}
		if(id)
		{
			Button_SetCheck(::GetDlgItem(dlg, id), BST_CHECKED);
			::SendMessage(dlg, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), 0);
		}

		::CoTaskMemFree(blist);
		::CoTaskMemFree(list);

		::SetProp(dlg, TEXT("JobItem"), (HANDLE)item);
		r = TRUE;
		break;

	default:
		r = FALSE;
		break;
	}

	return r;
}

INT_PTR CALLBACK JobList::job_property_credentials_proc(HWND dlg, UINT msg, WPARAM w, LPARAM l)
{
	INT_PTR r;
	JobItem *item;
	CredentialsDialog *cred;

	switch(msg)
	{
	case WM_DESTROY:
		::RemoveProp(dlg, TEXT("JobItem"));
		Debug(TEXT("job_property_credentials_proc: WM_DESTROY\n"));
		r = TRUE;
		break;

	case WM_COMMAND:
		switch(LOWORD(w))
		{
		case IDC_CRED_SRV:
		case IDC_CRED_PROXY:
			if(HIWORD(w) == BN_CLICKED)
			{
				item = (JobItem*)::GetProp(dlg, TEXT("JobItem"));
				cred = new CredentialsDialog(NULL);
				if(cred->Show(dlg))
				{
					PropSheet_Changed(::GetParent(dlg), dlg);
				}
				delete cred;
			}
			break;
		}
		r = TRUE;
		break;

	case WM_INITDIALOG:
		item = (JobItem*)((const PROPSHEETPAGE*)l)->lParam;
		::SetProp(dlg, TEXT("JobItem"), (HANDLE)item);
		r = TRUE;
		break;

	default:
		r = FALSE;
		break;
	}

	return r;
}
