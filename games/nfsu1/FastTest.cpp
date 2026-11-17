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
// Start position: FRS World Editor's Launch Game writes <game>\models\launch-pose.txt,
//   pose <x> <y> <z> <heading degrees>
//   track <number>            (optional: the track whose region the editor has open)
//   freeroam                  (optional: a drive, not a race -- see "Free roam" below)
//   embedded                  (optional: the editor shows the game in its view, so the window's place and size are
//                              left to it -- no centring on a monitor, no resizing, no keeping the focus away)
//   graphics <0..3>           (optional: a graphics preset -- see "Graphics" below)
//   graphics <NAME> <value>   (optional, any number: one graphics option, e.g. "graphics WorldLOD 3")
// read and deleted at start, so it only applies to the launch that wrote it. A second after the player's car is
// first there in gameplay, it is put at the pose the way the game puts it on the finish line at the end of a race
// (0x422F34): 0x427E60 (ecx = car, edx = position, eax = heading as a bAngle, stack: speed, ghost; ret 8), then
// 0x444840 (ecx = x, y; edx = the streamer 0x77A970; eax = 0) takes the streaming there. The heading's bAngle is
// the game's own 0x564DB0 (cdecl x, y of the direction). The player's car is [[0x7361F0] + 0x28]. U1's collision
// is the whole track's and always loaded, so the car lands on ground at once. It runs on the game's thread, ahead
// of the call at 0x447A10 in the main loop (0x447A06, beside it, is BayviewU1's).
//
// Where the car starts: the grid. The track bundle's start markers (chunk 0x34146, loader 0x441ED0 in slot 2 of
// the chunk loader table 0x7007E0: 0x30-byte records, +8 name hash, +0xC index, +0x10 xyz, +0x20 bAngle) are turned
// and moved before the loader takes them, ForwardStartingPosition 0 onto the pose -- as BayviewU1's return to its
// free roam does. The car, the streamer's first sections and any grid all start there, with nothing to teleport.
// Should the markers never come by, the car is put there a second into gameplay as before.
//
// Free roam (the editor's launches): the race skips what stands before driving.
//   - the camera fly-by (RCSTATE_CAMERA_FLYBY, 0x423070): its tick (message 3) waits for the fly-by camera
//     ([0x737064]); its skip (message 4, 0x423103) goes on to the pre-race. The jump table entry of the tick
//     (0x423268 + 2 * 4) is pointed at the skip.
//   - the pre-race animation (RCSTATE_PRERACE, 0x423280): it plays GLOBAL\PRERACEANIM.LZC when 0x448440 says that
//     is loaded (the call at 0x42333C); told it is not, the pre-race starts the countdown and the race at once.
//   - the start cut-scene (the NIS the pre-race plays): its tick (message 3, through 0x423500) goes to its
//     skip (message 4, 0x4234B1), which starts the countdown and the race.
//   - the countdown (0x434D10, 3-2-1 of 0x6B79E0 / 0x6B79E4 seconds each): 0.01 s, written every frame of the
//     session (BayviewU1 puts its own value back as each track loads).
//   - the track map: 0x58E810 (stdcall 1, ret 4, not FEng) is called from 0x409EF0 for each player; that call goes
//     through a stub that returns at once. The call is chained, not replaced: BayviewU1 jumps from the function's
//     first bytes and CockpitU1 may have redirected the call already.
//   - no race barriers: the editor writes <game>\models\freeroam\TRACKB<n>.LZC (the track's bundle with the
//     collision of every track of the region, the race barriers' walls left out) and it is opened in place of
//     TRACKS\TRACKB<n>.LZC (CreateFileA); the race furniture's models ("hide <hash>" lines) lose their scenery as
//     the sections load (slot 12, 0x4457C0: their SceneryInfos name no model).
//   - Esc, with the game's window in front, closes the game (ExitProcess, as Underground 2's QuitOnPause). Not the
//     pause itself: the game pauses on its own when it loses the focus, which is every click in the editor.
//   - no music: the profile's music volume, float [0x7588F4] (third of the six volumes from 0x7588EC, the master
//     first; the options screens' sliders), is held at 0, as the player's own slider at its lowest. The music voice (0x53C020, vtable
//     0x6BE0AC) starts a song only when music x master > 0, and 0x51CC10 (stdcall, the audio world [0x7356A0], whose
//     +8 the callers point at 0x7588EC) hands the volumes to every voice, once in gameplay. Sound effects, engine and
//     speech keep theirs.
//
// Graphics: the video options are ints in .data, named by the table 0x700EC0 ({name, int*, type 0}, BPP RES
// PERFORMANCE ENVMAP WORLDENVMAP SHADOW HEADLIGHTS CARLIGHTING DAMAGE CROWD ROADREFLECTION FOG MOTIONBLUR LIGHTSTREAKS
// LIGHTGLOW ANIMATEDTEXTURE PARTICLESYSTEMS DEPTHOFFIELD WorldLOD CarLOD SOUNDPERF SOUNDENABLE). At boot the device
// set-up (0x408830 -> 0x410E50) rates the hardware (0x410A00, from its CPU and card tables) and sets options from
// it: below 0.3, preset 0 (0x410ED0), everything at its lowest -- what SkipFE showed on today's PCs. The player's choices are only read when the front
// end loads a profile (0x41D030): <CSIDL_COMMON_APPDATA>\NFS Underground\<profile>.cfg, "NAME: value" lines, read by
// 0x41C680 through the table, then 0x410CE0 lowers each option to the hardware's cap (0x701040..) and checks RES
// against the display modes. SkipFE never loads a profile, so the boot's preset stayed. Now, ahead of 0x4B6390 (the
// device and its preset are set up by then, 0x447262 -> 0x53E020 -> 0x408830), the plugin reads the newest .cfg
// there (or [Graphics] Profile) the same way, then the overrides, then calls 0x410CE0:
//   FRSFastTest.ini [Graphics]   Profile = <name> (empty: the profile saved last; "-": none, the boot's preset)
//                                Preset = 0..3 (the game's PERFORMANCE presets, 0x410ED0 with edx = level, which
//                                         would also set RES: BPP and RES are kept; -1: none)
//                                <NAME> = <value>, any option of the table, e.g. WorldLOD = 3
//   launch-pose.txt              graphics <0..3>          a preset (after the ini)
//                                graphics <NAME> <value>  one option (after the preset), names as in the .cfg,
//                                                          any case
//
// Window: the window itself comes from NFSUnderground.WidescreenFix (WindowedMode). The game's own windowed flag
// [0x736374] (copied into D3DPRESENT_PARAMETERS.Windowed at 0x4086F0) is not used: with it CreateDevice fails and
// the game crashes on the null device (0x4082C7). 0x736378 is the HWND, 0x71A98C the Windowed field the device was
// made with. 0x4086B0 (cdecl(width), esi = height) applies a display mode; its first five bytes jump to a stub that
// substitutes [Window] Width x Height. A window is resized to that size and moved to [Window] Monitor.
#include "fasttest.h"

