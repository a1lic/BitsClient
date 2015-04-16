#include "JobItem.h"
#include <tchar.h>
#include <stdio.h>

const PCTSTR JobItem::state_text[] = {TEXT("キュー"),TEXT("接続中"),TEXT("転送中"),TEXT("停止中"),TEXT("エラー"),TEXT("転送エラー"),TEXT("完了"),TEXT("問い合わせ中"),TEXT("中止")};
const PCTSTR JobItem::type_text[] = {TEXT("ダウンロード"),TEXT("アップロード"),TEXT("アップロード応答")};

JobItem::JobItem(IBackgroundCopyManager *mgr,const GUID *id)
{
	HRESULT Result;
	//::memcpy(&job_id,id,sizeof(GUID));
	//::memcpy_s(&job_id, sizeof(this->job_id), id, sizeof(GUID));

	this->job_id = *id;
	this->guid_to_str();

	this->item_job = nullptr;
	Result = mgr->GetJob(*id, &this->item_job);

	if(this->item_job == nullptr)
	{
		throw Result;
	}

	// 追加された機能を使えるように、QueryInterfaceを呼び出す
	if(this->item_job->QueryInterface(IID_IBackgroundCopyJob5, (void**)&this->item_job_5) != S_OK)
	{
		this->item_job_5 = nullptr;
		if(this->item_job->QueryInterface(IID_IBackgroundCopyJob4, (void**)&this->item_job_4) != S_OK)
		{
			this->item_job_4 = nullptr;
			if(this->item_job->QueryInterface(IID_IBackgroundCopyJob3, (void**)&this->item_job_3) != S_OK)
			{
				this->item_job_3 = nullptr;
				if(this->item_job->QueryInterface(IID_IBackgroundCopyJob2, (void**)&this->item_job_2) != S_OK)
				{
					this->item_job_2 = nullptr;
				}
			}
		}
	}

	name = nullptr;
	type = BG_JOB_TYPE_DOWNLOAD;
	state = BG_JOB_STATE_QUEUED;
	update_time = 0;
}

JobItem::~JobItem()
{
	if(this->item_job_5)
	{
		this->item_job_5->Release();
	}
	if(this->item_job_4)
	{
		this->item_job_4->Release();
	}
	if(this->item_job_3)
	{
		this->item_job_3->Release();
	}
	if(this->item_job_2)
	{
		this->item_job_2->Release();
	}

	item_job->Release();

	if(name)
	{
		delete[] name;
	}

	//::OutputDebugString(TEXT("~JobItem()\n"));
}

void JobItem::UpdateStatus()
{
	size_t chars;
	PWSTR ole_str;
	BG_JOB_STATE jstate;
	BG_JOB_TYPE jtype;
	BG_JOB_PROGRESS jprogress;

	::QueryPerformanceCounter((LARGE_INTEGER*)&this->update_time);

	ole_str = nullptr;
	this->item_job->GetDisplayName(&ole_str);
	if(ole_str)
	{
		chars = ::wcslen(ole_str);
		if(name)
		{
			delete[] name;
		}

#ifdef UNICODE
		chars++;
		name = new wchar_t[chars];
		::wcscpy_s(name, chars, ole_str);
#else
		chars = WideCharToMultiByte(CP_ACP, 0, ole_str, -1, NULL, 0, NULL, NULL);
		name = new char[chars];
		WideCharToMultiByte(CP_ACP, 0, ole_str, -1, name, chars, NULL, NULL);
#endif
		::CoTaskMemFree(ole_str);
	}

	this->item_job->GetState(&jstate);
	this->item_job->GetType(&jtype);
	this->item_job->GetProgress(&jprogress);

	if(jstate != this->state)
	{
		this->state = jstate;
		this->update_flags[2] = true;
	}
	if(jtype != this->type)
	{
		this->type = jtype;
		this->update_flags[3] = true;
	}
	if(jprogress.BytesTotal != this->total_bytes)
	{
		this->total_bytes = jprogress.BytesTotal;
		::_stprintf_s(this->num_total, 32, _T("%llu"), this->total_bytes);
		this->update_flags[4] = true;
	}
	if(jprogress.BytesTransferred != this->complete_bytes)
	{
		this->complete_bytes = jprogress.BytesTransferred;
		::_stprintf_s(this->num_complete, 32, _T("%llu"), this->complete_bytes);
		this->update_flags[5] = true;
	}
}

