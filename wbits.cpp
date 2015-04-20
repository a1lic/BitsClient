#if defined(_M_IX86)
#error 32bit OSはさっさと滅べ
#endif

#define OEMRESOURCE
#include "wbits.h"
#include <tchar.h>
#include <commctrl.h>
#include <objbase.h>
#if defined(THREADED)
#include <process.h>
#endif
#include "resource.h"
#include "myedbox_sc.h"
#include "misc.h"

MainWindow::MainWindow(HINSTANCE instance, int show_cmd)
{
	this->window = nullptr;
#if defined(THREADED)
	this->thread = nullptr;
#endif
	this->instance = instance;
	this->show_window = show_cmd;
}

MainWindow::~MainWindow()
{
	if(this->IsValidInstance())
	{
		::SendMessage(this->window, WM_CLOSE, 0, 0);
	}

#if defined(THREADED)
	this->WaitForClose();
#endif
}

void MainWindow::Start()
{
#if defined(THREADED)
	unsigned int tid;
	this->thread = reinterpret_cast<HANDLE>(::_beginthreadex(nullptr, 0u, MainWindow::thread_proc, this, 0, &tid));
#else
	MainWindow::thread_proc(this);
#endif
}

int MainWindow::WaitForClose()
{
#if defined(THREADED)
	DWORD quit_code = 0xFFFFFFFFul;
	if(this->thread)
	{
		::WaitForSingleObject(this->thread, INFINITE);
		::GetExitCodeThread(this->thread, &quit_code);
		::CloseHandle(this->thread);
		this->thread = nullptr;
	}

	return quit_code;
#else
	return 0;
#endif
}

bool MainWindow::create(const CREATESTRUCT * create_param)
{
	::SetWindowLongPtr(this->window, 0, reinterpret_cast<LONG_PTR>(this));

	{
		RECT rect;
		::GetClientRect(this->window, &rect);
		try
		{
			this->job_list = new JobList(this, 1, &rect);
		}
		catch(...)
		{
			return false;
		}
		try
		{
			this->job_status = new JobStatus(this, 2);
		}
		catch(...)
		{
			return false;
		}
	}

	this->job_list->UpdateList();
	this->menu = ::GetMenu(this->window);

	::SetTimer(this->window, 37564, 1000, nullptr);
	return true;
}

void MainWindow::destroy()
{
	::KillTimer(this->window, 37564);
	delete this->job_list;
	delete this->job_status;
	::PostQuitMessage(0);
}

void MainWindow::size(WORD width, WORD height)
{
	this->job_status->SendResizeMessage(width, height);
	this->current_size.cx = width;
	this->current_size.cy = height - this->job_status->GetHeight();
	this->job_list->Resize(&this->current_size);
}

void MainWindow::set_focus()
{
	::SetFocus(this->job_list->GetWindow());
}

void MainWindow::system_color_change()
{
	this->job_list->UpdateWindowStyle();
}

void MainWindow::get_min_max_info(MINMAXINFO * min_max_info)
{
	// ウィンドウが生成されるタイミングではthisはnullptrになっている。
	if(!this)
	{
		return;
	}

	min_max_info->ptMinTrackSize.x = 800l;
	min_max_info->ptMinTrackSize.y = 192l;
}

void MainWindow::draw_item(const DRAWITEMSTRUCT * draw_item)
{
	if((draw_item->CtlType == ODT_LISTVIEW) && (draw_item->CtlID == 1))
	{
		this->job_list->DrawListItem(this->job_list, draw_item);
	}
}

void MainWindow::notify(const NMHDR * notification_header)
{
	auto ctlid = notification_header->idFrom;
	if(ctlid == 0)
	{
		if((notification_header->code <= HDN_FIRST) && (notification_header->code >= HDN_LAST))
		{
			auto parent_window = ::GetParent(notification_header->hwndFrom);
			ctlid = ::GetDlgCtrlID(parent_window);
		}
	}
	if(ctlid == 1)
	{
		this->job_list->Notify(const_cast<NMHDR*>(notification_header));
	}
}

