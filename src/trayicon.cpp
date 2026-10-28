#include "stdafx.h"
#include "trayicon.h"


TrayIcon::TrayIcon(UINT msg, UINT id)
:
    wnd(0), id(id), msg(msg)
{
}


TrayIcon::~TrayIcon()
{
    destroy();
}


// can only be called once
bool TrayIcon::setWnd(HWND wnd_)
{
    if (wnd) return false;

    wnd = wnd_;
    return true;
}


NOTIFYICONDATA TrayIcon::makeData(UINT flags) const
{
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd   = wnd;
    nid.uID    = id;
    nid.uFlags = flags;
    return nid;
}


bool TrayIcon::create(HICON icon, LPCWSTR tip)
{
    NOTIFYICONDATA nid = makeData(NIF_MESSAGE | NIF_ICON | NIF_TIP);
    nid.uCallbackMessage = msg;
    nid.hIcon            = icon;
    lstrcpyn(nid.szTip, tip, ARRAYSIZE(nid.szTip));

    return !!Shell_NotifyIcon(NIM_ADD, &nid);
}


bool TrayIcon::destroy()
{
    NOTIFYICONDATA nid = makeData(0);
    return !!Shell_NotifyIcon(NIM_DELETE, &nid);
}


bool TrayIcon::setTip(LPCWSTR tip)
{
    NOTIFYICONDATA nid = makeData(NIF_TIP);
    lstrcpyn(nid.szTip, tip, ARRAYSIZE(nid.szTip));

    return !!Shell_NotifyIcon(NIM_MODIFY, &nid);
}


bool TrayIcon::setIcon(HICON icon)
{
    NOTIFYICONDATA nid = makeData(NIF_ICON);
    nid.hIcon = icon;

    return !!Shell_NotifyIcon(NIM_MODIFY, &nid);
}
