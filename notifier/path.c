#include "path.h"
#include <Windows.h>
#include <strsafe.h>
#include <fltUser.h>

bool devpath_to_dospath(wchar_t * dos_path, size_t dos_path_count, wchar_t const * dev_path) {
	/* Check parameters. */
	if (dos_path == 0 || dos_path_count == 0 || dev_path == 0) {
		return FALSE;
	}

	/* Device looks like: \\device\\harddiskvolume*\\, split on third backslash. */
	wchar_t dev_name[MAX_PATH];
	wchar_t const* s = dev_path;
	wchar_t c;
	size_t i = 0;
	size_t n = 0;

	while (i < ARRAYSIZE(dev_name)) {
		c = *s;
		dev_name[i] = c;

		if (s == 0) {
			return FALSE;
		}

		if (c == L'\\') {
			n += 1;
			if (n == 3) {
				dev_name[i] = 0;
				break;
			}
		}

		++i;
		++s;
	}

	/* After splitting we take the device name, translate it into a dos drive mount (eg. C:). */
	if (FAILED(FilterGetDosName(dev_name, dos_path, (DWORD)dos_path_count)) || dos_path[0] == L'\0') {
		return FALSE;
	}

	/* And finally concatenate it all together: C: + filePath */
	return SUCCEEDED(StringCchCatW(dos_path, dos_path_count, s));
}
