#pragma once
#include "types.h"

/**
 * Firewall enumeration callback.
 */
typedef void (*firewall_callback_t)(wchar_t const *drive_path);

/**
 * Creates the firewall interface.
 */
void firewall_create(void);

/**
 * Destroys the firewall interface.
 */
void firewall_destroy(void);

/**
 * Adds a new rule to the firewall. Returns true on success.
 */
bool firewall_add(wchar_t const *name, wchar_t const *path, bool allow);

/**
 * Enumerates rules in the firewall.
 */
void firewall_enum(firewall_callback_t enum_callback);

/**
 * Returns true if the firewall is filtering outbound connections, false otherwise.
 */
bool firewall_get_filtering(void);

/**
 * Sets the filtering state for the firewall.
 */
bool firewall_set_filtering(bool is_filtering);
