#include "stdafx.h"
#include "options.h"
#include "optionsdlg.h"
#include "util.h"
#include "pinwnd.h"
#include "optpins.h"
#include "resource.h"


HBRUSH OptPins::getPinClrBrush(HWND wnd)
{
    HWND ctrl = GetDlgItem(wnd, IDC_PIN_COLOR_BOX);
    return HBRUSH(GetWindowLongPtr(ctrl, GWLP_USERDATA));
}


COLORREF OptPins::getPinClr(HWND wnd)
{
    HBRUSH brush = getPinClrBrush(wnd);
    LOGBRUSH lb;
    return GetObject(brush, sizeof(LOGBRUSH), &lb) ? lb.lbColor : 0;
}


void OptPins::setPinClrBrush(HWND wnd, COLORREF clr)
{
    DeleteObject(getPinClrBrush(wnd));

    HWND ctrl = GetDlgItem(wnd, IDC_PIN_COLOR_BOX);
    SetWindowLongPtr(ctrl, GWLP_USERDATA, LONG_PTR(CreateSolidBrush(clr)));
    InvalidateRect(ctrl, 0, true);
}


bool OptPins::evInitDialog(HWND wnd, HWND focus, LPARAM lparam)
{
    // must have a valid data ptr
    if (!lparam) {
        EndDialog(wnd, IDCANCEL);
        return false;
    }

    // save the data ptr
    PROPSHEETPAGE& psp = *reinterpret_cast<PROPSHEETPAGE*>(lparam);
    SetWindowLongPtr(wnd, DWLP_USER, psp.lParam);
    Options& opt = OptionsPropSheetData::get(wnd).opt;

    setPinClrBrush(wnd, opt.pinClr);

    SendDlgItemMessage(wnd, IDC_POLL_RATE_UD, UDM_SETRANGE, 0,
        MAKELONG(opt.trackRate.maxV,opt.trackRate.minV));
    SendDlgItemMessage(wnd, IDC_POLL_RATE_UD, UDM_SETPOS, 0,
        MAKELONG(opt.trackRate.value,0));

    CheckDlgButton(wnd,
        opt.dblClkTray ? IDC_TRAY_DOUBLE_CLICK : IDC_TRAY_SINGLE_CLICK,
        BST_CHECKED);

    if (opt.runOnStartup)
        CheckDlgButton(wnd, IDC_RUN_ON_STARTUP, BST_CHECKED);

    return false;
}


void OptPins::evTermDialog(HWND wnd)
{
    DeleteObject(getPinClrBrush(wnd));
}


bool OptPins::validate(HWND wnd)
{
    Options& opt = OptionsPropSheetData::get(wnd).opt;
    return opt.trackRate.validateUI(wnd, IDC_POLL_RATE, true);
}


void OptPins::cmPinClr(HWND wnd)
{
    static COLORREF userClrs[16] = {0};

    CHOOSECOLOR cc = {};
    cc.lStructSize  = sizeof(cc);
    cc.hwndOwner    = wnd;
    cc.rgbResult    = getPinClr(wnd);
    cc.lpCustColors = userClrs;
    cc.Flags        = CC_RGBINIT | CC_SOLIDCOLOR;
    if (ChooseColor(&cc)) {
        setPinClrBrush(wnd, cc.rgbResult);
        Util::Wnd::psChanged(wnd);
    }
}


BOOL CALLBACK OptPins::enumWndProcPinsUpdate(HWND wnd, LPARAM)
{
    if (Util::Text::strimatch(PinWnd::className, Util::Wnd::getClassName(wnd).c_str()))
        InvalidateRect(wnd, 0, false);
    return true;    // continue
}


void OptPins::updatePinWnds()
{
    EnumWindows(enumWndProcPinsUpdate, 0);
}


BOOL CALLBACK OptPins::resetPinTimersEnumProc(HWND wnd, LPARAM param)
{
    if (Util::Text::strimatch(PinWnd::className, Util::Wnd::getClassName(wnd).c_str()))
        SendMessage(wnd, App::WM_PIN_RESETTIMER, param, 0);
    return true;    // continue
}


void OptPins::apply(HWND wnd)
{
    Options& opt = OptionsPropSheetData::get(wnd).opt;

    COLORREF clr = getPinClr(wnd);
    if (opt.pinClr != clr) {
        app.createSmClrIcon(opt.pinClr = clr);
        app.trayIcon.setIcon(app.smClrIcon);
        if (app.pinShape.initImage(clr)) updatePinWnds();
    }

    int rate = opt.trackRate.getUI(wnd, IDC_POLL_RATE);
    if (opt.trackRate.value != rate)
        EnumWindows(resetPinTimersEnumProc, opt.trackRate.value = rate);

    opt.dblClkTray = IsDlgButtonChecked(wnd, IDC_TRAY_DOUBLE_CLICK) == BST_CHECKED;

    opt.runOnStartup = IsDlgButtonChecked(wnd, IDC_RUN_ON_STARTUP) == BST_CHECKED;
}


INT_PTR CALLBACK OptPins::dlgProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{

    switch (msg) {
        case WM_INITDIALOG:  return evInitDialog(wnd, HWND(wparam), lparam);
        case WM_DESTROY:     evTermDialog(wnd); return true;
        case WM_NOTIFY: {
            const NMHDR& nmhdr = *reinterpret_cast<NMHDR*>(lparam);
            if (OptionsDlg::handlePageNotify(wnd, nmhdr, validate))
                return true;
            switch (nmhdr.code) {
                case PSN_APPLY:
                    apply(wnd);
                    return true;
                case PSN_HELP: {
                    app.help.show(wnd, L"::\\optpins.htm");
                    return true;
                }
                case UDN_DELTAPOS: {
                    if (wparam == IDC_POLL_RATE_UD) {
                        NMUPDOWN& nmud = *(NMUPDOWN*)lparam;
                        nmud.iDelta *= OptionsPropSheetData::get(wnd).opt.trackRate.step;
                        Util::Wnd::setDlgResult(wnd, FALSE);   // allow change
                        return true;
                    }
                    else
                        return false;
                }
                default:
                    return false;
            }
        }
        case WM_HELP: {
            app.help.show(wnd, L"::\\optpins.htm");
            return true;
        }
        case WM_COMMAND: {
            WORD id = LOWORD(wparam), code = HIWORD(wparam);
            switch (id) {
                case IDC_PIN_COLOR:         cmPinClr(wnd); return true;
                case IDC_PIN_COLOR_BOX:     if (code == STN_DBLCLK) cmPinClr(wnd); return true;
                case IDC_TRAY_SINGLE_CLICK:
                case IDC_TRAY_DOUBLE_CLICK: Util::Wnd::psChanged(wnd); return true;
                case IDC_RUN_ON_STARTUP:    Util::Wnd::psChanged(wnd); return true;
                case IDC_POLL_RATE:         if (code == EN_CHANGE) Util::Wnd::psChanged(wnd); return true;
                default:                    return false;
            }
        }
        case WM_CTLCOLORSTATIC:
            if (HWND(lparam) == GetDlgItem(wnd, IDC_PIN_COLOR_BOX))
                return INT_PTR(getPinClrBrush(wnd));
            return 0;
        default:
            return false;
    }

}
