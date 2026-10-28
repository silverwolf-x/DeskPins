#include "stdafx.h"
#include "pinwnd.h"
#include "util.h"
#include "resource.h"


LONG_PTR Util::Wnd::getStyle(HWND wnd)
{
    return GetWindowLongPtr(wnd, GWL_STYLE);
}


LONG_PTR Util::Wnd::getExStyle(HWND wnd)
{
    return GetWindowLongPtr(wnd, GWL_EXSTYLE);
}


std::wstring Util::Wnd::getText(HWND wnd)
{
    std::wstring s(GetWindowTextLength(wnd) + 1, L'\0');
    s.resize(GetWindowText(wnd, &s[0], int(s.size())));
    return s;
}


std::wstring Util::Wnd::getClassName(HWND wnd)
{
    WCHAR buf[256];   // max class name length
    int n = GetClassName(wnd, buf, ARRAYSIZE(buf));
    return std::wstring(buf, n);
}


bool Util::Wnd::isWndRectEmpty(HWND wnd)
{
    RECT rc;
    return GetWindowRect(wnd, &rc) && IsRectEmpty(&rc);
}


bool Util::Wnd::isChild(HWND wnd)
{
    return (getStyle(wnd) & WS_CHILD) != 0;
}


HWND Util::Wnd::getNonChildParent(HWND wnd)
{
    while (isChild(wnd))
        wnd = GetParent(wnd);

    return wnd;
}


// Go up the window chain to find the ultimate top-level wnd:
// for WS_CHILD wnds use GetParent(), for the rest use GetWindow(GW_OWNER).
// Invisible parents (e.g. VCL's TApplication) are handled by the pin proxy mechanism.
HWND Util::Wnd::getTopParent(HWND wnd)
{
    for (;;) {
        HWND parent = isChild(wnd)
            ? GetParent(wnd)
            : GetWindow(wnd, GW_OWNER);

        if (!parent || parent == wnd) break;
        wnd = parent;
    }

    return wnd;
}


bool Util::Wnd::isProgManWnd(HWND wnd)
{
    return Util::Text::strimatch(getClassName(wnd).c_str(), L"ProgMan")
        && Util::Text::strimatch(getText(wnd).c_str(), L"Program Manager");
}


bool Util::Wnd::isTaskBar(HWND wnd)
{
    return Util::Text::strimatch(getClassName(wnd).c_str(), L"Shell_TrayWnd");
}


bool Util::Wnd::isTopMost(HWND wnd)
{
    return (getExStyle(wnd) & WS_EX_TOPMOST) != 0;
}


bool Util::Wnd::isVCLAppWnd(HWND wnd)
{
    return Util::Text::strimatch(L"TApplication", getClassName(wnd).c_str())
        && Util::Wnd::isWndRectEmpty(wnd);
}


void Util::App::error(HWND wnd, LPCWSTR s)
{
    Util::Res::ResStr caption(IDS_ERRBOXTTITLE, 50, DWORD_PTR(::App::APPNAME));
    MessageBox(wnd, s, caption, MB_ICONSTOP | MB_TOPMOST);
}


void Util::App::warning(HWND wnd, LPCWSTR s)
{
    Util::Res::ResStr caption(IDS_WRNBOXTTITLE, 50, DWORD_PTR(::App::APPNAME));
    MessageBox(wnd, s, caption, MB_ICONWARNING | MB_TOPMOST);
}


std::wstring Util::App::getHelpFileDescr(const std::wstring& path, const std::wstring& name)
{
    // Data layout:
    //  {CHM file data...}
    //  WCHAR data[size]  // no NULs
    //  DWORD size
    //  DWORD sig

    const DWORD CHM_MARKER_SIG = 0xefda7a00;  // from chmmark.py
    std::wstring ret;
    HANDLE file = CreateFile((path + name).c_str(), GENERIC_READ, FILE_SHARE_READ,
        0, OPEN_EXISTING, 0, 0);
    if (file == INVALID_HANDLE_VALUE)
        return ret;

    DWORD sig, len;
    if (SetFilePointer(file, 0, 0, FILE_END) != INVALID_SET_FILE_POINTER &&
        Util::Sys::readFileBack(file, &sig, sizeof(sig)) &&
        sig == CHM_MARKER_SIG &&
        Util::Sys::readFileBack(file, &len, sizeof(len)) &&
        len > 0 && len < 1024)
    {
        std::vector<char> buf(len);
        if (Util::Sys::readFileBack(file, &buf[0], len)) {
            std::vector<WCHAR> wbuf(len);
            int n = MultiByteToWideChar(CP_THREAD_ACP, 0, &buf[0], len, &wbuf[0], len);
            ret.assign(&wbuf[0], n);
        }
    }
    CloseHandle(file);
    return ret;
}


