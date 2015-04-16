#include <windows.h>
#include <bits.h>

class CredentialsDialog
{
	BG_AUTH_CREDENTIALS cred_info;
	HINSTANCE instance;

public:
	CredentialsDialog(const BG_AUTH_CREDENTIALS*);
	~CredentialsDialog();
	bool Show(HWND);
	void GetCredentials(BG_AUTH_CREDENTIALS*);
private:
	static INT_PTR CALLBACK cred_proc(HWND,UINT,WPARAM,LPARAM);
};