#include <shlobj.h>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#pragma comment(lib, "shell32.lib")   // SHGetFolderPathA, as the game finds its profiles

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
constexpr uintptr_t kWorld = 0x7361F0, kWorldPlayerCar = 0x28;
constexpr uintptr_t kPlaceCar = 0x427E60, kMoveStreamer = 0x444840, kStreamer = 0x77A970, kAngleOf = 0x564DB0;
constexpr uintptr_t kFrameCall = 0x447A10;
constexpr uintptr_t kLoaderTable = 0x7007E0, kMarkersSlot = 2;
constexpr uint32_t kChunkMarkers = 0x34146, kForwardStartingPosition = 0x8E55C895;   // bStringHash
constexpr uintptr_t kFlybyJumpTable = 0x423268, kFlybyTick = 0x4230FA, kFlybySkip = 0x423103;
// RCSTATE_PRERACE (0x423280): messages 1..10 through 0x423500; 3 the tick, 4 the skip
constexpr uintptr_t kPreRaceJumpTable = 0x423500, kPreRaceTick = 0x423448, kPreRaceSkip = 0x4234B1;
constexpr uintptr_t kPreRaceAnimCall = 0x42333C, kIsLoaded = 0x448440;
constexpr uintptr_t kCountdown = 0x6B79E0;
constexpr uintptr_t kRaceStart = 0x736194;   // the race's start object: +0xC the length of each count
constexpr uintptr_t kTrackMapCall = 0x409EF0;
constexpr uintptr_t kSceneryLoaderSlot = 12;   // 0x4457C0, chunk 0x80034100
constexpr uintptr_t kFileSystemCreateW = 0x5C7FDC, kFileSystemCreateA = 0x5C7FF7;   // the game file system's calls
constexpr uintptr_t kImportCreateFileW = 0x6971C4, kImportCreateFileA = 0x6971C0;   // speed.exe's import table
constexpr uintptr_t kSetMode = 0x4086B0, kSetModeResume = 0x4086B5;
constexpr uint8_t kSetModeStart[] = {0x53, 0x8B, 0x5C, 0x24, 0x08};   // push ebx; mov ebx, [esp+8]

// audio: the profile's six volumes (master first, music third) and their hand-out to the voices
constexpr uintptr_t kAudioWorld = 0x7356A0, kVolumes = 0x7588EC, kMusicVolume = 0x7588F4;
constexpr uintptr_t kApplyVolumes = 0x51CC10;
constexpr uint8_t kApplyVolumesStart[] = {0xA1, 0xD8, 0x1D, 0x6F, 0x00};   // mov eax, [0x6F1DD8] (audio on)

