#include "stdafx.h"
#include "util.h"
#include "options.h"
#include "apputils.h"


HWINEVENTHOOK WindowCreationMonitor::hook = 0;
HWND WindowCreationMonitor::wnd = 0;
int WindowCreationMonitor::msgId = 0;


void PendingWindows::add(HWND wnd)
{
    entries.push_back({ wnd, GetTickCount() });
}


void PendingWindows::check(HWND wnd, const Options& opt)
{
    for (int n = int(entries.size())-1; n >= 0; --n) {
        if (timeToChkWnd(entries[n].time, opt)) {
            if (checkWnd(entries[n].wnd, opt))
                Util::App::pinWindow(wnd, entries[n].wnd, opt.trackRate.value, true);
            entries.erase(entries.begin() + n);
        }
    }
}


bool PendingWindows::timeToChkWnd(DWORD t, const Options& opt)
{
    // ticks are unsigned, so wrap-around is ok
    return GetTickCount() - t >= DWORD(opt.autoPinDelay.value);
}


bool PendingWindows::checkWnd(HWND target, const Options& opt)
{
    for (const AutoPinRule& rule : opt.autoPinRules)
        if (rule.match(target))
            return true;
    return false;
}


bool WindowCreationMonitor::init(HWND wnd, int msgId)
{
    if (!hook) {
        hook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_CREATE,
            0, proc, 0, 0, WINEVENT_OUTOFCONTEXT);
        if (hook) {
            this->wnd = wnd;
            this->msgId = msgId;
        }
    }
    return hook != 0;
}


bool WindowCreationMonitor::term()
{
    if (hook && UnhookWinEvent(hook)) {
        hook = 0;
    }
    return !hook;
}


VOID CALLBACK WindowCreationMonitor::proc(HWINEVENTHOOK hook, DWORD event,
    HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (hook == WindowCreationMonitor::hook &&
        event == EVENT_OBJECT_CREATE &&
        idObject == OBJID_WINDOW)
    {
        PostMessage(wnd, msgId, (WPARAM)hwnd, 0);
    }
}
