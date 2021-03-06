/**
\author		Korotkov Andrey aka DRON
\date		26.04.2012 (c)Korotkov Andrey

This file is a part of DGLE project and is distributed
under the terms of the GNU Lesser General Public License.
See "DGLE.h" for more details.
*/

#include "MainWindow.h"
#include "..\..\..\..\build\windows\engine\resource.h"

extern HMODULE hModule;

CMainWindow::CMainWindow(uint uiInstIdx):
CInstancedObj(uiInstIdx), _hWnd(NULL), _hDC(NULL),
_hInst(GetModuleHandle(NULL)), _bFScreen(false),
_bIsLooping(false)
{}

CMainWindow::~CMainWindow()
{
	if (_hInst && ((InstIdx() == 0 || EngineInstance(0)->pclCore == NULL) &&
		UnregisterClass("DGLEWindowClass", _hInst)==FALSE))
	{
		_hInst = NULL;
		LOG("Can't unregister window class.", LT_ERROR);
	}
	else
		LOG("Window closed properly.", LT_INFO);	
}

int CMainWindow::_wWinMain(HINSTANCE hInstance)
{
	if (!_hWnd)
		return -1;

	_bIsLooping = true;
	
	MSG st_msg = {0};

	LOG("**Entering main loop**", LT_INFO);

	while (_bIsLooping)
		if (PeekMessage(&st_msg, NULL, 0, 0, PM_REMOVE ))
		{
			if (WM_QUIT == st_msg.message) 
				_bIsLooping = false;
			else
			{   
				TranslateMessage(&st_msg);
				DispatchMessage (&st_msg);
			}
		}
		else 
			_pDelMainLoop->Invoke();

	LOG("**Exiting main loop**", LT_INFO);

	Console()->UnRegCom("quit");

	_pDelMessageProc->Invoke(TWinMessage(WMT_RELEASED));

	return (int) st_msg.wParam;
}

void DGLE_API CMainWindow::_s_ConsoleQuit(void *pParametr, const char *pcParam)
{
	if (strlen(pcParam) != 0)
		CON(CMainWindow, "No parametrs expected.");
	else 
		::PostMessage(PTHIS(CMainWindow)->_hWnd, WM_CLOSE, NULL, NULL);
}

