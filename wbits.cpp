#if defined(_M_IX86)
#error 32bit OSはさっさと滅べ
#endif

#define OEMRESOURCE
#include "wbits.h"
#include <tchar.h>
#include <commctrl.h>
#include <objbase.h>
#include "resource.h"
#include "myedbox_sc.h"
#include "misc.h"

static LRESULT CALLBACK main_window_proc(HWND window, UINT message, WPARAM w, LPARAM l)
{
	MAINWINDOWSTRUCT *xthis;
	LRESULT r;
	RECT rect;
	unsigned int selected;
	UINT state;

	xthis = (MAINWINDOWSTRUCT*)::GetWindowLongPtr(window, 0);
	r = 0;

	switch(message)
	{
	case WM_CREATE:
		xthis = (MAINWINDOWSTRUCT*)((const CREATESTRUCT*)l)->lpCreateParams;
		::SetWindowLongPtr(window, 0, (LONG_PTR)xthis);

		::GetClientRect(window, &rect);
		try
		{
			xthis->JobListWindow = new JobList(window, 1, &rect);
		}
		catch(NTSTATUS)
		{
			::PostQuitMessage(1);
			return 0;
		}
		xthis->JobListWindow->UpdateList();

		xthis->Menu = ::GetMenu(window);

		::SetTimer(window, 37564, 1000, NULL);

		break;

	case WM_DESTROY:
		::KillTimer(window, 37564);
		if(xthis->JobListWindow)
		{
			delete xthis->JobListWindow;
		}

		::PostQuitMessage(0);
		break;

	case WM_SIZE:
		rect.left = 0;
		rect.top = 0;
		rect.right = LOWORD(l);
		rect.bottom = HIWORD(l);

		xthis->JobListWindow->Resize(&rect);

		break;

	case WM_SETFOCUS:
		::SetFocus(xthis->JobListWindow->GetWindow());
		break;

	case WM_DRAWITEM:
		if((((const DRAWITEMSTRUCT*)l)->CtlType == ODT_LISTVIEW) && (((const DRAWITEMSTRUCT*)l)->CtlID == 1))
		{
			xthis->JobListWindow->DrawListItem(xthis->JobListWindow, (const DRAWITEMSTRUCT*)l);
		}
		break;

	//case WM_MEASUREITEM:
	//	if((((const MEASUREITEMSTRUCT*)l)->CtlType == ODT_LISTVIEW) && (((const MEASUREITEMSTRUCT*)l)->CtlID == 1))
	//		xthis->JobListWindow->MeasureListItem((MEASUREITEMSTRUCT*)l);
	//	break;

	case WM_NOTIFY:
		if((((const NMHDR*)l)->idFrom == 1) && (((const NMHDR*)l)->code == LVN_GETDISPINFO))
		{
			xthis->JobListWindow->GetDispInfo((NMLVDISPINFO*)l);
		}
		break;

	case WM_COMMAND:
		if(!HIWORD(w) && !l)
		{
			switch(LOWORD(w))
			{
			case IDC_JOBPROPERTY:
				xthis->JobListWindow->ShowProperty();
				break;

			case IDC_COMPLETEJOB:
				if(::MessageBox(window, TEXT("完了させるとダウンロード済みの一時ファイルが指定したファイル名に変更され使用可能になります。ジョブを完了しますか？"), TEXT("確認"), MB_ICONQUESTION | MB_YESNO) == IDYES)
				{
					xthis->JobListWindow->CompleteSelectedJobs();
				}
				break;

			case IDC_ABORTJOB:
				if(::MessageBox(window, TEXT("中止するとダウンロード済みのファイルが削除されます。ジョブを中止しますか？"), TEXT("確認"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2) == IDYES)
				{
					xthis->JobListWindow->AbortSelectedJobs();
				}
				break;

			case IDC_PAUSEJOB:
				xthis->JobListWindow->PauseSelectedJobs();
				break;

			case IDC_RESUMEJOB:
				xthis->JobListWindow->ResumeSelectedJobs();
				break;
			}
		}
		break;

	case WM_TIMER:
		if((UINT_PTR)w == 37564)
		{
			xthis->JobListWindow->UpdateList();
		}
		break;

	case WM_ENTERMENULOOP:
		selected = xthis->JobListWindow->GetSelectedCount();
		state = MF_BYCOMMAND | (selected ? MF_ENABLED : MF_GRAYED);
		::EnableMenuItem(xthis->Menu, IDC_COMPLETEJOB, state);
		::EnableMenuItem(xthis->Menu, IDC_ABORTJOB, state);
		::EnableMenuItem(xthis->Menu, IDC_PAUSEJOB, state);
		::EnableMenuItem(xthis->Menu, IDC_RESUMEJOB, state);

		//EnableMenuItem(xthis->Menu,IDC_JOBPROPERTY,MF_BYCOMMAND | ((selected == 1) ? MF_ENABLED : MF_GRAYED));
		break;

	default:
		r = ::DefWindowProc(window, message, w, l);
		break;
	}

	return r;
}

static unsigned __stdcall main_window_thread(void *arg)
{
	HWND window;
	MSG msg;
	MAINWINDOWSTRUCT *rarg;

	rarg = new MAINWINDOWSTRUCT;
	//::memcpy(rarg, arg, sizeof(MAINWINDOWSTRUCT));
	*rarg = *(const MAINWINDOWSTRUCT*)arg;

	window = ::CreateWindowEx(0, MAINWINDOWCLASS, MAINWINDOWTITLE, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, rarg->Instance, rarg);
	if(window)
	{
		::ShowWindow(window, rarg->ShowWindow);
		// BOOLは符号付き整数なのでこのような比較でも問題はない
		while(::GetMessage(&msg, nullptr, 0, 0) > 0)
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	delete rarg;

	return (int)msg.wParam;
}

static void register_main_window_class(HINSTANCE instance,const HICON *icons)
{
	static const WNDCLASSEX main_window_class_t = {
		/* cbSize        */ sizeof(WNDCLASSEX),
		/* style         */ 0,
		/* lpfnWndProc   */ main_window_proc,
		/* cbClsExtra    */ 0,
		/* cbWndExtra    */ sizeof(void*), // ポインタを格納する領域を確保する
		/* hInstance     */ nullptr,
		/* hIcon         */ nullptr,
		/* hCursor       */ nullptr,
		/* hbrBackground */ (HBRUSH)(1 + COLOR_3DFACE),
		/* lpszMenuName  */ MAKEINTRESOURCE(IDM_MAIN),
		/* lpszClassName */ MAINWINDOWCLASS,
		/* hIconSm       */ 0};
	WNDCLASSEX main_window_class;

	//::memcpy(&main_window_class, &main_window_class_t, sizeof(WNDCLASSEX));
	main_window_class = main_window_class_t;
	main_window_class.hInstance = instance;
	main_window_class.hIcon = icons[0];
	main_window_class.hCursor = (HCURSOR)::LoadImage(NULL, MAKEINTRESOURCE(OCR_NORMAL), IMAGE_CURSOR, ::GetSystemMetrics(SM_CXCURSOR), ::GetSystemMetrics(SM_CYCURSOR), LR_SHARED);
	main_window_class.hIconSm = icons[1];

	::RegisterClassEx(&main_window_class);
}

extern "C" int WINAPI _tWinMain(HINSTANCE instance,HINSTANCE x,PTSTR command,int show)
{
	static const INITCOMMONCONTROLSEX common_control_descriptor = {
		/* dwSize */ sizeof(INITCOMMONCONTROLSEX),
		/* dwICC  */ ICC_LISTVIEW_CLASSES};
	// アイコンハンドル(0=大きいアイコン、1=小さいアイコン)
	HICON icons[2];
	MAINWINDOWSTRUCT main_st;
	int quit_code;

#if defined(_M_IX86)
	*((char*)nullptr) = 0;
#endif

	//CoInitializeEx(NULL,COINIT_MULTITHREADED);
	::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	::InitCommonControlsEx((LPINITCOMMONCONTROLSEX)&common_control_descriptor);
	::prepare_editbox(instance);

	// アイコンをリソースから読み込む
	// Windows95、WindowsNT4以降では小さいアイコンも読み込む必要がある
	icons[0] = (HICON)::LoadImage(instance, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), 0);
	icons[1] = (HICON)::LoadImage(instance, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
	::register_main_window_class(instance, icons);

	main_st.Instance = instance;
	main_st.ShowWindow = show;
	quit_code = (int)::main_window_thread(&main_st);

	// 後始末
	::UnregisterClass(MAINWINDOWCLASS, instance);
	// アイコンの削除
	::DestroyIcon(icons[1]);
	::DestroyIcon(icons[0]);

	::CoUninitialize();
	return quit_code;
}
