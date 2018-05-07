#pragma once
#include "types.h"

/**
 * Returns a dynamically allocated copy of the given string on success.
 * Returns NULL on failure.
 */
wchar_t* wstr_dup(wchar_t const* str);

/**
 * Returns the 64-bit hash of the given string.
 */
uint64_t wstr_hash(wchar_t const *str);

/**
 * Performs lowercase conversion on the given string.
 */
void wstr_lower(wchar_t *dest);
