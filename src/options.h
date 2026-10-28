#pragma once

#include "util.h"
#include "resource.h"


// Hotkey item.
// Manages its own activation, persistence and UI interaction.
//
struct HotKey {
    int  id;
    UINT vk;
    UINT mod;

    explicit HotKey(int id_, UINT vk_ = 0, UINT mod_ = 0)
        : id(id_), vk(vk_), mod(mod_) {}

    bool operator==(const HotKey& other) const {
        return vk == other.vk && mod == other.mod;
    }
    bool operator!=(const HotKey& other) const {
        return !(*this == other);
    }

    bool set(HWND wnd) const
    {
        return !!RegisterHotKey(wnd, id, mod, vk);
    }

    bool unset(HWND wnd) const
    {
        return !!UnregisterHotKey(wnd, id);
    }

    bool load(const Util::RegKey& key, LPCWSTR val);
    bool save(Util::RegKey& key, LPCWSTR val) const;

    void getUI(HWND wnd, int id)
    {
        LRESULT res = SendDlgItemMessage(wnd, id, HKM_GETHOTKEY, 0, 0);
        vk  = LOBYTE(res);
        mod = HIBYTE(res);
        if (!vk)
            mod = 0;
    }

    void setUI(HWND wnd, int id) const
    {
        SendDlgItemMessage(wnd, id, HKM_SETHOTKEY, MAKEWORD(vk, mod), 0);
    }

};


// Autopin rule.
// Manages its own persistence.
//
struct AutoPinRule {
    std::wstring descr;
    std::wstring ttl;
    std::wstring cls;
    bool enabled;

    AutoPinRule(const std::wstring& d = std::wstring(Util::Res::ResStr(IDS_NEWRULEDESCR)),
        const std::wstring& t = L"",
        const std::wstring& c = L"",
        bool b = true) : descr(d), ttl(t), cls(c), enabled(b) {}

    bool match(HWND wnd) const;

    bool load(const Util::RegKey& key, int i);
    bool save(Util::RegKey& key, int i) const;
    static void remove(Util::RegKey& key, int i);
};


// Simple integer option.
// Manages its own range and UI interaction.
//
struct IntOption {
    int value, minV, maxV, step;
    IntOption(int value_, int min_, int max_, int step_ = 1) :
        value(value_), minV(min_), maxV(std::max(min_, max_)), step(step_)
    {
        value = clamp(value);
    }

    int clamp(int n) const
    {
        return std::min(std::max(n, minV), maxV);
    }

    bool inRange(int t) const
    {
        return minV <= t && t <= maxV;
    }

    IntOption& operator=(int t)
    {
        value = clamp(t);
        return *this;
    }

    // get value from ctrl (use min val on error)
    // making sure it's in range
    int getUI(HWND wnd, int id) const
    {
        BOOL xlated;
        int t = int(GetDlgItemInt(wnd, id, &xlated, true));
        return xlated ? clamp(t) : minV;
    }

    // Check control for out-of-range values.
    // If clamp is false, show warning and move focus to control.
    // If clamp is true, set control to a proper value.
    // Return whether final value is valid.
    //
    bool validateUI(HWND wnd, int id, bool clampValue) const
    {
        BOOL xlated;
        int t = int(GetDlgItemInt(wnd, id, &xlated, true));
        if (xlated && inRange(t))
            return true;

        if (clampValue) {
            SetDlgItemInt(wnd, id, UINT(clamp(t)), true);
            return true;
        }
        else {
            // report error
            HWND prevSib = GetWindow(GetDlgItem(wnd, id), GW_HWNDPREV);
            std::wstring label = Util::Text::remAccel(Util::Wnd::getText(prevSib));
            Util::Res::ResStr str(IDS_WRN_UIRANGE, 256,
                DWORD_PTR(label.c_str()), DWORD_PTR(minV), DWORD_PTR(maxV));
            Util::App::warning(wnd, str);
            SetFocus(GetDlgItem(wnd, id));
            return false;
        }
    }

};

typedef std::vector<AutoPinRule> AutoPinRules;


// Program options.
// Provides default values and manages persistence.
//
class Options {
public:
    // pins
    COLORREF      pinClr;
    IntOption     trackRate;
    bool          dblClkTray;
    bool          runOnStartup;
    // hotkeys
    bool          hotkeysOn;
    HotKey        hotEnterPin, hotTogglePin;
    // autopin
    bool          autoPinOn;
    AutoPinRules  autoPinRules;
    IntOption     autoPinDelay;
    // lang
    std::wstring  uiLang;     // UI locale name (e.g. "zh-CN"); empty follows Windows
    std::wstring  helpFile;   // empty defaults to 'deskpins.chm'

    Options();

    bool save() const;
    bool load();
};


class WindowCreationMonitor;


// Used by options dialog to pass data to the various tabs.
//
struct OptionsPropSheetData {
    Options& opt;
    WindowCreationMonitor& winCreMon;

    // retrieve from a property page
    static OptionsPropSheetData& get(HWND page) {
        return *Util::Wnd::getDlgData<OptionsPropSheetData>(page);
    }
};
