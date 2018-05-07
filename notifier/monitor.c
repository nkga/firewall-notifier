#include "monitor.h"
#include <Windows.h>
#include <fwpmu.h>
#include <fwptypes.h>

static struct {
	monitor_callback_t drop_callback;
	HANDLE session;
	HANDLE subscription;
	GUID session_key;
	bool initialized;
} g;

static void monitor_drop_event(void* context, FWPM_NET_EVENT1 const* e) {
	(void)context;

	if (e == NULL) {
		return;
	}

	switch (e->type) {
		case FWPM_NET_EVENT_TYPE_CLASSIFY_DROP: {
			if (e->header.flags & FWPM_NET_EVENT_FLAG_APP_ID_SET && e->header.appId.data != NULL) {
				g.drop_callback((wchar_t*)e->header.appId.data);
			}
		} break;
	}
}

void monitor_create(void) {
	FWPM_SESSION0 session_desc = {0};
	session_desc.displayData.name = L"notifier filtering session";
	session_desc.displayData.description = L"Dropped connection monitoring.";

	if (FwpmEngineOpen0(NULL, RPC_C_AUTHN_DEFAULT, NULL, &session_desc, &g.session) != ERROR_SUCCESS) {
		return;
	}

	FWP_VALUE0 val = {0};
	val.type = FWP_UINT32;
	val.uint32 = 1;
	if (FwpmEngineSetOption0(g.session, FWPM_ENGINE_COLLECT_NET_EVENTS, &val) != ERROR_SUCCESS) {
		return;
	}

	g.session_key = session_desc.sessionKey;
	g.initialized = true;
}

void monitor_destroy(void) {
	monitor_stop();

	if (g.session) {
		FWP_VALUE0 val = {0};
		val.type = FWP_UINT32;
		val.uint32 = 0;
		FwpmEngineSetOption0(g.session, FWPM_ENGINE_COLLECT_NET_EVENTS, &val);

		FwpmEngineClose0(g.session);
	}

	SecureZeroMemory(&g, sizeof(g));
}

void monitor_start(monitor_callback_t drop_callback) {
	if (g.initialized == false || g.subscription || drop_callback == NULL) {
		return;
	}

	g.drop_callback = drop_callback;

	FWPM_NET_EVENT_SUBSCRIPTION0 sub_desc = {0};
	sub_desc.sessionKey = g.session_key;
	if (FwpmNetEventSubscribe0(g.session, &sub_desc, monitor_drop_event,
		NULL, &g.subscription) != ERROR_SUCCESS) {
		return;
	}
}

void monitor_stop(void) {
	if (g.session && g.subscription) {
		FwpmNetEventUnsubscribe0(g.session, g.subscription);
		g.subscription = NULL;
	}
}