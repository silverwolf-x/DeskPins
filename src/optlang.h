#pragma once


// Language options tab.
//
class OptLang
{
public:
    static INT_PTR CALLBACK dlgProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);

protected:
    // Combobox item data.
    //
    struct Data {
        std::wstring fname, dispName, descr;
    };

    static bool validate(HWND wnd);
    static void apply(HWND wnd);

    static std::wstring getComboSel(HWND combo);
    static void addComboItem(HWND combo, Data* data, const std::wstring& cur);
    static void loadUILangs(HWND combo, const std::wstring& cur);
    static void loadHelpFiles(HWND combo, const std::wstring& path, const std::wstring& cur);

    static bool evInitDialog(HWND wnd, HWND focus, LPARAM param);
    static void evDrawItem(const DRAWITEMSTRUCT& dis);
};
