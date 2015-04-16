#include <tchar.h>
#include <windows.h>
#include "main.hpp"

extern "C" int APIENTRY _tWinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPTSTR lpCmdLine,int nShowCmd)
{
	result_t r;

	r.w_hresult = CoInitializeEx(NULL,COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if(r.w_hresult == S_OK)
	{
		CoUninitialize();
	}
	else
	{
		MessageBoxf(NULL,NULL,MB_ICONERROR,TEXT("COMの初期化に失敗しました。"));
		r.c_int = 1;
	}

	
	return r.c_int;
}
