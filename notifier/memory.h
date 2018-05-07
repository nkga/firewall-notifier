#pragma once
#include "types.h"

/**
 * Helping for allocating a given number of items of a certain type.
 */
#define MEMORY_ALLOC_COUNT(dst, count) memory_alloc(sizeof(*(dst)) * count)

/**
 * Allocates a region of memory with the size given in bytes.
 * The contents are guaranteed to be zero initialized.
 * Returns a pointer to the allocated region on success, NULL otherwise.
 */
void* memory_alloc(size_t bytes);

/**
 * Frees a previously allocated region of memory.
 * If the pointer to the region is null no changes are made.
 */
void memory_free(void* ptr);
