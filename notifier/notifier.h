#pragma once
#include "types.h"

/**
 * Notification actions.
 */
enum NOTIFIER_ACTION
{
	NOTIFIER_ACTION_SKIP,
	NOTIFIER_ACTION_BLOCK,
	NOTIFIER_ACTION_ALLOW,
};

/**
 * Creates the notifier.
 */
void notifier_create(void);

/**
 * Destroys the notifier.
 */
void notifier_destroy(void);

/**
 * Shows a notification. Returns the action of the notification.
 */
enum NOTIFIER_ACTION notifier_show(wchar_t const *path);
