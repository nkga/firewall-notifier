#include "wstr.h"
#include "config.h"
#include "memory.h"
#include <wchar.h>

wchar_t* wstr_dup(wchar_t const* str) {
	if (str == 0) {
		return NULL;
	}

	size_t len = wcsnlen_s(str, MAX_EXT_PATH);
	if (len >= MAX_EXT_PATH) {
		return NULL;
	}

	size_t count = len + 1;
	wchar_t* res = MEMORY_ALLOC_COUNT(res, count);

	if (wcscpy_s(res, count, str) != 0) {
		memory_free(res);
		return NULL;
	}

	return res;
}

uint64_t wstr_hash(wchar_t const *src) {
	/* FNV1-a: http://www.isthe.com/chongo/tech/comp/fnv/ */
	uint64_t hash = 14695981039346656037;

	if (src) {
		for (size_t i = 0; src[i]; ++i) {
			hash ^= src[i];
			hash *= 1099511628211;
		}
	}

	return hash;
}

void wstr_lower(wchar_t *dest) {
	if (dest == NULL) {
		return;
	}

	for (int i = 0; dest[i]; ++i) {
		dest[i] = towlower(dest[i]);
	}
}
