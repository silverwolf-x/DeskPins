#include "stdafx.h"
#include "util.h"
#include "options.h"
#include "app.h"
#include "resource.h"

// enable visual styles
#pragma comment(linker, "/manifestdependency:\""                               \
    "type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' " \
    "processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'" \
    "\"")


// global app object
App app;

// options object visible only in this file
static Options opt;


int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, LPWSTR, int)
{
    app.inst = inst;

    // load settings as soon as possible
    opt.load();

    // setup UI language & help early to get correct language msgs
    App::setUILanguage(opt.uiLang);
    app.help.init(app.inst, opt.helpFile);

    if (!app.chkPrevInst() || !app.initComctl())
        return 0;

    if (!app.regWndCls() || !app.createMainWnd(opt)) {
        Util::App::error(0, Util::Res::ResStr(IDS_ERR_WNDCLSREG));
        return 0;
    }

    MSG msg;
    while (GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    opt.save();

    return int(msg.wParam);
}