void MainWindow::command(WORD id, WORD type, HWND invoke_from)
{
	if(!type && !invoke_from)
	{
		switch(id)
		{
		case IDC_JOBPROPERTY:
			this->job_list->ShowProperty();
			break;

		case IDC_COMPLETEJOB:
			if(::MessageBox(window, TEXT("完了させるとダウンロード済みの一時ファイルが指定したファイル名に変更され使用可能になります。ジョブを完了しますか？"), TEXT("確認"), MB_ICONQUESTION | MB_YESNO) == IDYES)
			{
				this->job_list->CompleteSelectedJobs();
			}
			break;

		case IDC_ABORTJOB:
			if(::MessageBox(window, TEXT("中止するとダウンロード済みのファイルが削除されます。ジョブを中止しますか？"), TEXT("確認"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2) == IDYES)
			{
				this->job_list->AbortSelectedJobs();
			}
			break;

		case IDC_PAUSEJOB:
			this->job_list->PauseSelectedJobs();
			break;

		case IDC_RESUMEJOB:
			this->job_list->ResumeSelectedJobs();
			break;
		}
	}
}

void MainWindow::timer(UINT_PTR id)
{
	if(static_cast<unsigned int>(id) == 37564u)
	{
		this->job_list->UpdateList();
	}
}

void MainWindow::enter_menu_loop()
{
	auto selected = this->job_list->GetSelectedCount();
	auto state = MF_BYCOMMAND | (selected ? MF_ENABLED : MF_GRAYED);

	::EnableMenuItem(this->menu, IDC_COMPLETEJOB, state);
	::EnableMenuItem(this->menu, IDC_ABORTJOB, state);
	::EnableMenuItem(this->menu, IDC_PAUSEJOB, state);
	::EnableMenuItem(this->menu, IDC_RESUMEJOB, state);
	//EnableMenuItem(this->menu, IDC_JOBPROPERTY, MF_BYCOMMAND | ((selected == 1) ? MF_ENABLED : MF_GRAYED));
}

void MainWindow::theme_changed()
{
	this->job_list->UpdateWindowStyle();
}

LRESULT CALLBACK MainWindow::main_window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto _this = static_cast<MainWindow*>(nullptr);
	auto r = (LRESULT)0;
	if(uMsg != WM_CREATE)
	{
		_this = reinterpret_cast<MainWindow*>(::GetWindowLongPtr(hwnd, 0));
	}

	switch(uMsg)
	{
	case WM_CREATE:
		{
			auto create_struct = reinterpret_cast<const CREATESTRUCT*>(lParam);
			_this = reinterpret_cast<MainWindow*>(create_struct->lpCreateParams);
			_this->window = hwnd;
			r = _this->create(create_struct) ? 0 : -1;
		}
		break;

	case WM_DESTROY:
		_this->destroy();
		break;

	case WM_SIZE:
		_this->size(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_SETFOCUS:
		_this->set_focus();
		break;

	//case WM_SYSCOLORCHANGE:
	//	_this->system_color_change();
	//	break;

	case WM_GETMINMAXINFO:
		_this->get_min_max_info(reinterpret_cast<MINMAXINFO*>(lParam));
		break;

	case WM_DRAWITEM:
		_this->draw_item(reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
		break;

	//case WM_MEASUREITEM:
	//	if((((const MEASUREITEMSTRUCT*)l)->CtlType == ODT_LISTVIEW) && (((const MEASUREITEMSTRUCT*)l)->CtlID == 1))
	//		xthis->JobListWindow->MeasureListItem((MEASUREITEMSTRUCT*)l);
	//	break;

	case WM_NOTIFY:
		_this->notify(reinterpret_cast<const NMHDR*>(lParam));
		break;

	case WM_COMMAND:
		_this->command(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
		break;

	case WM_TIMER:
		_this->timer(static_cast<UINT_PTR>(lParam));
		break;

	case WM_ENTERMENULOOP:
		_this->enter_menu_loop();
		break;

	case WM_THEMECHANGED:
		_this->theme_changed();
		break;

	default:
		r = ::DefWindowProc(hwnd, uMsg, wParam, lParam);
		break;
	}

	return r;
}

unsigned int __stdcall MainWindow::thread_proc(void * argument)
{
	::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); //COINIT_MULTITHREADED

	auto _this = static_cast<MainWindow*>(argument);

	_this->window = ::CreateWindow(MAINWINDOWCLASS, MAINWINDOWTITLE, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, _this->instance, _this);
	if(!_this->window)
	{
#if defined(THREADED)
		_this->thread = nullptr;
#endif
		::CoUninitialize();
		return 1;
	}

	::ShowWindow(_this->window, _this->show_window);
	auto quit_code = MainWindow::do_message_loop();
	_this->window = nullptr;

	::CoUninitialize();
	return static_cast<unsigned int>(quit_code);
}

int MainWindow::do_message_loop()
{
	MSG msg;

	// BOOLは符号付き整数なのでこのような比較でも問題はない
	while(::GetMessage(&msg, nullptr, 0, 0) > 0)
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	return static_cast<int>(msg.wParam);
}

const WNDCLASSEX MainWindow::main_window_class_t = {
	/* cbSize        */ sizeof(WNDCLASSEX),
	/* style         */ 0,
	/* lpfnWndProc   */ MainWindow::main_window_proc,
	/* cbClsExtra    */ 0,
	/* cbWndExtra    */ sizeof(void*), // ポインタを格納する領域を確保する
	/* hInstance     */ nullptr,
	/* hIcon         */ nullptr,
	/* hCursor       */ nullptr,
	/* hbrBackground */ (HBRUSH)(1 + COLOR_3DFACE),
	/* lpszMenuName  */ MAKEINTRESOURCE(IDM_MAIN),
	/* lpszClassName */ MAINWINDOWCLASS,
	/* hIconSm       */ 0 };

extern "C" unsigned __stdcall main_window_thread(void * arg)
{
	auto window = new MainWindow(static_cast<const MAINWINDOWSTRUCT*>(arg)->Instance, static_cast<const MAINWINDOWSTRUCT*>(arg)->ShowWindow);
	window->Start();
	auto exit_code = window->WaitForClose();
	delete window;
	return exit_code;
}

extern "C" void register_main_window_class(HINSTANCE instance, const HICON *icons)
{
	auto main_window_class = MainWindow::main_window_class_t;
	main_window_class.hInstance = instance;
	main_window_class.hIcon = icons[0];
	main_window_class.hCursor = static_cast<HCURSOR>(::LoadImage(nullptr, MAKEINTRESOURCE(OCR_NORMAL), IMAGE_CURSOR, ::GetSystemMetrics(SM_CXCURSOR), ::GetSystemMetrics(SM_CYCURSOR), LR_SHARED));
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

#if defined(_M_IX86)
	*((char*)nullptr) = 0;
#endif

	::InitCommonControlsEx(&common_control_descriptor);
	::prepare_editbox(instance);

	// アイコンをリソースから読み込む
	// Windows95、WindowsNT4以降では小さいアイコンも読み込む必要がある
	icons[0] = static_cast<HICON>(::LoadImage(instance, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), 0));
	icons[1] = static_cast<HICON>(::LoadImage(instance, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0));
	::register_main_window_class(instance, icons);

	main_st.Instance = instance;
	main_st.ShowWindow = show;
	auto quit_code = static_cast<int>(::main_window_thread(&main_st));

	// 後始末
	::UnregisterClass(MAINWINDOWCLASS, instance);
	// アイコンの削除
	::DestroyIcon(icons[1]);
	::DestroyIcon(icons[0]);

	::CoUninitialize();
	return quit_code;
}