UINT64 JobItem::GetUpdateTime()
{
	return this->update_time;
}

void JobItem::GetJobID(GUID *id)
{
	//::memcpy(id, &job_id, sizeof(GUID));
	*id = job_id;
}

bool JobItem::IsInfoUpdated(int n)
{
	bool r;

	if((n < 0) || (n >= 6))
	{
		return false;
	}

	r = this->update_flags[n];
	this->update_flags[n] = false;

	return r;
}

unsigned char JobItem::GetJobInterfaceVersion()
{
	unsigned char r;

	if(this->item_job_5)
	{
		r = 5;
	}
	else if(this->item_job_4)
	{
		r = 4;
	}
	else if(this->item_job_3)
	{
		r = 3;
	}
	else if(this->item_job_2)
	{
		r = 2;
	}
	else
	{
		r = 1;
	}

	return r;
}

BG_JOB_PRIORITY JobItem::JobGetPriority()
{
	BG_JOB_PRIORITY p;

	p = (BG_JOB_PRIORITY)0;
	this->item_job->GetPriority(&p);
	return p;
}

ULONG JobItem::JobGetMinimumRetryDelay()
{
	ULONG p;

	p = 0;
	this->item_job->GetMinimumRetryDelay(&p);
	return p;
}

ULONG JobItem::JobGetNoProgressTimeout()
{
	ULONG p;

	p = 0;
	this->item_job->GetNoProgressTimeout(&p);
	return p;
}

void JobItem::GetDispInfo(NMLVDISPINFO *info)
{
	if(info->item.mask & (LVIF_PARAM | LVIF_TEXT))
	{
		switch(info->item.iSubItem)
		{
		case 0: // ジョブ名
			info->item.pszText = ((JobItem*)info->item.lParam)->name;
			break;
		case 1: // GUID
			info->item.pszText = ((JobItem*)info->item.lParam)->job_id_str;
			break;
		case 2: // 状態
			info->item.pszText = (LPTSTR)state_text[((JobItem*)info->item.lParam)->state];
			break;
		case 3: // モード
			info->item.pszText = (LPTSTR)type_text[((JobItem*)info->item.lParam)->type];
			break;
		case 4: // ファイルサイズ
			info->item.pszText = ((JobItem*)info->item.lParam)->num_total;
			break;
		case 5: // 転送済みサイズ
			info->item.pszText = ((JobItem*)info->item.lParam)->num_complete;
			break;
		}
	}
}

