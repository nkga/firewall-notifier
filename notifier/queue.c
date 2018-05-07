#include "queue.h"
#include "config.h"
#include <Windows.h>

static struct {
	CONDITION_VARIABLE not_empty;
	CRITICAL_SECTION lock;
	bool running;
	uint32_t count;
	uint32_t offset;
	wchar_t* items[QUEUE_SIZE];
} g;

void queue_create(void) {
	g.running = true;
	InitializeConditionVariable(&g.not_empty);
	InitializeCriticalSection(&g.lock);
}

void queue_destroy(void) {
	EnterCriticalSection(&g.lock);
	g.running = false;
	LeaveCriticalSection(&g.lock);

	WakeAllConditionVariable(&g.not_empty);
}

bool queue_enqueue(wchar_t* path) {
	EnterCriticalSection(&g.lock);

	if (g.count >= QUEUE_SIZE || g.running == false) {
		LeaveCriticalSection(&g.lock);
		return false;
	}

	g.items[(g.offset + g.count) % QUEUE_SIZE] = path;
	g.count += 1;

	LeaveCriticalSection(&g.lock);
	WakeConditionVariable(&g.not_empty);

	return true;
}

wchar_t* queue_dequeue(void) {
	EnterCriticalSection(&g.lock);

	while (g.count == 0 && g.running == true) {
		SleepConditionVariableCS(&g.not_empty, &g.lock, INFINITE);
	}

	if (g.running == false) {
		LeaveCriticalSection(&g.lock);
		return NULL;
	}

	wchar_t* path = g.items[g.offset];

	g.count -= 1;
	g.offset += 1;

	if (g.offset == QUEUE_SIZE) {
		g.offset = 0;
	}

	LeaveCriticalSection(&g.lock);

	return path;
}
