// FRSFastTest for Need for Speed Carbon (NFSC.exe v1.4): boots straight into a race through the developers'
// SkipFE switch.
//
// The option parser is gone from the retail build; the globals remain:
//   0xA9E620  SkipFE. The boot (0x6B8EDB) then calls 0x7A9B90, which copies the globals below into the race
//             options (0xA7A1A8..), instead of starting the front end (0x6A8B70 on 0xA99B9C).
//   0xA63144  player car, char* ("viper")
//   0xA6313C  track number (5000; 0x6BF060 turns -1 into 2000)
//   0xA99BBC  game flow state
//
// Window: 0xAB0AD4 is the game's windowed flag, 0xAB0AD8 its HWND. 0x712AC0 ((int* w, int* h), as in Most Wanted)
// returns the size of display mode [0xA65388]; its five call sites are redirected to return [Window] Width x Height.
#include "fasttest.h"

namespace {

constexpr uintptr_t kSkipFE = 0xA9E620;
constexpr uintptr_t kSkipFECar = 0xA63144, kSkipFETrack = 0xA6313C;
constexpr uintptr_t kGameFlowState = 0xA99BBC;

constexpr uintptr_t kWindowed = 0xAB0AD4, kGameWindow = 0xAB0AD8;
constexpr uintptr_t kGetResolution = 0x712AC0;
constexpr uintptr_t kGetResolutionCalls[] = {0x72B411, 0x72B486, 0x72B5C1, 0x730995, 0x731211};

// mov eax, [0xA9E620]; test eax, eax; je +7; call ... in the boot
constexpr uint8_t kBootSkipFE[] = {0xA1, 0x20, 0xE6, 0xA9, 0x00, 0x85, 0xC0, 0x74, 0x07, 0xE8};
constexpr uintptr_t kBootSkipFEAt = 0x6B8EDB;

ft::WindowOptions g_window;
std::string g_car;

void __fastcall GetResolutionHook(void* self, void*, int* w, int* h) {
    ((void(__thiscall*)(void*, int*, int*))kGetResolution)(self, w, h);
    *w = g_window.width;
    *h = g_window.height;
}

// A failed IDirect3D9::CreateDevice (slot 16) is logged with its present parameters: the game falls back to mode 0
// and then dereferences the null device (0x730B1A).
using CreateDeviceFn = long(__stdcall*)(void*, UINT, int, HWND, DWORD, DWORD*, void**);
CreateDeviceFn g_createDevice = nullptr;

long __stdcall CreateDeviceHook(void* d3d, UINT adapter, int type, HWND hwnd, DWORD flags, DWORD* pp, void** out) {
    const long hr = g_createDevice(d3d, adapter, type, hwnd, flags, pp, out);
    if (hr < 0)
        ft::Log("CreateDevice %08X: %ux%u format %u, multisample %u/%u, swap %u, windowed %u, refresh %u, flags %X",
                (unsigned)hr, pp[0], pp[1], pp[2], pp[4], pp[5], pp[6], pp[8], pp[12], flags);
    return hr;
}

using Direct3DCreate9Fn = void*(WINAPI*)(UINT);
Direct3DCreate9Fn g_direct3DCreate9 = nullptr;

void* WINAPI Direct3DCreate9Hook(UINT version) {
    void* d3d = g_direct3DCreate9(version);
    void** slot = d3d ? &(*(void***)d3d)[16] : nullptr;
    if (slot && *slot != (void*)&CreateDeviceHook) {
        g_createDevice = (CreateDeviceFn)*slot;
        ft::WriteValue<void*>((uintptr_t)slot, (void*)&CreateDeviceHook);
    }
    return d3d;
}

void InstallWindow() {
    g_direct3DCreate9 = (Direct3DCreate9Fn)ft::HookImport("d3d9.dll", "Direct3DCreate9", (void*)&Direct3DCreate9Hook);
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
    bool checkedCalls = false;
    for (;;) {
        const int flow = *(volatile int*)kGameFlowState;
        if (flow != last) ft::Log("game flow %d", flow);
        last = flow;
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
    if (!ini.Bool("FastTest", "Enabled", true)) {
        ft::Log("disabled");
        return;
    }
    if (!ft::Matches(kBootSkipFEAt, kBootSkipFE, sizeof(kBootSkipFE))) {
        ft::Log("not NFSC.exe v1.4: nothing done");
        return;
    }
    ft::WriteValue<int>(kSkipFE, 1);

    g_car = ini.String("FastTest", "Car");
    if (!g_car.empty()) ft::WriteValue<const char*>(kSkipFECar, g_car.c_str());
    if (const int track = ini.Int("FastTest", "Track", 0)) ft::WriteValue<int>(kSkipFETrack, track);

    ft::Log("SkipFE on: car \"%s\", track %d", *(const char**)kSkipFECar, *(int*)kSkipFETrack);
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