LRESULT DGLE_API CMainWindow::_s_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CMainWindow *this_ptr = (CMainWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	if (this_ptr)
	{
		if (message == WM_DESTROY)
			PostQuitMessage(0);

		const TEngInstance &eng_inst = *EngineInstance(this_ptr->InstIdx());

		switch(message)
		{
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
			if (eng_inst.eGetEngFlags & GEF_FORCE_SINGLE_THREAD)
			{
				if (wParam == (uint32)eng_inst.pclConsole->GetWindowHandle())
					wParam = 0;
			}
			else
				break;

		case WM_ACTIVATEAPP:
			if (!(eng_inst.eGetEngFlags & GEF_FORCE_SINGLE_THREAD))
			{
				this_ptr->_pDelMessageProc->Invoke(TWinMessage(wParam == TRUE ? WMT_ACTIVATED : WMT_DEACTIVATED, lParam == eng_inst.pclConsole->GetThreadId() ? 1 : 0));
				break;
			}

		default:
			this_ptr->_pDelMessageProc->Invoke(WinAPIMsgToEngMsg(message, wParam, lParam));
		}

		if ((message == WM_SYSCOMMAND && ( (wParam == SC_KEYMENU && (lParam >> 16) <= 0) || wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER)) || message == WM_CLOSE)
			return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

DGLE_RESULT CMainWindow::InitWindow(TWinHandle tHandle, const TCRendererInitResult &stRndrInitResults, TProcDelegate *pDelMainLoop, TMsgProcDelegate *pDelMsgProc)
{
	_hWnd				= tHandle;
	_pDelMainLoop		= pDelMainLoop;
	_pDelMessageProc	= pDelMsgProc;

	WNDCLASSEX wcex;
	wcex.cbSize 		= sizeof(WNDCLASSEX); 
	wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc    = (WNDPROC)CMainWindow::_s_WndProc;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = _hInst;
	wcex.hIcon          = LoadIcon(hModule, MAKEINTRESOURCE(IDI_ICON1));
	wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground  = (HBRUSH)(0);
	wcex.lpszMenuName   = NULL;
	wcex.lpszClassName  = "DGLEWindowClass";
	wcex.hIconSm        = LoadIcon(hModule, MAKEINTRESOURCE(IDI_ICON1));

	bool need_register = true;

	if (InstIdx()!=0)
	{
		WNDCLASSEX tmp;
		need_register = GetClassInfoEx(_hInst, "DGLEWindowClass", &tmp) == FALSE;
	}

	if (need_register && RegisterClassEx(&wcex) == FALSE)
	{
		LOG("Couldn't register window class!", LT_FATAL);
		return E_FAIL;
	}

	if (_hWnd)
		return E_INVALIDARG;

	_hWnd = CreateWindowEx(WS_EX_APPWINDOW, "DGLEWindowClass", "DGLE Application", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 320, 240, NULL, NULL, _hInst, NULL);

	if (!_hWnd)
	{
		_hWnd = NULL;
		LOG("Failed to create window.", LT_FATAL);
		return E_FAIL;
	}

	SetWindowLongPtr(_hWnd, GWLP_USERDATA, (LONG_PTR)this);

	if (!(_hDC = GetDC(_hWnd)))
	{
		LOG("Can't get window Draw Context.", LT_FATAL);
		return E_FAIL;
	}

	Console()->RegComProc("quit", "Quits engine and releases all resources.", &_s_ConsoleQuit, (void*)this);

	LOG("Window created successfully.", LT_INFO);

	return S_OK;
}

DGLE_RESULT CMainWindow::SendMessage(const TWinMessage &stMsg)
{
	if (!_hWnd)
		return E_FAIL;

	UINT msg; WPARAM wparam; LPARAM lparam;
	EngMsgToWinAPIMsg(stMsg, msg, wparam, lparam);
	::SendMessage(_hWnd, msg, wparam, lparam);

	return S_OK;
}

DGLE_RESULT CMainWindow::GetWindowAccessType(E_WINDOW_ACCESS_TYPE &eType)
{
	eType = WAT_FULL_ACCESS;

	return S_OK;
}

DGLE_RESULT CMainWindow::GetWindowHandle(TWinHandle &stHandle)
{
	stHandle = _hWnd;

	return S_OK;
}

DGLE_RESULT CMainWindow::GetDrawContext(HDC &hDC)
{
	if (!_hDC)
		return E_FAIL;

	hDC = _hDC;

	return S_OK;
}

DGLE_RESULT CMainWindow::GetWinRect(int &iX, int &iY, int &iWidth, int &iHeight)
{
	if (!_hWnd)
		return E_FAIL;

	RECT rect;
	
	if (GetClientRect(_hWnd, &rect) == FALSE)
	{
		iX = iY = iWidth = iHeight = 0;
		LOG("Can't get window client rect.", LT_ERROR);
		return E_FAIL;
	}

	POINT lt, rb;
	
	lt.x = rect.left;
	lt.y = rect.top;
	rb.x = rect.right;
	rb.y = rect.bottom;

	ClientToScreen(_hWnd, &lt);
	ClientToScreen(_hWnd, &rb);

	iX = lt.x;
	iY = lt.y;
	iWidth = rb.x - lt.x;
	iHeight = rb.y - lt.y;

	return S_OK;
}

DGLE_RESULT CMainWindow::ScreenToClient(int &iX, int &iY)
{
	if (!_hWnd)
		return E_FAIL;

	POINT p;

	p.x = iX; p.y = iY;
	::ScreenToClient(_hWnd, &p);
	iX = p.x; iY = p.y;

	return S_OK;
}

DGLE_RESULT CMainWindow::SetCaption(const char *pcTxt)
{
	if (!_hWnd)
		return E_FAIL;

	SetWindowText(_hWnd, pcTxt);

	return S_OK;
}

DGLE_RESULT CMainWindow::Minimize()
{
	if (!_hWnd)
		return E_FAIL;

	ShowWindow(_hWnd, SW_MINIMIZE);

	return S_OK;
}

DGLE_RESULT CMainWindow::BeginMainLoop()
{
	return _wWinMain(GetModuleHandle(NULL)) != -1 ? S_OK : E_FAIL;
}

DGLE_RESULT CMainWindow::KillWindow()
{
	if (_hDC && !ReleaseDC(_hWnd,_hDC))
		LOG("Failed to release Device Context.", LT_ERROR);

	if (DestroyWindow(_hWnd) == FALSE)
	{
		LOG("Can't destroy window.", LT_ERROR);
		return S_FALSE;
	}

	return S_OK;
}

DGLE_RESULT CMainWindow::ConfigureWindow(const TEngWindow &stWind, bool bSetFocus)
{
	if (!_hWnd)
		return E_FAIL;

	bool builtin_fscreen;

	Core()->pCoreRenderer()->IsFeatureSupported(CRSF_BUILTIN_FSCREEN_MODE, builtin_fscreen);

	if (builtin_fscreen)
		return S_OK;

	DGLE_RESULT res = S_OK;

	if (_bFScreen && !stWind.bFullScreen)
		if (ChangeDisplaySettings(NULL, 0) != DISP_CHANGE_SUCCESSFUL)
		{
			LOG("Can't switch off fullscreen mode.", LT_ERROR);
			res = S_FALSE;
		}

	if (stWind.bFullScreen)												
	{
		DEVMODE dm_scr_settings;								
		memset(&dm_scr_settings, 0, sizeof(dm_scr_settings));	
		dm_scr_settings.dmSize			= sizeof(dm_scr_settings);		
		dm_scr_settings.dmPelsWidth		= stWind.uiWidth;				
		dm_scr_settings.dmPelsHeight	= stWind.uiHeight;				
		dm_scr_settings.dmBitsPerPel	= Core()->InitFlags() & EIF_FORCE_16_BIT_COLOR ? 16 : 32;					
		dm_scr_settings.dmFields		= DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		if (ChangeDisplaySettingsEx(NULL ,&dm_scr_settings, NULL, CDS_FULLSCREEN, NULL) != DISP_CHANGE_SUCCESSFUL)
		{
			LOG("Can't set fullscreen mode(" +IntToStr(stWind.uiWidth)+"X"+IntToStr(stWind.uiHeight)+"), switching back to windowed mode.", LT_ERROR);
			_bFScreen = false;
			const_cast<TEngWindow *>(&stWind)->bFullScreen = false;
			res = S_FALSE;
		}
		else
			_bFScreen = true;
	}

	DWORD dw_style = NULL;

	if (stWind.bFullScreen)	
		dw_style = WS_POPUP;
	else
	{
		if (stWind.uiFlags & EWF_ALLOW_SIZEING)
			dw_style = WS_OVERLAPPEDWINDOW;
		else
			dw_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	}

	DWORD dw_style_ex = WS_EX_APPWINDOW;

	if (stWind.uiFlags & EWF_TOPMOST)
		dw_style_ex |= WS_EX_TOPMOST;

	if (SetWindowLong(_hWnd, GWL_EXSTYLE, dw_style_ex) == 0)
	{
		LOG("Can't change window styleEx.", LT_ERROR);
		res = S_FALSE;
	}

	if (SetWindowLong(_hWnd, GWL_STYLE, dw_style) == 0)
	{
		LOG("Can't change window style.", LT_ERROR);
		res = S_FALSE;
	}

	uint desktop_width = 0, desktop_height = 0;

	RECT rc = {0, 0, stWind.uiWidth, stWind.uiHeight};
	int	 top_x = 0, top_y = 0;

	AdjustWindowRectEx(&rc, dw_style , FALSE, dw_style_ex);

	if (!stWind.bFullScreen)
	{
		GetDisplaySize(desktop_width, desktop_height);

		top_x = (int)(desktop_width - (rc.right - rc.left))/2, 
		top_y = (int)(desktop_height - (rc.bottom - rc.top))/2;

		if (top_x < 0) top_x = 0;
		if (top_y < 0) top_y = 0;
	}

	if (_bFScreen && !stWind.bFullScreen)
	{
		RECT r;
		r.left = 0; r.top = 0;
		r.right = desktop_width; r.bottom = desktop_height;
		InvalidateRect(NULL, &r, TRUE);
		_bFScreen = false;
	}

	SetWindowPos(_hWnd, HWND_TOP, top_x, top_y, rc.right - rc.left, rc.bottom - rc.top, SWP_FRAMECHANGED | SWP_NOACTIVATE);

	if (IsWindowVisible(_hWnd) == FALSE)
		ShowWindow(_hWnd, SW_SHOWNA);

	if (bSetFocus)
	{
		SetForegroundWindow(_hWnd);
		SetCursorPos(top_x + (rc.right - rc.left)/2, top_y + (rc.bottom - rc.top)/2);
	}

	return res;
}

DGLE_RESULT CMainWindow::Free()
{
	delete this;
	return S_OK;
}