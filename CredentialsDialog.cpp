#include "CredentialsDialog.h"
#include <windows.h>
#include <windowsx.h>
#include "resource.h"
#include "misc.h"

CredentialsDialog::CredentialsDialog(const BG_AUTH_CREDENTIALS *info)
{
	if(info)
	{
		//::memcpy(&cred_info, info, sizeof(BG_AUTH_CREDENTIALS));
		this->cred_info = *info;
	}
	else
	{
		::memset(&cred_info, 0, sizeof(BG_AUTH_CREDENTIALS));
	}

	this->instance = NULL;
}

CredentialsDialog::~CredentialsDialog()
{
}

bool CredentialsDialog::Show(HWND dlg)
{
	BG_AUTH_CREDENTIALS temp;
	bool r;

	// キャンセルしたときに書き戻す
	//::memcpy(&temp, &cred_info, sizeof(BG_AUTH_CREDENTIALS));
	temp = this->cred_info;

	r = (::DialogBoxParam(instance, MAKEINTRESOURCE(IDD_PROXYCRED), dlg, CredentialsDialog::cred_proc, (LPARAM)this) == IDOK);
	if(!r)
	{
		//::memcpy(&cred_info, &temp, sizeof(BG_AUTH_CREDENTIALS));
		this->cred_info = temp;
	}

	return r;
}

void CredentialsDialog::GetCredentials(BG_AUTH_CREDENTIALS *info)
{
	//::memcpy(info, &cred_info, sizeof(BG_AUTH_CREDENTIALS));
	*info = this->cred_info;
}

INT_PTR CALLBACK CredentialsDialog::cred_proc(HWND dlg, UINT msg, WPARAM w, LPARAM l)
{
	INT_PTR r;
	CredentialsDialog *cred;
	int i;
	HWND ditem;

	switch(msg)
	{
	case WM_DESTROY:
		::RemoveProp(dlg, TEXT("CredentialsDialog"));
		r = TRUE;
		break;

	case WM_CLOSE:
		::EndDialog(dlg, w);
		r = FALSE;
		break;

	case WM_COMMAND:
		switch(LOWORD(w))
		{
		case IDOK:
		case IDCANCEL:
			::PostMessage(dlg, WM_CLOSE, LOWORD(w), 0);
			break;
		case IDC_SCHEME:
			if(HIWORD(w) == CBN_SELCHANGE)
			{
				i = ComboBox_GetCurSel(GetDlgItem(dlg, IDC_SCHEME));
				::EnableWindows(dlg, IDC_USER_LABEL, i ? TRUE : FALSE);
			}
			break;
		}
		r = TRUE;
		break;

	case WM_INITDIALOG:
		cred = (CredentialsDialog*)l;
		::SetProp(dlg, TEXT("CredentialsDialog"), (HANDLE)cred);

		ditem = ::GetDlgItem(dlg, IDC_SCHEME);
		ComboBox_AddString(ditem, TEXT("なし"));
		ComboBox_AddString(ditem, TEXT("基本認証"));
		ComboBox_AddString(ditem, TEXT("ダイジェスト"));
		ComboBox_AddString(ditem, TEXT("NTLM認証"));
		ComboBox_AddString(ditem, TEXT("自動検出"));
		ComboBox_AddString(ditem, TEXT(".NETパスポート"));
		ComboBox_SetCurSel(ditem, 0);

		::CenteringWindowToParent(dlg, (HWND)::GetWindowLongPtr(dlg, GWLP_HWNDPARENT));

		r = TRUE;
		break;

	default:
		r = FALSE;
		break;
	}

	return r;
}
