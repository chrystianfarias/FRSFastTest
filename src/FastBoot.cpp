// FRSFastBoot -- Need for Speed Underground 2 (SPEED2.EXE v1.2): boot
// straight into free roam, with no screen to click through.
//
// Licensed under CC BY-NC 4.0: share and adapt with attribution, for
// non-commercial purposes only. See LICENSE.
//
// Two ways in, chosen in the ini:
//
//   Mode=career  (the default) the career's free roam, from the player's own
//                save: the front end still runs, since that is where the save
//                is read, but every screen that waits for the player is
//                answered or left out.
//   Mode=dev     the developers' own switch, SkipFE: the front end never runs
//                and the game loads the whole city at once -- faster, but with
//                no save loaded (no money, a default car, no shops).
//
// ---- Mode=career
//
// What Career > Resume runs was read out of the executable:
//
//   0x495A30  cdecl(port)  the resume handler. Sets career mode
//             ([0x850078] = 1). A save whose crib is open (0x500B80 on the
//             career table 0x85AD40 with 0xDD60E403) gets the crib screen
//             first, and a career not started ([0x86347C] == 0) its intro;
//             both are front-end screens pushed over the career menu, and
//             from the main menu they crash (0x5377C1). Otherwise it picks the
//             start spot and jumps to 0x5404A0, which fills TheRaceParameters
//             for track 4000 (the whole city), race type 0 (free roam), and
//             starts the load from the front end (0x578320).
//   0x4AF325  the menu's own call: `push [0x836538]; call 0x495A30`, after
//             checking byte [0x83A9E0 + 0x20] (0x502FE0): "a career is loaded".
//   0x8654A4  game flow: 3 is the front end, 6 gameplay.
//
// The plugin runs that handler's last branch only (0x495A93..0x495ADC): the
// port, the start spot, the career flags, and 0x5404A0 -- no screen at all.
//
// Before that, the "press Enter" screen never shows. The front end's boot
// screens come from a static table of package names (0x7F65E8..0x7F6620, and
// a second copy at 0x7F6620..0x7F6654 for the other configuration), walked by
// 0x4D9580, which skips any entry whose name is empty:
//
//   DiscErrorPC, MC_Bootup, LS_BlankMovie, LS_EAlogo, LS_THXMovie,
//   LS_PSAMovie, UG_LS_IntroFMV, UG_LS_Splash, MC_Background, UI_Main
//
// UG_LS_Splash.fng is the "press Enter" screen, and the four movies and the
// blank one before it are the intro. Their entries are pointed at the empty
// string the table already uses (0x783FE5) before the front end starts, and
// the boot goes on without them. An entry is only changed while it still
// names the shipped screen: another mod that has taken it (NFSU2ExtraOptions
// skips the movies its own way) keeps it, and the two never fight.
//
// And the profile's "Continue" is answered without the screen. The front end
// opens the memory-card dialog MC_Main.fng from MC_Background (0x4C5235,
// 0x4A3100 with flags 0x121); it reads the profile, then waits for a choice:
//
//   [0x836498]       the memory-card boot object; byte +4 is set while it
//                    waits for the player's choice, and +0x60 is the choice.
//   0x4A3AE0         what MC_Main's buttons do (from 0x4F3780): "Continue"
//                    (button hash 0xB8A7C6CE, as its neighbours CD and CF)
//                    writes choice 1 when +4 is set; the rest only hides the
//                    buttons. The boot object's own state machine then commits
//                    the profile and closes the dialog.
//
// So the plugin writes choice 1 the moment the boot dialog (flags 0x121) waits
// for it, before any career is loaded -- never for any other memory-card
// question.
//
// A button press also names the controller: MC_Main's handler stores the
// pressing pad's mask in [0x836538] (1 = pad 0, 2 = pad 1, ...; 0x4FB9AC),
// and the career takes the player's pad from it (0x50D2A0: mask -> index,
// -1 for none). With no press it stays 0, the player gets pad -1, and the
// world load reads past the pad table (0x579434, 0x8655A0 + pad * 0x1C8). So
// the answer also writes mask 1, pad 0 -- what the keyboard gave every time.
//
// ---- Mode=dev
//
// [0x864FAC] is the SkipFE flag: with it set, the boot never opens the menus
// (0x57DCB0 calls the front end 0x4C57C0 only when it is zero) and goes to
// 0x53FEB0 instead (0x580E3C), which fills TheRaceParameters from the SkipFE
// globals beside it. The race type [0x7FAA28] picks the track when none is
// named ([0x7FAA18] = -1): type 0 is track 4000, "L4RA (40XX-41XX)", free
// roam. The release build switches the flag off at 0x58176B
// (`mov [0x864FAC], ebx`, ebx zero); that one store is removed and the flag
// set before the game starts.
//
// Any other track can be named instead (DevTrack): 0x53FEB0 takes [0x7FAA18]
// as it is when it is not -1. The race type then has to be the track's: the
// career's race records (GLOBALB 0x34A11, byte +0xF) give 0 circuit / URL /
// SUV, 1 sprint, 2 street X, 4 drag, 5 drift. Drag and drift also have a flag
// each ([0x864FC0], [0x864FC4]; 0x53FFD3..0x540018 copy them into the race as
// its drag and drift markers), set here from the race type.
//
// ---- Both
//
// The plugin runs on the game's thread, from the first of the two calls the
// main loop makes to an empty stub (0x581470; SpeedLoader takes the second).
// The call is re-pointed at the plugin, which calls whatever was there first.
// NFSU2ExtraOptions re-points the same call, and it loads after this plugin,
// so a watcher thread puts the plugin back on top -- still calling the one it
// replaces -- whenever it stops being called during the boot.
//
// Optionally, a session that came in this way ends at the pause menu: the
// game quits the moment UI_Pause.fng is up. The menu itself is what is looked
// for -- 0x52CF60 cdecl(name), the game's own "is this package up" -- not a
// key, since mods use Esc, and the pause menu has more than one way in (the
// joystick-event handler at 0x606730 opens it on several paths).
//
// FRSFastBoot.ini beside the plugin:
//   [FastBoot]
//   Enabled=1        0 leaves the game alone
//   Mode=career      career: the career's free roam; dev: SkipFE, no save
//   SkipIntro=1      career: leave the intro movies out of the boot
//   SkipSplash=1     career: leave the "press Enter" screen out of the boot
//   AutoContinue=1   career: answer the profile's "Continue" by itself
//   DelayMs=0        career: extra wait once the career is loaded
//   DevTrack=-1      dev: the track number (-1: the race type's own, 4000 for 0)
//   DevRaceType=0    dev: 0 free roam / circuit, 1 sprint, 2 street X, 4 drag, 5 drift
//   QuitOnPause=0    1 quits the game when the pause menu opens, in a session
//                    this plugin started
// In career mode, holding Shift when the career loads stays in the menus.

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

