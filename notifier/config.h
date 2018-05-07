#pragma once

/**
 * The time (in milliseconds) between pruning and rebuilding the cache.
 * (default: 300000)
 */
#define CACHE_AGE 300000

/**
 * The maximum length for an extended path, null termination not included.
 * (default: 32768)
 */
#define MAX_EXT_PATH 32768

/**
 * Maximum number of block events before newer ones are dropped.
 * (default: 32)
 */
#define QUEUE_SIZE 2
