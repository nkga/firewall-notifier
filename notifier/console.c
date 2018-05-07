#include "console.h"
#include "config.h"
#include "firewall.h"
#include "resource.h"
#include "types.h"
#include <Windows.h>
#include <strsafe.h>
#include <ShlObj.h>
#include <shellapi.h>
#include <assert.h>

/**
 * Tray icon commands.
 */
#define CMD_EXIT 101
#define CMD_REBUILD 103
#define CMD_RULES 104
#define CMD_TOGGLE 105

 /**
  * Custom messages.
  */
#define WM_TRAY_COMMAND (WM_USER + 1)

static struct {
	console_callback_t callback;
	HWND window;
	HMENU tray_menu;
	HICON icon_filter_on;
	HICON icon_filter_off;
	bool open;
	NOTIFYICONDATA nid;
} g;

/**
 * The message handler for the console window.
 */
static LRESULT CALLBACK message_handler(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
		case WM_CLOSE:
		{
			g.open = false;
		} break;

		case WM_COMMAND:
		{
			switch (LOWORD(wparam)) {
				case CMD_EXIT:
				{
					g.open = FALSE;
				} break;

				case CMD_REBUILD:
				{
					g.callback(CONSOLE_ACTION_REBUILD_CACHE);
				} break;

				case CMD_RULES:
				{
					static wchar_t command[MAX_EXT_PATH + 1];

					if (SHGetFolderPathW(NULL, CSIDL_SYSTEM, NULL, SHGFP_TYPE_DEFAULT, command) == S_OK) {
						if (wcscat_s(command, MAX_EXT_PATH, L"\\mmc.exe wf.msc") == 0) {
							STARTUPINFOW si = {0};
							si.cb = sizeof(si);

							PROCESS_INFORMATION pi = {0};
							if (CreateProcessW(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
								CloseHandle(pi.hThread);
								CloseHandle(pi.hProcess);

								g.callback(CONSOLE_ACTION_OPEN_RULES);
							}
						}
					}
				} break;

				case CMD_TOGGLE: {
					bool filter = !firewall_get_filtering();
					g.nid.hIcon = filter ? g.icon_filter_on : g.icon_filter_off;
					Shell_NotifyIconW(NIM_MODIFY, &g.nid);
					firewall_set_filtering(filter);
				} break;
			}
		} break;

		case WM_TRAY_COMMAND:
		{
			switch (lparam) {
				case WM_LBUTTONDOWN:
				case WM_LBUTTONDBLCLK:
				{
					SendMessageW(wnd, WM_COMMAND, MAKEWPARAM(CMD_RULES, 0), 0);
				} break;

				case WM_RBUTTONUP:
				{
					POINT p;
					if (GetCursorPos(&p)) {
						SetForegroundWindow(wnd);
						CheckMenuItem(g.tray_menu, CMD_TOGGLE, firewall_get_filtering() ? MF_CHECKED : MF_UNCHECKED);
						TrackPopupMenu(g.tray_menu, TPM_RIGHTBUTTON, p.x, p.y, 0, wnd, NULL);
					}
				} break;
			}
		} break;
	}

	return DefWindowProcW(wnd, msg, wparam, lparam);
}

void console_run(console_callback_t action_callback) {
	if (action_callback == NULL) {
		return;
	}

	g.callback = action_callback;

	WNDCLASS wc = {0};
	wc.hInstance = GetModuleHandleW(NULL);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wc.hIcon = LoadIconW(wc.hInstance, MAKEINTRESOURCEW(IDI_ICON1));
	wc.lpfnWndProc = message_handler;
	wc.lpszClassName = L"notifier_class_console";
	RegisterClassW(&wc);

	HWND wnd = CreateWindowExW(0, wc.lpszClassName, L"Console", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, wc.hInstance, 0);
	if (wnd == NULL) {
		return;
	}

	g.window = wnd;
	g.open = TRUE;
	g.tray_menu = CreatePopupMenu();

	if (g.tray_menu) {
		AppendMenuW(g.tray_menu, MF_DEFAULT | MF_STRING, CMD_RULES, L"Rules");
		AppendMenuW(g.tray_menu, MF_STRING, CMD_TOGGLE, L"Toggle Firewall");
		AppendMenuW(g.tray_menu, MF_STRING, CMD_REBUILD, L"Rebuild Cache");
		AppendMenuW(g.tray_menu, MF_SEPARATOR, 0, NULL);
		AppendMenuW(g.tray_menu, MF_STRING, CMD_EXIT, L"Exit");
		SetMenuDefaultItem(g.tray_menu, CMD_RULES, FALSE);
	}

	g.icon_filter_on = LoadImageW(wc.hInstance, MAKEINTRESOURCEW(IDI_ICON1), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE | LR_SHARED);
	g.icon_filter_off = LoadImageW(wc.hInstance, MAKEINTRESOURCEW(IDI_ICON2), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE | LR_SHARED);

	memset(&g.nid, 0, sizeof(g.nid));
	g.nid.cbSize = sizeof(g.nid);
	g.nid.hWnd = wnd;
	g.nid.uCallbackMessage = WM_TRAY_COMMAND;
	g.nid.uID = 0;
	g.nid.uVersion = NOTIFYICON_VERSION;
	g.nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	g.nid.hIcon = g.icon_filter_on;

	wcscpy_s(g.nid.szTip, ARRAYSIZE(g.nid.szTip), L"Firewall Notifier");
	bool tray = Shell_NotifyIconW(NIM_ADD, &g.nid);

	MSG msg = {0};
	while (g.open && GetMessageW(&msg, NULL, 0, 0)) {
		if (IsDialogMessageW(wnd, &msg) == FALSE) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

	if (tray) {
		Shell_NotifyIconW(NIM_DELETE, &g.nid);
	}

	DestroyWindow(wnd);
	UnregisterClassW(wc.lpszClassName, wc.hInstance);

	memset(&g, 0, sizeof(g));
}
