// FRSFastTest for Need for Speed Underground (speed.exe v1.4): boots straight into a race through the developers'
// SkipFE switch.
//
// main (0x447309) reads [0x73465C]: when set it calls 0x4B6390 instead of scheduling the front-end load (0x448B10).
// The retail build never sets it (the only store, 0x5687F3, restores it from a replay file), but 0x4B6390 still
// fills the race parameters (0x78A2E0..) from the SkipFE globals beside it. They are written from the call to
// 0x4B6390 at 0x447315, after the boot has set its own (0x447040 sets the track from a missing Config.bin: -1):
//   0x70095C  track number (-1 = the first track of the track list). 0x4484F0 looks it up in the track list,
//             [0x736160] (count [0x736164], 0xFC bytes each, number at +0x6C); a number not in it crashes the load
//             (0x449723), so the plugin checks it first. The race is always a lap race: drag and drift
//             gameplay (a DragDriver, 0x45D660, picked by 0x425FE0) comes from the front end's race type [0x777CB8],
//             which this path never sets up.
//   0x6F1D4C  laps (2)
//   0x734660  AI opponents (0)
//   0x758C18  the player's car: bStringHash("CUSTOMIZABLE_<car>") (0x4AC460 sets the first garage car at boot).
//             The garage, 0x758C18 + 0x2038: 0x348-byte slots whose id is at +0x344, count at 0x758C18 + 0x61D8.
//   0x77A910  game flow state (3 = front end, 6 = racing)
//
// Window: the window itself comes from NFSUnderground.WidescreenFix (WindowedMode). The game's own windowed flag
// [0x736374] (copied into D3DPRESENT_PARAMETERS.Windowed at 0x4086F0) is not used: with it CreateDevice fails and
// the game crashes on the null device (0x4082C7). 0x736378 is the HWND, 0x71A98C the Windowed field the device was
// made with. 0x4086B0 (cdecl(width), esi = height) applies a display mode; its first five bytes jump to a stub that
// substitutes [Window] Width x Height. A window is resized to that size and moved to [Window] Monitor.
#include "fasttest.h"