// graphics: the option table, the PERFORMANCE presets and the hardware caps
constexpr uintptr_t kVideoOptions = 0x700EC0;   // {name, int* value, type}, 12 bytes each
// The table ends with an entry whose name is an empty string (0x6B9172) and whose value is null -- not with a null
// name: past it lie other data. An entry is an option while its name is a non-empty string and its value is set.
inline bool IsVideoOption(const uint8_t* e) {
    const char* name = *(const char* const*)e;
    return name && *name && *(int* const*)(e + 4);
}
constexpr uintptr_t kVideoPreset = 0x410ED0, kVideoClamp = 0x410CE0;
constexpr uint8_t kVideoPresetStart[] = {0x33, 0xC9, 0x3B, 0xD1};               // xor ecx, ecx; cmp edx, ecx
constexpr uint8_t kVideoClampStart[] = {0xA1, 0x40, 0x10, 0x70, 0x00};          // mov eax, [0x701040]

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

// ---- the start position (see the top of the file)

struct LaunchPose {
    bool pending = false;
    float x = 0, y = 0, z = 0, heading = 0;
    DWORD readySince = 0;
} g_pose;
uintptr_t g_frameNext = 0;
bool g_embedded = false;
bool g_freeRoam = false;
using Loader = int(__cdecl*)(void*);
Loader g_markersLoader = nullptr;
uintptr_t g_trackMapNext = 0;
std::vector<uint32_t> g_hide;     // solids whose scenery is left out of a free roam ("hide <hash>"), sorted
Loader g_sceneryLoader = nullptr;
using CreateFileAFn = HANDLE(WINAPI*)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
CreateFileAFn g_createFileA = nullptr;
bool g_musicMuted = false;        // the music volume was handed to the voices at 0

// graphics: one source's settings, applied in order (profile, ini, launch-pose.txt)
struct GraphicsSettings {
    int preset = -1;                                  // 0..3, -1 = none
    std::vector<std::pair<std::string, int>> options;   // option name, value
};
GraphicsSettings g_graphicsIni, g_graphicsPose;
std::string g_graphicsProfile;    // [Graphics] Profile

std::string GameFolder() {
    char path[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string folder = path;
    const size_t slash = folder.find_last_of("\\/");
    return slash == std::string::npos ? "." : folder.substr(0, slash);
}

void ReadLaunchPose() {
    const std::string path = GameFolder() + "\\models\\launch-pose.txt";
    FILE* file = fopen(path.c_str(), "rt");
    if (!file) return;
    char line[128];
    while (fgets(line, sizeof(line), file)) {
        int track = 0;
        if (sscanf(line, "pose %f %f %f %f", &g_pose.x, &g_pose.y, &g_pose.z, &g_pose.heading) == 4)
            g_pose.pending = true;
        else if (sscanf(line, "track %d", &track) == 1 && track > 0)
            g_track = track;
        else if (strncmp(line, "embedded", 8) == 0)
            g_embedded = true;
        else if (strncmp(line, "freeroam", 8) == 0)
            g_freeRoam = true;
        else if (strncmp(line, "graphics ", 9) == 0) {
            char name[64] = {}, value[32] = {};
            const int n = sscanf(line + 9, "%63s %31s", name, value);
            if (n == 2)
                g_graphicsPose.options.emplace_back(name, (int)strtol(value, nullptr, 0));
            else if (n == 1)
                g_graphicsPose.preset = (int)strtol(name, nullptr, 10);
        } else {
            unsigned hash = 0;
            if (sscanf(line, "hide %x", &hash) == 1) g_hide.push_back(hash);
        }
    }
    fclose(file);
    std::sort(g_hide.begin(), g_hide.end());
    DeleteFileA(path.c_str());
    if (g_pose.pending)
        ft::Log("launch pose: the car goes to (%.1f, %.1f, %.1f) heading %.1f once in gameplay", g_pose.x, g_pose.y,
                g_pose.z, g_pose.heading);
}

// ---- graphics (see the top of the file)

int* VideoOption(const char* name) {
    for (const uint8_t* e = (const uint8_t*)kVideoOptions; IsVideoOption(e); e += 12)
        if (*(const int*)(e + 8) == 0 && _stricmp(*(const char* const*)e, name) == 0) return *(int* const*)(e + 4);
    return nullptr;
}

void SetVideoOptions(const GraphicsSettings& settings, const char* from) {
    for (const auto& option : settings.options) {
        if (int* value = VideoOption(option.first.c_str())) *value = option.second;
        else ft::Log("graphics: %s: no option \"%s\"", from, option.first.c_str());
    }
}

// 0x410ED0 (edx = level): the options screen's PERFORMANCE presets, then the hardware caps. A preset also sets
// the display mode (RES 0 at level 0, 1 above), which stays the player's here.
void VideoPreset(int level, const char* from) {
    if (level < 0) return;
    level = std::min(level, 3);
    int* bpp = VideoOption("BPP");
    int* res = VideoOption("RES");
    const int keepBpp = bpp ? *bpp : 0, keepRes = res ? *res : 0;
    __asm {
        mov edx, level
        mov eax, kVideoPreset
        call eax
    }
    if (bpp) *bpp = keepBpp;
    if (res) *res = keepRes;
    ft::Log("graphics: %s: preset %d", from, level);
}

std::string ProfileFolder() {
    char path[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathA(nullptr, CSIDL_COMMON_APPDATA, nullptr, 0, path))) return {};
    return std::string(path) + "\\NFS Underground\\";
}

