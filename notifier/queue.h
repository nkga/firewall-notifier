#pragma once
#include "types.h"

/**
 * Creates the queue.
 */
void queue_create(void);

/**
 * Destroys the queue.
 */
void queue_destroy(void);

/**
 * Enqueues a path if the queue is not full.
 * Returns false if the queue is full.
 */
bool queue_enqueue(wchar_t* path);

/**
 * Dequeues a path. Waits until the queue contains an item or the
 * queue is no longer valid.
 *
 * Returns a pointer to the path if available, NULL otherwise.
 */
wchar_t* queue_dequeue(void);
