#include "notifier.h"
#include "resource.h"
#include <Windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <ShlObj.h>

/**
 * Notification window commands.
 */
#define ID_ALLOW 101
#define ID_BLOCK 102
#define ID_SKIP 103
#define ID_OPEN_PATH 104

 /**
  * The window class for the notification window.
  */
static wchar_t const CLASS_NAME[] = L"notifier_class_notify";

/**
 * Label positions.
 */
static RECT INFO_RECT = {46, 7, 246, 19};
static RECT PATH_RECT = {46, 26, 246, 38};
static RECT ICON_RECT = {7, 7, 38, 38};

static struct {
	HINSTANCE instance;
	HFONT font;
	HFONT font_underlined;
	HICON app_icon;
	HICON path_icon;
	wchar_t const *path;
	enum notifier_action action;
	bool open;
	bool class_registered;
} g;

/**
* The message handler for the notification window.
*/
static LRESULT CALLBACK message_handler(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
		case WM_CLOSE: {
			g.open = FALSE;
		} return 0;

		case WM_COMMAND: {
			DWORD id = LOWORD(wparam);

			switch (id) {
				case ID_ALLOW:
				{
					g.action = NOTIFIER_ACTION_ALLOW;
					g.open = FALSE;
				} break;

				case ID_BLOCK:
				{
					g.action = NOTIFIER_ACTION_BLOCK;
					g.open = FALSE;
				} break;

				case ID_SKIP:
				{
					g.action = NOTIFIER_ACTION_SKIP;
					g.open = FALSE;
				} break;

				case ID_OPEN_PATH:
				{
					LPITEMIDLIST idl = ILCreateFromPathW(g.path);
					if (idl) {
						SHOpenFolderAndSelectItems(idl, 0, NULL, 0);
						ILFree(idl);
					}
				} break;
			}

		} break;

		case WM_LBUTTONDOWN: {
			POINT p;
			p.x = GET_X_LPARAM(lparam);
			p.y = GET_Y_LPARAM(lparam);

			if (PtInRect(&PATH_RECT, p) || PtInRect(&ICON_RECT, p)) {
				SendMessageW(wnd, WM_COMMAND, LOWORD(ID_OPEN_PATH), 0);
			}
		} break;

		case WM_PAINT: {
			PAINTSTRUCT ps = {0};
			HDC dc = BeginPaint(wnd, &ps);

			SetBkMode(dc, OPAQUE);
			SetBkColor(dc, GetSysColor(COLOR_MENU));

			SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
			SelectObject(dc, g.font);
			DrawTextW(dc, L"Outbound connection was blocked:", -1, &INFO_RECT, DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_NOCLIP);

			SetTextColor(dc, GetSysColor(COLOR_HOTLIGHT));
			SelectObject(dc, g.font_underlined);
			DrawTextW(dc, g.path, -1, &PATH_RECT, DT_SINGLELINE | DT_PATH_ELLIPSIS | DT_NOCLIP);

			if (g.path_icon) {
				DrawIconEx(dc, 7, 7, g.path_icon, 32, 32, 0, NULL, DI_NORMAL);
			}

			EndPaint(wnd, &ps);
		} break;
	}

	return DefWindowProcW(wnd, msg, wparam, lparam);
}

void notifier_create(void) {
	g.instance = GetModuleHandleW(NULL);
	g.app_icon = LoadIconW(g.instance, MAKEINTRESOURCEW(IDI_ICON1));

	WNDCLASS wc = {0};
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wc.hIcon = g.app_icon;
	wc.hInstance = g.instance;
	wc.lpfnWndProc = message_handler;
	wc.lpszClassName = CLASS_NAME;
	wc.cbWndExtra = sizeof(void*);

	NONCLIENTMETRICS ncm = {0};
	ncm.cbSize = sizeof(ncm);
	if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {
		g.font = CreateFontIndirectW(&ncm.lfMessageFont);

		ncm.lfMessageFont.lfUnderline = TRUE;
		g.font_underlined = CreateFontIndirectW(&ncm.lfMessageFont);
	}

	g.class_registered = (RegisterClassW(&wc) != 0);
}

