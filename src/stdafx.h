#pragma once

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

// Windows 7+
#define WINVER       0x0601
#define _WIN32_WINNT 0x0601
#define _WIN32_IE    0x0800

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlwapi.h>    // ColorRGBToHLS
#include <windowsx.h>   // GET_X/Y_LPARAM
#include <prsht.h>
#include <commdlg.h>
#include <dwmapi.h>

#include <cstdlib>
#include <cwctype>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>


// Base for classes that own OS resources.
//
class NonCopyable {
protected:
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};