void Util::App::pinWindow(HWND wnd, HWND hitWnd, int trackRate, bool silent)
{
    int err = 0, wrn = 0;

    if (!hitWnd)
        wrn = IDS_ERR_COULDNOTFINDWND;
    else if (Util::Wnd::isProgManWnd(hitWnd))
        wrn = IDS_ERR_CANNOTPINDESKTOP;
    // NOTE: after creating the layer wnd, the taskbar becomes non-topmost;
    // use this check to avoid pinning it
    else if (Util::Wnd::isTaskBar(hitWnd))
        wrn = IDS_ERR_CANNOTPINTASKBAR;
    else if (Util::Wnd::isTopMost(hitWnd))
        wrn = IDS_ERR_ALREADYTOPMOST;
    // hidden wnds are handled by the proxy mechanism
    else {
        // create a pin wnd
        HWND pin = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            PinWnd::className,
            L"",
            WS_POPUP | WS_VISIBLE,
            0, 0, 0, 0,   // real pos/size set on wnd assignment
            0, 0, app.inst, 0);

        if (!pin)
            err = IDS_ERR_PINCREATE;
        else if (!SendMessage(pin, ::App::WM_PIN_ASSIGNWND, WPARAM(hitWnd), trackRate)) {
            err = IDS_ERR_PINWND;
            DestroyWindow(pin);
        }
    }

    if (!silent && (err || wrn)) {
        if (err)
            Util::App::error(wnd, Util::Res::ResStr(err));
        else
            Util::App::warning(wnd, Util::Res::ResStr(wrn));
    }

}


// If the specified window (top parent) is pinned,
// return the pin wnd's handle; otherwise return 0.
//
HWND Util::App::hasPin(HWND wnd)
{
    // enumerate all pin windows
    HWND pin = 0;
    while ((pin = FindWindowEx(0, pin, PinWnd::className, 0)) != 0)
        if (HWND(SendMessage(pin, ::App::WM_PIN_GETPINNEDWND, 0, 0)) == wnd)
            return pin;

    return 0;
}


void Util::App::togglePin(HWND wnd, HWND target, int trackRate)
{
    target = Util::Wnd::getTopParent(target);
    HWND pin = hasPin(target);
    if (pin)
        DestroyWindow(pin);
    else
        Util::App::pinWindow(wnd, target, trackRate);
}


void Util::App::markWnd(HWND wnd, bool mode)
{
    const int blinkDelay = 50;  // msec
    // thickness of highlight border
    const int width = 3;
    // first val can vary; second should be zero
    const int flashes = mode ? 1 : 0;
    // amount to deflate if wnd is maximized, to make the highlight visible
    const int zoomFix = IsZoomed(wnd) ? GetSystemMetrics(SM_CXFRAME) : 0;

    // when composition is enabled, drawing on the glass frame is prohibited
    // (GetWindowDC() returns a DC that clips the frame); in that case,
    // we use the screen DC and draw a simple rect (since GetWindowRgn()
    // returns ERROR); this rect overlaps other windows in front of the target,
    // but it's better than nothing
    const bool composition = app.dwm.isCompositionEnabled();

    HDC dc = composition ? GetDC(0) : GetWindowDC(wnd);
    if (dc) {
        int orgRop2 = SetROP2(dc, R2_XORPEN);

        HRGN rgn = CreateRectRgn(0,0,0,0);
        bool hasRgn = GetWindowRgn(wnd, rgn) != ERROR;
        const int loops = flashes*2+1;

        if (hasRgn) {
            for (int m = 0; m < loops; ++m) {
                FrameRgn(dc, rgn, HBRUSH(GetStockObject(WHITE_BRUSH)), width, width);
                GdiFlush();
                if (mode && m < loops-1)
                    Sleep(blinkDelay);
            }
        }
        else {
            RECT rc;
            GetWindowRect(wnd, &rc);
            if (!composition)
                OffsetRect(&rc, -rc.left, -rc.top);
            InflateRect(&rc, -zoomFix, -zoomFix);

            HGDIOBJ orgPen = SelectObject(dc, GetStockObject(WHITE_PEN));
            HGDIOBJ orgBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));

            RECT tmp;
            for (int m = 0; m < loops; ++m) {
                CopyRect(&tmp, &rc);
                for (int n = 0; n < width; ++n) {
                    Rectangle(dc, tmp.left, tmp.top, tmp.right, tmp.bottom);
                    InflateRect(&tmp, -1, -1);
                }
                GdiFlush();
                if (mode && m < loops-1)
                    Sleep(blinkDelay);
            }
            SelectObject(dc, orgBrush);
            SelectObject(dc, orgPen);
        }

        SetROP2(dc, orgRop2);
        ReleaseDC(composition ? 0 : wnd, dc);
        DeleteObject(rgn);
    }

}


