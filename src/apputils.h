#pragma once


// Monitors system window creation using SetWinEventHook().
// Posts a message to the client window whenever a window is created.
//
class WindowCreationMonitor : NonCopyable {
public:
    WindowCreationMonitor() {}
    ~WindowCreationMonitor() { term(); }

    bool init(HWND wnd, int msgId);
    bool term();

private:
    static HWINEVENTHOOK hook;
    static HWND wnd;
    static int msgId;

    static VOID CALLBACK proc(HWINEVENTHOOK hook, DWORD event,
        HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
};


class Options;


// Autopin checking of created windows.
// Remembers the time each window is added, so when checking
// it only processes those that have passed the autopin delay.
//
class PendingWindows {
public:
    void add(HWND wnd);
    void check(HWND wnd, const Options& opt);

protected:
    struct Entry {
        HWND  wnd;
        DWORD time;
    };
    std::vector<Entry> entries;

    bool timeToChkWnd(DWORD t, const Options& opt);
    bool checkWnd(HWND target, const Options& opt);

};
