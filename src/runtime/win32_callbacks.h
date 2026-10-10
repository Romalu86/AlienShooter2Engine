#pragma once

#include "basetype.h"
#include <windows.h>

INT_PTR __stdcall AppStart(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall AppWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