// <profile>.cfg ("NAME: value" lines, as 0x41C7A0 writes them): [Graphics] Profile, or the one saved last
bool ReadProfileGraphics(GraphicsSettings& out, std::string& file) {
    const std::string folder = ProfileFolder();
    if (folder.empty()) return false;
    if (!g_graphicsProfile.empty()) {
        file = g_graphicsProfile + ".cfg";
    } else {
        WIN32_FIND_DATAA found;
        const HANDLE find = FindFirstFileA((folder + "*.cfg").c_str(), &found);
        if (find == INVALID_HANDLE_VALUE) return false;
        FILETIME newest{};
        do {
            if (CompareFileTime(&found.ftLastWriteTime, &newest) > 0) {
                newest = found.ftLastWriteTime;
                file = found.cFileName;
            }
        } while (FindNextFileA(find, &found));
        FindClose(find);
    }
    FILE* f = fopen((folder + file).c_str(), "rt");
    if (!f) return false;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        char name[64] = {}, value[32] = {};
        if (sscanf(line, " %63[^: ] : %31s", name, value) == 2)
            out.options.emplace_back(name, (int)strtol(value, nullptr, 0));
    }
    fclose(f);
    file = folder + file;
    return true;
}

// Ahead of 0x4B6390: the player's profile, then the ini, then launch-pose.txt, then the hardware caps (0x410CE0)
void ApplyGraphics() {
    if (!ft::Matches(kVideoPreset, kVideoPresetStart, sizeof(kVideoPresetStart)) ||
        !ft::Matches(kVideoClamp, kVideoClampStart, sizeof(kVideoClampStart)) || !VideoOption("WorldLOD")) {
        ft::Log("graphics: the option code differs from v1.4 (another mod?): the boot's settings stay");
        return;
    }
    if (g_graphicsProfile != "-") {
        GraphicsSettings profile;
        std::string file;
        if (ReadProfileGraphics(profile, file)) {
            SetVideoOptions(profile, file.c_str());
            ft::Log("graphics: the player's settings from %s", file.c_str());
        } else {
            ft::Log("graphics: no profile .cfg in %s: the boot's preset stays", ProfileFolder().c_str());
        }
    }
    VideoPreset(g_graphicsIni.preset, "FRSFastTest.ini");
    SetVideoOptions(g_graphicsIni, "FRSFastTest.ini");
    VideoPreset(g_graphicsPose.preset, "launch-pose.txt");
    SetVideoOptions(g_graphicsPose, "launch-pose.txt");
    ((void(__cdecl*)())kVideoClamp)();
    std::string all;
    for (const uint8_t* e = (const uint8_t*)kVideoOptions; IsVideoOption(e); e += 12) {
        if (*(const int*)(e + 8) != 0) continue;
        char item[64];
        snprintf(item, sizeof(item), "%s%s %d", all.empty() ? "" : ", ", *(const char* const*)e, **(int* const*)(e + 4));
        all += item;
    }
    ft::Log("graphics: %s", all.c_str());
}

void ReadGraphicsIni(const ft::Ini& ini) {
    g_graphicsProfile = ini.String("Graphics", "Profile");
    g_graphicsIni.preset = ini.Int("Graphics", "Preset", -1);
    for (const uint8_t* e = (const uint8_t*)kVideoOptions; IsVideoOption(e); e += 12) {
        const char* name = *(const char* const*)e;
        char text[32] = {};
        GetPrivateProfileStringA("Graphics", name, "", text, sizeof(text), ini.path.c_str());
        if (text[0]) g_graphicsIni.options.emplace_back(name, (int)strtol(text, nullptr, 0));
    }
}

// ---- music (see "Free roam" at the top of the file)

// every frame of a free roam: the music volume back to 0 should anything set it, handed to the voices in gameplay
void HoldMusicOff() {
    volatile float* music = (volatile float*)kMusicVolume;
    if (*music != 0.0f) {
        static int logged = 0;
        if (logged++ < 3) ft::Log("free roam: music volume %.2f -> 0", *music);
        *music = 0.0f;
        g_musicMuted = false;
    }
    if (g_musicMuted || *(volatile int*)kGameFlowState != kFlowRacing) return;
    g_musicMuted = true;
    uint8_t* world = *(uint8_t* volatile*)kAudioWorld;
    if (!world || !ft::Matches(kApplyVolumes, kApplyVolumesStart, sizeof(kApplyVolumesStart))) return;
    *(uintptr_t*)(world + 8) = kVolumes;   // as each caller of 0x51CC10 does
    ((void(__stdcall*)(void*))kApplyVolumes)(world);
    ft::Log("free roam: music volume 0 handed to the voices");
}

