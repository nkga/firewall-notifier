#pragma once

/**
 * Console callback actions.
 */
enum CONSOLE_ACTION
{
	CONSOLE_ACTION_INVALID,
	CONSOLE_ACTION_OPEN_RULES,
	CONSOLE_ACTION_REBUILD_CACHE
};

/**
 * Console action callback function.
 */
typedef void(*console_callback_t)(enum CONSOLE_ACTION action);

/**
 * Runs the console until the user closes the application.
 */
void console_run(console_callback_t action_callback);

