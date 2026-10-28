#pragma once


// Program notification icon management.
//
class TrayIcon : NonCopyable {
public:
    TrayIcon(UINT msg, UINT id);
    ~TrayIcon();

    bool setWnd(HWND wnd_);

    bool create(HICON icon, LPCWSTR tip);
    bool destroy();

    bool setTip(LPCWSTR tip);
    bool setIcon(HICON icon);

private:
    HWND wnd;
    UINT id;
    UINT msg;

    NOTIFYICONDATA makeData(UINT flags) const;
};
