#ifndef GLOBALDEFINE_H
#define GLOBALDEFINE_H

#ifdef DEBUG
constexpr const char *const CONFIG_PATH         = "monitor.conf";
constexpr const char *const REMINDER_PATH       = "reminder";
constexpr const char *const SCREENRECORDER_PATH = "ScreenRecorder";
constexpr const char *const PASTEFLOW_PATH      = "PasteFlow";
#else
constexpr const char *const CONFIG_PATH         = "/etc/monitor.conf";
constexpr const char *const REMINDER_PATH       = "/usr/bin/reminder";
constexpr const char *const SCREENRECORDER_PATH = "/usr/bin/ScreenRecorder";
constexpr const char *const PASTEFLOW_PATH      = "/usr/bin/PasteFlow";
#endif

#define UNUSED(x) (void)x;
#define SAFE_DELETE(x) \
    if (nullptr != x)  \
        delete x;      \
    x = nullptr;

#endif  // GLOBALDEFINE_H
