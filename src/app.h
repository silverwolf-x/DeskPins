#pragma once

#include "pinshape.h"
#include "trayicon.h"
#include "help.h"


class Options;


// Desktop Window Manager state.
// The main window needs to call this to handle broadcast messages.
//
class Dwm : NonCopyable
{
public:
    Dwm() { wmDwmCompositionChanged(); }
    bool isCompositionEnabled() const {
        return cachedIsCompositionEnabled;
    }
    // WM_DWMCOMPOSITIONCHANGED handler
    void wmDwmCompositionChanged() {
        BOOL b;
        cachedIsCompositionEnabled = SUCCEEDED(DwmIsCompositionEnabled(&b)) && b;
    }
private:
    bool cachedIsCompositionEnabled;
};


// Main application state.
// A single global instance is available to all translation units.
//
struct App : NonCopyable {
    HANDLE    instMutex;
    HWND      mainWnd, aboutDlg, layerWnd;
    HINSTANCE inst;
    PinShape  pinShape;
    HICON     smIcon, smClrIcon;
    Help      help;
    int       pinsUsed;
    int       optionPage;
    TrayIcon  trayIcon;
    Dwm       dwm;

    static LPCWSTR APPNAME;

    enum {
        // app messages
        WM_TRAYICON = WM_APP,
        WM_PINSTATUS,
        WM_PINREQ,
        WM_QUEUEWINDOW,

        // pin wnd messages
        WM_PIN_ASSIGNWND = WM_USER,
        WM_PIN_RESETTIMER,
        WM_PIN_GETPINNEDWND,

        // system hotkey IDs
        HOTID_ENTERPINMODE = 0,
        HOTID_TOGGLEPIN    = 1,

        // app timers
        TIMERID_AUTOPIN = 1,
    };

    App() : instMutex(0),
        mainWnd(0), aboutDlg(0), layerWnd(0),
        inst(0),
        smIcon(0), smClrIcon(0),
        pinsUsed(0), optionPage(0), trayIcon(WM_TRAYICON, 0) {}
    ~App();

    static void setUILanguage(const std::wstring& locale);
    bool initComctl();
    bool chkPrevInst();
    bool regWndCls();
    bool createMainWnd(Options& opt);
    void createSmClrIcon(COLORREF clr);
    std::wstring trayIconTip();
};


extern App app;
