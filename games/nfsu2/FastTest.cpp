// FRSFastTest for Need for Speed Underground 2 (SPEED2.EXE v1.2): boots straight into free roam.
//
// Mode=career: the career's free roam from the player's save. The front end still runs (it reads the save), but
// no screen waits for input:
//   * Boot screens come from a table of package names (0x7F65E8..0x7F6654, two copies) walked by 0x4D9580, which
//     skips empty names. The intro movies and UG_LS_Splash.fng ("press Enter") are pointed at the empty string
//     0x783FE5, but only while they still hold the shipped name, so mods that already replaced them win.
//   * The profile dialog MC_Main.fng (opened from MC_Background with flags 0x121) waits on the memory-card boot
//     object [0x836498]: byte +4 is set while waiting, +0x60 is the choice. Choice 1 is "Continue".
//     [0x836538] holds the pad mask of the button press; the career derives the player's pad from it (0x50D2A0),
//     and with no press the world load indexes the pad table with -1 (0x579434). Mask 1 (pad 0) is written too.
//   * Career > Resume is 0x495A30. Its crib and career-intro branches push front-end screens that crash from the
//     main menu (0x5377C1), so only its last branch is run (0x495A93..0x495ADC): port, start spot, career flags,
//     then 0x5404A0, which fills TheRaceParameters for track 4000, race type 0, and starts the load.
//   * Game flow [0x8654A4]: 3 = front end, 6 = gameplay. "Career loaded" is byte [0x83A9E0 + 0x20].
//
// Mode=dev: the developers' SkipFE flag [0x864FAC]. The boot (0x57DCB0) then skips the front end and calls
// 0x53FEB0, which fills TheRaceParameters from the globals beside it: track [0x7FAA18] (-1 = race type default,
// 4000 for type 0), race type [0x7FAA28], drag and drift markers [0x864FC0] / [0x864FC4]. Retail clears the flag
// at 0x58176B (mov [0x864FAC], ebx); that store is NOPed.
//
// Both modes run on the game thread from the first of the main loop's two calls to an empty stub (0x581470).
// NFSU2ExtraOptions loads later and redirects the same call; during boot a watcher re-chains this plugin on top
// when it stops being called. QuitOnPause exits once UI_Pause.fng is active (0x52CF60, cdecl(name) -> bool),
// which catches every way into the pause menu.

#include <windows.h>

#include <cstdlib>
#include <cstring>
#include <string>