void Util::Res::ResStr::init(UINT id, size_t bufLen, DWORD_PTR* params)
{
    std::vector<WCHAR> buf(bufLen, L'\0');
    LoadString(app.inst, id, &buf[0], int(bufLen));
    str = &buf[0];
    if (params) {
        DWORD flags = FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY;
        va_list* va = reinterpret_cast<va_list*>(params);
        DWORD n = FormatMessage(flags, str.c_str(), 0, 0, &buf[0], DWORD(bufLen), va);
        str.assign(&buf[0], n);
    }
}


bool Util::Gfx::rectContains(const RECT& rc1, const RECT& rc2)
{
    return rc2.left   >= rc1.left
        && rc2.top    >= rc1.top
        && rc2.right  <= rc1.right
        && rc2.bottom <= rc1.bottom;
}


// enable/disable all ctrls that lie inside the specified ctrl
// (usually a group, or maybe a tab, etc)
void Util::Wnd::enableGroup(HWND wnd, int id, bool mode)
{
    HWND container = GetDlgItem(wnd, id);
    RECT rc;
    GetWindowRect(container, &rc);

    // deflate a bit (4 DLUs) to be on the safe side
    RECT rc2 = {0, 0, 4, 4};
    MapDialogRect(wnd, &rc2);
    InflateRect(&rc, rc2.left-rc2.right, rc2.top-rc2.bottom);

    for (HWND child = GetWindow(wnd, GW_CHILD);
        child; child = GetWindow(child, GW_HWNDNEXT)) {
            if (child == container)
                continue;
            GetWindowRect(child, &rc2);
            if (Util::Gfx::rectContains(rc, rc2))
                EnableWindow(child, mode);
    }

}


std::vector<std::wstring> Util::Sys::getFiles(const std::wstring& mask)
{
    std::vector<std::wstring> ret;
    WIN32_FIND_DATA fd;
    HANDLE h = FindFirstFile(mask.c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                ret.push_back(fd.cFileName);
        } while (FindNextFile(h, &fd));
        FindClose(h);
    }
    return ret;
}


bool Util::Sys::readFileBack(HANDLE file, void* buf, int bytes)
{
    DWORD read;
    return SetFilePointer(file, -bytes, 0, FILE_CURRENT) != INVALID_SET_FILE_POINTER
        && ReadFile(file, buf, bytes, &read, 0)
        && int(read) == bytes
        && SetFilePointer(file, -bytes, 0, FILE_CURRENT) != INVALID_SET_FILE_POINTER;
}


std::wstring Util::Sys::getModulePath(HMODULE mod)
{
    std::vector<WCHAR> buf(MAX_PATH);
    for (;;) {
        DWORD n = GetModuleFileName(mod, &buf[0], DWORD(buf.size()));
        if (n == 0)
            return L"";
        if (n < buf.size())
            return std::wstring(&buf[0], n);
        buf.resize(buf.size() * 2);
    }
}