void notifier_destroy(void) {
	if (g.app_icon) {
		DestroyIcon(g.app_icon);
	}

	if (g.class_registered) {
		UnregisterClassW(CLASS_NAME, g.instance);
	}

	SecureZeroMemory(&g, sizeof(g));
}

enum notifier_action notifier_show(wchar_t const *path) {
	enum notify_action action = NOTIFIER_ACTION_SKIP;

	if (path == NULL) {
		return action;
	}

	RECT screen;
	if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &screen, 0) == FALSE) {
		screen.right = 0;
		screen.bottom = 0;
	}

	DWORD style = WS_POPUP | WS_SYSMENU | WS_BORDER;
	RECT window = {0};
	window.right = 253;
	window.bottom = 76;
	AdjustWindowRect(&window, style, FALSE);

	int window_width = (window.right - window.left);
	int window_height = (window.bottom - window.top);

	HWND wnd = CreateWindowExW(
		WS_EX_TOPMOST,
		CLASS_NAME,
		L"Firewall Notification",
		style,
		screen.right - window_width - 11,
		screen.bottom - window_height - 11,
		window_width,
		window_height,
		NULL, NULL, g.instance, NULL);

	if (wnd == NULL) {
		return action;
	}

	g.path = path;
	g.open = true;

	g.path_icon = ExtractIconW(NULL, path, 0);
	if (g.path_icon == NULL) {
		g.path_icon = (HICON)LoadImageW(NULL, IDI_APPLICATION, IMAGE_ICON, 32, 32,
			LR_DEFAULTSIZE);
	}

	HWND tooltip = CreateWindowExW(
		WS_EX_TOPMOST, TOOLTIPS_CLASSW,
		NULL,
		TTS_NOANIMATE | TTS_ALWAYSTIP | TTS_NOPREFIX | WS_POPUP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		wnd, NULL, g.instance, NULL);

	SetWindowPos(tooltip, HWND_TOPMOST, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	TOOLINFO ti = {0};
	ti.cbSize = sizeof(ti);
	ti.uFlags = TTF_SUBCLASS;
	ti.hwnd = wnd;
	ti.lpszText = (LPWSTR)path;
	ti.rect = PATH_RECT;

	SendMessageW(tooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);

	CreateWindowExW(
		0, WC_BUTTONW,
		L"Allow",
		WS_VISIBLE | WS_CHILD,
		7, 46, 75, 23,
		wnd, (HMENU)ID_ALLOW, g.instance, NULL);

	CreateWindowExW(
		0, WC_BUTTONW,
		L"Block",
		WS_VISIBLE | WS_CHILD,
		89, 46, 75, 23,
		wnd, (HMENU)ID_BLOCK, g.instance, NULL);

	CreateWindowExW(
		0, WC_BUTTONW,
		L"Skip",
		WS_VISIBLE | WS_CHILD,
		171, 46, 75, 23,
		wnd, (HMENU)ID_SKIP, g.instance, NULL);

	for (HWND temp = GetTopWindow(wnd); temp; temp = GetWindow(temp, GW_HWNDNEXT)) {
		SendMessageW(temp, WM_SETFONT, (WPARAM)g.font, TRUE);
	}

	ShowWindow(wnd, SW_SHOW);

	FLASHWINFO fi = {0};
	fi.cbSize = sizeof(fi);
	fi.hwnd = wnd;
	fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
	FlashWindowEx(&fi);

	MSG msg;
	while (g.open && GetMessageW(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	action = g.action;

	if (g.path_icon) {
		DestroyIcon(g.path_icon);
	}

	DestroyWindow(tooltip);
	DestroyWindow(wnd);

	return action;
}