namespace {

constexpr uintptr_t kMainLoopStubCall = 0x581470;   // call 0x4022C0
constexpr uintptr_t kGameFlow         = 0x8654A4;
constexpr int       kFlowFrontEnd     = 3;
constexpr int       kFlowGameplay     = 6;

// Mode=career
constexpr uintptr_t kCareerLoaded     = 0x83A9E0 + 0x20;
constexpr uintptr_t kMenuPort         = 0x836538;   // the pad mask of the last menu press
constexpr int       kKeyboardPadMask  = 1;          // pad 0
constexpr uintptr_t kResumeCareer     = 0x495A30;   // only checked, not called
constexpr uintptr_t kCareerMode       = 0x850078;
constexpr uintptr_t kCareerStarted    = 0x86347C;
constexpr uintptr_t kResumePort       = 0x83A9D8;
constexpr uintptr_t kResumed          = 0x863484;
constexpr uintptr_t kCareerTable      = 0x85AD40;
constexpr unsigned  kStartSpot        = 0xDD60E402;
constexpr uintptr_t kPortFor          = 0x50D2A0;   // cdecl(pad mask) -> pad
constexpr uintptr_t kTableHas         = 0x500B80;   // thiscall(table, hash) -> bool
constexpr uintptr_t kTableFind        = 0x5003F0;   // thiscall(table, hash) -> entry
constexpr uintptr_t kSetStartSpot     = 0x5410E0;   // thiscall(table, entry)
constexpr uintptr_t kStartFreeRoam    = 0x5404A0;   // cdecl(), fills the race and loads
constexpr uintptr_t kMemCardBoot      = 0x836498;
constexpr size_t    kWaitingForChoice = 0x04;       // byte
constexpr size_t    kChoice           = 0x60;       // 1 = "Continue"
constexpr uintptr_t kDialogFlags      = 0x836514;   // the memory-card dialog's flags
constexpr unsigned  kBootDialogFlags  = 0x121;      // MC_Background's profile dialog
// The boot-screen tables: two copies of pointers to package names.
constexpr uintptr_t kBootTableStart   = 0x7F65E8;
constexpr uintptr_t kBootTableEnd     = 0x7F6654;
constexpr uintptr_t kEmptyName        = 0x783FE5;
constexpr uintptr_t kSplashName       = 0x78EA60;   // UG_LS_Splash.fng
constexpr uintptr_t kIntroNames[]     = {
    0x78EAB8,   // LS_BlankMovie.fng
    0x78EAA8,   // LS_EAlogo.fng
    0x78EA98,   // LS_THXMovie.fng
    0x78EA88,   // LS_PSAMovie.fng
    0x78EA74,   // UG_LS_IntroFMV.fng
};

// Mode=dev
constexpr uintptr_t kSkipFeFlag       = 0x864FAC;
constexpr uintptr_t kSkipFeClear      = 0x58176B;   // mov [0x864FAC], ebx
constexpr uintptr_t kSkipFeRaceType   = 0x7FAA28;
constexpr uintptr_t kSkipFeTrack      = 0x7FAA18;   // -1: the race type's default
constexpr uintptr_t kSkipFeDrag       = 0x864FC0;
constexpr uintptr_t kSkipFeDrift      = 0x864FC4;
constexpr int       kRaceTypeDrag     = 4;
constexpr int       kRaceTypeDrift    = 5;

// QuitOnPause
constexpr uintptr_t kPackageActive    = 0x52CF60;   // cdecl(name) -> bool
const char* const   kPauseMenu        = "UI_Pause.fng";

enum class Mode { Career, Dev };

HMODULE g_module = nullptr;
using StubFn = void(__cdecl*)();
StubFn g_previous = nullptr;

bool  g_enabled = true;
Mode  g_mode = Mode::Career;
DWORD g_delayMs = 0;
bool  g_skipSplash = true;
bool  g_skipIntro = true;
bool  g_autoContinue = true;
int   g_devRaceType = 0;
int   g_devTrack = -1;
bool  g_quitOnPause = false;

volatile bool g_done = false;      // the career boot has nothing left to do
volatile DWORD g_lastCall = 0;
volatile bool g_jumped = false;    // this session went to the street through the plugin
DWORD g_readySince = 0;
bool  g_answered = false;

std::wstring BesidePlugin(const wchar_t* extension) {
    wchar_t path[MAX_PATH] = {};
    ::GetModuleFileNameW(g_module, path, MAX_PATH);
    std::wstring out = path;
    const size_t dot = out.find_last_of(L'.');
    if (dot != std::wstring::npos) out.resize(dot);
    return out + extension;
}

template <typename T> T Read(uintptr_t address) { return *(volatile T*)address; }

// ---------------------------------------------------------------- dev

bool EnableSkipFrontEnd() {
    static const unsigned char kClear[6] = { 0x89, 0x1D, 0xAC, 0x4F, 0x86, 0x00 };
    unsigned char* code = (unsigned char*)kSkipFeClear;
    if (std::memcmp(code, kClear, sizeof(kClear)) != 0) return false;
    DWORD old = 0;
    if (!::VirtualProtect(code, sizeof(kClear), PAGE_EXECUTE_READWRITE, &old)) return false;
    std::memset(code, 0x90, sizeof(kClear));
    ::VirtualProtect(code, sizeof(kClear), old, &old);
    ::FlushInstructionCache(::GetCurrentProcess(), code, sizeof(kClear));

    ::VirtualProtect((void*)kSkipFeTrack, 0x14, PAGE_READWRITE, &old);   // 0x7FAA18..0x7FAA2B
    *(int*)kSkipFeTrack = g_devTrack;
    *(int*)kSkipFeRaceType = g_devRaceType;
    *(int*)kSkipFeDrag = g_devRaceType == kRaceTypeDrag;
    *(int*)kSkipFeDrift = g_devRaceType == kRaceTypeDrift;
    *(int*)kSkipFeFlag = 1;
    return true;
}

// ------------------------------------------------------------- career

// Empties every boot-table entry (both copies) that still points at the shipped name.
void DropBootScreen(uintptr_t name) {
    for (uintptr_t entry = kBootTableStart; entry < kBootTableEnd; entry += 4) {
        if (*(uintptr_t*)entry != name) continue;
        DWORD old = 0;
        if (!::VirtualProtect((void*)entry, 4, PAGE_READWRITE, &old)) continue;
        *(uintptr_t*)entry = kEmptyName;
        ::VirtualProtect((void*)entry, 4, old, &old);
    }
}

// Answers "Continue" (choice 1) in the boot profile dialog once it waits for input.
void AnswerProfileDialog() {
    if (!g_autoContinue || g_answered || Read<unsigned char>(kCareerLoaded)) return;
    const uintptr_t boot = Read<uintptr_t>(kMemCardBoot);
    if (!boot || !Read<unsigned char>(boot + kWaitingForChoice)) return;
    const unsigned flags = Read<unsigned>(kDialogFlags);
    if (flags != kBootDialogFlags) {   // a different memory-card prompt: left to the player
        g_answered = true;
        return;
    }
    if (!Read<int>(kMenuPort)) *(volatile int*)kMenuPort = kKeyboardPadMask;
    *(volatile int*)(boot + kChoice) = 1;
    g_answered = true;
}

// Starts free roam once the career is loaded in the front end.
void CareerTick(int flow) {
    if (flow == kFlowFrontEnd) AnswerProfileDialog();
    if (g_done) return;

    const int career = Read<unsigned char>(kCareerLoaded);

    // already past the front end
    if (flow > kFlowFrontEnd) {
        g_done = true;
        return;
    }
    if (flow != kFlowFrontEnd || !career) {
        g_readySince = 0;
        return;
    }
    const DWORD now = ::GetTickCount();
    if (!g_readySince) {
        g_readySince = now;
        return;
    }
    if (now - g_readySince < g_delayMs) return;

    g_done = true;
    if (::GetAsyncKeyState(VK_SHIFT) & 0x8000) return;   // Shift held: stay in the menus
    // a career not yet started has an intro, which is a front-end screen
    if (!Read<unsigned char>(kCareerStarted)) return;
    // 0x495A93..0x495ADC
    if (!Read<int>(kMenuPort)) *(volatile int*)kMenuPort = kKeyboardPadMask;   // no press recorded: pad 0
    const int port = Read<int>(kMenuPort);
    *(volatile int*)kCareerMode = 1;
    *(volatile int*)kResumePort = reinterpret_cast<int(__cdecl*)(int)>(kPortFor)(port);
    const bool has = reinterpret_cast<bool(__thiscall*)(void*, unsigned)>(kTableHas)(
        (void*)kCareerTable, kStartSpot);
    const unsigned spot = kStartSpot + (has ? 1u : 0u);
    void* entry = reinterpret_cast<void*(__thiscall*)(void*, unsigned)>(kTableFind)((void*)kCareerTable, spot);
    reinterpret_cast<void(__thiscall*)(void*, void*)>(kSetStartSpot)((void*)kCareerTable, entry);
    *(volatile unsigned char*)kResumed = 1;
    reinterpret_cast<void(__cdecl*)()>(kStartFreeRoam)();
    g_jumped = true;
}

// --------------------------------------------------------------- both

// Exits at the pause menu in a session this plugin started.
void QuitAtPauseMenu(int flow) {
    if (!g_quitOnPause || !g_jumped || flow != kFlowGameplay) return;
    if (!reinterpret_cast<bool(__cdecl*)(const char*)>(kPackageActive)(kPauseMenu)) return;
    ::ExitProcess(0);
}

void Tick() {
    const int flow = Read<int>(kGameFlow);
    if (g_mode == Mode::Career) CareerTick(flow);
    QuitAtPauseMenu(flow);
}

void __cdecl OnMainLoop() {
    g_lastCall = ::GetTickCount();
    if (g_previous) g_previous();
    Tick();
}

uintptr_t CallTarget() {
    const unsigned char* call = (const unsigned char*)kMainLoopStubCall;
    return kMainLoopStubCall + 5 + *(const int*)(call + 1);
}

bool HookMainLoop() {
    unsigned char* call = (unsigned char*)kMainLoopStubCall;
    if (call[0] != 0xE8) return false;
    g_previous = (StubFn)CallTarget();
    DWORD old = 0;
    if (!::VirtualProtect(call, 5, PAGE_EXECUTE_READWRITE, &old)) return false;
    *(int*)(call + 1) = (int)((uintptr_t)&OnMainLoop - (kMainLoopStubCall + 5));
    ::VirtualProtect(call, 5, old, &old);
    ::FlushInstructionCache(::GetCurrentProcess(), call, 5);
    return true;
}

// Keeps the plugin on the stub call during boot. A later hook that still chains to us keeps g_lastCall fresh and is
// left alone; one that dropped us is chained underneath. Limited to the boot: a long loading screen would make a
// chaining hook look stale, and re-chaining it would create a call cycle.
DWORD WINAPI Watch(LPVOID) {
    for (int i = 0; i < 600; ++i) {
        ::Sleep(100);
        const DWORD now = ::GetTickCount();
        if (CallTarget() == (uintptr_t)&OnMainLoop) continue;
        if (g_lastCall && now - g_lastCall < 500) continue;
        HookMainLoop();
    }
    return 0;
}

// Identifies SPEED2.EXE v1.2 by the start of the resume handler.
bool RightExecutable() {
    static const unsigned char kResumeStart[] = { 0x55, 0x8B, 0xEC, 0x68, 0x03, 0xE4, 0x60, 0xDD };
    return std::memcmp((const void*)kResumeCareer, kResumeStart, sizeof(kResumeStart)) == 0;
}

} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    g_module = module;
    ::DisableThreadLibraryCalls(module);

    const std::wstring ini = BesidePlugin(L".ini");
    const wchar_t* section = L"FastTest";
    wchar_t mode[16] = {};
    ::GetPrivateProfileStringW(section, L"Mode", L"career", mode, 16, ini.c_str());
    g_mode = ::_wcsicmp(mode, L"dev") == 0 ? Mode::Dev : Mode::Career;
    g_enabled = ::GetPrivateProfileIntW(section, L"Enabled", 1, ini.c_str()) != 0;
    g_delayMs = ::GetPrivateProfileIntW(section, L"DelayMs", 0, ini.c_str());
    g_skipSplash = ::GetPrivateProfileIntW(section, L"SkipSplash", 1, ini.c_str()) != 0;
    g_skipIntro = ::GetPrivateProfileIntW(section, L"SkipIntro", 1, ini.c_str()) != 0;
    g_autoContinue = ::GetPrivateProfileIntW(section, L"AutoContinue", 1, ini.c_str()) != 0;
    g_devRaceType = (int)::GetPrivateProfileIntW(section, L"DevRaceType", 0, ini.c_str());
    // read as text: GetPrivateProfileInt does not take negative values
    wchar_t track[16] = {};
    ::GetPrivateProfileStringW(section, L"DevTrack", L"-1", track, 16, ini.c_str());
    g_devTrack = (int)std::wcstol(track, nullptr, 10);
    if (g_devTrack <= 0) g_devTrack = -1;
    g_quitOnPause = ::GetPrivateProfileIntW(section, L"QuitOnPause", 0, ini.c_str()) != 0;
    if (!g_enabled || !RightExecutable()) return TRUE;

    if (g_mode == Mode::Dev) {
        if (!EnableSkipFrontEnd()) return TRUE;
        g_jumped = true;
    }
    if (!HookMainLoop()) return TRUE;
    if (g_mode == Mode::Career && g_skipIntro)
        for (uintptr_t name : kIntroNames) DropBootScreen(name);
    if (g_mode == Mode::Career && g_skipSplash) DropBootScreen(kSplashName);
    if (HANDLE thread = ::CreateThread(nullptr, 0, &Watch, nullptr, 0, nullptr)) ::CloseHandle(thread);
    return TRUE;
}