std::wstring Util::Sys::dirSpec(const std::wstring& path)
{
    std::wstring::size_type i = path.find_last_of(L"\\/");
    return i == std::wstring::npos ? L"" : path.substr(0, i + 1);
}


std::wstring Util::Sys::fileSpec(const std::wstring& path)
{
    std::wstring::size_type i = path.find_last_of(L"\\/");
    return i == std::wstring::npos ? path : path.substr(i + 1);
}


std::wstring Util::Sys::getLastErrorStr()
{
    LPWSTR msg = 0;
    DWORD n = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        0, GetLastError(), 0, LPWSTR(&msg), 0, 0);
    std::wstring ret(msg ? msg : L"", n);
    LocalFree(msg);
    return ret;
}


bool Util::Sys::isWin8orGreater()
{
    // without a compatibility manifest the reported version is capped at 6.2 (Win8),
    // which is still enough for this check
    OSVERSIONINFOEX osvi = { sizeof(osvi), 6, 2 };
    DWORDLONG mask = 0;
    VER_SET_CONDITION(mask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(mask, VER_MINORVERSION, VER_GREATER_EQUAL);
    return !!VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION, mask);
}


COLORREF Util::Clr::light(COLORREF clr)
{
    WORD h, l, s;
    ColorRGBToHLS(clr, &h, &l, &s);
    return ColorHLSToRGB(h, WORD(std::min(240, l + 60)), s);
}


COLORREF Util::Clr::dark(COLORREF clr)
{
    WORD h, l, s;
    ColorRGBToHLS(clr, &h, &l, &s);
    return ColorHLSToRGB(h, WORD(std::max(0, l - 60)), s);
}


BOOL Util::Wnd::moveWindow(HWND wnd, const RECT& rc, BOOL repaint)
{
    return MoveWindow(wnd, rc.left, rc.top,
        rc.right-rc.left, rc.bottom-rc.top, repaint);
}


bool Util::Wnd::psChanged(HWND page)
{
    return !!PropSheet_Changed(GetParent(page), page);
}


bool Util::Text::wildimatch(const std::wstring& pattern, const std::wstring& s)
{
    // iterative matcher with single-star backtracking
    size_t p = 0, i = 0, starP = std::wstring::npos, starI = 0;
    while (i < s.size()) {
        if (p < pattern.size() && (pattern[p] == L'?' ||
            towlower(pattern[p]) == towlower(s[i]))) {
            ++p; ++i;
        }
        else if (p < pattern.size() && pattern[p] == L'*') {
            starP = p++;
            starI = i;
        }
        else if (starP != std::wstring::npos) {
            p = starP + 1;
            i = ++starI;
        }
        else
            return false;
    }
    while (p < pattern.size() && pattern[p] == L'*')
        ++p;
    return p == pattern.size();
}


// removes the first accelerator prefix ('&')
// from a string and returns the result
std::wstring Util::Text::remAccel(std::wstring s)
{
    std::wstring::size_type i = s.find(L'&');
    if (i != std::wstring::npos) s.erase(i, 1);
    return s;
}


bool Util::Gfx::getBmpSize(HBITMAP bmp, SIZE& sz)
{
    BITMAP bm;
    if (!GetObject(bmp, sizeof(bm), &bm))
        return false;
    sz.cx = bm.bmWidth;
    sz.cy = abs(bm.bmHeight);
    return true;
}


// Replace each clrs[n][0] pixel with clrs[n][1].
// NOTE: Bmp must *not* be selected in any DC for this to succeed.
//
bool Util::Gfx::remapBmpColors(HBITMAP bmp, const COLORREF clrs[][2], int cnt)
{
    bool ok = false;
    SIZE sz;
    if (getBmpSize(bmp, sz)) {
        if (HDC dc = CreateCompatibleDC(0)) {
            if (HGDIOBJ orgBmp = SelectObject(dc, bmp)) {
                for (int y = 0; y < sz.cy; ++y) {
                    for (int x = 0; x < sz.cx; ++x) {
                        COLORREF clr = GetPixel(dc, x, y);
                        for (int n = 0; n < cnt; ++n)
                            if (clr == clrs[n][0]) {
                                SetPixelV(dc, x, y, clrs[n][1]);
                                break;
                            }
                    }
                }
                ok = true;
                SelectObject(dc, orgBmp);
            }
            DeleteDC(dc);
        }
    }
    return ok;
}


