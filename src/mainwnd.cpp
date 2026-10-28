#include "stdafx.h"
#include "options.h"
#include "optionsdlg.h"
#include "optpins.h"
#include "optautopin.h"
#include "opthotkeys.h"
#include "optlang.h"
#include "apputils.h"
#include "util.h"
#include "pinlayerwnd.h"
#include "pinwnd.h"
#include "mainwnd.h"


LPCWSTR MainWnd::className = L"EFDeskPins";


// About dialog.
//
class AboutDlg {
public:
    static INT_PTR CALLBACK proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        static HFONT boldGUIFont;

        switch (msg) {
            case WM_INITDIALOG:
                return evInitDlg(wnd, boldGUIFont);
            case WM_DESTROY:
                app.aboutDlg = 0;
                if (boldGUIFont) {
                    DeleteObject(boldGUIFont);
                    boldGUIFont = 0;
                }
                return true;
            case App::WM_PINSTATUS:
                updateStatusMessage(wnd);
                return true;
            case WM_NOTIFY: {
                NMHDR& nm = *reinterpret_cast<NMHDR*>(lparam);
                if ((nm.idFrom == IDC_MAIL || nm.idFrom == IDC_WEB) &&
                    (nm.code == NM_CLICK || nm.code == NM_RETURN))
                {
                    const NMLINK& link = *reinterpret_cast<NMLINK*>(lparam);
                    ShellExecute(wnd, L"open", link.item.szUrl, 0, 0, SW_SHOWNORMAL);
                    return true;
                }
                return false;
            }
            case WM_COMMAND:
                switch (LOWORD(wparam)) {
                    case IDOK:
                    case IDCANCEL:
                        DestroyWindow(wnd);
                        return true;
                }
                return false;
        }
        return false;
    }

    static void updateStatusMessage(HWND wnd)
    {
        SetDlgItemInt(wnd, IDC_STATUS, app.pinsUsed, false);
    }

    static bool evInitDlg(HWND wnd, HFONT& boldGUIFont)
    {
        app.aboutDlg = wnd;

        updateStatusMessage(wnd);

        boldGUIFont = Util::Gfx::createDerivedFont(HFONT(GetStockObject(DEFAULT_GUI_FONT)), FW_BOLD);
        if (boldGUIFont) {
            // make status and its label bold
            HWND status = GetDlgItem(wnd, IDC_STATUS);
            SendMessage(status, WM_SETFONT, WPARAM(boldGUIFont), false);
            SendMessage(GetWindow(status, GW_HWNDPREV), WM_SETFONT, WPARAM(boldGUIFont), false);
        }

        // set dlg icons
        HICON appIcon = LoadIcon(app.inst, MAKEINTRESOURCE(IDI_APP));
        SendMessage(wnd, WM_SETICON, ICON_BIG,   LPARAM(appIcon));
        SendMessage(wnd, WM_SETICON, ICON_SMALL, LPARAM(app.smIcon));

        return true;
    }

};


ATOM MainWnd::registerClass()
{
    WNDCLASS wc = {};
    wc.lpfnWndProc   = proc;
    wc.hInstance     = app.inst;
    wc.lpszClassName = className;
    return RegisterClass(&wc);
}


