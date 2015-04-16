#include <tchar.h>
#include <windows.h>
#include <windowsx.h>
#include "myedbox_sc.h"

static const BYTE referer_bmp[] = {0x33,0x00,0x33,0x00};

HINSTANCE inst;

static void RedrawNC(HWND hwnd)
{
	SetWindowPos(hwnd,0,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_DRAWFRAME);
}

static void button_rect(EDITBOXDESC *d,RECT *r)
{
	r->right -= d->ButtonRect.right;
	r->left = r->right - 20;
	r->top += d->ButtonRect.top;
	r->bottom -= d->ButtonRect.bottom;

	if(r->right > r->left)
		OffsetRect(r,d->ButtonRect.right - d->ButtonRect.left,0);
}

unsigned short GetEditBoxWindowHeight(HWND window_handle,HFONT font,unsigned int lines)
{
	HDC dc;
	HFONT default_font;
	int sm_cyedge,r;
	TEXTMETRIC tmet;
	DWORD style,style_ex;
	RECT rect;

	style = (DWORD)GetWindowLongPtr(window_handle,GWL_STYLE);
	style_ex = (DWORD)GetWindowLongPtr(window_handle,GWL_EXSTYLE);

	/* 現在のフォント(DEFAULT_GUI_FONT)のメトリック */
	if(!(dc = GetDC(window_handle)))
	{
		GetWindowRect(window_handle,&rect);
		return (unsigned short)(rect.bottom - rect.top);
	}
	if(font)
		default_font = (HFONT)SelectObject(dc,(HGDIOBJ)font);
	GetTextMetrics(dc,&tmet);
	if(font)
		SelectObject(dc,(HGDIOBJ)default_font);
	ReleaseDC(window_handle,dc);

	sm_cyedge = 2 * GetSystemMetrics((style_ex & WS_EX_CLIENTEDGE) ? SM_CYEDGE : SM_CYBORDER);
	/* 単一行のエディットボックスで、文字の上下が欠けない程度の高さ */
	/* SM_CYEDGEは3D効果のくぼみを表現するための高さ */
	/* 2は既定の余白(上に1px、下に1px)なのでそれも計算に含める */
	/* これらにフォントの高さを合わせると、必要最低限の高さがわかる */
	r = 2 + sm_cyedge;
	if(style & ES_MULTILINE)
		r += (tmet.tmHeight * lines);
	else
		r += tmet.tmHeight;
	/* WS_BORDER、WS_EX_CLIENTEDGEの両方を指定している場合はSM_CYBORDERも入れる */
	if((style_ex & WS_EX_CLIENTEDGE) && (style & WS_BORDER))
		r += (2 * GetSystemMetrics(SM_CYBORDER));

	return (unsigned short)r;
}

