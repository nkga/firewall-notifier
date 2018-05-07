#include "firewall.h"
#include "types.h"
#include "wstr.h"
#include <netfw.h>

/**
 * Windows firewall built in profiles.
 */
static NET_FW_PROFILE_TYPE2 const PROFILE_TYPES[] = {
	NET_FW_PROFILE2_PUBLIC,
	NET_FW_PROFILE2_PRIVATE,
	NET_FW_PROFILE2_DOMAIN
};

/**
 * Firewall interface state.
 */
static struct {
	INetFwPolicy2 *policy;
	INetFwRules *rules;
	bool initialized;
} g;

/**
 * Returns true if the firewall rule is a valid one for the block cache.
 */
static bool firewall_is_valid_rule(INetFwRule *rule) {
	if (rule == NULL) {
		return false;
	}

	NET_FW_RULE_DIRECTION dir;
	if (FAILED(rule->lpVtbl->get_Direction(rule, &dir)) || dir == NET_FW_RULE_DIR_IN) {
		return false;
	}

	VARIANT_BOOL status;
	if (FAILED(rule->lpVtbl->get_Enabled(rule, &status)) || status != VARIANT_TRUE) {
		return false;
	}

	bool result = false;

	BSTR ports;
	if (SUCCEEDED(rule->lpVtbl->get_LocalPorts(rule, &ports))) {
		NET_FW_ACTION action;
		if (SUCCEEDED(rule->lpVtbl->get_Action(rule, &action))) {
			if (action == NET_FW_ACTION_BLOCK ||
				(action == NET_FW_ACTION_ALLOW && (ports == NULL || wcscmp(ports, L"*") == 0))) {
				result = true;
			}
		}

		SysFreeString(ports);
	}

	return result;
}

void firewall_create(void) {
	if (FAILED(CoCreateInstance(&CLSID_NetFwPolicy2, NULL, CLSCTX_INPROC_SERVER, &IID_INetFwPolicy2, &g.policy))) {
		return;
	}

	if (FAILED(g.policy->lpVtbl->get_Rules(g.policy, &g.rules))) {
		return;
	}

	g.initialized = true;
}

void firewall_destroy(void) {
	if (g.rules) {
		g.rules->lpVtbl->Release(g.rules);
	}

	if (g.policy) {
		g.policy->lpVtbl->Release(g.policy);
	}

	SecureZeroMemory(&g, sizeof(g));
}


bool firewall_add(wchar_t const *name, wchar_t const *path, bool allow) {
	if (name == NULL || path == NULL) {
		return false;
	}

	if (g.initialized == false) {
		return false;
	}

	INetFwRule *rule;
	if (FAILED(CoCreateInstance(&CLSID_NetFwRule, NULL, CLSCTX_INPROC_SERVER, &IID_INetFwRule, &rule))) {
		return false;
	}

	bool result = false;

	BSTR com_name = SysAllocString(name);
	if (com_name) {
		BSTR com_path = SysAllocString(path);
		if (com_path) {
			rule->lpVtbl->put_Name(rule, com_name);
			rule->lpVtbl->put_ApplicationName(rule, com_path);
			rule->lpVtbl->put_Profiles(rule, NET_FW_PROFILE2_ALL);
			rule->lpVtbl->put_Protocol(rule, NET_FW_IP_PROTOCOL_ANY);
			rule->lpVtbl->put_Direction(rule, NET_FW_RULE_DIR_OUT);
			rule->lpVtbl->put_Enabled(rule, VARIANT_TRUE);
			rule->lpVtbl->put_Action(rule, allow ? NET_FW_ACTION_ALLOW : NET_FW_ACTION_BLOCK);

			HRESULT hr = g.rules->lpVtbl->Add(g.rules, rule);
			result = SUCCEEDED(hr);

			SysFreeString(com_path);
		}

		SysFreeString(com_name);
	}

	rule->lpVtbl->Release(rule);

	return result;
}

void firewall_enum(firewall_callback_t enum_callback) {
	if (g.initialized == false || enum_callback == NULL) {
		return;
	}

	IUnknown *temp;
	if (FAILED(g.rules->lpVtbl->get__NewEnum(g.rules, &temp))) {
		return;
	}

	IEnumVARIANT *enum_var;
	if (SUCCEEDED(temp->lpVtbl->QueryInterface(temp, &IID_IEnumVARIANT, &enum_var))) {
		for (;;) {
			ULONG fetched;
			VARIANT var;

			HRESULT hr = enum_var->lpVtbl->Next(enum_var, 1, &var, &fetched);
			if (FAILED(hr) || hr == S_FALSE) {
				break;
			}

			if (var.vt == VT_DISPATCH && var.pdispVal != NULL) {
				INetFwRule *rule;
				if (SUCCEEDED(var.pdispVal->lpVtbl->QueryInterface(var.pdispVal, &IID_INetFwRule, &rule))) {
					if (firewall_is_valid_rule(rule)) {
						BSTR name;
						if (SUCCEEDED(rule->lpVtbl->get_ApplicationName(rule, &name)) && name) {
							wstr_lower(name);
							enum_callback(name);
							SysFreeString(name);
						}
					}

					rule->lpVtbl->Release(rule);
				}
			}

			VariantClear(&var);
		}

		enum_var->lpVtbl->Release(enum_var);
	}

	temp->lpVtbl->Release(temp);
}

bool firewall_get_filtering(void) {
	if (g.initialized == false) {
		return false;
	}

	long profile;
	if (FAILED(g.policy->lpVtbl->get_CurrentProfileTypes(g.policy, &profile))) {
		return false;
	}

	for (size_t i = 0; i < ARRAYSIZE(PROFILE_TYPES); ++i) {
		NET_FW_PROFILE_TYPE2 profile_type = PROFILE_TYPES[i];
		if ((profile & profile_type) != 0) {
			NET_FW_ACTION action;
			if (FAILED(g.policy->lpVtbl->get_DefaultOutboundAction(g.policy, profile_type, &action)) || action != NET_FW_ACTION_BLOCK) {
				return false;
			}
		}
	}

	return true;
}

bool firewall_set_filtering(bool is_filtering) {
	if (g.initialized == false) {
		return false;
	}

	bool result = true;
	for (size_t i = 0; i < ARRAYSIZE(PROFILE_TYPES); ++i) {
		if (FAILED(g.policy->lpVtbl->put_DefaultOutboundAction(g.policy, PROFILE_TYPES[i], is_filtering ? NET_FW_ACTION_BLOCK : NET_FW_ACTION_ALLOW))) {
			result = false;
		}
	}

	return result;
}
