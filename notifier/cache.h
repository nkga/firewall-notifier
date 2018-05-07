#pragma once
#include "types.h"

/**
 * Clears the block cache.
 */
void cache_clear(void);

/**
 * Returns true if the path is in the block cache.
 */
bool cache_contains(wchar_t const *path);

/**
 * Inserts a path into the block cache. If the path already exists
 * in the cache its time is refreshed if newer.
 * Returns true if the path was added or updated.
 */
bool cache_insert(wchar_t const *path, int64_t time);

/**
 * Prunes the cache of old entries.
 */
void cache_prune(int64_t time, int64_t max_age);
