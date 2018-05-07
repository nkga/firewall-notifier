#include "cache.h"
#include "config.h"
#include "console.h"
#include "firewall.h"
#include "memory.h"
#include "monitor.h"
#include "notifier.h"
#include "path.h"
#include "queue.h"
#include "wstr.h"
#include <Windows.h>
#include <CommCtrl.h>
#include <objbase.h>
#include <strsafe.h>

/**
 * Global application state.
 */
static struct {
	CRITICAL_SECTION lock;
	int64_t cache_time;
	bool closed;
} g;

/**
 * Handles an enumerated firewall rule.
 */
static void rule_enum(wchar_t const *drive_path) {
	cache_insert(drive_path, g.cache_time);
}

/**
 * Rebuilds the firewall block cache.
 */
static void build_cache(int64_t time) {
	g.cache_time = time;
	firewall_enum(rule_enum);
}

/**
 * Handles a dropped network event. May occur from multiple threads at
 * once so care is taken to avoid race conditions.
 */
static void drop_event(wchar_t const *dev_path) {
	/* Fix the path name. */
	EnterCriticalSection(&g.lock);

	if (g.closed) {
		LeaveCriticalSection(&g.lock);
		return;
	}

	static wchar_t path[MAX_EXT_PATH];
	if (devpath_to_dospath(path, ARRAYSIZE(path), dev_path) == false) {
		LeaveCriticalSection(&g.lock);
		return;
	}

	wstr_lower(path);

	/* Update the cache if applicable, then search it for the given rule. */
	int64_t now = GetTickCount64();

	if (now - g.cache_time >= CACHE_AGE) {
		cache_prune(now, CACHE_AGE);
		build_cache(now);
	}

	if (cache_contains(path) == false) {
		wchar_t* dup = wstr_dup(path);
		if (queue_enqueue(dup)) {
			cache_insert(path, GetTickCount64());
		}
	}

	LeaveCriticalSection(&g.lock);
}

/**
 * Handles a console action.
 */
static void console_event(enum CONSOLE_ACTION action) {
	switch (action) {
		case CONSOLE_ACTION_OPEN_RULES:
		{
			/* Invalidate current cache since the user is messing around with the firewall. */
			/* The cache itself will be updated next time a block event arrives. */
			EnterCriticalSection(&g.lock);
			g.cache_time = 0;
			LeaveCriticalSection(&g.lock);
		} break;

		case CONSOLE_ACTION_REBUILD_CACHE:
		{
			/* Do an explicit rebuild of the cache now. */
			EnterCriticalSection(&g.lock);
			cache_clear();
			build_cache(GetTickCount64());
			LeaveCriticalSection(&g.lock);
		} break;
	}
}

/**
 * Notification thread.
 */
static DWORD WINAPI notifier_thread(LPVOID lp) {
	UNREFERENCED_PARAMETER(lp);

	while (g.closed == false) {
		wchar_t* path = queue_dequeue();
		if (path == NULL) {
			break;
		}

		enum notify_action a = notifier_show(path);
		if (a != NOTIFIER_ACTION_SKIP) {
			if (firewall_add(path, path, a == NOTIFIER_ACTION_ALLOW)) {
				EnterCriticalSection(&g.lock);
				cache_insert(path, GetTickCount64());
				LeaveCriticalSection(&g.lock);
			}
		}

		memory_free(path);
	}

	return 0;
}

int CALLBACK wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prev, _In_ PWSTR cmd, _In_ int cmd_show) {
	UNREFERENCED_PARAMETER(instance);
	UNREFERENCED_PARAMETER(prev);
	UNREFERENCED_PARAMETER(cmd);
	UNREFERENCED_PARAMETER(cmd_show);

	/* The monitor and firewall interfaces need COM. */
	if (FAILED(CoInitializeEx(0, COINIT_MULTITHREADED))) {
		return 0;
	}

	/* Support modern window styling. */
	INITCOMMONCONTROLSEX icex = {0};
	icex.dwSize = sizeof(icex);
	icex.dwICC = ICC_STANDARD_CLASSES | ICC_TAB_CLASSES | ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icex);

	InitializeCriticalSection(&g.lock);

	queue_create();
	HANDLE thread = CreateThread(0, 0, notifier_thread, 0, 0, 0);
	if (thread) {
		firewall_create();
		notifier_create();
		monitor_create();

		/* Initialize filtering immediately in case the user had turned it off. */
		firewall_set_filtering(true);

		build_cache(GetTickCount64());
		monitor_start(drop_event);
		console_run(console_event);

		g.closed = true;
		monitor_stop();

		monitor_destroy();
		notifier_destroy();
		firewall_destroy();
		cache_clear();
	}

	queue_destroy();

	if (thread) {
		WaitForSingleObject(thread, INFINITE);
	}

	DeleteCriticalSection(&g.lock);
	CoUninitialize();

	return 0;
}