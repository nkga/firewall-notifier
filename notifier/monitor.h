#pragma once
#include "types.h"

/**
 * Monitor drop event callback. Passes a DOS device styled path.
 */
typedef void(*monitor_callback_t)(wchar_t const *device_path);

/**
 * Creates the filtering platform monitoring interface.
 */
void monitor_create(void);

/**
 * Destroys the filtering platform monitoring interface.
 */
void monitor_destroy(void);

/**
 * Starts monitoring dropped network events. The drop callback may
 * be called from multiple threads at once.
 */
void monitor_start(monitor_callback_t drop_callback);

/**
 * Stops monitoring dropped network events. Waits until all events have
 * finished processing.
 */
void monitor_stop(void);