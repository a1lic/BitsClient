#pragma once
#include <windows.h>
#include <commctrl.h>
#include <bits.h>
#include "JobList.h"

//enum JobType { JT_DOWNLOAD = 0,JT_UPLOAD,JT_UPLOADREPLY };
//enum JobState { JS_QUEUED = 0,JS_CONNECTING,JS_TRANSFERRING,JS_SUSPENDED,JS_ERROR,JS_TRANSIENT_ERROR,JS_TRANSFERRED,JS_ACKNOWLEDGED,JS_CANCELLED };

class JobItem
{
	GUID job_id;
	BG_JOB_TYPE type;
	BG_JOB_STATE state;
	PTSTR name;
	UINT64 total_bytes;
	UINT64 complete_bytes;
	UINT64 total_bytes_o;
	UINT64 complete_bytes_o;
	UINT64 update_time;
	TCHAR job_id_str[37];
	TCHAR num_total[32];
	TCHAR num_complete[32];
	bool update_flags[6];
	IBackgroundCopyJob *item_job;
	IBackgroundCopyJob2 *item_job_2;
	IBackgroundCopyJob3 *item_job_3;
	IBackgroundCopyJob4 *item_job_4;
	IBackgroundCopyJob5 *item_job_5;

	static const PCTSTR JobItem::state_text[];
	static const PCTSTR JobItem::type_text[];

public:
	JobItem(IBackgroundCopyManager*,const GUID*);
	~JobItem();
	void UpdateStatus();
	UINT64 GetUpdateTime();
	void GetJobID(GUID*);
	bool IsInfoUpdated(int);
	void Complete() { item_job->Complete(); }
	void Abort() { item_job->Cancel(); }
	void Pause() { item_job->Suspend(); }
	void Resume() { item_job->Resume(); }
	unsigned char GetJobInterfaceVersion();
	inline bool JobGetTimes(BG_JOB_TIMES *t) { return (item_job->GetTimes(t) == S_OK); }
	BG_JOB_PRIORITY JobGetPriority();
	ULONG JobGetMinimumRetryDelay();
	ULONG JobGetNoProgressTimeout();
	inline void JobGetProxySettings(BG_JOB_PROXY_USAGE *u,PWSTR *pl,PWSTR *bl) { item_job->GetProxySettings(u,pl,bl); }
	inline bool JobEnumFiles(IEnumBackgroundCopyFiles **f) { return (item_job->EnumFiles(f) == S_OK); }
	inline bool JobGetDescription(PWSTR *s) { return (item_job->GetDescription(s) == S_OK); }
	inline bool JobGetNotifyCommandLine(PWSTR *c,PWSTR *a) { return item_job_2 ? (item_job_2->GetNotifyCmdLine(c,a) == S_OK) : false; }
	inline BG_JOB_STATE JobGetState() { return this->state; }
	static void GetDispInfo(NMLVDISPINFO*);
	static void DrawListItem(class JobList*,const DRAWITEMSTRUCT*);
private:
	void guid_to_str();
	//static PTSTR tstrdup(PCTSTR);
	static void fill_gradiant(HDC,const RECT*,COLORREF,COLORREF);
};