void PlaceCar(void* car, float* position, int angle) {
    __asm {
        push 0              // ghost: no
        push 0              // speed 0.0f
        mov ecx, car
        mov edx, position
        mov eax, angle
        mov ebx, kPlaceCar
        call ebx
    }
}

void MoveStreamer(float* position) {
    __asm {
        mov ecx, position
        mov edx, kStreamer
        xor eax, eax
        mov ebx, kMoveStreamer
        call ebx
    }
}

// the start markers, ForwardStartingPosition 0 onto the pose, the rest of the grid turned and moved with it
void MoveGrid(uint8_t* chunk) {
    const uint32_t size = *(const uint32_t*)(chunk + 4);
    uint8_t* first = (uint8_t*)(((uintptr_t)chunk + 0x17) & ~(uintptr_t)0xF);   // as the loader (0x441EE1)
    const int n = (int)((chunk + 8 + size - first) / 0x30);
    const uint8_t* ref = nullptr;
    for (int i = 0; i < n && !ref; i++) {
        const uint8_t* r = first + i * 0x30;
        if (*(const uint32_t*)(r + 8) == kForwardStartingPosition && *(const uint32_t*)(r + 0xC) == 0) ref = r;
    }
    if (!ref) {
        ft::Log("launch pose: no ForwardStartingPosition 0 among %d markers: the car is put there in gameplay", n);
        return;
    }
    const uint16_t angle = (uint16_t)((int)lroundf(g_pose.heading * 65536.0f / 360.0f) & 0xFFFF);
    const float rx = *(const float*)(ref + 0x10), ry = *(const float*)(ref + 0x14), rz = *(const float*)(ref + 0x18);
    const uint16_t turn = (uint16_t)(angle - *(const uint16_t*)(ref + 0x20));
    const float a = turn * 6.2831853f / 65536.0f, c = cosf(a), sn = sinf(a);
    for (int i = 0; i < n; i++) {
        uint8_t* r = first + i * 0x30;
        float* q = (float*)(r + 0x10);
        const float dx = q[0] - rx, dy = q[1] - ry, dz = q[2] - rz;
        q[0] = g_pose.x + dx * c - dy * sn;
        q[1] = g_pose.y + dx * sn + dy * c;
        q[2] = g_pose.z + dz;
        *(uint16_t*)(r + 0x20) = (uint16_t)(*(const uint16_t*)(r + 0x20) + turn);
    }
    g_pose.pending = false;   // the car starts there: nothing to put right in gameplay
    ft::Log("launch pose: %d start markers moved to (%.1f, %.1f, %.1f), bAngle %04X", n, g_pose.x, g_pose.y,
            g_pose.z, angle);
}

int __cdecl MarkersLoader(void* chunk) {
    if (g_pose.pending && *(const uint32_t*)chunk == kChunkMarkers) MoveGrid((uint8_t*)chunk);
    return g_markersLoader(chunk);
}

// Slot 12 (0x4457C0) loads a section's scenery, 0x80034100: its SceneryInfos (0x34102, 72 bytes, the model's hash
// for each of six detail levels first) are what the instances draw. An info naming a hidden model gets no model at
// all -- the way BayviewU1 hides scenery; taking instances out shifts indices something else keeps.
int __cdecl SceneryLoader(void* chunk) {
    if (g_freeRoam && !g_hide.empty() && *(const uint32_t*)chunk == 0x80034100u) {
        uint8_t* at = (uint8_t*)chunk + 8;
        uint8_t* end = at + *(const uint32_t*)((uint8_t*)chunk + 4);
        int hidden = 0;
        while (at + 8 <= end) {
            const uint32_t id = *(const uint32_t*)at, size = *(const uint32_t*)(at + 4);
            uint8_t* data = at + 8;
            uint8_t* stop = data + size;
            if (stop > end) break;
            if (id == 0x34102) {
                while (data + 4 <= stop && *(const uint32_t*)data == 0x11111111u) data += 4;
                for (uint8_t* info = data; info + 72 <= stop; info += 72) {
                    uint32_t* hashes = (uint32_t*)info;
                    bool hide = false;
                    for (int k = 0; k < 6 && !hide; k++)
                        hide = hashes[k] && std::binary_search(g_hide.begin(), g_hide.end(), hashes[k]);
                    if (hide) {
                        for (int k = 0; k < 6; k++) hashes[k] = 0;
                        hidden++;
                    }
                }
            }
            at = stop;
        }
        (void)hidden;   // no logging from inside the loader: it runs on the loading thread
    }
    return g_sceneryLoader(chunk);
}

