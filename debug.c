#include <tchar.h>
#include <stdarg.h>
#include <windows.h>

void debug(const void *fmt, ...)
{
	va_list ap;
	TCHAR debug_msg[1024];

	if(IsDebuggerPresent())
	{
		va_start(ap,fmt);
		_vstprintf_s(debug_msg,1024,fmt,ap);
		va_end(ap);

		OutputDebugString(debug_msg);
	}
}