static LRESULT CALLBACK my_editbox(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	EDITBOXDESC *d;
	HANDLE heap;
	WNDPROC default_proc;
	LRESULT r;
	HDC dc;
	RECT rc;
	BOOLEAN s;
	POINT pt;
	int tp[2];
	WNDCLASSEX wnd_class;

	d = (EDITBOXDESC*)GetWindowLongPtr(w,GWLP_USERDATA);
	r = 0;
	switch(msg)
	{
	case WM_SETFONT: /* 0x0030 */
		r = d->DefaultWndProc(w,WM_SETFONT,wp,lp);
		GetWindowRect(w,&rc);
		OffsetRect(&rc,-rc.left,-rc.top);
		SetWindowPos(w,NULL,0,0,rc.right,GetEditBoxWindowHeight(w,(HFONT)wp,1),SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
		break;
	case WM_NCCREATE: /* 0x0081 */
		heap = GetProcessHeap();
		if(!(d = HeapAlloc(heap,HEAP_ZERO_MEMORY,sizeof(EDITBOXDESC))))
			return FALSE;

		wnd_class.cbSize = sizeof(WNDCLASSEX);
		GetClassInfoEx(inst,WINUSER_WNDCLASS_EDIT,&wnd_class);

		d->Heap = heap;
		d->DefaultWndProc = wnd_class.lpfnWndProc;
		d->OldUserData = SetWindowLongPtr(w,GWLP_USERDATA,(LONG_PTR)d);

		if(dc = GetDC(w))
		{
			d->ButtonLabel.DC = CreateCompatibleDC(dc);
			d->ButtonLabel.Bitmap = CreateBitmap(REFERER_BMP_WIDTH,REFERER_BMP_HEIGHT,1,1,referer_bmp);
			d->ButtonLabel.DefaultBitmap = (HBITMAP)SelectObject(d->ButtonLabel.DC,(HGDIOBJ)d->ButtonLabel.Bitmap);
			ReleaseDC(w,dc);
		}

		SetWindowPos(w,0,0,0,0,0,SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
		r = d->DefaultWndProc(w,WM_NCCREATE,wp,lp);
		break;
	case WM_NCDESTROY: /* 0x0082 */
		if(d->ButtonLabel.DC)
		{
			SelectObject(d->ButtonLabel.DC,(HGDIOBJ)d->ButtonLabel.DefaultBitmap);
			DeleteObject(d->ButtonLabel.Bitmap);
			DeleteDC(d->ButtonLabel.DC);
		}
		heap = d->Heap;
		default_proc = d->DefaultWndProc;
		HeapFree(heap,0,d);
		r = default_proc(w,WM_NCDESTROY,wp,lp);
		break;
	case WM_NCCALCSIZE: /* 0x0083 */
		memcpy(&rc,(void*)lp,sizeof(RECT));
		d->DefaultWndProc(w,WM_NCCALCSIZE,wp,lp);
		d->ButtonRect.left = ((const RECT*)lp)->left - rc.left;
		d->ButtonRect.right = rc.right - ((const RECT*)lp)->right;
		d->ButtonRect.top = ((const RECT*)lp)->top - rc.top;
		d->ButtonRect.bottom = rc.bottom - ((const RECT*)lp)->bottom;
		((RECT*)lp)->right -= 20;
		break;
	case WM_NCHITTEST: /* 0x0084 */
		pt.x = GET_X_LPARAM(lp);
		pt.y = GET_Y_LPARAM(lp);
		GetWindowRect(w,&rc);
		button_rect(d,&rc);
		if(PtInRect(&rc,pt))
			r = HTBORDER;
		else
			r = d->DefaultWndProc(w,WM_NCHITTEST,wp,lp);
		break;
	case WM_NCPAINT: /* 0x0085 */
		d->DefaultWndProc(w,WM_NCPAINT,wp,lp);
		GetWindowRect(w,&rc);
		OffsetRect(&rc,-rc.left,-rc.top);
		button_rect(d,&rc);
		if(dc = GetWindowDC(w))
		{
			tp[0] = rc.left + ((rc.right - rc.left) - REFERER_BMP_WIDTH) / 2;
			tp[1] = rc.top + ((rc.bottom - rc.top) - REFERER_BMP_HEIGHT) / 2;
			DrawEdge(dc,&rc,EDGE_RAISED,BF_RECT | BF_ADJUST | (d->Pressed ? BF_FLAT : 0));
			FillRect(dc,&rc,GetSysColorBrush(COLOR_BTNFACE));
			if(d->Pressed)
			{
				tp[0]++;
				tp[1]++;
			}
			/* SRCANDで白黒ビットマップを描画すると、黒いところだけ描画される */
			BitBlt(dc,tp[0],tp[1],REFERER_BMP_WIDTH,REFERER_BMP_HEIGHT,d->ButtonLabel.DC,0,0,SRCAND);
			ReleaseDC(w,dc);
		}
		break;
	case WM_NCLBUTTONDOWN: /* 0x00A1 */
		pt.x = GET_X_LPARAM(lp);
		pt.y = GET_Y_LPARAM(lp);
		GetWindowRect(w,&rc);
		button_rect(d,&rc);
		if(PtInRect(&rc,pt))
		{
			SetCapture(w);
			d->Pressed = TRUE;
			d->Mouse = TRUE;
			RedrawNC(w);
		}
		break;
	case WM_MOUSEMOVE: /* 0x0200 */
		if(d->Mouse)
		{
			pt.x = GET_X_LPARAM(lp);
			pt.y = GET_Y_LPARAM(lp);
			ClientToScreen(w,&pt);
			GetWindowRect(w,&rc);
			button_rect(d,&rc);
			s = d->Pressed;
			d->Pressed = PtInRect(&rc,pt) ? TRUE : FALSE;
			if(s != d->Pressed)
				RedrawNC(w);
		}
		else
			r = d->DefaultWndProc(w,WM_MOUSEMOVE,wp,lp);
		break;
	case WM_LBUTTONUP: /* 0x0202 */
		if(d->Mouse)
		{
			pt.x = GET_X_LPARAM(lp);
			pt.y = GET_Y_LPARAM(lp);
			ClientToScreen(w,&pt);
			GetWindowRect(w,&rc);
			button_rect(d,&rc);
			ReleaseCapture();
			d->Pressed = FALSE;
			d->Mouse = FALSE;
			RedrawNC(w);
			SendMessage(w,WM_NULL,0,0);
			if(PtInRect(&rc,pt))
				PostMessage(GetParent(w),WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(w),BN_CLICKED),(LPARAM)w);
		}
		else
			r = d->DefaultWndProc(w,WM_LBUTTONUP,wp,lp);
		break;
	default:
		r = d->DefaultWndProc(w,msg,wp,lp);
		break;
	}

	return r;
}

#if 0
HWND init_editbox(HINSTANCE inst,HWND parent,UINT id,const POINT *pos,const SIZE *size,DWORD style,DWORD exstyle)
{
	return CreateWindowEx(exstyle,SUBCLASSED_EDIT,NULL,WS_CHILD | WS_VISIBLE | style,pos->x,pos->y,size->cx,size->cy,parent,(HMENU)id,inst,NULL);
}
#endif

ATOM prepare_editbox(HINSTANCE xinst)
{
	/* エディットコントロールのスーパークラスを作成 */
	WNDCLASSEX wnd_class;

	inst = xinst;

	wnd_class.cbSize = sizeof(WNDCLASSEX);
	GetClassInfoEx(inst,WINUSER_WNDCLASS_EDIT,&wnd_class);
	wnd_class.cbClsExtra += sizeof(WNDPROC);
	wnd_class.lpfnWndProc = my_editbox;
	wnd_class.lpszClassName = SUBCLASSED_EDIT;

	return RegisterClassEx(&wnd_class);
}