// A free roam's track bundle: <game>\models\freeroam\TRACKB<n>.LZC, written by the editor at launch (every track's
// collision of the region together, without the race barriers), opened in place of TRACKS\TRACKB<n>.LZC.
HANDLE WINAPI CreateFileAHook(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security, DWORD how,
                              DWORD flags, HANDLE templ) {
    if (g_freeRoam && name) {
        const char* file = strrchr(name, '\\');
        const char* slash = strrchr(name, '/');
        if (slash > file) file = slash;
        file = file ? file + 1 : name;
        if (_strnicmp(file, "TRACKB", 6) == 0) {
            const std::string mine = GameFolder() + "\\models\\freeroam\\" + file;
            if (GetFileAttributesA(mine.c_str()) != INVALID_FILE_ATTRIBUTES) {
                const HANDLE handle = g_createFileA(mine.c_str(), access, share, security, how, flags, templ);
                if (handle != INVALID_HANDLE_VALUE) return handle;
            }
        }
    }
    return g_createFileA(name, access, share, security, how, flags, templ);
}

// The same for CreateFileW -- the one the game really uses: its file system (0x5C7FB9) turns the name into UTF-16
// and opens it with CreateFileW, falling back to CreateFileA only when that conversion fails.
using CreateFileWFn = HANDLE(WINAPI*)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
CreateFileWFn g_createFileW = nullptr;
volatile LONG g_bundleRedirects = 0;

HANDLE WINAPI CreateFileWHook(LPCWSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security, DWORD how,
                              DWORD flags, HANDLE templ) {
    if (g_freeRoam && name) {
        const wchar_t* file = wcsrchr(name, L'\\');
        const wchar_t* slash = wcsrchr(name, L'/');
        if (slash > file) file = slash;
        file = file ? file + 1 : name;
        if (_wcsnicmp(file, L"TRACKB", 6) == 0) {
            const std::string folder = GameFolder();
            const std::wstring mine = std::wstring(folder.begin(), folder.end()) + L"\\models\\freeroam\\" + file;
            if (GetFileAttributesW(mine.c_str()) != INVALID_FILE_ATTRIBUTES) {
                const HANDLE handle = g_createFileW(mine.c_str(), access, share, security, how, flags, templ);
                if (handle != INVALID_HANDLE_VALUE) {
                    InterlockedIncrement(&g_bundleRedirects);   // logged from the frame: no file work in here
                    return handle;
                }
            }
        }
    }
    return g_createFileW(name, access, share, security, how, flags, templ);
}

// Hooking the import table is not enough: a plugin loaded after this one (one that moves the user files, say)
// writes its own hook over the CreateFileW entry and calls the system's function, not what it found there. So the
// game's file system is taken at its two calls -- 0x5C7FDC (call [0x6971C4], CreateFileW) and 0x5C7FF7 (call
// [0x6971C0], CreateFileA) -- and the track bundle's name changed there; the call then goes on through whatever
// the import table holds by then, every other plugin's hook included.
HANDLE WINAPI FileSystemCreateFileW(LPCWSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security,
                                    DWORD how, DWORD flags, HANDLE templ) {
    const auto next = *(CreateFileWFn volatile*)kImportCreateFileW;
    if (g_freeRoam && name) {
        const wchar_t* file = wcsrchr(name, L'\\');
        const wchar_t* slash = wcsrchr(name, L'/');
        if (slash > file) file = slash;
        file = file ? file + 1 : name;
        if (_wcsnicmp(file, L"TRACKB", 6) == 0) {
            const std::string folder = GameFolder();
            const std::wstring mine = std::wstring(folder.begin(), folder.end()) + L"\\models\\freeroam\\" + file;
            if (GetFileAttributesW(mine.c_str()) != INVALID_FILE_ATTRIBUTES) {
                const HANDLE handle = next(mine.c_str(), access, share, security, how, flags, templ);
                if (handle != INVALID_HANDLE_VALUE) {
                    InterlockedIncrement(&g_bundleRedirects);
                    return handle;
                }
            }
        }
    }
    return next(name, access, share, security, how, flags, templ);
}

HANDLE WINAPI FileSystemCreateFileA(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security,
                                    DWORD how, DWORD flags, HANDLE templ) {
    const auto next = *(CreateFileAFn volatile*)kImportCreateFileA;
    if (g_freeRoam && name) {
        const char* file = strrchr(name, '\\');
        const char* slash = strrchr(name, '/');
        if (slash > file) file = slash;
        file = file ? file + 1 : name;
        if (_strnicmp(file, "TRACKB", 6) == 0) {
            const std::string mine = GameFolder() + "\\models\\freeroam\\" + file;
            if (GetFileAttributesA(mine.c_str()) != INVALID_FILE_ATTRIBUTES) {
                const HANDLE handle = next(mine.c_str(), access, share, security, how, flags, templ);
                if (handle != INVALID_HANDLE_VALUE) {
                    InterlockedIncrement(&g_bundleRedirects);
                    return handle;
                }
            }
        }
    }
    return next(name, access, share, security, how, flags, templ);
}

