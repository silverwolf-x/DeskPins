#pragma once


// Options dialog.
// Uses a property sheet to host the options tabs.
//
class OptionsDlg {
public:
    static void buildOptPropSheet(PROPSHEETHEADER& psh, PROPSHEETPAGE psp[],
        const int dlgIds[], const DLGPROC dlgProcs[], int pageCnt, HWND parentWnd,
        OptionsPropSheetData& data, const Util::Res::ResStr& capStr);

    // Common property page notifications (PSN_SETACTIVE, PSN_KILLACTIVE).
    // Returns true if handled.
    static bool handlePageNotify(HWND page, const NMHDR& nmhdr, bool (*validate)(HWND));

private:
    static WNDPROC orgOptPSProc;

    static void fixOptPSPos(HWND wnd);
    static LRESULT CALLBACK optPSSubclass(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);
    static int CALLBACK optPSCallback(HWND wnd, UINT msg, LPARAM param);
};
