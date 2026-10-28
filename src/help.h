#pragma once


// Help file manager.
// Locates the help file path and displays requested topics.
//
class Help : NonCopyable {
public:
    Help() : hhctrl(0) {}
    ~Help();

    bool init(HINSTANCE inst, const std::wstring& fname);
    HWND show(HWND wnd, const std::wstring& topic = L"");

protected:
    std::wstring hlpFile;
    HMODULE hhctrl;   // HTML Help runtime, loaded on demand
};