// Takes a screen out of the boot, in both copies of the table: every entry
// that still names it (by the shipped string's address) is emptied.
void DropBootScreen(uintptr_t name) {
    for (uintptr_t entry = kBootTableStart; entry < kBootTableEnd; entry += 4) {
        if (*(uintptr_t*)entry != name) continue;
        DWORD old = 0;
        if (!::VirtualProtect((void*)entry, 4, PAGE_READWRITE, &old)) continue;
        *(uintptr_t*)entry = kEmptyName;
        ::VirtualProtect((void*)entry, 4, old, &old);
    }
}

// Choice 1 -- "Continue" -- for the boot's profile dialog, once it waits.
void AnswerProfileDialog() {
    if (!g_autoContinue || g_answered || Read<unsigned char>(kCareerLoaded)) return;
    const uintptr_t boot = Read<uintptr_t>(kMemCardBoot);
    if (!boot || !Read<unsigned char>(boot + kWaitingForChoice)) return;
    const unsigned flags = Read<unsigned>(kDialogFlags);
    if (flags != kBootDialogFlags) {   // some other memory-card question: the player's
        g_answered = true;
        return;
    }
    if (!Read<int>(kMenuPort)) *(volatile int*)kMenuPort = kKeyboardPadMask;
    *(volatile int*)(boot + kChoice) = 1;
    g_answered = true;
}

// Once the career is loaded in the front end: straight to its free roam.
void CareerTick(int flow) {
    if (flow == kFlowFrontEnd) AnswerProfileDialog();
    if (g_done) return;

    const int career = Read<unsigned char>(kCareerLoaded);

    // Past the menus already (a race, free roam): nothing left to do.
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
    if (::GetAsyncKeyState(VK_SHIFT) & 0x8000) return;   // the player stays in the menus
    // A career not started yet has an intro, and that is a front-end screen.
    if (!Read<unsigned char>(kCareerStarted)) return;
    // 0x495A93..0x495ADC, as the game runs it.
    if (!Read<int>(kMenuPort)) *(volatile int*)kMenuPort = kKeyboardPadMask;   // no pad chose: pad 0
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

// The pause menu of a session this plugin started: the game ends there.
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

// Keeps the plugin on the call through the boot. A hook installed over this
// one that still calls it keeps g_lastCall fresh and is left alone; one that
// dropped it is chained under the plugin. Only during the boot: later, a long
// loading screen could make a hook that does call this one look like it
// dropped it, and re-chaining it would make the two call each other.
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

// The resume handler must be the one this was written against.
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
    const wchar_t* section = L"FastBoot";
    wchar_t mode[16] = {};
    ::GetPrivateProfileStringW(section, L"Mode", L"career", mode, 16, ini.c_str());
    g_mode = ::_wcsicmp(mode, L"dev") == 0 ? Mode::Dev : Mode::Career;
    g_enabled = ::GetPrivateProfileIntW(section, L"Enabled", 1, ini.c_str()) != 0;
    g_delayMs = ::GetPrivateProfileIntW(section, L"DelayMs", 0, ini.c_str());
    g_skipSplash = ::GetPrivateProfileIntW(section, L"SkipSplash", 1, ini.c_str()) != 0;
    g_skipIntro = ::GetPrivateProfileIntW(section, L"SkipIntro", 1, ini.c_str()) != 0;
    g_autoContinue = ::GetPrivateProfileIntW(section, L"AutoContinue", 1, ini.c_str()) != 0;
    g_devRaceType = (int)::GetPrivateProfileIntW(section, L"DevRaceType", 0, ini.c_str());
    // As text: GetPrivateProfileInt turns a negative value into 0.
    wchar_t track[16] = {};
    ::GetPrivateProfileStringW(section, L"DevTrack", L"-1", track, 16, ini.c_str());
    g_devTrack = (int)std::wcstol(track, nullptr, 10);
    if (g_devTrack <= 0) g_devTrack = -1;
    g_quitOnPause = ::GetPrivateProfileIntW(section, L"QuitOnPause", 0, ini.c_str()) != 0;
    // Only the executable this was written against.
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
