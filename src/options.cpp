#include "stdafx.h"
#include "util.h"
#include "options.h"


namespace {
    const HKEY    HKCU             = HKEY_CURRENT_USER;
    const LPCWSTR REG_PATH_APP     = L"Software\\Elias Fotinis\\DeskPins";
    const LPCWSTR REG_APR_SUBPATH  = L"AutoPinRules";
    const LPCWSTR REG_PATH_RUN     = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

    const LPCWSTR REG_PINCLR       = L"PinColor";
    const LPCWSTR REG_POLLRATE     = L"PollRate";
    const LPCWSTR REG_TRAYDBLCLK   = L"TrayDblClk";
    const LPCWSTR REG_AUTOPINON    = L"Enabled";
    const LPCWSTR REG_AUTOPINDELAY = L"Delay";
    const LPCWSTR REG_AUTOPINCOUNT = L"Count";
    const LPCWSTR REG_HOTKEYSON    = L"HotKeysOn";
    const LPCWSTR REG_HOTNEWPIN    = L"HotKeyNewPin";
    const LPCWSTR REG_HOTTOGGLEPIN = L"HotKeyTogglePin";
    const LPCWSTR REG_UILANG       = L"UILanguage";
    const LPCWSTR REG_LCLHELP      = L"LocalizedHelp";

    // autopin rule value name: rule index + field flag (e.g. "3D")
    std::wstring ruleValueName(int i, WCHAR flag) {
        return std::to_wstring(i) + flag;
    }
}


bool HotKey::load(const Util::RegKey& key, LPCWSTR val)
{
    DWORD dw;
    if (!key.getDWord(val, dw))
        return false;

    vk  = LOBYTE(dw);
    mod = HIBYTE(dw);

    if (!vk)  // oops
        mod = 0;

    return true;
}


bool HotKey::save(Util::RegKey& key, LPCWSTR val) const
{
    return key.setDWord(val, MAKEWORD(vk, mod));
}


bool AutoPinRule::match(HWND wnd) const
{
    return enabled
        && Util::Text::wildimatch(ttl, Util::Wnd::getText(wnd))
        && Util::Text::wildimatch(cls, Util::Wnd::getClassName(wnd));
}


bool AutoPinRule::load(const Util::RegKey& key, int i)
{
    DWORD enabled_dw;
    if (!key.getString(ruleValueName(i, L'D'), descr) ||
        !key.getString(ruleValueName(i, L'T'), ttl) ||
        !key.getString(ruleValueName(i, L'C'), cls) ||
        !key.getDWord(ruleValueName(i, L'E'), enabled_dw))
    {
        return false;
    }
    enabled = enabled_dw != 0;
    return true;
}


bool AutoPinRule::save(Util::RegKey& key, int i) const
{
    return key.setString(ruleValueName(i, L'D'), descr) &&
           key.setString(ruleValueName(i, L'T'), ttl) &&
           key.setString(ruleValueName(i, L'C'), cls) &&
           key.setDWord(ruleValueName(i, L'E'), enabled);
}


void AutoPinRule::remove(Util::RegKey& key, int i)
{
    const WCHAR flags[] = { L'D', L'T', L'C', L'E' };
    for (WCHAR flag : flags)
        key.deleteValue(ruleValueName(i, flag));
}


Options::Options() :
    pinClr(RGB(255,0,0)),
    trackRate(20,10,1000,10),
    dblClkTray(false),
    runOnStartup(false),
    hotkeysOn(true),
    hotEnterPin(App::HOTID_ENTERPINMODE, VK_F11, MOD_CONTROL),
    hotTogglePin(App::HOTID_TOGGLEPIN, VK_F12, MOD_CONTROL),
    autoPinOn(false),
    autoPinDelay(200,100,10000,50),
    helpFile(L"DeskPins.chm")    // init, in case key doesn't exist
{
}


bool Options::save() const
{
    Util::RegKey key;
    if (!key.create(HKCU, REG_PATH_APP)) return false;

    key.setDWord(REG_PINCLR, pinClr);
    key.setDWord(REG_POLLRATE, trackRate.value);
    key.setDWord(REG_TRAYDBLCLK, dblClkTray);

    Util::RegKey runKey;
    if (runKey.create(HKCU, REG_PATH_RUN)) {
        if (runOnStartup)
            runKey.setString(App::APPNAME, L"\"" + Util::Sys::getModulePath(0) + L"\"");
        else
            runKey.deleteValue(App::APPNAME);
    }

    key.setDWord(REG_HOTKEYSON, hotkeysOn);
    hotEnterPin.save(key, REG_HOTNEWPIN);
    hotTogglePin.save(key, REG_HOTTOGGLEPIN);

    Util::RegKey apKey;
    if (apKey.create(key, REG_APR_SUBPATH)) {
        DWORD oldCount;
        if (!apKey.getDWord(REG_AUTOPINCOUNT, oldCount))
            oldCount = 0;
        apKey.setDWord(REG_AUTOPINON, autoPinOn);
        apKey.setDWord(REG_AUTOPINDELAY, autoPinDelay.value);
        apKey.setDWord(REG_AUTOPINCOUNT, DWORD(autoPinRules.size()));
        int n;
        for (n = 0; n < int(autoPinRules.size()); ++n)
            autoPinRules[n].save(apKey, n);
        // remove old left-over rules
        for (; n < int(oldCount); ++n)
            AutoPinRule::remove(apKey, n);
    }

    key.setString(REG_UILANG, uiLang);
    key.setString(REG_LCLHELP, helpFile);

    return true;
}


bool Options::load()
{
    Util::RegKey key;
    if (!key.open(HKCU, REG_PATH_APP)) return false;

    DWORD dw;
    std::wstring buf;

    if (key.getDWord(REG_PINCLR, dw))
        pinClr = COLORREF(dw & 0xFFFFFF);
    if (key.getDWord(REG_POLLRATE, dw) && trackRate.inRange(int(dw)))
        trackRate = int(dw);
    if (key.getDWord(REG_TRAYDBLCLK, dw))
        dblClkTray = dw != 0;

    Util::RegKey runKey;
    if (runKey.open(HKCU, REG_PATH_RUN))
        runOnStartup = runKey.getValueType(App::APPNAME) == REG_SZ;

    if (key.getDWord(REG_HOTKEYSON, dw))
        hotkeysOn = dw != 0;
    hotEnterPin.load(key, REG_HOTNEWPIN);
    hotTogglePin.load(key, REG_HOTTOGGLEPIN);

    Util::RegKey apKey;
    if (apKey.open(key, REG_APR_SUBPATH)) {
        if (apKey.getDWord(REG_AUTOPINON, dw))
            autoPinOn = dw != 0;
        if (apKey.getDWord(REG_AUTOPINDELAY, dw) && autoPinDelay.inRange(int(dw)))
            autoPinDelay = int(dw);
        if (apKey.getDWord(REG_AUTOPINCOUNT, dw)) {
            for (int n = 0; n < int(dw); ++n) {
                AutoPinRule rule;
                if (rule.load(apKey, n))
                    autoPinRules.push_back(rule);
            }
        }
    }

    if (key.getString(REG_UILANG, buf))
        uiLang = buf;
    if (key.getString(REG_LCLHELP, buf))
        helpFile = buf;

    if (helpFile.empty())
        helpFile = L"DeskPins.chm";

    return true;
}
