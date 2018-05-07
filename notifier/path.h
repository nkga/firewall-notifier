#pragma once
#include "types.h"

/**
 * Converts an NT device path to DOS path.
 * Returns true if a valid conversion was available.
 */
bool devpath_to_dospath(wchar_t* dos_path, size_t dos_path_count, wchar_t const* dev_path);