// Create a region from all pixels of a bitmap not matching the mask color.
//
HRGN Util::Gfx::makeRegionFromBmp(HBITMAP bmp, COLORREF clrMask)
{
    SIZE sz;
    if (!getBmpSize(bmp, sz))
        return 0;

    HDC dc = CreateCompatibleDC(0);
    if (!dc)
        return 0;
    HGDIOBJ orgBmp = SelectObject(dc, bmp);

    HRGN rgn = CreateRectRgn(0, 0, 0, 0);
    for (int y = 0; rgn && y < sz.cy; ++y) {
        for (int x = 0; x < sz.cx; ) {
            // skip transparent pixels, then collect an opaque run
            while (x < sz.cx && GetPixel(dc, x, y) == clrMask)
                ++x;
            int x0 = x;
            while (x < sz.cx && GetPixel(dc, x, y) != clrMask)
                ++x;
            if (x > x0) {
                HRGN run = CreateRectRgn(x0, y, x, y + 1);
                CombineRgn(rgn, rgn, run, RGN_OR);
                DeleteObject(run);
            }
        }
    }

    SelectObject(dc, orgBmp);
    DeleteDC(dc);
    return rgn;
}


HFONT Util::Gfx::createDerivedFont(HFONT base, LONG weight)
{
    LOGFONT lf;
    if (!GetObject(base, sizeof(lf), &lf))
        return 0;
    lf.lfWeight = weight;
    return CreateFontIndirect(&lf);
}


bool Util::RegKey::create(HKEY parent, const std::wstring& path)
{
    close();
    return RegCreateKeyEx(parent, path.c_str(), 0, 0, 0,
        KEY_READ | KEY_WRITE, 0, &key, 0) == ERROR_SUCCESS;
}


bool Util::RegKey::open(HKEY parent, const std::wstring& path)
{
    close();
    return RegOpenKeyEx(parent, path.c_str(), 0, KEY_READ | KEY_WRITE, &key) == ERROR_SUCCESS;
}


void Util::RegKey::close()
{
    if (key) {
        RegCloseKey(key);
        key = 0;
    }
}


bool Util::RegKey::getDWord(const std::wstring& name, DWORD& value) const
{
    DWORD type, size = sizeof(value);
    return RegQueryValueEx(key, name.c_str(), 0, &type, LPBYTE(&value), &size) == ERROR_SUCCESS
        && type == REG_DWORD;
}


bool Util::RegKey::setDWord(const std::wstring& name, DWORD value)
{
    return RegSetValueEx(key, name.c_str(), 0, REG_DWORD,
        LPBYTE(&value), sizeof(value)) == ERROR_SUCCESS;
}


bool Util::RegKey::getString(const std::wstring& name, std::wstring& value) const
{
    DWORD type, size = 0;
    if (RegQueryValueEx(key, name.c_str(), 0, &type, 0, &size) != ERROR_SUCCESS
        || (type != REG_SZ && type != REG_EXPAND_SZ))
        return false;
    std::vector<WCHAR> buf(size / sizeof(WCHAR) + 1, L'\0');
    size = DWORD(buf.size() * sizeof(WCHAR));
    if (RegQueryValueEx(key, name.c_str(), 0, &type, LPBYTE(&buf[0]), &size) != ERROR_SUCCESS)
        return false;
    value = &buf[0];
    return true;
}


bool Util::RegKey::setString(const std::wstring& name, const std::wstring& value)
{
    return RegSetValueEx(key, name.c_str(), 0, REG_SZ,
        reinterpret_cast<const BYTE*>(value.c_str()),
        DWORD((value.size() + 1) * sizeof(WCHAR))) == ERROR_SUCCESS;
}


bool Util::RegKey::deleteValue(const std::wstring& name)
{
    return RegDeleteValue(key, name.c_str()) == ERROR_SUCCESS;
}


DWORD Util::RegKey::getValueType(const std::wstring& name) const
{
    DWORD type;
    return RegQueryValueEx(key, name.c_str(), 0, &type, 0, 0) == ERROR_SUCCESS
        ? type : REG_NONE;
}
