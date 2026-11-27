// FRSFastTest for Need for Speed Most Wanted (speed.exe v1.3): boots straight into a race through the
// developers' SkipFE switch, or into career free roam from an existing save.
//
// The option parser (0x78A900, "SKIPFE", "SKIPFEPLAYERCAR", ...) is never fed in the retail build, but the globals
// it writes are still read:
//   0x926064  SkipFE. The boot state (0x666590) then calls 0x57ECD0, which copies the globals below into the race
//             options (0x904FB8..), instead of starting the front end.
//   0x8F86A8  player car, char* ("bmwm3gtre46")
//   0x8F86A4  track number (2000)
//   0x8F86BC  laps (2)
//   0x926080  AI cars
//   0x90D5FA  AICONTROL toggle: one-shot byte read and cleared by the player update (0x42F140)
//   0x925E90  game flow state (6 = racing)
//
// Window: 0x982BF0 is the game's windowed flag, 0x982BF4 its HWND. 0x6C27D0 (thiscall, (int* w, int* h)) returns
// the size of display mode [0x90181C]; its five call sites are redirected to return [Window] Width x Height.
#include "fasttest.h"

namespace {

constexpr uintptr_t kSkipFE = 0x926064;
constexpr uintptr_t kSkipFECar = 0x8F86A8, kSkipFETrack = 0x8F86A4, kSkipFELaps = 0x8F86BC;
constexpr uintptr_t kSkipFEAICars = 0x926080;
constexpr uintptr_t kAIControlToggle = 0x90D5FA;
constexpr uintptr_t kGameFlowState = 0x925E90;
constexpr int kFlowRacing = 6;

constexpr uintptr_t kWindowed = 0x982BF0, kGameWindow = 0x982BF4;
constexpr uintptr_t kGetResolution = 0x6C27D0;
constexpr uintptr_t kGetResolutionCalls[] = {0x6DB1A9, 0x6DB216, 0x6DB2E1, 0x6E6BB5, 0x6E729C};

// mov [0x926064], eax in the option parser
constexpr uint8_t kParserSkipFE[] = {0xA3, 0x64, 0x60, 0x92, 0x00};
constexpr uintptr_t kParserSkipFEAt = 0x78A928;

ft::WindowOptions g_window;
std::string g_car;
bool g_aiControl = false;

constexpr int kFlowFrontEnd = 3;
constexpr UINT kCareerMessage = WM_APP + 0x53;
WNDPROC g_previousWndProc = nullptr;
volatile LONG g_careerStarted = 0;

HWND FocusGameWindow() {
    HWND game = nullptr;
    EnumWindows([](HWND hwnd, LPARAM out) -> BOOL {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid != GetCurrentProcessId()) return TRUE;
        char name[64]{};
        GetClassNameA(hwnd, name, sizeof name);
        if (std::strcmp(name, "GameFrame") != 0) return TRUE;
        ShowWindow(hwnd, SW_RESTORE);
        BringWindowToTop(hwnd);
        const DWORD thread = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
        const DWORD self = GetCurrentThreadId();
        const bool attached = thread && thread != self && AttachThreadInput(self, thread, TRUE);
        SetForegroundWindow(hwnd);
        if (attached) AttachThreadInput(self, thread, FALSE);
        *reinterpret_cast<HWND*>(out) = hwnd;
        return FALSE;
    }, reinterpret_cast<LPARAM>(&game));
    return game;
}

// The native career handoff has to run on the game's window thread.
LRESULT CALLBACK CareerWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == kCareerMessage) {
        reinterpret_cast<void(*)()>(0x56C530)();
        InterlockedExchange(&g_careerStarted, 1);
        return 0;
    }
    return CallWindowProcW(g_previousWndProc, hwnd, message, wParam, lParam);
}

DWORD WINAPI EnterCareer(LPVOID) {
    HWND window = nullptr;
    const DWORD started = GetTickCount();
    DWORD frontEndSince = 0, lastFocus = started;
    bool ready = false;
    while (GetTickCount() - started < 180000) {
        const int flow = *reinterpret_cast<const volatile int*>(kGameFlowState);
        if (flow == kFlowRacing) return 0;
        const DWORD now = GetTickCount();
        if (!window || now - lastFocus >= 1000) {
            window = FocusGameWindow();
            lastFocus = now;
        }
        if (flow != kFlowFrontEnd) frontEndSince = 0;
        else if (!frontEndSince) frontEndSince = now;
        else if (window && now - frontEndSince >= 5500) {
            ready = true;
            break;
        }
        Sleep(100);
    }
    if (!ready) {
        ft::Log("career: front end did not become ready");
        return 0;
    }
    g_previousWndProc = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window, GWLP_WNDPROC));
    if (!g_previousWndProc ||
        !SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&CareerWindowProc))) {
        ft::Log("career: could not reach the game window thread");
        return 0;
    }

    DWORD_PTR reply = 0;
    const bool sent = SendMessageTimeoutW(window, kCareerMessage, 0, 0,
                                          SMTO_ABORTIFHUNG | SMTO_BLOCK, 10000, &reply) != 0;
    if (GetWindowLongPtrW(window, GWLP_WNDPROC) == reinterpret_cast<LONG_PTR>(&CareerWindowProc))
        SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_previousWndProc));
    if (!sent || !InterlockedCompareExchange(&g_careerStarted, 0, 0)) {
        ft::Log("career: game did not enter the career");
        return 0;
    }

    const DWORD loading = GetTickCount();
    lastFocus = loading;
    while (GetTickCount() - loading < 120000) {
        if (*reinterpret_cast<const volatile int*>(kGameFlowState) == kFlowRacing) {
            ft::Log("career: free roam ready");
            return 0;
        }
        if (GetTickCount() - lastFocus >= 1000) {
            FocusGameWindow();
            lastFocus = GetTickCount();
        }
        Sleep(100);
    }
    ft::Log("career: free roam load timed out");
    return 0;
}

