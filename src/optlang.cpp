#include "stdafx.h"
#include "options.h"
#include "optionsdlg.h"
#include "util.h"
#include "optlang.h"
#include "resource.h"


std::wstring OptLang::getComboSel(HWND combo)
{
    LRESULT n = SendMessage(combo, CB_GETCURSEL, 0, 0);
    if (n != CB_ERR) {
        LRESULT i = SendMessage(combo, CB_GETITEMDATA, n, 0);
        if (i != CB_ERR && i != 0)
            return reinterpret_cast<Data*>(i)->fname;
    }
    return L"";
}


// Add item to combo (taking ownership of data) and select it if it's the current one.
void OptLang::addComboItem(HWND combo, Data* data, const std::wstring& cur)
{
    LRESULT i = SendMessage(combo, CB_ADDSTRING, 0, LPARAM(data));
    if (i == CB_ERR || i == CB_ERRSPACE)
        delete data;
    else if (Util::Text::strimatch(data->fname.c_str(), cur.c_str()))
        SendMessage(combo, CB_SETCURSEL, i, 0);
}


// Languages built into the exe; names are shown in their own language.
void OptLang::loadUILangs(HWND combo, const std::wstring& cur)
{
    static const struct { LPCWSTR locale, name; } langs[] = {
        { L"",      0 },   // automatic
        { L"en-US", L"English" },
        { L"zh-CN", L"简体中文" },
    };

    for (const auto& lang : langs) {
        Data* data = new Data;
        data->fname    = lang.locale;
        data->dispName = lang.locale;
        data->descr    = lang.name ? lang.name : LPCWSTR(Util::Res::ResStr(IDS_LANG_AUTO));
        addComboItem(combo, data, cur);
    }

}


void OptLang::loadHelpFiles(HWND combo, const std::wstring& path, const std::wstring& cur)
{
    for (const std::wstring& file : Util::Sys::getFiles(path + L"DeskPins*.chm")) {
        Data* data = new Data;
        data->fname    = file;
        data->dispName = file;
        data->descr    = Util::App::getHelpFileDescr(path, file);
        addComboItem(combo, data, cur);
    }

}


bool OptLang::evInitDialog(HWND wnd, HWND focus, LPARAM param)
{
    // must have a valid data ptr
    if (!param) {
        EndDialog(wnd, IDCANCEL);
        return true;
    }

    // save the data ptr
    PROPSHEETPAGE& psp = *reinterpret_cast<PROPSHEETPAGE*>(param);
    SetWindowLongPtr(wnd, DWLP_USER, psp.lParam);
    Options& opt = OptionsPropSheetData::get(wnd).opt;

    loadUILangs(GetDlgItem(wnd, IDC_UILANG), opt.uiLang);

    std::wstring exePath = Util::Sys::dirSpec(Util::Sys::getModulePath(app.inst));
    if (!exePath.empty())
        loadHelpFiles(GetDlgItem(wnd, IDC_HELPLANG), exePath, opt.helpFile);

    return true;
}


bool OptLang::validate(HWND wnd)
{
    return true;
}


void OptLang::apply(HWND wnd)
{
    Options& opt = OptionsPropSheetData::get(wnd).opt;

    // takes effect for windows and menus created from now on
    std::wstring uiLang = getComboSel(GetDlgItem(wnd, IDC_UILANG));
    if (opt.uiLang != uiLang) {
        opt.uiLang = uiLang;
        App::setUILanguage(uiLang);
    }

    opt.helpFile = getComboSel(GetDlgItem(wnd, IDC_HELPLANG));
    app.help.init(app.inst, opt.helpFile);
}


void OptLang::evDrawItem(const DRAWITEMSTRUCT& dis)
{
    const RECT& rc = dis.rcItem;
    HDC dc = dis.hDC;
    if (dis.itemID == UINT(-1))
        return;

    bool sel = (dis.itemState & ODS_SELECTED) != 0;
    FillRect(dc, &rc, GetSysColorBrush(sel ? COLOR_HIGHLIGHT : COLOR_WINDOW));

    if (!dis.itemData)
        return;
    const Data& data = *reinterpret_cast<const Data*>(dis.itemData);

    UINT     orgAlign  = SetTextAlign(dc, TA_LEFT);
    COLORREF orgTxtClr = SetTextColor(dc, GetSysColor(sel ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
    int      orgBkMode = SetBkMode(dc, TRANSPARENT);

    TextOut(dc, rc.left+2, rc.top+1, data.descr.c_str(), int(data.descr.length()));
    SetTextAlign(dc, TA_RIGHT);
    if (!sel) SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
    TextOut(dc, rc.right-2, rc.top+1, data.dispName.c_str(), int(data.dispName.length()));

    SetBkMode   (dc, orgBkMode);
    SetTextColor(dc, orgTxtClr);
    SetTextAlign(dc, orgAlign);
}


INT_PTR CALLBACK OptLang::dlgProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
        case WM_INITDIALOG:  return evInitDialog(wnd, HWND(wparam), lparam);
        case WM_NOTIFY: {
            const NMHDR& nmhdr = *reinterpret_cast<NMHDR*>(lparam);
            if (OptionsDlg::handlePageNotify(wnd, nmhdr, validate))
                return true;
            switch (nmhdr.code) {
                case PSN_APPLY:
                    apply(wnd);
                    return true;
                case PSN_HELP:
                    app.help.show(wnd, L"::\\optlang.htm");
                    return true;
                default:
                    return false;
            }
        }
        case WM_COMMAND: {
            WORD code = HIWORD(wparam), id = LOWORD(wparam);
            if (code == CBN_SELCHANGE && (id == IDC_UILANG || id == IDC_HELPLANG))
                Util::Wnd::psChanged(wnd);
            return true;
        }
        case WM_HELP:
            app.help.show(wnd, L"::\\optlang.htm");
            return true;
        case WM_DELETEITEM:
            if (wparam == IDC_UILANG || wparam == IDC_HELPLANG)
                delete reinterpret_cast<Data*>(LPDELETEITEMSTRUCT(lparam)->itemData);
            return true;
        case WM_MEASUREITEM: {
            MEASUREITEMSTRUCT& mis = *(MEASUREITEMSTRUCT*)lparam;
            HDC dc = GetDC(wnd);
            HGDIOBJ orgFont = SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT));
            TEXTMETRIC tm;
            GetTextMetrics(dc, &tm);
            SelectObject(dc, orgFont);
            ReleaseDC(wnd, dc);
            mis.itemHeight = tm.tmHeight + 2;
            return true;
        }
        case WM_DRAWITEM:
            evDrawItem(*(DRAWITEMSTRUCT*)lparam);
            return true;
        case WM_COMPAREITEM: {
            const COMPAREITEMSTRUCT& cis = *(COMPAREITEMSTRUCT*)lparam;
            if (wparam == IDC_UILANG || wparam == IDC_HELPLANG) {
                const Data* d1 = (const Data*)cis.itemData1;
                const Data* d2 = (const Data*)cis.itemData2;
                if (d1 && d2) {
                    int order = CompareString(cis.dwLocaleId, NORM_IGNORECASE,
                        d1->descr.c_str(), int(d1->descr.length()),
                        d2->descr.c_str(), int(d2->descr.length()));
                    if (order) return order - 2;    // convert 1/2/3 to -1/0/1
                }
            }
            return 0;
        }
        default:
            return false;
    }
}
