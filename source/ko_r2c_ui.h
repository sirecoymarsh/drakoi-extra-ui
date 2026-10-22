#ifndef DRAKOI_KO_R2C_UI_H
#define DRAKOI_KO_R2C_UI_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/*
 * Additive R2c UI translations that are not part of the immutable R2b map.
 *
 * ko_r2c_find_exact_translation returns a process-lifetime string literal.
 * ko_r2c_make_dynamic_translation returns a GetProcessHeap allocation; the
 * caller must release a non-NULL result with HeapFree(GetProcessHeap(), 0, p).
 */
LPCWSTR ko_r2c_find_exact_translation(LPCWSTR source);
WCHAR *ko_r2c_make_dynamic_translation(LPCWSTR source);

#endif