bool EditorCareerLaunch() {
    char value[2]{};
    return GetEnvironmentVariableA("FRS_MW_FASTTEST_CAREER", value, sizeof value) == 1 && value[0] == '1';
}

bool SupportedGame() {
    unsigned char bytes[sizeof kParserSkipFE]{};
    SIZE_T read = 0;
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(kParserSkipFEAt),
                             bytes, sizeof bytes, &read) && read == sizeof bytes &&
           std::memcmp(bytes, kParserSkipFE, sizeof bytes) == 0;
}

void __fastcall GetResolutionHook(void* self, void*, int* w, int* h) {
    ((void(__thiscall*)(void*, int*, int*))kGetResolution)(self, w, h);
    *w = g_window.width;
    *h = g_window.height;
}

void InstallWindow() {
    if (g_window.windowed) ft::WriteValue<int>(kWindowed, 1);
    if (g_window.HasSize()) {
        int n = 0;
        for (uintptr_t site : kGetResolutionCalls) {
            if (!ft::IsCallTo(site, kGetResolution)) continue;
            ft::WriteBranch(site, (void*)&GetResolutionHook);
            n++;
        }
        ft::Log("window: resolution redirected at %d of 5 call sites", n);
    }
    ft::InstallBackground(g_window);
    ft::Log("window: windowed %d, monitor %d, %dx%d, background %d", (int)g_window.windowed, g_window.monitor,
            g_window.width, g_window.height, (int)g_window.background);
}

DWORD WINAPI Watch(LPVOID) {
    int last = -1;
    DWORD racingSince = 0;
    bool checkedCalls = false;
    for (;;) {
        const int flow = *(volatile int*)kGameFlowState;
        if (flow != last) ft::Log("game flow %d", flow);
        last = flow;
        if (flow == kFlowRacing && !racingSince) racingSince = GetTickCount();
        if (g_aiControl && racingSince && GetTickCount() - racingSince > 1000) {
            *(volatile uint8_t*)kAIControlToggle = 1;
            g_aiControl = false;
            ft::Log("AI control on");
        }
        HWND hwnd = *(HWND volatile*)kGameWindow;
        // ASIs load in name order: a later one may have taken the same call sites
        if (!checkedCalls && hwnd && g_window.HasSize()) {
            checkedCalls = true;
            for (uintptr_t site : kGetResolutionCalls)
                if (!ft::IsCallTo(site, (uintptr_t)&GetResolutionHook))
                    ft::Log("window: call site %08X was taken over by another mod", site);
        }
        ft::WatchWindow(hwnd, g_window);
        Sleep(50);
    }
}

void Start() {
    ft::OpenLog();
    const ft::Ini ini{ft::BesidePlugin(".ini")};
    const bool editorCareer = EditorCareerLaunch();
    if (!editorCareer && !ini.Bool("FastTest", "Enabled", true)) {
        ft::Log("disabled");
        return;
    }
    if (!SupportedGame()) {
        ft::Log("not speed.exe v1.3: nothing done");
        return;
    }
    const auto mode = ini.String("FastTest", "Mode", "race");
    if (editorCareer || _stricmp(mode.c_str(), "career") == 0) {
        if (HANDLE thread = CreateThread(nullptr, 0, EnterCareer, nullptr, 0, nullptr)) CloseHandle(thread);
        return;
    }
    if (_stricmp(mode.c_str(), "race") != 0) {
        ft::Log("unknown mode: %s", mode.c_str());
        return;
    }
    ft::WriteValue<int>(kSkipFE, 1);

    g_car = ini.String("FastTest", "Car");
    if (!g_car.empty()) ft::WriteValue<const char*>(kSkipFECar, g_car.c_str());
    if (const int track = ini.Int("FastTest", "Track", 0)) ft::WriteValue<int>(kSkipFETrack, track);
    if (const int laps = ini.Int("FastTest", "Laps", 0)) ft::WriteValue<int>(kSkipFELaps, laps);
    const int aiCars = ini.Int("FastTest", "AICars", -1);
    if (aiCars >= 0) ft::WriteValue<int>(kSkipFEAICars, aiCars);
    g_aiControl = ini.Bool("FastTest", "AIControl", false);

    ft::Log("SkipFE on: car \"%s\", track %d, laps %d, AI cars %d", *(const char**)kSkipFECar, *(int*)kSkipFETrack,
            *(int*)kSkipFELaps, *(int*)kSkipFEAICars);
    g_window.Read(ini);
    if (!g_window.windowed) g_window.monitor = 0;   // a fullscreen window is not moved
    InstallWindow();
    if (HANDLE thread = CreateThread(nullptr, 0, Watch, nullptr, 0, nullptr)) CloseHandle(thread);
}

} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        ft::Module() = module;
        DisableThreadLibraryCalls(module);
        Start();
    }
    return TRUE;
}
