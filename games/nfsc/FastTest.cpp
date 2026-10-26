// FRSFastTest for Need for Speed Carbon (NFSC.exe v1.4): boots straight into a race through the developers'
// SkipFE switch.
//
// The option parser is gone from the retail build; the globals remain, laid out as in Most Wanted (0x8F86A4..):
//   0xA9E620  SkipFE. The boot (0x6B8EDB) then calls 0x7A9B90, which copies the globals below into the race
//             options (0xA7A1A8..), instead of starting the front end (0x6A8B70 on 0xA99B9C).
//   0xA63140  event, char* (""). Without one the session is free roam. With one, 0x75E3B0 (called once the world
//             is loaded) hashes it (0x4609B0) and looks it up by EventID in the race database [0xA9828C]
//             (0x61BEF0, called at 0x75E3FA), then builds the race from it with the laps and AI cars below.
//   0xA63144  player car, char* ("viper"); hashed with Attrib::StringToKey (0x4639D0) at 0x767E44
//   0xA63148  opponent car, char* ("viper"); read at 0x76816E and as the default vehicle at 0x7CA6E7
//   0xA63154  laps (2) -> [0xA7A1C8]
//   0xA9E630  AI cars (0) -> [0xA7A1D0]; racers = AI + 1 -> [0xA7A1DC]
//   0xA6313C  track number (5000). TrackInfo (chunk 0x34201 in GLOBALB.BUN, 0x110-byte records, number at +0x8A)
//             lists 2000, 5000..5900, but only 5000 (L5RA, the world) and 5200 (L5RB, the front end) ship.
//   0xA99BBC  game flow state
//
// SMS: a SkipFE session starts in free roam, and the game pops a text message (FeSmsMessage.fng) that holds the
// screen until it is dismissed. Messages are shown by the EShowSMS event, processed in its destructor (0x672B20):
// it reads value 0xBC4 (0x4A0490 on 0xA8AD30), stores the message in [0xA5EC6C] and switches to FESmsStateManager
// (0x5B02B0). SkipSMS turns its "nothing to show" branch at 0x672B71 into a jump.
//
// Window: 0xAB0AD4 is the game's windowed flag, 0xAB0AD8 its HWND. 0x712AC0 ((int* w, int* h), as in Most Wanted)
// returns the size of display mode [0xA65388]; its five call sites are redirected to return [Window] Width x Height.
#include "fasttest.h"

namespace {

constexpr uintptr_t kSkipFE = 0xA9E620;
constexpr uintptr_t kSkipFEEvent = 0xA63140, kSkipFECar = 0xA63144, kSkipFEOpponentCar = 0xA63148;
constexpr uintptr_t kFindRace = 0x61BEF0, kCallFindRace = 0x75E3FA;
constexpr uintptr_t kSkipFELaps = 0xA63154, kSkipFEAICars = 0xA9E630;
constexpr uintptr_t kGameFlowState = 0xA99BBC;

constexpr uintptr_t kWindowed = 0xAB0AD4, kGameWindow = 0xAB0AD8;
constexpr uintptr_t kGetResolution = 0x712AC0;
constexpr uintptr_t kGetResolutionCalls[] = {0x72B411, 0x72B486, 0x72B5C1, 0x730995, 0x731211};

constexpr uintptr_t kShowSmsTest = 0x672B6F;
constexpr uint8_t kShowSmsTestCode[] = {0x84, 0xC0, 0x74, 0x45};   // test al, al; je 0x672BB8

// mov eax, [0xA9E620]; test eax, eax; je +7; call ... in the boot
constexpr uint8_t kBootSkipFE[] = {0xA1, 0x20, 0xE6, 0xA9, 0x00, 0x85, 0xC0, 0x74, 0x07, 0xE8};
constexpr uintptr_t kBootSkipFEAt = 0x6B8EDB;

ft::WindowOptions g_window;
std::string g_event, g_car, g_opponentCar;

void* __fastcall FindRaceHook(void* db, void*, uint32_t key) {
    void* race = ((void*(__thiscall*)(void*, uint32_t))kFindRace)(db, key);
    if (race) {
        ft::Log("event \"%s\" found", g_event.c_str());
        return race;
    }
    ft::Log("event \"%s\" not found: free roam. Events:", g_event.c_str());
    // The database (0x61BEF0) holds [db] races at [db+0xC], 0x14 bytes each, then [db+4] more by pointer at db+0x10;
    // a race's attributes are at +8, its EventID read with Attrib::Instance::GetAttributePointer (0x463A80).
    const uint32_t* d = (const uint32_t*)db;
    for (uint32_t i = 0; i < d[0] + d[1]; i++) {
        const uintptr_t entry = i < d[0] ? d[3] + i * 0x14 : d[4 + i - d[0]];
        ((void(__thiscall*)(uintptr_t))0x63C3C0)(entry);
        auto id = ((const char** (__thiscall*)(uintptr_t, uint32_t, int))0x463A80)(*(uintptr_t*)(entry + 8), 0xA78403EC, 0);
        if (id && *id) ft::Log("  %s", *id);
    }
    return race;
}

// A car name becomes an attribute key without any check, so it is only accepted when CARS\<name> exists.
bool CarExists(const std::string& name) {
    char exe[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exe, MAX_PATH);
    std::string path = exe;
    path = path.substr(0, path.find_last_of("\\/")) + "\\CARS\\" + name;
    const DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

void SetCar(uintptr_t global, std::string& storage, const ft::Ini& ini, const char* key) {
    storage = ini.String("FastTest", key);
    if (storage.empty()) return;
    if (!CarExists(storage)) {
        ft::Log("%s \"%s\" is not in CARS: using \"%s\"", key, storage.c_str(), *(const char**)global);
        storage.clear();
        return;
    }
    ft::WriteValue<const char*>(global, storage.c_str());
}

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

    g_event = ini.String("FastTest", "Event");
    if (!g_event.empty()) {
        ft::WriteValue<const char*>(kSkipFEEvent, g_event.c_str());
        if (ft::IsCallTo(kCallFindRace, kFindRace)) ft::WriteBranch(kCallFindRace, (void*)&FindRaceHook);
    }
    SetCar(kSkipFECar, g_car, ini, "Car");
    SetCar(kSkipFEOpponentCar, g_opponentCar, ini, "OpponentCar");
    if (const int laps = ini.Int("FastTest", "Laps", 0); laps > 0) ft::WriteValue<int>(kSkipFELaps, laps);
    if (const int aiCars = ini.Int("FastTest", "AICars", -1); aiCars >= 0) ft::WriteValue<int>(kSkipFEAICars, aiCars);

    if (ini.Bool("FastTest", "SkipSMS", true)) {
        if (ft::Matches(kShowSmsTest, kShowSmsTestCode, sizeof(kShowSmsTestCode))) ft::WriteValue<uint8_t>(kShowSmsTest + 2, 0xEB);
        else ft::Log("SMS: the EShowSMS code is not the expected one, messages are shown");
    }

    ft::Log("SkipFE on: event \"%s\", car \"%s\", opponents %d in \"%s\", laps %d", g_event.c_str(),
            *(const char**)kSkipFECar, *(int*)kSkipFEAICars, *(const char**)kSkipFEOpponentCar, *(int*)kSkipFELaps);
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
