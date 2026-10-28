#pragma once

#include "app.h"


namespace Util {

    namespace Sys {
        bool isWin8orGreater();
        std::vector<std::wstring> getFiles(const std::wstring& mask);
        bool readFileBack(HANDLE file, void* buf, int bytes);
        std::wstring getModulePath(HMODULE mod);
        std::wstring dirSpec(const std::wstring& path);   // incl. trailing '\'
        std::wstring fileSpec(const std::wstring& path);
        std::wstring getLastErrorStr();
    }

    namespace Wnd {
        LONG_PTR getStyle(HWND wnd);
        LONG_PTR getExStyle(HWND wnd);
        std::wstring getText(HWND wnd);
        std::wstring getClassName(HWND wnd);
        bool isChild(HWND wnd);
        bool isWndRectEmpty(HWND wnd);
        HWND getNonChildParent(HWND wnd);
        HWND getTopParent(HWND wnd);
        bool isProgManWnd(HWND wnd);
        bool isTaskBar(HWND wnd);
        bool isTopMost(HWND wnd);
        BOOL moveWindow(HWND wnd, const RECT& rc, BOOL repaint = TRUE);
        bool psChanged(HWND page);
        void enableGroup(HWND wnd, int id, bool mode);
        bool isVCLAppWnd(HWND wnd);

        // Dialog data pointer stored in DWLP_USER.
        template <typename T> T* getDlgData(HWND wnd) {
            return reinterpret_cast<T*>(GetWindowLongPtr(wnd, DWLP_USER));
        }
        inline void setDlgResult(HWND wnd, LONG_PTR result) {
            SetWindowLongPtr(wnd, DWLP_MSGRESULT, result);
        }
    }

    namespace App {
        void error(HWND wnd, LPCWSTR s);
        void warning(HWND wnd, LPCWSTR s);
        void pinWindow(HWND wnd, HWND hitWnd, int trackRate, bool silent = false);
        HWND hasPin(HWND wnd);
        void togglePin(HWND wnd, HWND target, int trackRate);
        void markWnd(HWND wnd, bool mode);
        std::wstring getHelpFileDescr(const std::wstring& path, const std::wstring& name);
    }

    namespace Text {
        inline bool strimatch(LPCWSTR s1, LPCWSTR s2) {
            return _wcsicmp(s1, s2) == 0;
        }
        // Case-insensitive match with '*' and '?' wildcards.
        bool wildimatch(const std::wstring& pattern, const std::wstring& s);
        std::wstring remAccel(std::wstring s);
    }

    namespace Gfx {
        HRGN makeRegionFromBmp(HBITMAP bmp, COLORREF clrMask);
        bool getBmpSize(HBITMAP bmp, SIZE& sz);
        bool remapBmpColors(HBITMAP bmp, const COLORREF clrs[][2], int cnt);
        bool rectContains(const RECT& rc1, const RECT& rc2);
        HFONT createDerivedFont(HFONT base, LONG weight);
    }

    namespace Res {

        // Resource string loading and formatting.
        // Automatically uses loaded language DLL, if any.
        //
        class ResStr : NonCopyable {
        public:
            ResStr(UINT id, size_t bufLen = 256) {
                init(id, bufLen, nullptr); }
            ResStr(UINT id, size_t bufLen, DWORD_PTR p1) {
                DWORD_PTR params[] = {p1}; init(id, bufLen, params); }
            ResStr(UINT id, size_t bufLen, DWORD_PTR p1, DWORD_PTR p2, DWORD_PTR p3) {
                DWORD_PTR params[] = {p1,p2,p3}; init(id, bufLen, params); }
            operator LPCWSTR() const { return str.c_str(); }
        private:
            std::wstring str;
            void init(UINT id, size_t bufLen, DWORD_PTR* params);
        };

    }

    namespace Clr {
        enum : COLORREF {
            maroon = 0x000080, gray = 0x808080, silver = 0xc0c0c0,
            red    = 0x0000ff, white = 0xffffff,
        };
        COLORREF light(COLORREF clr);
        COLORREF dark(COLORREF clr);
    }


    // Minimal RAII registry key.
    //
    class RegKey : NonCopyable {
    public:
        RegKey() : key(0) {}
        ~RegKey() { close(); }

        bool create(HKEY parent, const std::wstring& path);
        bool open(HKEY parent, const std::wstring& path);
        void close();
        explicit operator bool() const { return key != 0; }
        operator HKEY() const { return key; }

        bool getDWord(const std::wstring& name, DWORD& value) const;
        bool setDWord(const std::wstring& name, DWORD value);
        bool getString(const std::wstring& name, std::wstring& value) const;
        bool setString(const std::wstring& name, const std::wstring& value);
        bool deleteValue(const std::wstring& name);
        DWORD getValueType(const std::wstring& name) const;   // REG_NONE on error

    private:
        HKEY key;
    };

}