// "call dword ptr [import]" (FF 15 imm32, six bytes) made "call hook; nop".
bool TakeImportCall(uintptr_t site, uintptr_t import, void* hook) {
    uint8_t expected[6] = { 0xFF, 0x15 };
    std::memcpy(expected + 2, &import, 4);
    if (!ft::Matches(site, expected, sizeof(expected))) return false;
    uint8_t code[6] = { 0xE8 };
    const int32_t rel = (int32_t)((uintptr_t)hook - (site + 5));
    std::memcpy(code + 1, &rel, 4);
    code[5] = 0x90;
    ft::Write(site, code, sizeof(code));
    return true;
}

// 0x448440 for the pre-race: GLOBAL\PRERACEANIM.LZC is "not loaded" in a free roam
// the call at 0x409EF0 (stdcall, the player's map view): no track map in a free roam
__declspec(naked) void TrackMapStub() {
    __asm {
        cmp byte ptr [g_freeRoam], 0
        je draw
        ret 4
    draw:
        jmp g_trackMapNext
    }
}

int __cdecl PreRaceAnimLoaded(const char* name) {
    return g_freeRoam ? 0 : ((int(__cdecl*)(const char*))kIsLoaded)(name);
}

void __cdecl LaunchPoseFrame() {
    static LONG redirectsLogged = 0;
    if (g_bundleRedirects != redirectsLogged) {
        redirectsLogged = g_bundleRedirects;
        ft::Log("free roam: the track bundle was opened from models\\freeroam (%ld times)", redirectsLogged);
    }
    // In gameplay too: BayviewU1 writes its value back as a track loads, and
    // the countdown takes it when the pre-race starts it, in gameplay.
    if (g_freeRoam) {
        HoldMusicOff();
        static const float fast[2] = {0.01f, 0.01f};
        if (memcmp((const void*)kCountdown, fast, sizeof fast) != 0) ft::Write(kCountdown, fast, sizeof fast);
        // The race's start object ([0x736194]) took its length of each count (+0xC) from there when it was
        // set up, which can be before this ran; 0x434D10 compares the time since the count began with it.
        uint8_t* start = *(uint8_t* volatile*)kRaceStart;
        if (start && !IsBadWritePtr(start + 0xC, 4) && *(volatile float*)(start + 0xC) > 0.01f)
            *(volatile float*)(start + 0xC) = 0.01f;
    }
    // Esc closes the game, in front only (an Esc in the editor is the editor's)
    static bool escDown = false;
    const bool esc = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    if (g_freeRoam && esc && !escDown && GetForegroundWindow() == *(HWND volatile*)kGameWindow) {
        ft::Log("free roam: Esc, the game closes");
        ExitProcess(0);
    }
    escDown = esc;
    if (!g_pose.pending) return;
    const uint8_t* world = *(const uint8_t* const*)kWorld;
    void* car = world && *(volatile int*)kGameFlowState == kFlowRacing
                    ? *(void* const*)(world + kWorldPlayerCar) : nullptr;
    if (!car) { g_pose.readySince = 0; return; }
    if (!g_pose.readySince) { g_pose.readySince = GetTickCount() | 1; return; }
    if (GetTickCount() - g_pose.readySince < 1000) return;
    g_pose.pending = false;

    constexpr float kDropHeight = 1.0f;   // over the ground, to settle onto it
    float position[3] = {g_pose.x, g_pose.y, g_pose.z + kDropHeight};
    const float radians = g_pose.heading * 3.14159265f / 180.0f;
    const int angle = ((int(__cdecl*)(float, float))kAngleOf)(cosf(radians), sinf(radians)) & 0xFFFF;
    PlaceCar(car, position, angle);
    MoveStreamer(position);
    ft::Log("launch pose: car placed (bAngle %d)", angle);
}

// the main loop's call at 0x447A10, whatever it calls (another mod's hook too), after the pose
__declspec(naked) void FrameHook() {
    __asm {
        pushad
        call LaunchPoseFrame
        popad
        jmp g_frameNext
    }
}