LRESULT CALLBACK MainWnd::proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    static UINT taskbarMsg = RegisterWindowMessage(L"TaskbarCreated");
    static PendingWindows pendWnds;
    static WindowCreationMonitor winCreMon;
    static Options* opt;

    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCT* cs = (CREATESTRUCT*)lparam;
            if (!cs || !cs->lpCreateParams)
                return -1;
            opt = (Options*)cs->lpCreateParams;

            app.mainWnd = wnd;

            if (opt->autoPinOn && !winCreMon.init(wnd, App::WM_QUEUEWINDOW)) {
                Util::App::error(wnd, Util::Res::ResStr(IDS_ERR_HOOKDLL));
                opt->autoPinOn = false;
            }

            app.smIcon = HICON(LoadImage(app.inst, MAKEINTRESOURCE(IDI_APP), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
            app.createSmClrIcon(opt->pinClr);

            // first set the wnd (this can only be done once)...
            app.trayIcon.setWnd(wnd);
            // .. and then create the notification icon
            app.trayIcon.create(app.smClrIcon, app.trayIconTip().c_str());

            // setup hotkeys
            if (opt->hotkeysOn) {
                bool allKeysSet = true;
                allKeysSet &= opt->hotEnterPin.set(wnd);
                allKeysSet &= opt->hotTogglePin.set(wnd);
                if (!allKeysSet)
                    Util::App::error(wnd, Util::Res::ResStr(IDS_ERR_HOTKEYSSET));
            }

            if (opt->autoPinOn)
                SetTimer(wnd, App::TIMERID_AUTOPIN, opt->autoPinDelay.value, 0);

            // init pin image/shape/dims
            app.pinShape.initShape();
            app.pinShape.initImage(opt->pinClr);

            break;
        }
        case WM_DESTROY: {
            app.mainWnd = 0;

            winCreMon.term();

            SendMessage(wnd, WM_COMMAND, CM_REMOVEPINS, 0);

            // remove hotkeys
            opt->hotEnterPin.unset(wnd);
            opt->hotTogglePin.unset(wnd);

            PostQuitMessage(0);
            break;
        }
        case App::WM_TRAYICON:
            evTrayIcon(wnd, wparam, lparam, opt);
            break;
        case App::WM_PINREQ:
            evPinReq(wnd, int(wparam), int(lparam), opt);
            break;
        case WM_HOTKEY:
            evHotkey(wnd, int(wparam), opt);
            break;
        case WM_TIMER:
            if (wparam == App::TIMERID_AUTOPIN) pendWnds.check(wnd, *opt);
            break;
        case App::WM_QUEUEWINDOW:
            pendWnds.add(HWND(wparam));
            break;
        case App::WM_PINSTATUS:
            if (lparam) ++app.pinsUsed; else --app.pinsUsed;
            if (app.aboutDlg) SendMessage(app.aboutDlg, App::WM_PINSTATUS, 0, 0);
            app.trayIcon.setTip(app.trayIconTip().c_str());
            break;
        case WM_COMMAND:
            switch (LOWORD(wparam)) {
                case IDHELP:
                    app.help.show(wnd);
                    break;
                case CM_ABOUT:
                    if (app.aboutDlg)
                        SetForegroundWindow(app.aboutDlg);
                    else {
                        CreateDialog(app.inst, MAKEINTRESOURCE(IDD_ABOUT), 0, AboutDlg::proc);
                        ShowWindow(app.aboutDlg, SW_SHOW);
                    }
                    break;
                case CM_NEWPIN:
                    cmNewPin(wnd);
                    break;
                case CM_REMOVEPINS:
                    cmRemovePins(wnd);
                    break;
                case CM_OPTIONS:
                    cmOptions(wnd, winCreMon, opt);
                    break;
                case CM_CLOSE:
                    DestroyWindow(wnd);
                    break;
                default:
                    break;
            }
            break;
        case WM_ENDSESSION: {
            if (wparam)
                opt->save();
            return 0;
        }
        case WM_DWMCOMPOSITIONCHANGED: {
            app.dwm.wmDwmCompositionChanged();
            return 0;
        }
        default:
            if (msg == taskbarMsg) {
                // taskbar recreated; reset the tray icon
                app.trayIcon.create(app.smClrIcon, app.trayIconTip().c_str());
                break;
            }
            return DefWindowProc(wnd, msg, wparam, lparam);
    }

    return 0;
}


void MainWnd::evHotkey(HWND wnd, int idHotKey, Options* opt)
{
    // ignore if there's an active pin layer
    if (app.layerWnd) return;

    switch (idHotKey) {
    case App::HOTID_ENTERPINMODE:
        PostMessage(wnd, WM_COMMAND, CM_NEWPIN, 0);
        break;
    case App::HOTID_TOGGLEPIN:
        Util::App::togglePin(wnd, GetForegroundWindow(), opt->trackRate.value);
        break;
    }

}


void MainWnd::evPinReq(HWND wnd, int x, int y, Options* opt)
{
    POINT pt = {x,y};
    HWND hitWnd = Util::Wnd::getTopParent(WindowFromPoint(pt));
    Util::App::pinWindow(wnd, hitWnd, opt->trackRate.value);
}


namespace
{
    // Set and manage specific tray menu images.
    //
    class TrayMenuDecorations : NonCopyable {
    public:
        TrayMenuDecorations(HMENU menu)
        {
            int w = GetSystemMetrics(SM_CXMENUCHECK);
            int h = GetSystemMetrics(SM_CYMENUCHECK);

            HDC    memDC  = CreateCompatibleDC(0);
            HFONT  fnt    = CreateFont(h, 0, 0, 0, FW_NORMAL, false, false, false,
                SYMBOL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH, L"Marlett");
            HBRUSH bkBrush = HBRUSH(GetStockObject(WHITE_BRUSH));

            HGDIOBJ orgFnt = GetCurrentObject(memDC, OBJ_FONT);
            HGDIOBJ orgBmp = GetCurrentObject(memDC, OBJ_BITMAP);

            bmpClose = makeBmp(memDC, w, h, bkBrush, menu, CM_CLOSE, fnt, L"r");
            bmpAbout = makeBmp(memDC, w, h, bkBrush, menu, CM_ABOUT, fnt, L"s");

            SelectObject(memDC, orgBmp);
            SelectObject(memDC, orgFnt);

            DeleteObject(fnt);
            DeleteDC(memDC);
        }

        ~TrayMenuDecorations()
        {
            if (bmpClose) DeleteObject(bmpClose);
            if (bmpAbout) DeleteObject(bmpAbout);
        }

