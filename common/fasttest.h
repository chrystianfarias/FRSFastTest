// Shared helpers for the per-game FRSFastTest plugins: logging, code patching, import hooks and the [Window]
// options (monitor placement and running without focus).
#pragma once

#include <windows.h>

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace ft {

// ---------------------------------------------------------------- files beside the plugin

inline HMODULE& Module() {
    static HMODULE module = nullptr;
    return module;
}

// The plugin's path with its extension replaced: BesidePlugin(".ini") -> ...\scripts\FRSFastTest.ini
inline std::string BesidePlugin(const char* extension) {
    char path[MAX_PATH] = {};
    GetModuleFileNameA(Module(), path, MAX_PATH);
    std::string out = path;
    const size_t dot = out.find_last_of('.');
    if (dot != std::string::npos) out.resize(dot);
    return out + extension;
}

inline FILE*& LogFile() {
    static FILE* file = nullptr;
    return file;
}

inline void OpenLog() {
    LogFile() = std::fopen(BesidePlugin(".log").c_str(), "w");
}

inline void Log(const char* fmt, ...) {
    FILE* f = LogFile();
    if (!f) return;
    SYSTEMTIME t;
    GetLocalTime(&t);
    std::fprintf(f, "%02d:%02d:%02d.%03d ", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
    va_list args;
    va_start(args, fmt);
    std::vfprintf(f, fmt, args);
    va_end(args);
    std::fputc('\n', f);
    std::fflush(f);
}

// ---------------------------------------------------------------- ini

struct Ini {
    std::string path;

    int Int(const char* section, const char* key, int fallback) const {
        // read as text: GetPrivateProfileInt does not take negative values
        char text[32] = {};
        GetPrivateProfileStringA(section, key, "", text, sizeof(text), path.c_str());
        return text[0] ? (int)std::strtol(text, nullptr, 10) : fallback;
    }

    bool Bool(const char* section, const char* key, bool fallback) const {
        return Int(section, key, fallback ? 1 : 0) != 0;
    }

    std::string String(const char* section, const char* key, const char* fallback = "") const {
        char text[256] = {};
        GetPrivateProfileStringA(section, key, fallback, text, sizeof(text), path.c_str());
        return text;
    }
};

// ---------------------------------------------------------------- patching

inline void Write(uintptr_t at, const void* bytes, size_t n) {
    DWORD old;
    VirtualProtect((void*)at, n, PAGE_EXECUTE_READWRITE, &old);
    std::memcpy((void*)at, bytes, n);
    VirtualProtect((void*)at, n, old, &old);
    FlushInstructionCache(GetCurrentProcess(), (void*)at, n);
}

template <typename T> void WriteValue(uintptr_t at, const T& value) {
    Write(at, &value, sizeof(T));
}

inline bool Matches(uintptr_t at, const void* expected, size_t n) {
    return std::memcmp((const void*)at, expected, n) == 0;
}

inline uintptr_t CallTarget(uintptr_t site) {
    return site + 5 + *(const int32_t*)(site + 1);
}

inline bool IsCallTo(uintptr_t site, uintptr_t target) {
    return *(const uint8_t*)site == 0xE8 && CallTarget(site) == target;
}

// Writes a 5-byte CALL (0xE8) or JMP (0xE9) at `site`.
inline void WriteBranch(uintptr_t site, const void* to, uint8_t opcode = 0xE8) {
    uint8_t code[5] = {opcode};
    *(int32_t*)(code + 1) = (int32_t)((uintptr_t)to - (site + 5));
    Write(site, code, sizeof(code));
}

// Replaces the executable's import slot for dll!name with `hook`; returns the previous function, or null.
inline void* HookImport(const char* dll, const char* name, void* hook) {
    const uintptr_t base = (uintptr_t)GetModuleHandleA(nullptr);
    const auto* nt = (const IMAGE_NT_HEADERS*)(base + ((const IMAGE_DOS_HEADER*)base)->e_lfanew);
    const DWORD rva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    const uintptr_t target = (uintptr_t)GetProcAddress(GetModuleHandleA(dll), name);
    for (auto* imp = (const IMAGE_IMPORT_DESCRIPTOR*)(base + rva); rva && imp->Name; imp++) {
        if (_stricmp((const char*)(base + imp->Name), dll)) continue;
        for (auto* thunk = (IMAGE_THUNK_DATA*)(base + imp->FirstThunk); thunk->u1.Function; thunk++) {
            if (thunk->u1.Function != target) continue;
            void* previous = (void*)thunk->u1.Function;
            WriteValue<uint32_t>((uintptr_t)&thunk->u1.Function, (uint32_t)(uintptr_t)hook);
            return previous;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------- [Window]
//
// Monitor     1 = primary, 2.. = the others in enumeration order. The window is centred there whenever its size
//             changes, since the game re-centres it on the primary after every mode change.
// Background  the game never takes the foreground and keeps running without focus: SetForegroundWindow is a
//             no-op, ShowWindow does not activate, and deactivation messages are rewritten before the game's
//             WndProc sees them (the games pause and drop their input devices on WM_ACTIVATE / WM_ACTIVATEAPP).
//             Device creation still activates the window once, so the previous foreground window is restored
//             during the first 30 s.

struct WindowOptions {
    int monitor = 0, width = 0, height = 0;
    bool windowed = false, background = false;

    void Read(const Ini& ini) {
        monitor = ini.Int("Window", "Monitor", 0);
        width = ini.Int("Window", "Width", 0);
        height = ini.Int("Window", "Height", 0);
        windowed = ini.Bool("Window", "Windowed", false);
        background = ini.Bool("Window", "Background", false);
    }

    bool HasSize() const { return width > 0 && height > 0; }
};

namespace detail {

inline WNDPROC& PrevWndProc() {
    static WNDPROC proc = nullptr;
    return proc;
}

inline HWND& PrevForeground() {
    static HWND hwnd = nullptr;
    return hwnd;
}

inline BOOL WINAPI SetForegroundHook(HWND) {
    return TRUE;
}

inline BOOL WINAPI ShowWindowHook(HWND hwnd, int cmd) {
    if (cmd == SW_SHOWNORMAL || cmd == SW_SHOW) cmd = SW_SHOWNOACTIVATE;
    return ShowWindow(hwnd, cmd);
}

inline LRESULT CALLBACK BackgroundWndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_ACTIVATE && LOWORD(w) == WA_INACTIVE) w = MAKEWPARAM(WA_ACTIVE, HIWORD(w));
    if (msg == WM_ACTIVATEAPP && !w) w = TRUE;
    if (msg == WM_KILLFOCUS) return 0;
    return CallWindowProcA(PrevWndProc(), hwnd, msg, w, l);
}

struct MonitorPick {
    int want, seen = 0;
    RECT work{};
    bool found = false;
};

inline BOOL CALLBACK PickMonitor(HMONITOR m, HDC, LPRECT, LPARAM p) {
    auto* pick = (MonitorPick*)p;
    MONITORINFO mi{sizeof(mi)};
    GetMonitorInfoA(m, &mi);
    const int index = (mi.dwFlags & MONITORINFOF_PRIMARY) ? 1 : 2 + pick->seen++;
    if (index == pick->want) {
        pick->work = mi.rcWork;
        pick->found = true;
    }
    return TRUE;
}

inline void Centre(HWND hwnd, int monitor) {
    MonitorPick pick{monitor};
    EnumDisplayMonitors(nullptr, nullptr, PickMonitor, (LPARAM)&pick);
    if (!pick.found) {
        Log("window: no monitor %d", monitor);
        return;
    }
    RECT r;
    GetWindowRect(hwnd, &r);
    const int w = r.right - r.left, h = r.bottom - r.top;
    const int x = pick.work.left + ((pick.work.right - pick.work.left) - w) / 2;
    const int y = pick.work.top + ((pick.work.bottom - pick.work.top) - h) / 2;
    SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    Log("window: %dx%d on monitor %d at %d,%d", w, h, monitor, x, y);
}

} // namespace detail

// Call before the game creates its window.
inline void InstallBackground(const WindowOptions& options) {
    if (!options.background) return;
    detail::PrevForeground() = GetForegroundWindow();
    HookImport("USER32.dll", "SetForegroundWindow", (void*)&detail::SetForegroundHook);
    HookImport("USER32.dll", "ShowWindow", (void*)&detail::ShowWindowHook);
}

// Call periodically with the game's window (null until it exists).
inline void WatchWindow(HWND hwnd, const WindowOptions& options) {
    static HWND subclassed = nullptr;
    static SIZE size{};
    static const DWORD start = GetTickCount();
    if (!hwnd || !IsWindow(hwnd)) return;
    if (options.background && hwnd != subclassed) {
        detail::PrevWndProc() = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)&detail::BackgroundWndProc);
        subclassed = hwnd;
    }
    HWND previous = detail::PrevForeground();
    if (options.background && previous && GetForegroundWindow() == hwnd && GetTickCount() - start < 30000) {
        SetForegroundWindow(previous);
        Log("window: foreground restored");
    }
    if (!options.monitor) return;
    RECT r;
    GetWindowRect(hwnd, &r);
    const SIZE now{r.right - r.left, r.bottom - r.top};
    if (now.cx != size.cx || now.cy != size.cy) {
        size = now;
        detail::Centre(hwnd, options.monitor);
    }
}

} // namespace ft