void InstallFreeRoam() {
    const uintptr_t tick = kFlybyJumpTable + 2 * 4;
    if (*(const uint32_t*)tick != kFlybyTick || *(const uint32_t*)(kFlybyJumpTable + 3 * 4) != kFlybySkip ||
        !ft::IsCallTo(kPreRaceAnimCall, kIsLoaded)) {
        ft::Log("free roam: the race coordinator differs from v1.4 (another mod?): the start is the race's");
        g_freeRoam = false;
        return;
    }
    const uint32_t skip = kFlybySkip;
    ft::Write(tick, &skip, 4);
    ft::WriteBranch(kPreRaceAnimCall, (void*)&PreRaceAnimLoaded, 0xE8);
    // The start NIS (the W sections' cut-scene at the start line): the
    // pre-race's tick waits for it to end; its skip starts the countdown and
    // the race at once. The tick is pointed at the skip, as the fly-by's.
    const uintptr_t preRaceTick = kPreRaceJumpTable + 2 * 4;
    if (*(const uint32_t*)preRaceTick == kPreRaceTick && *(const uint32_t*)(kPreRaceJumpTable + 3 * 4) == kPreRaceSkip) {
        const uint32_t preRaceSkip = kPreRaceSkip;
        ft::Write(preRaceTick, &preRaceSkip, 4);
    } else {
        ft::Log("free roam: the pre-race's handlers differ from v1.4 (another mod?): the start cut-scene stays");
    }
    if (*(const uint8_t*)kTrackMapCall == 0xE8) {
        g_trackMapNext = ft::CallTarget(kTrackMapCall);
        ft::WriteBranch(kTrackMapCall, (void*)&TrackMapStub, 0xE8);
    } else {
        ft::Log("free roam: 0x409EF0 is not a call (another mod?): the track map stays");
    }
    ft::Log("free roam: no fly-by, no pre-race animation or cut-scene, a 0.01 s countdown, no track map; Esc closes the game");

    // the race furniture's scenery: whatever slot 12 holds is called after
    if (!g_hide.empty()) {
        g_sceneryLoader = *(Loader*)(kLoaderTable + kSceneryLoaderSlot * 4);
        const uintptr_t to = (uintptr_t)&SceneryLoader;
        ft::Write(kLoaderTable + kSceneryLoaderSlot * 4, &to, 4);
        ft::Log("free roam: %d race furniture models hidden", (int)g_hide.size());
    }
    // the free roam's track bundle, when the editor wrote one
    g_createFileA = (CreateFileAFn)ft::HookImport("KERNEL32.dll", "CreateFileA", (void*)&CreateFileAHook);
    g_createFileW = (CreateFileWFn)ft::HookImport("KERNEL32.dll", "CreateFileW", (void*)&CreateFileWHook);
    const bool takenW = TakeImportCall(kFileSystemCreateW, kImportCreateFileW, (void*)&FileSystemCreateFileW);
    const bool takenA = TakeImportCall(kFileSystemCreateA, kImportCreateFileA, (void*)&FileSystemCreateFileA);
    ft::Log("free roam: the game's file system calls taken: CreateFileW %s, CreateFileA %s", takenW ? "yes" : "NO",
            takenA ? "yes" : "NO");
    const std::string bundle = GameFolder() + "\\models\\freeroam\\TRACKB" + std::to_string(g_track) + ".LZC";
    if (!g_createFileW && !takenW)
        ft::Log("free roam: CreateFileW is hooked by something else already: the track's own collision is used");
    else if (GetFileAttributesA(bundle.c_str()) != INVALID_FILE_ATTRIBUTES)
        ft::Log("free roam: the collision of the editor's %s (every track of the region, no race barriers)",
                bundle.c_str());
    else
        ft::Log("free roam: no %s: the track's own collision is used", bundle.c_str());
}

void InstallLaunchPose() {
    ReadLaunchPose();
    if (g_freeRoam) InstallFreeRoam();
    if (!g_pose.pending && !g_freeRoam) return;
    // the grid: whatever slot 2 holds (BayviewU1 may have wrapped it already) is called after the move
    if (g_pose.pending) {
        g_markersLoader = *(Loader*)(kLoaderTable + kMarkersSlot * 4);
        const uintptr_t to = (uintptr_t)&MarkersLoader;
        ft::Write(kLoaderTable + kMarkersSlot * 4, &to, 4);
    }
    if (*(const uint8_t*)kFrameCall != 0xE8) {
        ft::Log("launch pose: 0x447A10 is not a call (another mod?): no fallback placing, countdown as it is");
        return;
    }
    g_frameNext = ft::CallTarget(kFrameCall);
    ft::WriteBranch(kFrameCall, (void*)&FrameHook, 0xE8);
}

// 0x4B6390 with the [FastTest] settings in place of the boot's
void __cdecl StartSkipFE() {
    ApplyGraphics();
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
    ReadGraphicsIni(ini);
    InstallLaunchPose();   // may name the track: the editor's region
    for (char& c : g_car) c = (char)toupper((uint8_t)c);
    if (ft::IsCallTo(kSkipFEStartCall, kSkipFEStart)) ft::WriteBranch(kSkipFEStartCall, (void*)&StartSkipFE);
    else ft::Log("the call to 0x4B6390 was changed by another mod: the game's defaults are used");
    ft::Log("SkipFE on: track %d, laps %d, AI cars %d, car %s", g_track, g_laps, g_aiCars,
            g_car.empty() ? "(default)" : g_car.c_str());

    g_window.Read(ini);
    if (g_embedded) {
        g_window.monitor = 0;
        g_window.width = g_window.height = 0;
        // The game is the window in front while the editor shows it: it keeps the focus it is given.
        g_window.background = false;
        ft::Log("window: in the editor's view (its place, size and focus are the editor's)");
    }
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
