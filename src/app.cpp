#include "stdafx.h"
#include "util.h"
#include "mainwnd.h"
#include "pinwnd.h"
#include "pinlayerwnd.h"
#include "app.h"
#include "resource.h"


LPCWSTR App::APPNAME = L"DeskPins";


App::~App()
{
    if (smIcon) DestroyIcon(smIcon);
    if (smClrIcon) DestroyIcon(smClrIcon);
    if (instMutex) CloseHandle(instMutex);
}


// Select the language used for loading resources (dialogs, menus, strings).
// All translations are built into the exe; an empty locale name
// follows the user's Windows display language.
void App::setUILanguage(const std::wstring& locale)
{
    if (locale.empty()) {
        SetThreadPreferredUILanguages(0, 0, 0);
        SetThreadUILanguage(GetUserDefaultUILanguage());
        return;
    }

    const std::wstring list = locale + L'\0';   // double-NUL terminated
    SetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, list.c_str(), 0);
    if (LANGID id = LANGIDFROMLCID(LocaleNameToLCID(locale.c_str(), 0)))
        SetThreadUILanguage(id);
}


bool App::initComctl()
{
    INITCOMMONCONTROLSEX iccx;
    iccx.dwSize = sizeof(iccx);
    iccx.dwICC = ICC_WIN95_CLASSES | ICC_LINK_CLASS;
    bool ret = !!InitCommonControlsEx(&iccx);
    if (!ret)
        Util::App::error(0, Util::Res::ResStr(IDS_ERR_CCINIT));
    return ret;
}


// Return true to continue loading or false to abort.
// If automatic detection fails, ask the user.
bool App::chkPrevInst()
{
    instMutex = CreateMutex(0, false, L"EFDeskPinsRunning");
    if (!instMutex)
        return MessageBox(0, Util::Res::ResStr(IDS_ERR_MUTEXFAILCONFIRM), APPNAME, MB_ICONEXCLAMATION | MB_YESNO) != IDNO;

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBox(0, Util::Res::ResStr(IDS_ERR_ALREADYRUNNING), APPNAME, MB_ICONINFORMATION);
        return false;
    }

    return true;
}


bool App::regWndCls()
{
    return MainWnd::registerClass() &&
           PinWnd::registerClass() &&
           PinLayerWnd::registerClass();
}


bool App::createMainWnd(Options& opt)
{
    // app.mainWnd set in WM_CREATE
    CreateWindow(MainWnd::className, App::APPNAME,
        WS_POPUP, 0,0,0,0, 0, 0, app.inst, &opt);

    return app.mainWnd != 0;
}


// Make a new small icon, by painting the original.
// On failure, we get the original icon.
void App::createSmClrIcon(COLORREF clr)
{
    if (smClrIcon) DestroyIcon(smClrIcon);
    smClrIcon = HICON(LoadImage(app.inst, MAKEINTRESOURCE(IDI_APP),
        IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));

    ICONINFO ii;
    if (GetIconInfo(smClrIcon, &ii)) {
        // (assuming non-monochrome)
        const COLORREF clrMap[][2] = {
            { Util::Clr::red, clr },
            { Util::Clr::maroon, Util::Clr::dark(clr) }
        };
        Util::Gfx::remapBmpColors(ii.hbmColor, clrMap, ARRAYSIZE(clrMap));
        HICON newIcon = CreateIconIndirect(&ii);
        if (newIcon) {
            DestroyIcon(smClrIcon);
            smClrIcon = newIcon;
        }
        // destroy bmps returned from GetIconInfo()
        DeleteObject(ii.hbmColor);
        DeleteObject(ii.hbmMask);
    }
}


std::wstring App::trayIconTip()
{
    return std::wstring(APPNAME) + L" - " +
        LPCWSTR(Util::Res::ResStr(IDS_TRAYTIP, 50)) + L": " + std::to_wstring(pinsUsed);
}