    protected:
        HBITMAP bmpClose, bmpAbout;

        HBITMAP makeBmp(HDC dc, int w, int h, HBRUSH bkBrush, HMENU menu, int idCmd, HFONT fnt, LPCWSTR c)
        {
            HBITMAP retBmp = CreateBitmap(w, h, 1, 1, 0);
            if (retBmp) {
                RECT rc = {0, 0, w, h};
                SelectObject(dc, fnt);
                SelectObject(dc, retBmp);
                FillRect(dc, &rc, bkBrush);
                DrawText(dc, c, 1, &rc, DT_CENTER | DT_NOPREFIX);
                SetMenuItemBitmaps(menu, idCmd, MF_BYCOMMAND, retBmp, retBmp);
            }
            return retBmp;
        }

    };
}


void MainWnd::evTrayIcon(HWND wnd, WPARAM id, LPARAM msg, Options* opt)
{
    static bool gotLButtonDblClk = false;

    if (id != 0) return;

    switch (msg) {
        case WM_LBUTTONUP: {
            if (!opt->dblClkTray || gotLButtonDblClk) {
                SendMessage(wnd, WM_COMMAND, CM_NEWPIN, 0);
                gotLButtonDblClk = false;
            }
            break;
        }
        case WM_LBUTTONDBLCLK: {
            gotLButtonDblClk = true;
            break;
        }
        case WM_RBUTTONDOWN: {
            SetForegroundWindow(wnd);
            HMENU dummy = LoadMenu(app.inst, MAKEINTRESOURCE(IDM_TRAY));
            HMENU menu = GetSubMenu(dummy,0);
            SetMenuDefaultItem(menu, CM_NEWPIN, false);

            TrayMenuDecorations tmd(menu);

            POINT pt;
            GetCursorPos(&pt);
            TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, wnd, 0);
            DestroyMenu(dummy);
            SendMessage(wnd, WM_NULL, 0, 0);

            break;
        }
    }

}


void MainWnd::cmNewPin(HWND wnd)
{
    // avoid re-entrancy
    if (app.layerWnd) return;

    // NOTE: fix for Win8+ (the top-left corner doesn't work)
    const POINT layerWndPos = Util::Sys::isWin8orGreater() ? POINT{100, 100} : POINT{0, 0};
    const SIZE layerWndSize = { 1, 1 };

    app.layerWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        PinLayerWnd::className,
        L"DeskPin",
        WS_POPUP | WS_VISIBLE,
        layerWndPos.x, layerWndPos.y, layerWndSize.cx, layerWndSize.cy,
        wnd, 0, app.inst, 0);

    if (!app.layerWnd) return;

    ShowWindow(app.layerWnd, SW_SHOW);

    // synthesize a WM_LBUTTONDOWN on layerWnd, so that it captures the mouse
    POINT pt;
    GetCursorPos(&pt);
    SetCursorPos(layerWndPos.x, layerWndPos.y);
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    SetCursorPos(pt.x, pt.y);
}


void MainWnd::cmRemovePins(HWND wnd)
{
    HWND pin;
    while ((pin = FindWindow(PinWnd::className, 0)) != 0)
        DestroyWindow(pin);
}


void MainWnd::cmOptions(HWND wnd, WindowCreationMonitor& winCreMon, Options* opt)
{
    // sentry to avoid multiple dialogs
    static bool isOptDlgOn;
    if (isOptDlgOn) return;
    isOptDlgOn = true;

    const int pageCnt = 4;
    PROPSHEETPAGE psp[pageCnt];
    PROPSHEETHEADER psh;
    const int dlgIds[pageCnt] = {
        IDD_OPT_PINS, IDD_OPT_AUTOPIN, IDD_OPT_HOTKEYS, IDD_OPT_LANG
    };
    const DLGPROC dlgProcs[pageCnt] = {
        OptPins::dlgProc, OptAutoPin::dlgProc, OptHotKeys::dlgProc, OptLang::dlgProc
    };

    // This must remain in scope until PropertySheet() call,
    // because we use a char* from it in BuildOptPropSheet().
    Util::Res::ResStr capStr(IDS_OPTIONSTITLE);

    OptionsPropSheetData data = { *opt, winCreMon };
    OptionsDlg::buildOptPropSheet(psh, psp, dlgIds, dlgProcs, pageCnt, 0, data, capStr);

    if (PropertySheet(&psh) == -1) {
        std::wstring msg = LPCWSTR(Util::Res::ResStr(IDS_ERR_DLGCREATE));
        msg += L"\r\n";
        msg += Util::Sys::getLastErrorStr();
        Util::App::error(wnd, msg.c_str());
    }

    // reset tray tip, in case lang was changed
    app.trayIcon.setTip(app.trayIconTip().c_str());

    isOptDlgOn = false;
}
