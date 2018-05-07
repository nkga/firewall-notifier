#include "cache.h"
#include "config.h"
#include "memory.h"
#include "wstr.h"
#include <stdlib.h>
#include <wchar.h>

/**
 * The number of buckets in the block cache hash table.
 */
#define BLOCK_CACHE_SIZE 2053

 /**
  * Linked list node, for chained buckets.
  */
struct node {
	int64_t time;
	wchar_t *path;
	struct node *next;
};

static struct {
	struct node *table[BLOCK_CACHE_SIZE];
} g;

static struct node* cache_find(struct node* n, wchar_t const* path) {
	while (n) {
		if (wcscmp(path, n->path) == 0) {
			return n;
		}

		n = n->next;
	}

	return NULL;
}

void cache_clear(void) {
	for (size_t i = 0; i < BLOCK_CACHE_SIZE; ++i) {
		struct node *n = g.table[i];
		while (n) {
			struct node *temp = n->next;

			free(n->path);
			free(n);

			n = temp;
		}

		g.table[i] = NULL;
	}
}

bool cache_contains(wchar_t const *path) {
	if (path == NULL) {
		return false;
	}

	size_t i = wstr_hash(path) % BLOCK_CACHE_SIZE;
	return cache_find(g.table[i], path) != NULL;
}

bool cache_insert(wchar_t const *path, int64_t time) {
	if (path == NULL) {
		return false;
	}

	size_t i = wstr_hash(path) % BLOCK_CACHE_SIZE;
	struct node* n = cache_find(g.table[i], path);

	if (n != NULL) {
		if (n->time < time) {
			n->time = time;
		}

		return true;
	}

	n = memory_alloc(sizeof(*n));

	if (n == NULL) {
		return false;
	}

	n->path = wstr_dup(path);

	if (n->path) {
		n->time = time;
		n->next = g.table[i];
		g.table[i] = n;

		return true;
	}

	free(n);

	return false;
}

void cache_prune(int64_t time, int64_t max_age) {
	for (size_t i = 0; i < BLOCK_CACHE_SIZE; ++i) {
		struct node* prev = 0;
		struct node* n = g.table[i];

		while (n) {
			struct node* next = n->next;

			if (time - n->time > max_age) {
				free(n->path);
				free(n);

				if (prev) {
					prev->next = next;
				} else {
					g.table[i] = NULL;
				}
			}

			n = next;
		}
	}
}