void JobItem::DrawListItem(JobList *lv,const DRAWITEMSTRUCT *draw_info)
{
	//HGDIOBJ old;
	HWND h;
#if 0
	HICON icon;
#endif
	int y;
	int sdc;
	RECT drect, trect, pbar_rect;
	LVITEM item;
	//HDITEM hi;
	PTSTR item_text;
	unsigned char header_index;
	bool isfocus;
	int tc, bc;
	double rate;

	sdc = ::SaveDC(draw_info->hDC);

	h = lv->GetWindow();
	isfocus = (GetFocus() == h);

	//CopyRect(&drect,&draw_info->rcItem);
	y = (draw_info->rcItem.bottom - draw_info->rcItem.top) / 2;

	if(draw_info->itemState & ODS_SELECTED)
	{
		if(isfocus)
		{
			bc = COLOR_HIGHLIGHT;
			tc = COLOR_HIGHLIGHTTEXT;
		}
		else
		{
			bc = COLOR_BTNFACE;
			tc = COLOR_BTNTEXT;
		}
	}
	else
	{
		bc = COLOR_WINDOW;
		tc = COLOR_WINDOWTEXT;
	}
	ListView_GetSubItemRect(h, draw_info->itemID, 5, LVIR_LABEL, &pbar_rect);
	::CopyRect(&drect, &draw_info->rcItem);
	drect.right = pbar_rect.left;
	::FillRect(draw_info->hDC, &drect, ::GetSysColorBrush(bc));

	::SetTextColor(draw_info->hDC, ::GetSysColor(tc));

	item_text = new TCHAR[1024];
	for(header_index = 0; header_index < 5; header_index++)
	{
		item.mask = LVIF_TEXT;
		item.iItem = draw_info->itemID;
		item.iSubItem = header_index;
		item.pszText = item_text;
		item.cchTextMax = 1024;
		ListView_GetItem(h, &item);

		if(!header_index)
		{
			ListView_GetItemRect(h, draw_info->itemID, &drect, LVIR_LABEL);
		}
		else
		{
			ListView_GetSubItemRect(h, draw_info->itemID, header_index, LVIR_LABEL, &drect);
		}
		::CopyRect(&trect, &drect);

		::InflateRect(&trect, -1, 0);
		::DrawText(draw_info->hDC, item_text, -1, &trect, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS | DT_VCENTER);
	}

	::CopyRect(&drect, &pbar_rect);
	::CopyRect(&trect, &pbar_rect);

	item.mask = LVIF_TEXT;
	item.iItem = draw_info->itemID;
	item.iSubItem = header_index;
	item.pszText = item_text;
	item.cchTextMax = 1024;
	ListView_GetItem(h,&item);

	if((((JobItem*)draw_info->itemData)->total_bytes) && (((JobItem*)draw_info->itemData)->total_bytes != BG_SIZE_UNKNOWN))
	{
		rate = (double)((JobItem*)draw_info->itemData)->complete_bytes / (double)((JobItem*)draw_info->itemData)->total_bytes;
		drect.right = drect.left + (LONG)((drect.right - drect.left) * rate);
		JobItem::fill_gradiant(draw_info->hDC, &drect, 0xffffff, 0xc0ffc0);
		drect.left = drect.right;
		drect.right = trect.right;
	}
	JobItem::fill_gradiant(draw_info->hDC, &drect, 0xffffff, 0xc0c0c0);

	::SetTextColor(draw_info->hDC, GetSysColor(COLOR_WINDOWTEXT));
	::InflateRect(&trect, -1, 0);
	::DrawText(draw_info->hDC, item_text, -1, &trect, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS | DT_VCENTER);
	delete [] item_text;

	::RestoreDC(draw_info->hDC, sdc);

	if(isfocus && (draw_info->itemState & ODS_FOCUS))
	{
		::CopyRect(&drect, &draw_info->rcItem);
		drect.right = pbar_rect.left;
		::DrawFocusRect(draw_info->hDC, &drect);
	}
}

void JobItem::guid_to_str()
{
	TCHAR hexb[4];
	unsigned char i;

	::_stprintf_s(job_id_str, 37, _T("%08X-%04hX-%04hX-"), job_id.Data1, job_id.Data2, job_id.Data3);
	for(i = 0; i < 8; i++)
	{
		::_stprintf_s(hexb, 4, _T("%02X"), job_id.Data4[i]);
		::_tcscat_s(job_id_str, 37, hexb);
	}
}

#if 0
PTSTR JobItem::tstrdup(PCTSTR s)
{
	TCHAR *nstr;
	size_t l;

	if(l = _tcslen(s))
	{
		l++;
		nstr = new TCHAR[l];
		_tcscpy_s(nstr,l,s);
	}
	else
		nstr = NULL;

	return nstr;
}
#endif

void JobItem::fill_gradiant(HDC dc,const RECT *r,COLORREF start,COLORREF end)
{
	const static GRADIENT_RECT gr = {0,1};
	TRIVERTEX v[2];

	v[0].x = r->left;
	v[0].y = r->top;
	v[0].Red = GetRValue(start) << 8;
	v[0].Green = GetGValue(start) << 8;
	v[0].Blue = GetBValue(start) << 8;
	v[0].Alpha = 0xff00;

	v[1].x = r->right;
	v[1].y = r->bottom;
	v[1].Red = GetRValue(end) << 8;
	v[1].Green = GetGValue(end) << 8;
	v[1].Blue = GetBValue(end) << 8;
	v[1].Alpha = 0xff00;

	::GradientFill(dc, v, 2, (GRADIENT_RECT*)&gr, 1, GRADIENT_FILL_RECT_V);
}
