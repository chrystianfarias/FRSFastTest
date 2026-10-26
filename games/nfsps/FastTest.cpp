// FRSFastTest for Need for Speed ProStreet (nfs.exe v1.1): boots straight into an event through the developers'
// SkipFE switch.
//
//   0xBFBC08  SkipFE. The boot (0x6D9819) clears it unless 0xBFBC0C, an override never written, is set; both are
//             set before the game runs.
//   0xA9D978  SkipFE event, char*: hashed with Attrib::StringToKey (0x52B8D0) and looked up in the race database
//             [0xAB9D80] by 0x685C10, called at 0x4D599D.
//   0x4D4B41  mov eax, 0x9EE8C4 ("350z_track"): the player's car preset. The global next to the event (0xA9D97C)
//             was folded into this immediate, so the immediate is repointed.
//   0xBFB400  GameFlowManager*; state at +0x24 (6 = racing)
#include "fasttest.h"

namespace {

constexpr uintptr_t kSkipFE = 0xBFBC08, kSkipFEOverride = 0xBFBC0C;
constexpr uintptr_t kSkipFEEvent = 0xA9D978;
constexpr uintptr_t kCarImm = 0x4D4B42;
constexpr uintptr_t kDefaultCar = 0x9EE8C4;
constexpr uintptr_t kCallFindRace = 0x4D599D, kFindRace = 0x685C10;
constexpr uintptr_t kStringToKey = 0x52B8D0;
constexpr uintptr_t kGameFlow = 0xBFB400;

std::string g_event, g_car;

uint32_t AttribKey(const char* s) {
    return ((uint32_t(__cdecl*)(const char*))kStringToKey)(s);
}

void* __fastcall FindRaceHook(void* db, void*, uint32_t key) {
    void* race = ((void*(__thiscall*)(void*, uint32_t))kFindRace)(db, key);
    if (race) ft::Log("event \"%s\" found", g_event.c_str());
    else ft::Log("event \"%s\" not found (see FRSFastTest-Events.txt): using the default", g_event.c_str());
    return race;
}

// ---------------------------------------------------------------- display modes
// With no display modes listed (a Remote Desktop session lists none) the mode sort (0x7098E0, called at 0x70B4A5)
// reads from null, and the mode chosen later (0x701001) is read from an empty list. GetAdapterModeCount and
// EnumAdapterModes (IDirect3D9 slots 6 and 7, called as `mov eax, [ecx+0x18|0x1C]; call eax` at 0x70B3A9 and
// 0x70B3D1) then report a single 1920x1080 mode, and the sort is skipped on an empty list.

constexpr uintptr_t kModeSort = 0x7098E0, kModeSortCall = 0x70B4A5;
constexpr uintptr_t kModeCountCall = 0x70B3A9, kEnumModesCall = 0x70B3D1;
constexpr uint8_t kModeCountCode[] = {0x8B, 0x41, 0x18, 0xFF, 0xD0};
constexpr uint8_t kEnumModesCode[] = {0x8B, 0x41, 0x1C, 0xFF, 0xD0};

struct DisplayMode {
    UINT width, height, refresh;
    int format;
};

bool g_modeFaked = false;

void __cdecl ModeSortGuard(char* begin, char* end, void* pred) {
    if (begin && end > begin) ((void(__cdecl*)(char*, char*, void*))kModeSort)(begin, end, pred);
}

UINT __stdcall ModeCountHook(void** d3d, UINT adapter, int format) {
    const UINT n = ((UINT(__stdcall*)(void*, UINT, int))(*(void***)d3d)[6])(d3d, adapter, format);
    g_modeFaked = n == 0;
    if (g_modeFaked) ft::Log("display: no modes listed, reporting 1920x1080");
    return g_modeFaked ? 1 : n;
}

long __stdcall EnumModesHook(void** d3d, UINT adapter, int format, UINT mode, DisplayMode* out) {
    if (!g_modeFaked)
        return ((long(__stdcall*)(void*, UINT, int, UINT, DisplayMode*))(*(void***)d3d)[7])(d3d, adapter, format, mode, out);
    DisplayMode desktop{};   // GetAdapterDisplayMode, slot 8, for the refresh rate
    const long hr = ((long(__stdcall*)(void*, UINT, DisplayMode*))(*(void***)d3d)[8])(d3d, adapter, &desktop);
    *out = DisplayMode{1920, 1080, hr >= 0 && desktop.refresh ? desktop.refresh : 60, format};
    return 0;
}

// ---------------------------------------------------------------- player vehicle
// GLocalHumanOpponent's vehicle key (vtable 0x983024, slot 11 = 0x660BA0) reads +8 of the player's car record in
// the profile garage (FEPlayerCarDB at [0xAB0EAC]+0x240, searched by 0x5332F0). A SkipFE preset is not in the
// garage and the original dereferences null, so the preset model's key is returned instead.

constexpr uintptr_t kHumanVehicleKeySlot = 0x983050, kHumanVehicleKey = 0x660BA0;
constexpr uintptr_t kProfile = 0xAB0EAC, kFindCarRecord = 0x5332F0;
uint32_t g_vehicleKey = 0;

uint32_t __fastcall HumanVehicleKeyHook(char* self, void*) {
    char* profile = *(char**)kProfile;
    void** handleSource = (void**)(self + 0x80);
    const uint32_t handle = ((uint32_t(__thiscall*)(void*))(*(void***)handleSource)[4])(handleSource);
    const char* record = profile ? ((const char*(__thiscall*)(void*, uint32_t))kFindCarRecord)(profile + 0x240, handle)
                                 : nullptr;
    if (record) return *(const uint32_t*)(record + 8);
    static bool logged = false;
    if (!logged) ft::Log("player car %08X not in the garage: using vehicle key %08X", handle, g_vehicleKey);
    logged = true;
    return g_vehicleKey;
}

// ---------------------------------------------------------------- window
// Resolution: the settings load (0x6FA030, called at 0x70E229) reads g_Width / g_Height through the RegQueryValueExA
// import slot 0x967018. Other mods replace that slot after ASIs load, so it is swapped only for the duration of the
// call, chaining to whatever is there. RegSetValueExA drops writes of those two values so the saved setting is kept.

constexpr uintptr_t kGameWindow = 0xAC6ED8;
constexpr uintptr_t kCallLoadSettings = 0x70E229, kLoadSettings = 0x6FA030, kRegQuerySlot = 0x967018;

using RegQueryFn = LSTATUS(WINAPI*)(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
using RegSetFn = LSTATUS(WINAPI*)(HKEY, LPCSTR, DWORD, DWORD, const BYTE*, DWORD);

ft::WindowOptions g_window;
RegQueryFn g_regQuery = nullptr;
RegSetFn g_regSet = nullptr;

int ResolutionValue(LPCSTR name) {
    if (!name || !g_window.HasSize()) return 0;
    if (!_stricmp(name, "g_Width")) return g_window.width;
    if (!_stricmp(name, "g_Height")) return g_window.height;
    return 0;
}

LSTATUS WINAPI RegQueryHook(HKEY key, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD size) {
    const int v = ResolutionValue(name);
    if (!v) return g_regQuery(key, name, reserved, type, data, size);
    if (type) *type = REG_DWORD;
    if (!data) {
        if (size) *size = 4;
        return ERROR_SUCCESS;
    }
    if (!size || *size < 4) {
        if (size) *size = 4;
        return ERROR_MORE_DATA;
    }
    *(DWORD*)data = (DWORD)v;
    *size = 4;
    return ERROR_SUCCESS;
}

LSTATUS WINAPI RegSetHook(HKEY key, LPCSTR name, DWORD reserved, DWORD type, const BYTE* data, DWORD size) {
    if (ResolutionValue(name)) return ERROR_SUCCESS;
    return g_regSet(key, name, reserved, type, data, size);
}

void __fastcall LoadSettingsHook(void* self, void*, void* arg) {
    void* const slot = *(void**)kRegQuerySlot;
    g_regQuery = (RegQueryFn)slot;
    ft::WriteValue<uint32_t>(kRegQuerySlot, (uint32_t)(uintptr_t)&RegQueryHook);
    ((void(__thiscall*)(void*, void*))kLoadSettings)(self, arg);
    ft::WriteValue<void*>(kRegQuerySlot, slot);
}

void InstallWindow() {
    if (g_window.HasSize()) {
        if (ft::IsCallTo(kCallLoadSettings, kLoadSettings)) ft::WriteBranch(kCallLoadSettings, (void*)&LoadSettingsHook);
        else ft::Log("window: settings load call not found, resolution unchanged");
        g_regSet = (RegSetFn)ft::HookImport("ADVAPI32.dll", "RegSetValueExA", (void*)&RegSetHook);
    }
    ft::InstallBackground(g_window);
    ft::Log("window: monitor %d, %dx%d, background %d", g_window.monitor, g_window.width, g_window.height,
            (int)g_window.background);
}

// ---------------------------------------------------------------- race intro (NIS)
// INIS::mInstance (0xFFD744) exists while a cut-scene plays; state at +0x2E4 (> 2 while playing), skip flag at
// +0x3E8. The singleton is reused across cut-scenes, so every one is skipped as it starts.
// Its last part (state 6, the cars rolling to the line) ignores the flag. It is skipped the way the player's
// button does it: in the NIS input handler (0x4DA260), on the game's thread, call vtable +0x2C (0x4DA630).

constexpr uintptr_t kNisInstance = 0xFFD744;
constexpr size_t kNisState = 0x2E4, kNisSkip = 0x3E8, kNisNotSkippable = 0x400;
constexpr uintptr_t kNisInput = 0x4DA260;
constexpr uint8_t kNisInputHead[] = {0x53, 0x8B, 0x5C, 0x24, 0x08};   // push ebx; mov ebx, [esp+8]
bool g_skipNis = false;

void SkipNis() {
    char* nis = *(char* volatile*)kNisInstance;
    if (!nis) return;
    const int state = *(volatile int*)(nis + kNisState);
    if (state <= 2 || *(volatile uint8_t*)(nis + kNisSkip)) return;
    *(volatile uint8_t*)(nis + kNisSkip) = 1;
    ft::Log("NIS skipped (state %d)", state);
}

void __cdecl NisInput() {
    char* nis = *(char* volatile*)kNisInstance;
    if (!g_skipNis || !nis || *(int*)(nis + kNisState) != 6) return;
    using SkipFn = bool(__thiscall*)(void*);
    *(uint8_t*)(nis + kNisNotSkippable) = 0;
    const bool done = (*(SkipFn*)(*(char**)nis + 0x2C))(nis);
    ft::Log("NIS: intro skipped (%d)", (int)done);
}

__declspec(naked) void NisInputStub() {
    __asm {
        pushad
        call NisInput
        popad
        push ebx
        mov ebx, [esp + 8]
        push 0x4DA265   // kNisInput + 5, past the two replaced instructions
        ret
    }
}

// ---------------------------------------------------------------- pre-race matchup
// FEPreRaceMatchupStateManager's state start (vtable 0x977364, slot 11 = 0x5C4B40) shows FeEventMatchup.fng when
// a race is current; otherwise it moves to its next state (0x5C4C03). The race test at 0x5C4B66 jumps there.

constexpr uintptr_t kMatchupRaceTest = 0x5C4B66, kMatchupNoRace = 0x5C4C03;
constexpr uint8_t kMatchupTest[] = {0xA1, 0x7C, 0x9D, 0xAB, 0x00};   // mov eax, [0xAB9D7C]

// ----------------------------------------------------------------

DWORD WINAPI Watch(LPVOID) {
    int last = -1;
    for (;;) {
        const char* flowManager = *(char* volatile*)kGameFlow;
        const int flow = flowManager ? *(volatile int*)(flowManager + 0x24) : -1;
        if (flow != last) ft::Log("game flow %d", flow);
        last = flow;
        if (g_skipNis) SkipNis();
        ft::WatchWindow(*(HWND volatile*)kGameWindow, g_window);
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
    static const uint8_t kOverride[] = {0x39, 0x1D, 0x0C, 0xBC, 0xBF, 0x00};   // cmp [0xBFBC0C], ebx
    static const uint8_t kCar[] = {0xB8, 0xC4, 0xE8, 0x9E, 0x00};              // mov eax, 0x9EE8C4
    if (!ft::Matches(0x6D9819, kOverride, sizeof(kOverride)) || !ft::Matches(0x4D4B41, kCar, sizeof(kCar)) ||
        !ft::IsCallTo(kCallFindRace, kFindRace)) {
        ft::Log("not nfs.exe v1.1, or the code was patched by another mod: nothing done");
        return;
    }
    g_event = ini.String("FastTest", "Event");
    g_car = ini.String("FastTest", "Car");

    ft::WriteValue<int>(kSkipFE, 1);
    ft::WriteValue<int>(kSkipFEOverride, 1);
    if (!g_event.empty()) ft::WriteValue<const char*>(kSkipFEEvent, g_event.c_str());
    if (!g_car.empty()) ft::WriteValue<uint32_t>(kCarImm, (uint32_t)(uintptr_t)g_car.c_str());
    ft::WriteBranch(kCallFindRace, (void*)&FindRaceHook);

    g_skipNis = ini.Bool("FastTest", "SkipIntroNIS", true);
    if (g_skipNis && ft::Matches(kNisInput, kNisInputHead, sizeof(kNisInputHead)))
        ft::WriteBranch(kNisInput, (void*)&NisInputStub, 0xE9);
    if (ini.Bool("FastTest", "SkipMatchup", true) && ft::Matches(kMatchupRaceTest, kMatchupTest, sizeof(kMatchupTest)))
        ft::WriteBranch(kMatchupRaceTest, (void*)kMatchupNoRace, 0xE9);

    if (*(const uint32_t*)kHumanVehicleKeySlot == kHumanVehicleKey) {
        // the preset's model is its name up to the first '_' (bmwm3e92_track -> bmwm3e92)
        std::string model = g_car.empty() ? (const char*)kDefaultCar : g_car;
        model = model.substr(0, model.find('_'));
        g_vehicleKey = AttribKey(model.c_str());
        ft::WriteValue<uint32_t>(kHumanVehicleKeySlot, (uint32_t)(uintptr_t)&HumanVehicleKeyHook);
    }
    ft::Log("SkipFE on: event \"%s\", car \"%s\"", g_event.c_str(), g_car.empty() ? (const char*)kDefaultCar : g_car.c_str());

    if (ft::Matches(kModeCountCall, kModeCountCode, 5)) ft::WriteBranch(kModeCountCall, (void*)&ModeCountHook);
    if (ft::Matches(kEnumModesCall, kEnumModesCode, 5)) ft::WriteBranch(kEnumModesCall, (void*)&EnumModesHook);
    if (ft::IsCallTo(kModeSortCall, kModeSort)) ft::WriteBranch(kModeSortCall, (void*)&ModeSortGuard);

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
