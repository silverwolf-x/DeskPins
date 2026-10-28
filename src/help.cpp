#include "stdafx.h"
#include "util.h"
#include "help.h"
#include "resource.h"


Help::~Help()
{
    if (hhctrl)
        FreeLibrary(hhctrl);
}


bool Help::init(HINSTANCE inst, const std::wstring& fname)
{
    hlpFile = L"";

    std::wstring path = Util::Sys::dirSpec(Util::Sys::getModulePath(inst));
    if (path.empty())
        return false;

    hlpFile = path + fname;
    return true;
}


// if 'topic' is empty, use just the filename (goes to default topic)
// otherwise, append it to file spec
//   topic format is:  "::\someTopic.html"  or
//                     "::\someTopic.html#namedAnchor"
HWND Help::show(HWND wnd, const std::wstring& topic)
{
    typedef HWND (WINAPI* HtmlHelpW_Ptr)(HWND, LPCWSTR, UINT, DWORD_PTR);
    const UINT HH_DISPLAY_TOPIC = 0;

    if (!hhctrl)
        hhctrl = LoadLibrary(L"hhctrl.ocx");
    HtmlHelpW_Ptr htmlHelp = hhctrl
        ? HtmlHelpW_Ptr(GetProcAddress(hhctrl, "HtmlHelpW")) : 0;

    if (hlpFile.empty() || !htmlHelp ||
        GetFileAttributes(hlpFile.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        Util::App::error(wnd, Util::Res::ResStr(IDS_ERR_HELPMISSING));
        return 0;
    }

    return htmlHelp(wnd, (hlpFile + topic).c_str(), HH_DISPLAY_TOPIC, 0);
}