namespace {

constexpr uintptr_t kSkipFE = 0x73465C;
constexpr uintptr_t kSkipFETrack = 0x70095C, kSkipFELaps = 0x6F1D4C, kSkipFEAICars = 0x734660;
constexpr uintptr_t kSkipFEStart = 0x4B6390, kSkipFEStartCall = 0x447315;
constexpr uintptr_t kTrackList = 0x736160, kTrackCount = 0x736164;
constexpr size_t kTrackSize = 0xFC, kTrackNumber = 0x6C;
constexpr uintptr_t kPlayerCar = 0x758C18, kGarageSlots = kPlayerCar + 0x2038, kGarageCount = kPlayerCar + 0x61D8;
constexpr size_t kGarageSlotSize = 0x348, kGarageSlotId = 0x344;
constexpr uintptr_t kGameFlowState = 0x77A910;
constexpr int kFlowRacing = 6;

constexpr uintptr_t kGameWindow = 0x736378, kPresentWindowed = 0x71A98C;
constexpr uintptr_t kSetMode = 0x4086B0, kSetModeResume = 0x4086B5;
constexpr uint8_t kSetModeStart[] = {0x53, 0x8B, 0x5C, 0x24, 0x08};   // push ebx; mov ebx, [esp+8]

// main: mov eax, [0x73465C]; add esp, 8; test eax, eax
constexpr uint8_t kMainSkipFE[] = {0xA1, 0x5C, 0x46, 0x73, 0x00, 0x83, 0xC4, 0x08, 0x85, 0xC0};
constexpr uintptr_t kMainSkipFEAt = 0x447309;

ft::WindowOptions g_window;
int g_width = 0, g_height = 0;
std::string g_car;
int g_track = 0, g_laps = 0, g_aiCars = -1;

uint32_t StringHash(const char* s) {
    uint32_t h = 0xFFFFFFFF;
    while (*s) h = h * 33 + (uint8_t)*s++;
    return h;
}

// 0x4B6390 with the [FastTest] settings in place of the boot's
void __cdecl StartSkipFE() {
    if (g_track > 0) {
        const uint8_t* tracks = *(const uint8_t**)kTrackList;
        const int count = *(int*)kTrackCount;
        bool found = false;
        for (int i = 0; i < count && !found; i++)
            found = *(const int16_t*)(tracks + i * kTrackSize + kTrackNumber) == g_track;
        if (found) *(int*)kSkipFETrack = g_track;
        else ft::Log("track %d is not in the track list (%d tracks): the default track is used", g_track, count);
    }
    if (g_laps > 0) *(int*)kSkipFELaps = g_laps;
    if (g_aiCars >= 0) *(int*)kSkipFEAICars = g_aiCars;
    if (!g_car.empty()) {
        const uint32_t id = StringHash(("CUSTOMIZABLE_" + g_car).c_str());
        const int count = *(int*)kGarageCount;
        bool found = false;
        for (int i = 0; i < count && !found; i++)
            found = *(uint32_t*)(kGarageSlots + i * kGarageSlotSize + kGarageSlotId) == id;
        if (found) {
            *(uint32_t*)kPlayerCar = id;
            ft::Log("car %s", g_car.c_str());
        } else {
            ft::Log("car %s is not in the garage (%d cars): the default car is used", g_car.c_str(), count);
        }
    }
    ft::Log("race: track %d, laps %d, AI cars %d", *(int*)kSkipFETrack, *(int*)kSkipFELaps, *(int*)kSkipFEAICars);
    ((void(__cdecl*)())kSkipFEStart)();
}

// 0x4086B0 with [Window] Width x Height in place of the mode's
__declspec(naked) void SetModeHook() {
    __asm {
        mov esi, g_height
        mov eax, g_width
        mov [esp + 4], eax
        push ebx
        mov ebx, [esp + 8]
        mov eax, kSetModeResume
        jmp eax
    }
}

void InstallWindow() {
    if (g_window.HasSize()) {
        if (ft::Matches(kSetMode, kSetModeStart, sizeof(kSetModeStart))) {
            g_width = g_window.width;
            g_height = g_window.height;
            ft::WriteBranch(kSetMode, (void*)&SetModeHook, 0xE9);
        } else {
            ft::Log("window: 0x4086B0 was changed by another mod: resolution left alone");
        }
    }
    ft::InstallBackground(g_window);
    ft::Log("window: monitor %d, %dx%d, background %d", g_window.monitor,
            g_window.width, g_window.height, (int)g_window.background);
}

// A window (not fullscreen) gets the client size it renders at.
void SizeWindow(HWND hwnd) {
    if (!hwnd || !g_window.HasSize()) return;
    RECT r;
    GetClientRect(hwnd, &r);
    if (r.right - r.left == g_window.width && r.bottom - r.top == g_window.height) return;
    RECT want{0, 0, g_window.width, g_window.height};
    AdjustWindowRectEx(&want, (DWORD)GetWindowLongA(hwnd, GWL_STYLE), FALSE, (DWORD)GetWindowLongA(hwnd, GWL_EXSTYLE));
    SetWindowPos(hwnd, nullptr, 0, 0, want.right - want.left, want.bottom - want.top,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    ft::Log("window: resized to %dx%d", g_window.width, g_window.height);
}

DWORD WINAPI Watch(LPVOID) {
    int last = -1;
    for (;;) {
        const int flow = *(volatile int*)kGameFlowState;
        if (flow != last) ft::Log("game flow %d", flow);
        last = flow;
        HWND hwnd = *(HWND volatile*)kGameWindow;
        // a fullscreen device is neither resized nor moved
        ft::WindowOptions now = g_window;
        now.windowed = *(volatile int*)kPresentWindowed != 0;
        if (!now.windowed) now.monitor = 0;
        if (now.windowed) SizeWindow(hwnd);
        ft::WatchWindow(hwnd, now);
        Sleep(50);
    }
}

void Start() {
    ft::OpenLog();
    const ft::Ini ini{ft::BesidePlugin(".ini")};
    if (!ini.Bool("FastTest", "Enabled", true)) {
        ft::Log("disabled");
        return;
    }
    if (!ft::Matches(kMainSkipFEAt, kMainSkipFE, sizeof(kMainSkipFE))) {
        ft::Log("not speed.exe v1.4: nothing done");
        return;
    }
    ft::WriteValue<int>(kSkipFE, 1);
    g_track = ini.Int("FastTest", "Track", 0);
    g_laps = ini.Int("FastTest", "Laps", 0);
    g_aiCars = ini.Int("FastTest", "AICars", -1);
    g_car = ini.String("FastTest", "Car");
    for (char& c : g_car) c = (char)toupper((uint8_t)c);
    if (ft::IsCallTo(kSkipFEStartCall, kSkipFEStart)) ft::WriteBranch(kSkipFEStartCall, (void*)&StartSkipFE);
    else ft::Log("the call to 0x4B6390 was changed by another mod: the game's defaults are used");
    ft::Log("SkipFE on: track %d, laps %d, AI cars %d, car %s", g_track, g_laps, g_aiCars,
            g_car.empty() ? "(default)" : g_car.c_str());

    g_window.Read(ini);
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
