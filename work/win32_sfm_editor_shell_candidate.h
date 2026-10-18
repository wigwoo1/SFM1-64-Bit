#pragma once
#include <windows.h>
#include <commctrl.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>
#include "session_timeline_segment.h"

struct SfmEditorElementRowCandidate {
    uint32_t index = 0;
    uint32_t attributeCount = 0;
    uint32_t hierarchyDepth = 0;
    uint32_t additionalParents = 0;
    bool dag = false;
    uint32_t childCount = 0;
    std::string type;
    std::string name;
    std::string parent;
    std::vector<std::string> attributeDetails;
};
struct SfmEditorGraphKeyCandidate { int32_t tick = 0; float value = 0; };
struct SfmEditorGraphCurveCandidate { uint32_t element = 0xffffffffu; std::string label; std::vector<SfmEditorGraphKeyCandidate> keys; uint8_t vectorComponent = 0xff; };
struct SfmEditorGraphKeyEditRequestCandidate { uint32_t element = 0xffffffffu; size_t index = SIZE_MAX; float value = 0; int32_t tick = 0; uint8_t vectorComponent = 0xff; };
inline POINT sfmEditorGraphPointCandidate(const std::vector<SfmEditorGraphKeyCandidate>& keys, size_t index,
                                           const RECT& plot, float overrideValue, int32_t overrideTick, bool useOverride);
inline unsigned sfmEditorGraphSelectionModeCandidate(bool control, bool alt) noexcept { return control ? (alt ? 1u : 3u) : (alt ? 2u : 0u); }
inline void applySfmEditorGraphSelectionModeCandidate(std::vector<size_t>& selection, size_t index, unsigned mode) {
    auto found = std::find(selection.begin(), selection.end(), index);
    switch (mode) {
    case 0: selection.assign(1, index); break;
    case 1: if (found == selection.end()) selection.push_back(index); break;
    case 2: if (found == selection.end()) selection.push_back(index); else selection.erase(found); break;
    case 3: if (found != selection.end()) selection.erase(found); break;
    default: throw std::runtime_error("unknown graph key selection mode");
    }
}
inline RECT normalizeSfmEditorGraphSelectionRectCandidate(POINT a, POINT b) noexcept {
    return RECT{std::min(a.x,b.x), std::min(a.y,b.y), std::max(a.x,b.x), std::max(a.y,b.y)};
}
inline std::vector<size_t> sfmEditorGraphKeysInRectCandidate(const std::vector<SfmEditorGraphKeyCandidate>& keys,
                                                              const RECT& plot, RECT selection) {
    selection.left = std::max(selection.left, plot.left); selection.top = std::max(selection.top, plot.top);
    selection.right = std::min(selection.right, plot.right); selection.bottom = std::min(selection.bottom, plot.bottom);
    std::vector<size_t> hits;
    if (selection.left > selection.right || selection.top > selection.bottom) return hits;
    for (size_t i = 0; i < keys.size(); ++i) {
        const auto p = sfmEditorGraphPointCandidate(keys, i, plot, 0.0f, 0, false);
        if (p.x >= selection.left && p.x <= selection.right && p.y >= selection.top && p.y <= selection.bottom) hits.push_back(i);
    }
    return hits;
}
enum class SfmEditorTimeDisplayModeCandidate : uint32_t { TotalSeconds = 0, TotalFrames = 1, TimecodeSeconds = 2, TimecodeFrames = 3 };
inline const char* sfmEditorTimeDisplayModeLabelCandidate(SfmEditorTimeDisplayModeCandidate mode) {
    switch (mode) {
    case SfmEditorTimeDisplayModeCandidate::TotalSeconds: return "Total Seconds";
    case SfmEditorTimeDisplayModeCandidate::TotalFrames: return "Total Frames";
    case SfmEditorTimeDisplayModeCandidate::TimecodeSeconds: return "Timecode:Seconds";
    case SfmEditorTimeDisplayModeCandidate::TimecodeFrames: return "Timecode:Frames";
    }
    return "Total Frames";
}
inline SfmEditorTimeDisplayModeCandidate sfmEditorNextTimeDisplayModeCandidate(SfmEditorTimeDisplayModeCandidate mode) {
    const auto value = uint32_t(mode);
    return SfmEditorTimeDisplayModeCandidate((value + 1u) % 4u);
}
inline std::string sfmEditorFormatTimelineTimeCandidate(SfmEditorTimeDisplayModeCandidate mode, size_t zeroBasedFrame,
                                                         size_t frameCount, double framesPerSecond) {
    if (!frameCount || !std::isfinite(framesPerSecond) || framesPerSecond <= 0) return "No animation frames loaded";
    const size_t frame = std::min(zeroBasedFrame, frameCount - 1);
    const double seconds = double(frame + 1) / framesPerSecond;
    char text[96]{};
    switch (mode) {
    case SfmEditorTimeDisplayModeCandidate::TotalSeconds:
        std::snprintf(text, sizeof(text), "%.3f s", seconds);
        break;
    case SfmEditorTimeDisplayModeCandidate::TotalFrames:
        std::snprintf(text, sizeof(text), "%zu frames", frame + 1);
        break;
    case SfmEditorTimeDisplayModeCandidate::TimecodeSeconds: {
        const auto hours = uint64_t(seconds / 3600.0);
        const auto minutes = uint64_t(seconds / 60.0) % 60;
        const double secondPart = seconds - double(hours * 3600 + minutes * 60);
        std::snprintf(text, sizeof(text), "%02llu:%02llu:%06.3f", (unsigned long long)hours,
                      (unsigned long long)minutes, secondPart);
        break;
    }
    case SfmEditorTimeDisplayModeCandidate::TimecodeFrames: {
        const auto nominalRate = uint64_t(std::max(1.0, std::floor(framesPerSecond + 0.5)));
        const auto wholeSeconds = uint64_t(seconds);
        const auto hours = wholeSeconds / 3600;
        const auto minutes = (wholeSeconds / 60) % 60;
        const auto framePart = uint64_t(std::floor((seconds - std::floor(seconds)) * double(nominalRate) + 1e-9));
        std::snprintf(text, sizeof(text), "%02llu:%02llu:%02llu:%02llu", (unsigned long long)hours,
                      (unsigned long long)minutes, (unsigned long long)(wholeSeconds % 60),
                      (unsigned long long)framePart);
        break;
    }
    }
    return text;
}

struct SfmEditorDockLayoutCandidate {
    int leftWidth = 0;
    int motionWidth = 0;
    int viewportLeft = 0;
    int viewportWidth = 0;
    int timelineLeft = 0;
    int timelineWidth = 0;
    int timelineTop = 0;
    int timelineHeight = 0;
};

inline SfmEditorDockLayoutCandidate sfmEditorDockLayoutCandidate(int width, int height,
                                                                uint32_t rootSplitBasisPoints = 23819,
                                                                uint32_t timeSplitBasisPoints = 33233,
                                                                uint32_t uiScalePercent = 100) {
    if (width < 820 || height < 420) throw std::runtime_error("editor window is below its minimum dock size");
    if (rootSplitBasisPoints < 15000 || rootSplitBasisPoints > 50000 ||
        timeSplitBasisPoints < 18000 || timeSplitBasisPoints > 50000 ||
        uiScalePercent < 38 || uiScalePercent > 2000)
        throw std::runtime_error("editor dock split ratio outside supported range");
    const auto scaled = [uiScalePercent](int value) { return int((int64_t(value) * uiScalePercent + 50) / 100); };
    const int left = std::max(scaled(240), int((int64_t(width) * rootSplitBasisPoints + 50000) / 100000));
    const int motion = std::clamp(width * 22 / 100, scaled(240), scaled(320));
    const int timeline = std::max(scaled(145), int((int64_t(height) * timeSplitBasisPoints + 50000) / 100000));
    if (left + motion + scaled(300) > width) throw std::runtime_error("editor docks leave no usable viewport");
    return {left, motion, left + motion, width - left - motion,
            left, width - left, height - timeline, timeline};
}
inline constexpr UINT SFM_EDITOR_SEEK_FRAME = WM_APP + 0x164;
inline constexpr UINT_PTR SFM_EDITOR_FILTER_REFRESH_TIMER = 0x53464D;
inline constexpr UINT SFM_EDITOR_FILTER_REFRESH_DELAY_MS = 180;
inline size_t timelineFrameAtPixelCandidate(int x, int left, int right, size_t count) {
    if (!count || right <= left) throw std::runtime_error("invalid timeline ruler mapping");
    if (x <= left || count == 1) return 0;
    if (x >= right) return count - 1;
    const auto span = uint64_t(right - left);
    const auto offset = uint64_t(x - left);
    return size_t((offset * uint64_t(count - 1) + span / 2) / span);
}
inline int timelinePixelAtFrameCandidate(size_t frame, int left, int right, size_t count) {
    if (!count || right < left) throw std::runtime_error("invalid timeline ruler projection");
    if (count == 1) return left;
    const size_t selected = std::min(frame, count - 1);
    return left + int((uint64_t(selected) * uint64_t(right - left) + uint64_t(count - 1) / 2) / uint64_t(count - 1));
}

struct SfmEditorShellCandidate {
    HWND parent = nullptr;
    HWND animationTab = nullptr;
    HWND elementTab = nullptr;
    HWND filter = nullptr;
    HWND elements = nullptr;
    HWND details = nullptr;
    HWND motionTab = nullptr;
    HWND graphTab = nullptr;
    HWND clipTab = nullptr;
    HWND modeStatus = nullptr;
    HWND graphCurves = nullptr;
    HWND clipShots = nullptr;
    HWND verticalDockSplitter = nullptr;
    HWND horizontalDockSplitter = nullptr;
    std::function<bool(uint32_t,size_t,int32_t,float,uint8_t)> graphKeyQueue;
    std::function<void(uint32_t,size_t,uint8_t)> graphKeyDialog;
    std::function<bool(uint32_t,const std::vector<size_t>&)> graphKeyDeleteQueue;
    HWND timelinePanel = nullptr;
    HWND primaryViewportControls = nullptr;
    bool primaryViewportControlsVisible = true;
    bool showFocalPlane = true;
    bool showCameraFrustum = true;
    bool showViewTargets = true;
    UINT activeViewportToolMode = 0;
    bool elementViewer = false;
    unsigned editorMode = 0;
    size_t frameCount = 0;
    size_t selectedFrame = 0;
    double framesPerSecond = 24.0;
    SfmEditorTimeDisplayModeCandidate timeDisplayMode = SfmEditorTimeDisplayModeCandidate::TotalFrames;
    SfmEditorTimeDisplayModeCandidate globalTimeDisplayMode = SfmEditorTimeDisplayModeCandidate::TotalFrames;
    SfmEditorTimeDisplayModeCandidate localTimeDisplayMode = SfmEditorTimeDisplayModeCandidate::TotalFrames;
    size_t totalSceneElements = 0;
    uint32_t selectedElement = 0xffffffffu;
    std::vector<uint32_t> selectedElements;
    std::vector<SfmEditorElementRowCandidate> rows;
    std::vector<size_t> visibleRows;
    std::vector<SfmEditorTimelineSegmentCandidate> timelineSegments;
    std::vector<SfmEditorTimelineLaneCandidate> timelineLanes;
    size_t selectedTimelineLane = SIZE_MAX;
    uint32_t selectedTimelineClip = UINT32_MAX;
    size_t timelineDragPointerFrame = 0;
    size_t timelineDragClipStartFrame = 0;
    size_t pendingTimelineClipMoveStartFrame = SIZE_MAX;
    int timelineScrollOffsetPixels = 0;
    int timelineWheelDeltaRemainder = 0;
    std::vector<size_t> bookmarkFrames;
    std::vector<SfmEditorGraphKeyCandidate> graphKeys;
    std::string graphLayerLabel;
    std::string sceneAssetWarning;
    std::vector<SfmEditorGraphCurveCandidate> graphCurveList;
    size_t selectedGraphCurve = SIZE_MAX;
    size_t selectedGraphKey = SIZE_MAX;
    std::vector<size_t> selectedGraphKeys;
    bool draggingGraphKey = false;
    bool selectingGraphKeys = false;
    POINT graphSelectionStart{};
    POINT graphSelectionEnd{};
    unsigned graphSelectionMode = 0;
    float graphPreviewValue = 0;
    int32_t graphPreviewTick = 0;
    bool draggingTimeline = false;
    bool draggingTimelineClip = false;
    bool showTimelineSnapMarker = false;
    bool showTimelineToolTips = true;
    bool showTimelineOffFrameWarning = true;
    bool autoKeyBookmarks = true;
    bool engineSyncedToFixedFps = true;
    bool hideWaveformOnSelect = false;
    uint32_t rootSplitBasisPoints = 23819;
    uint32_t timeSplitBasisPoints = 33233;
    uint32_t uiScalePercent = 100;
    uint32_t dpiScalePercent = 100;
    std::string dockSettingsPath;
    HFONT uiFont = nullptr;
    HFONT compactTabFont = nullptr;
    bool draggingRootSplitter = false;
    bool draggingTimeSplitter = false;
};
inline bool applySfmEditorTimeDisplayShortcutCandidate(SfmEditorShellCandidate& shell, UINT key,
                                                        bool shift, bool repeat) {
    if (key != VK_LEFT || repeat) return false;
    if (shift) {
        shell.localTimeDisplayMode = sfmEditorNextTimeDisplayModeCandidate(shell.localTimeDisplayMode);
        shell.timeDisplayMode = shell.localTimeDisplayMode;
    } else {
        shell.globalTimeDisplayMode = sfmEditorNextTimeDisplayModeCandidate(shell.globalTimeDisplayMode);
        shell.timeDisplayMode = shell.globalTimeDisplayMode;
    }
    return true;
}
inline SfmEditorShellCandidate sfmEditorShellCandidate;
inline void setSfmEditorMissingAssetSummaryCandidate(std::string summary){
 sfmEditorShellCandidate.sceneAssetWarning=std::move(summary);
 if(sfmEditorShellCandidate.modeStatus)InvalidateRect(sfmEditorShellCandidate.modeStatus,nullptr,FALSE);
}
inline HWND vulkanTimelineSlider = nullptr;
inline bool applySfmEditorTimeDisplayCommandCandidate(SfmEditorShellCandidate& shell, UINT command) {
    if (command >= 41101 && command <= 41104) {
        shell.globalTimeDisplayMode = SfmEditorTimeDisplayModeCandidate(command - 41101);
        shell.timeDisplayMode = shell.globalTimeDisplayMode;
        return true;
    }
    if (command >= 41111 && command <= 41114) {
        shell.localTimeDisplayMode = SfmEditorTimeDisplayModeCandidate(command - 41111);
        shell.timeDisplayMode = shell.localTimeDisplayMode;
        return true;
    }
    return false;
}
inline constexpr UINT SFM_EDITOR_ANIMATION_TAB = 41200;
inline constexpr UINT SFM_EDITOR_ELEMENT_TAB = 41201;
inline constexpr UINT SFM_EDITOR_FILTER = 41202;
inline constexpr UINT SFM_EDITOR_ELEMENTS = 41203;
inline constexpr UINT SFM_EDITOR_DETAILS = 41204;
inline constexpr UINT SFM_EDITOR_MOTION_TAB = 41205;
inline constexpr UINT SFM_EDITOR_GRAPH_TAB = 41206;
inline constexpr UINT SFM_EDITOR_CLIP_TAB = 41207;
inline constexpr UINT SFM_EDITOR_MODE_STATUS = 41208;
inline constexpr UINT SFM_EDITOR_GRAPH_CURVES = 41209;
inline constexpr UINT SFM_EDITOR_CLIP_SHOTS = 41210;
inline constexpr UINT SFM_EDITOR_GRAPH_KEY_EDIT = WM_APP + 0x165;
inline constexpr UINT SFM_EDITOR_VIEW_PRIMARY_CONTROLS = 41300;
inline constexpr UINT SFM_EDITOR_VIEW_FULLSCREEN = 41301;
inline constexpr UINT SFM_EDITOR_VIEWPORT_HOME = 41310;
inline constexpr UINT SFM_EDITOR_VIEWPORT_PREVIOUS = 41311;
inline constexpr UINT SFM_EDITOR_VIEWPORT_PLAY = 41312;
inline constexpr UINT SFM_EDITOR_VIEWPORT_NEXT = 41313;
inline constexpr UINT SFM_EDITOR_VIEWPORT_END = 41314;
inline constexpr UINT SFM_EDITOR_VIEWPORT_MODE_MENU = 41320;
inline constexpr UINT SFM_EDITOR_VIEWPORT_MODE_SELECTION = 41321;
inline constexpr UINT SFM_EDITOR_VIEWPORT_MODE_TRANSLATION = 41322;
inline constexpr UINT SFM_EDITOR_VIEWPORT_MODE_GLOBAL_TRANSLATION = 41323;
inline constexpr UINT SFM_EDITOR_VIEWPORT_MODE_ROTATION = 41324;
inline constexpr UINT SFM_EDITOR_VIEWPORT_MODE_GLOBAL_ROTATION = 41325;
inline constexpr UINT SFM_EDITOR_VIEWPORT_MODE_SCREEN = 41326;
inline constexpr UINT SFM_EDITOR_VIEWPORT_DISPLAY_FOCAL_PLANE = 41330;
inline constexpr UINT SFM_EDITOR_VIEWPORT_DISPLAY_CAMERA_FRUSTUM = 41331;
inline constexpr UINT SFM_EDITOR_VIEWPORT_DISPLAY_VIEW_TARGETS = 41332;
inline constexpr UINT SFM_EDITOR_PRIMARY_VIEWPORT_CONTROLS = 41211;
inline constexpr UINT SFM_EDITOR_UI_SCALE_75 = 41030;
inline constexpr UINT SFM_EDITOR_UI_SCALE_100 = 41031;
inline constexpr UINT SFM_EDITOR_UI_SCALE_125 = 41032;
inline constexpr UINT SFM_EDITOR_UI_SCALE_150 = 41033;
inline constexpr UINT SFM_EDITOR_UI_SCALE_175 = 41034;
inline constexpr UINT SFM_EDITOR_UI_SCALE_200 = 41035;
inline constexpr UINT SFM_EDITOR_UI_SCALE_250 = 41036;
inline constexpr UINT SFM_EDITOR_TRACK_GROUP_VISIBILITY = 41340;
inline constexpr UINT SFM_EDITOR_TIMELINE_SHOW_SNAP_MARKER = 41350;
inline constexpr UINT SFM_EDITOR_TIMELINE_SHOW_TOOLTIPS = 41351;
inline constexpr UINT SFM_EDITOR_TIMELINE_SHOW_OFF_FRAME_WARNING = 41352;
inline constexpr UINT SFM_EDITOR_TIMELINE_AUTOKEY_BOOKMARKS = 41353;
inline constexpr UINT SFM_EDITOR_TIMELINE_FIXED_FPS = 41354;
inline constexpr UINT SFM_EDITOR_TIMELINE_HIDE_WAVEFORM = 41355;
inline uint32_t sfmEditorEffectiveScalePercentCandidate(const SfmEditorShellCandidate& shell) {
    if (shell.uiScalePercent < 75 || shell.uiScalePercent > 250 ||
        shell.dpiScalePercent < 50 || shell.dpiScalePercent > 800)
        throw std::runtime_error("UI scale or display DPI outside supported range");
    return uint32_t((uint64_t(shell.uiScalePercent) * shell.dpiScalePercent + 50) / 100);
}
inline bool setSfmEditorDpiScaleCandidate(SfmEditorShellCandidate& shell, uint32_t dpi) {
    if (dpi < 48 || dpi > 768) return false;
    shell.dpiScalePercent = uint32_t((uint64_t(dpi) * 100 + 48) / 96);
    return true;
}
inline void enableSfmEditorPerMonitorDpiAwarenessCandidate() {
    using SetAwarenessFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
    const auto user32 = GetModuleHandleW(L"user32.dll");
    const auto setAwareness = user32 ? reinterpret_cast<SetAwarenessFn>(GetProcAddress(user32, "SetProcessDpiAwarenessContext")) : nullptr;
    if (setAwareness) {
        if (setAwareness(reinterpret_cast<DPI_AWARENESS_CONTEXT>(-4))) return; // Per-monitor V2.
        const DWORD v2Error = GetLastError();
        if (v2Error == ERROR_ACCESS_DENIED) return; // A manifest or host already fixed the process context.
        if (setAwareness(reinterpret_cast<DPI_AWARENESS_CONTEXT>(-3))) return; // Per-monitor V1 fallback.
        const DWORD v1Error = GetLastError();
        if (v1Error == ERROR_ACCESS_DENIED) return;
    }
    if (SetProcessDPIAware()) return; // Legacy system-DPI fallback for older Windows.
    if (GetLastError() == ERROR_ACCESS_DENIED) return;
    throw std::runtime_error("per-monitor DPI awareness could not be enabled");
}
inline int sfmEditorScaleMetricCandidate(const SfmEditorShellCandidate& shell, int pixels) {
    const auto effective = sfmEditorEffectiveScalePercentCandidate(shell);
    return int((int64_t(pixels) * effective + 50) / 100);
}
inline int sfmEditorScaleMetricCandidate(int pixels) {
    return sfmEditorScaleMetricCandidate(sfmEditorShellCandidate, pixels);
}
inline void layoutSfmEditorShellCandidate(HWND parent);
inline void handleSfmEditorCommandCandidate(HWND parent, WPARAM command, LPARAM source);
inline LRESULT CALLBACK sfmEditorGraphSubclassProcCandidate(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
inline LRESULT CALLBACK sfmEditorDockSplitterSubclassProcCandidate(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
inline LRESULT CALLBACK sfmEditorPrimaryControlsSubclassProcCandidate(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);

inline UINT sfmEditorPrimaryControlCommandAtCandidate(int x, int inset, int modeWidth, int gap, int buttonWidth) noexcept {
    if (inset < 0 || modeWidth <= 0 || gap < 0 || buttonWidth <= 0 || x < inset) return 0;
    const int toolEnd = inset + modeWidth;
    if (x < toolEnd) return SFM_EDITOR_VIEWPORT_MODE_MENU;
    const int firstTransport = toolEnd + gap;
    if (x < firstTransport) return 0;
    const int index = (x - firstTransport) / buttonWidth;
    if (index < 0 || index > 4) return 0;
    return SFM_EDITOR_VIEWPORT_HOME + UINT(index);
}

inline const char* sfmEditorViewportModeNameCandidate(UINT mode) noexcept {
    static constexpr const char* modes[] = {"Selection", "Translation", "Global Translation", "Rotation", "Global Rotation", "Screen"};
    return mode < std::size(modes) ? modes[mode] : modes[0];
}

inline const char* sfmEditorViewportModeMenuLabelCandidate(UINT mode) noexcept {
    static constexpr const char* labels[] = {
        "Selection\tQ", "Translation\tW", "Global Translation\tShift+W",
        "Rotation\tE", "Global Rotation\tShift+E", "Screen\tR"
    };
    return mode < std::size(labels) ? labels[mode] : labels[0];
}

inline bool sfmEditorViewportModeFromCommandCandidate(UINT command, UINT& mode) noexcept {
    if (command < SFM_EDITOR_VIEWPORT_MODE_SELECTION || command > SFM_EDITOR_VIEWPORT_MODE_SCREEN) return false;
    mode = command - SFM_EDITOR_VIEWPORT_MODE_SELECTION;
    return true;
}

inline bool applySfmEditorViewportModeCommandCandidate(SfmEditorShellCandidate& shell, UINT command) noexcept {
    UINT mode = 0;
    if (!sfmEditorViewportModeFromCommandCandidate(command, mode)) return false;
    shell.activeViewportToolMode = mode;
    return true;
}

inline HACCEL createSfmEditorAcceleratorTableCandidate() {
    const ACCEL keys[] = {
        {FVIRTKEY | FCONTROL, 'T', 41001},
        {FVIRTKEY | FCONTROL, 'Z', 41002},
        {FVIRTKEY | FCONTROL, 'Y', 41003},
        {FVIRTKEY | FCONTROL, 'S', 41022},
        {FVIRTKEY | FCONTROL | FSHIFT, 'S', 41004},
        {FVIRTKEY, VK_F2, SFM_EDITOR_CLIP_TAB},
        {FVIRTKEY, VK_F3, SFM_EDITOR_MOTION_TAB},
        {FVIRTKEY, VK_F4, SFM_EDITOR_GRAPH_TAB},
        {FVIRTKEY, VK_F9, SFM_EDITOR_ELEMENT_TAB},
        {FVIRTKEY, 'Q', SFM_EDITOR_VIEWPORT_MODE_SELECTION},
        {FVIRTKEY, 'W', SFM_EDITOR_VIEWPORT_MODE_TRANSLATION},
        {FVIRTKEY | FSHIFT, 'W', SFM_EDITOR_VIEWPORT_MODE_GLOBAL_TRANSLATION},
        {FVIRTKEY, 'E', SFM_EDITOR_VIEWPORT_MODE_ROTATION},
        {FVIRTKEY | FSHIFT, 'E', SFM_EDITOR_VIEWPORT_MODE_GLOBAL_ROTATION},
        {FVIRTKEY, 'R', SFM_EDITOR_VIEWPORT_MODE_SCREEN},
    };
    return CreateAcceleratorTableA(const_cast<LPACCEL>(keys), int(std::size(keys)));
}

inline bool applySfmEditorViewportDisplayCommandCandidate(SfmEditorShellCandidate& shell, UINT command) noexcept {
    bool* setting = command == SFM_EDITOR_VIEWPORT_DISPLAY_FOCAL_PLANE ? &shell.showFocalPlane :
        command == SFM_EDITOR_VIEWPORT_DISPLAY_CAMERA_FRUSTUM ? &shell.showCameraFrustum :
        command == SFM_EDITOR_VIEWPORT_DISPLAY_VIEW_TARGETS ? &shell.showViewTargets : nullptr;
    if (!setting) return false;
    *setting = !*setting;
    return true;
}

struct SfmEditorFullscreenStateCandidate {
    bool active = false;
    LONG_PTR style = 0;
    LONG_PTR extendedStyle = 0;
    WINDOWPLACEMENT placement{sizeof(WINDOWPLACEMENT)};
};

inline const char* sfmEditorFullscreenMenuLabelCandidate(bool active) noexcept {
    return active ? "Show Normal" : "Show Fullscreen";
}

inline bool setSfmEditorFullscreenCandidate(HWND window, SfmEditorFullscreenStateCandidate& state,
                                             bool fullscreen) noexcept {
    if (!window || !IsWindow(window)) return false;
    if (fullscreen == state.active) return true;
    if (fullscreen) {
        WINDOWPLACEMENT placement{sizeof(WINDOWPLACEMENT)};
        if (!GetWindowPlacement(window, &placement)) return false;
        const LONG_PTR style = GetWindowLongPtrW(window, GWL_STYLE);
        const LONG_PTR extendedStyle = GetWindowLongPtrW(window, GWL_EXSTYLE);
        if (!style && GetLastError() != ERROR_SUCCESS) return false;
        MONITORINFO monitor{sizeof(MONITORINFO)};
        const HMONITOR handle = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
        if (!handle || !GetMonitorInfoW(handle, &monitor)) return false;
        SetLastError(ERROR_SUCCESS);
        const LONG_PTR result = SetWindowLongPtrW(window, GWL_STYLE,
            (style & ~LONG_PTR(WS_OVERLAPPEDWINDOW)) | WS_POPUP);
        if (!result && GetLastError() != ERROR_SUCCESS) return false;
        if (!SetWindowPos(window, HWND_TOP, monitor.rcMonitor.left, monitor.rcMonitor.top,
            monitor.rcMonitor.right - monitor.rcMonitor.left,
            monitor.rcMonitor.bottom - monitor.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_NOOWNERZORDER)) {
            SetWindowLongPtrW(window, GWL_STYLE, style);
            SetWindowLongPtrW(window, GWL_EXSTYLE, extendedStyle);
            SetWindowPlacement(window, &placement);
            SetWindowPos(window, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            return false;
        }
        state.style = style;
        state.extendedStyle = extendedStyle;
        state.placement = placement;
        state.active = true;
        return true;
    }
    SetLastError(ERROR_SUCCESS);
    const LONG_PTR result = SetWindowLongPtrW(window, GWL_STYLE, state.style);
    if (!result && GetLastError() != ERROR_SUCCESS) return false;
    SetLastError(ERROR_SUCCESS);
    const LONG_PTR extendedResult = SetWindowLongPtrW(window, GWL_EXSTYLE, state.extendedStyle);
    if (!extendedResult && GetLastError() != ERROR_SUCCESS) return false;
    if (!SetWindowPlacement(window, &state.placement)) return false;
    SetLastError(ERROR_SUCCESS);
    const LONG_PTR restoredStyle = SetWindowLongPtrW(window, GWL_STYLE, state.style);
    if (!restoredStyle && GetLastError() != ERROR_SUCCESS) return false;
    if (!SetWindowPos(window, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED)) return false;
    state.active = false;
    return true;
}

inline HMENU createSfmEditorViewportToolModeMenuCandidate(UINT activeMode) {
    const UINT commands[] = {SFM_EDITOR_VIEWPORT_MODE_SELECTION, SFM_EDITOR_VIEWPORT_MODE_TRANSLATION,
        SFM_EDITOR_VIEWPORT_MODE_GLOBAL_TRANSLATION, SFM_EDITOR_VIEWPORT_MODE_ROTATION,
        SFM_EDITOR_VIEWPORT_MODE_GLOBAL_ROTATION, SFM_EDITOR_VIEWPORT_MODE_SCREEN};
    HMENU menu = CreatePopupMenu();
    if (!menu) return nullptr;
    for (UINT mode = 0; mode < std::size(commands); ++mode) {
        if (!AppendMenuA(menu, MF_STRING, commands[mode], sfmEditorViewportModeMenuLabelCandidate(mode))) {
            DestroyMenu(menu);
            return nullptr;
        }
    }
    const UINT checked = commands[activeMode < std::size(commands) ? activeMode : 0];
    CheckMenuRadioItem(menu, commands[0], commands[5], checked, MF_BYCOMMAND);
    return menu;
}

inline UINT trackSfmEditorViewportToolModeMenuCandidate(HWND owner, POINT screenPoint, UINT activeMode) {
    HMENU menu = createSfmEditorViewportToolModeMenuCandidate(activeMode);
    if (!menu) return 0;
    const UINT command = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON,
        screenPoint.x, screenPoint.y, 0, owner, nullptr);
    DestroyMenu(menu);
    return command;
}

inline HMENU createSfmEditorViewportContextMenuCandidate(const SfmEditorShellCandidate& shell) {
    HMENU root = CreatePopupMenu();
    HMENU display = CreatePopupMenu();
    if (!root || !display) {
        if (root) DestroyMenu(root);
        if (display) DestroyMenu(display);
        return nullptr;
    }
    const bool populated =
        AppendMenuA(display, MF_STRING | (shell.showFocalPlane ? MF_CHECKED : MF_UNCHECKED),
            SFM_EDITOR_VIEWPORT_DISPLAY_FOCAL_PLANE, "Show Focal Plane") &&
        AppendMenuA(display, MF_STRING | (shell.showCameraFrustum ? MF_CHECKED : MF_UNCHECKED),
            SFM_EDITOR_VIEWPORT_DISPLAY_CAMERA_FRUSTUM, "Show Camera Frustum") &&
        AppendMenuA(display, MF_STRING | (shell.showViewTargets ? MF_CHECKED : MF_UNCHECKED),
            SFM_EDITOR_VIEWPORT_DISPLAY_VIEW_TARGETS, "Show View Targets") &&
        AppendMenuA(root, MF_POPUP, reinterpret_cast<UINT_PTR>(display), "Display");
    if (!populated) {
        DestroyMenu(root);
        DestroyMenu(display);
        return nullptr;
    }
    return root;
}

inline RECT sfmEditorViewportClientRectCandidate(int width, int height, const SfmEditorShellCandidate& shell) {
    const auto layout = sfmEditorDockLayoutCandidate(width, height, shell.rootSplitBasisPoints,
        shell.timeSplitBasisPoints, sfmEditorEffectiveScalePercentCandidate(shell));
    const int bottom = std::max(0, layout.timelineTop - sfmEditorScaleMetricCandidate(shell, 40));
    return RECT{layout.viewportLeft, 0, width, bottom};
}

inline bool showSfmEditorViewportContextMenuCandidate(HWND owner, LPARAM screenPosition) {
    if (!owner || !IsWindow(owner) || sfmEditorShellCandidate.parent != owner) return false;
    POINT screen{short(LOWORD(screenPosition)), short(HIWORD(screenPosition))};
    if (screen.x == -1 && screen.y == -1 && !GetCursorPos(&screen)) return false;
    POINT client = screen;
    if (!ScreenToClient(owner, &client)) return false;
    RECT bounds{}, viewport{};
    if (!GetClientRect(owner, &bounds)) return false;
    try {
        viewport = sfmEditorViewportClientRectCandidate(bounds.right, bounds.bottom, sfmEditorShellCandidate);
    } catch (...) { return false; }
    if (!PtInRect(&viewport, client)) return false;
    HMENU menu = createSfmEditorViewportContextMenuCandidate(sfmEditorShellCandidate);
    if (!menu) return false;
    const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
        screen.x, screen.y, 0, owner, nullptr);
    DestroyMenu(menu);
    if (command) SendMessageA(owner, WM_COMMAND, MAKEWPARAM(command, 0), 0);
    return true;
}

inline void setSfmEditorPrimaryControlsVisibleCandidate(SfmEditorShellCandidate& shell, bool visible) noexcept {
    shell.primaryViewportControlsVisible = visible;
    if (shell.primaryViewportControls)
        ShowWindow(shell.primaryViewportControls, visible ? SW_SHOW : SW_HIDE);
}

inline void paintSfmEditorPrimaryControlsCandidate(const DRAWITEMSTRUCT& draw, bool playing = false) {
    if (draw.CtlType != ODT_STATIC) return;
    const auto scale = [](int value) { return sfmEditorScaleMetricCandidate(value); };
    RECT area = draw.rcItem;
    HBRUSH background = CreateSolidBrush(RGB(49, 49, 49));
    FillRect(draw.hDC, &area, background);
    DeleteObject(background);
    const char* labels[] = {"|<", "<", playing ? "Pause" : "Play", ">", ">|"};
    const int width = scale(44), modeWidth = scale(82), gap = scale(3), inset = scale(6), height = scale(26);
    const int top = std::max(0, (int(area.bottom - area.top) - height) / 2);
    SetBkMode(draw.hDC, TRANSPARENT);
    SetTextColor(draw.hDC, RGB(215, 215, 215));
    HGDIOBJ previousFont = SelectObject(draw.hDC, sfmEditorShellCandidate.uiFont ? sfmEditorShellCandidate.uiFont : GetStockObject(DEFAULT_GUI_FONT));
    for (int i = -1; i < 5; ++i) {
        RECT button{};
        const char* label = nullptr;
        if (i < 0) {
            button = RECT{inset, top, inset + modeWidth, top + height};
            label = sfmEditorViewportModeNameCandidate(sfmEditorShellCandidate.activeViewportToolMode);
        } else {
            const int left = inset + modeWidth + gap + i * width;
            button = RECT{left, top, left + width - scale(2), top + height};
            label = labels[i];
        }
        HBRUSH fill = CreateSolidBrush(RGB(67, 67, 67));
        FillRect(draw.hDC, &button, fill);
        DeleteObject(fill);
        FrameRect(draw.hDC, &button, reinterpret_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH)));
        DrawTextA(draw.hDC, label, -1, &button, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    if (previousFont) SelectObject(draw.hDC, previousFont);
}

inline LRESULT CALLBACK sfmEditorPrimaryControlsSubclassProcCandidate(HWND control, UINT message,
    WPARAM key, LPARAM value, UINT_PTR, DWORD_PTR) {
    if (message == WM_LBUTTONUP) {
        const UINT command = sfmEditorPrimaryControlCommandAtCandidate(short(LOWORD(value)),
            sfmEditorScaleMetricCandidate(6), sfmEditorScaleMetricCandidate(82),
            sfmEditorScaleMetricCandidate(3), sfmEditorScaleMetricCandidate(44));
        if (command) SendMessageA(GetParent(control), WM_COMMAND, MAKEWPARAM(command, BN_CLICKED), LPARAM(control));
        return 0;
    }
    return DefSubclassProc(control, message, key, value);
}

inline bool updateSfmEditorDockBoundaryCandidate(SfmEditorShellCandidate& shell, bool rootSplitter,
                                                  int coordinate, int width, int height) {
    if (width < 820 || height < 420) return false;
    if (rootSplitter) {
        const int minLeft = sfmEditorScaleMetricCandidate(shell, 240);
        const int motion = std::clamp(width * 22 / 100, sfmEditorScaleMetricCandidate(shell, 240), sfmEditorScaleMetricCandidate(shell, 320));
        const int boundary = std::clamp(coordinate, minLeft, width - motion - sfmEditorScaleMetricCandidate(shell, 300));
        const auto ratio = uint32_t((int64_t(boundary) * 100000 + width / 2) / width);
        if (ratio < 15000 || ratio > 50000) return false;
        shell.rootSplitBasisPoints = ratio;
    } else {
        const int timelineHeight = std::clamp(height - coordinate, sfmEditorScaleMetricCandidate(shell, 145), height - sfmEditorScaleMetricCandidate(shell, 160));
        const auto ratio = uint32_t((int64_t(timelineHeight) * 100000 + height / 2) / height);
        if (ratio < 18000 || ratio > 50000) return false;
        shell.timeSplitBasisPoints = ratio;
    }
    return true;
}

inline bool saveSfmEditorDockRatiosCandidate(const SfmEditorShellCandidate& shell, const std::string& path) {
    if (path.empty() || shell.rootSplitBasisPoints < 15000 || shell.rootSplitBasisPoints > 50000 ||
        shell.timeSplitBasisPoints < 18000 || shell.timeSplitBasisPoints > 50000 ||
        shell.uiScalePercent < 75 || shell.uiScalePercent > 250) return false;
    const std::string temporary = path + ".tmp";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) return false;
        output << "[EditorDock]\nRootSplitBasisPoints=" << shell.rootSplitBasisPoints
               << "\nTimeSplitBasisPoints=" << shell.timeSplitBasisPoints
               << "\nUiScalePercent=" << shell.uiScalePercent
               << "\nShowFocalPlane=" << unsigned(shell.showFocalPlane)
               << "\nShowCameraFrustum=" << unsigned(shell.showCameraFrustum)
               << "\nShowViewTargets=" << unsigned(shell.showViewTargets)
               << "\nShowPrimaryViewportControls=" << unsigned(shell.primaryViewportControlsVisible)
               << "\nAutoKeyBookmarks=" << unsigned(shell.autoKeyBookmarks)
               << "\nFixedFpsSync=" << unsigned(shell.engineSyncedToFixedFps)
               << "\nHideWaveformOnSelect=" << unsigned(shell.hideWaveformOnSelect)
               << "\nShowTimelineToolTips=" << unsigned(shell.showTimelineToolTips)
               << "\nShowTimelineSnapMarker=" << unsigned(shell.showTimelineSnapMarker)
               << "\nShowTimelineOffFrameWarning=" << unsigned(shell.showTimelineOffFrameWarning) << "\n";
        output.flush();
        if (!output) { output.close(); DeleteFileA(temporary.c_str()); return false; }
    }
    if (!MoveFileExA(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileA(temporary.c_str());
        return false;
    }
    return true;
}

inline bool parseSfmEditorSettingCandidate(const std::string& line, const char* name, unsigned& value) {
    const std::string prefix = std::string(name) + "=";
    if (line.compare(0, prefix.size(), prefix) != 0) return false;
    const auto first = line.data() + prefix.size();
    const auto last = line.data() + line.size();
    const auto parsed = std::from_chars(first, last, value);
    return parsed.ec == std::errc{} && parsed.ptr == last;
}

inline bool loadSfmEditorDockRatiosCandidate(SfmEditorShellCandidate& shell, const std::string& path) {
    if (path.empty()) return false;
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    std::string header, rootLine, timeLine, scaleLine, focalLine, frustumLine, targetsLine, controlsLine;
    std::string autoKeyLine, fixedFpsLine, waveformLine, toolTipsLine, snapMarkerLine, offFrameLine, trailing;
    uint32_t root = 0, time = 0;
    unsigned rootValue = 0, timeValue = 0, scaleValue = 100;
    unsigned focalValue = unsigned(shell.showFocalPlane), frustumValue = unsigned(shell.showCameraFrustum), targetsValue = unsigned(shell.showViewTargets);
    unsigned controlsValue = unsigned(shell.primaryViewportControlsVisible);
    unsigned autoKeyValue = unsigned(shell.autoKeyBookmarks), fixedFpsValue = unsigned(shell.engineSyncedToFixedFps);
    unsigned waveformValue = unsigned(shell.hideWaveformOnSelect), toolTipsValue = unsigned(shell.showTimelineToolTips);
    unsigned snapMarkerValue = unsigned(shell.showTimelineSnapMarker), offFrameValue = unsigned(shell.showTimelineOffFrameWarning);
    if (!std::getline(input, header) || header != "[EditorDock]" ||
        !std::getline(input, rootLine) || !parseSfmEditorSettingCandidate(rootLine, "RootSplitBasisPoints", rootValue) ||
        !std::getline(input, timeLine) || !parseSfmEditorSettingCandidate(timeLine, "TimeSplitBasisPoints", timeValue) ||
        (std::getline(input, scaleLine) && !parseSfmEditorSettingCandidate(scaleLine, "UiScalePercent", scaleValue)) ||
        (std::getline(input, focalLine) && !parseSfmEditorSettingCandidate(focalLine, "ShowFocalPlane", focalValue)) ||
        (std::getline(input, frustumLine) && !parseSfmEditorSettingCandidate(frustumLine, "ShowCameraFrustum", frustumValue)) ||
        (std::getline(input, targetsLine) && !parseSfmEditorSettingCandidate(targetsLine, "ShowViewTargets", targetsValue)) ||
        (std::getline(input, controlsLine) && !parseSfmEditorSettingCandidate(controlsLine, "ShowPrimaryViewportControls", controlsValue)) ||
        (std::getline(input, autoKeyLine) && !parseSfmEditorSettingCandidate(autoKeyLine, "AutoKeyBookmarks", autoKeyValue)) ||
        (std::getline(input, fixedFpsLine) && !parseSfmEditorSettingCandidate(fixedFpsLine, "FixedFpsSync", fixedFpsValue)) ||
        (std::getline(input, waveformLine) && !parseSfmEditorSettingCandidate(waveformLine, "HideWaveformOnSelect", waveformValue)) ||
        (std::getline(input, toolTipsLine) && !parseSfmEditorSettingCandidate(toolTipsLine, "ShowTimelineToolTips", toolTipsValue)) ||
        (std::getline(input, snapMarkerLine) && !parseSfmEditorSettingCandidate(snapMarkerLine, "ShowTimelineSnapMarker", snapMarkerValue)) ||
        (std::getline(input, offFrameLine) && !parseSfmEditorSettingCandidate(offFrameLine, "ShowTimelineOffFrameWarning", offFrameValue)) ||
        std::getline(input, trailing) ||
        rootValue > UINT32_MAX || timeValue > UINT32_MAX || scaleValue < 75 || scaleValue > 250 ||
        focalValue > 1 || frustumValue > 1 || targetsValue > 1 || controlsValue > 1 || autoKeyValue > 1 ||
        fixedFpsValue > 1 || waveformValue > 1 || toolTipsValue > 1 || snapMarkerValue > 1 || offFrameValue > 1)
        return false;
    root = uint32_t(rootValue);
    time = uint32_t(timeValue);
    if (root < 15000 || root > 50000 || time < 18000 || time > 50000) return false;
    shell.rootSplitBasisPoints = root;
    shell.timeSplitBasisPoints = time;
    shell.uiScalePercent = scaleValue;
    shell.showFocalPlane = focalValue != 0;
    shell.showCameraFrustum = frustumValue != 0;
    shell.showViewTargets = targetsValue != 0;
    shell.primaryViewportControlsVisible = controlsValue != 0;
    shell.autoKeyBookmarks = autoKeyValue != 0;
    shell.engineSyncedToFixedFps = fixedFpsValue != 0;
    shell.hideWaveformOnSelect = waveformValue != 0;
    shell.showTimelineToolTips = toolTipsValue != 0;
    shell.showTimelineSnapMarker = snapMarkerValue != 0;
    shell.showTimelineOffFrameWarning = offFrameValue != 0;
    return true;
}

inline void applySfmEditorUiScaleCandidate(HWND parent) {
    auto& shell = sfmEditorShellCandidate;
    if (shell.uiScalePercent < 75 || shell.uiScalePercent > 250) throw std::runtime_error("UI scale outside supported range");
    if (const UINT dpi = GetDpiForWindow(parent); dpi && !setSfmEditorDpiScaleCandidate(shell, dpi))
        throw std::runtime_error("window DPI outside supported range");
    LOGFONTA font{};
    if (GetObjectA(GetStockObject(DEFAULT_GUI_FONT), sizeof(font), &font) != sizeof(font))
        throw std::runtime_error("default UI font query failed");
    font.lfHeight = sfmEditorScaleMetricCandidate(font.lfHeight);
    HFONT replacement = CreateFontIndirectA(&font);
    if (!replacement) throw std::runtime_error("scaled UI font creation failed");
    LOGFONTA compactFont = font;
    compactFont.lfHeight = int((int64_t(font.lfHeight) * 65) / 100);
    HFONT compactReplacement = CreateFontIndirectA(&compactFont);
    if (!compactReplacement) {
        DeleteObject(replacement);
        throw std::runtime_error("compact editor-tab font creation failed");
    }
    RECT outer{}, client{};
    if (GetWindowRect(parent, &outer) && GetClientRect(parent, &client)) {
        const int frameWidth = int(outer.right - outer.left) - int(client.right);
        const int frameHeight = int(outer.bottom - outer.top) - int(client.bottom);
        const int minimumContentWidth = sfmEditorScaleMetricCandidate(240 + 240 + 300);
        const uint32_t remainingRatio = 100000 - shell.rootSplitBasisPoints;
        const int requiredWidth = int((int64_t(minimumContentWidth) * 100000 + remainingRatio - 1) / remainingRatio);
        const int requiredHeight = sfmEditorScaleMetricCandidate(420);
        const int width = std::max(int(outer.right - outer.left), requiredWidth + frameWidth);
        const int height = std::max(int(outer.bottom - outer.top), requiredHeight + frameHeight);
        if (width != outer.right - outer.left || height != outer.bottom - outer.top)
            SetWindowPos(parent, nullptr, outer.left, outer.top, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    HFONT previous = shell.uiFont;
    HFONT previousCompact = shell.compactTabFont;
    shell.uiFont = replacement;
    shell.compactTabFont = compactReplacement;
    for (HWND control : {shell.animationTab, shell.elementTab, shell.filter, shell.elements, shell.details,
                         shell.motionTab, shell.graphTab, shell.clipTab, shell.modeStatus, shell.graphCurves,
                         shell.clipShots, shell.primaryViewportControls, shell.verticalDockSplitter, shell.horizontalDockSplitter,
                         vulkanTimelineSlider})
        if (control) SendMessageA(control, WM_SETFONT, reinterpret_cast<WPARAM>(replacement), TRUE);
    for (HWND control : {shell.animationTab, shell.elementTab, shell.motionTab, shell.graphTab, shell.clipTab})
        if (control) SendMessageA(control, WM_SETFONT, reinterpret_cast<WPARAM>(compactReplacement), TRUE);
    if (previous) DeleteObject(previous);
    if (previousCompact) DeleteObject(previousCompact);
    layoutSfmEditorShellCandidate(parent);
    if (shell.timelinePanel) InvalidateRect(shell.timelinePanel, nullptr, TRUE);
    if (shell.modeStatus) InvalidateRect(shell.modeStatus, nullptr, TRUE);
    if (shell.primaryViewportControls) InvalidateRect(shell.primaryViewportControls, nullptr, TRUE);
}

inline void destroySfmEditorUiFontCandidate() {
    auto& shell = sfmEditorShellCandidate;
    if (shell.uiFont) DeleteObject(shell.uiFont);
    if (shell.compactTabFont) DeleteObject(shell.compactTabFont);
    shell.uiFont = nullptr;
    shell.compactTabFont = nullptr;
}

inline std::string defaultSfmEditorDockSettingsPathCandidate() {
    char executable[MAX_PATH]{};
    const DWORD length = GetModuleFileNameA(nullptr, executable, MAX_PATH);
    if (!length || length >= MAX_PATH) return {};
    return (std::filesystem::path(executable).parent_path() / "sfm64-editor.ini").string();
}

inline HMENU createSfmEditorGraphKeyContextMenuCandidate(bool canDeleteKey) {
    HMENU menu = CreatePopupMenu();
    if (!menu) throw std::runtime_error("graph key context menu creation");
    if (!AppendMenuA(menu, MF_STRING, 41006, "Edit animation key...") ||
        (canDeleteKey && !AppendMenuA(menu, MF_STRING, 41026, "Delete key"))) {
        DestroyMenu(menu);
        throw std::runtime_error("graph key context menu population");
    }
    return menu;
}

inline void setSfmEditorGraphCurveCandidate(std::string label, std::vector<SfmEditorGraphKeyCandidate> keys) {
    auto& shell = sfmEditorShellCandidate;
    shell.graphLayerLabel = std::move(label);
    shell.graphKeys = std::move(keys);
    if (shell.modeStatus) InvalidateRect(shell.modeStatus, nullptr, FALSE);
}

inline void selectSfmEditorGraphCurveCandidate(size_t index) {
    auto& shell = sfmEditorShellCandidate;
    if (index >= shell.graphCurveList.size()) throw std::runtime_error("graph curve selection outside loaded curves");
    shell.selectedGraphCurve = index;
    shell.selectedGraphKey = SIZE_MAX;
    shell.selectedGraphKeys.clear();
    shell.graphLayerLabel = shell.graphCurveList[index].label;
    shell.graphKeys = shell.graphCurveList[index].keys;
    if (shell.graphCurves && SendMessageA(shell.graphCurves, CB_GETCURSEL, 0, 0) != LRESULT(index))
        SendMessageA(shell.graphCurves, CB_SETCURSEL, index, 0);
    if (shell.modeStatus) InvalidateRect(shell.modeStatus, nullptr, FALSE);
}

inline void setSfmEditorGraphCurvesCandidate(std::vector<SfmEditorGraphCurveCandidate> curves) {
    auto& shell = sfmEditorShellCandidate;
    shell.graphCurveList = std::move(curves);
    if (shell.graphCurves) {
        SendMessageA(shell.graphCurves, CB_RESETCONTENT, 0, 0);
        for (const auto& curve : shell.graphCurveList) SendMessageA(shell.graphCurves, CB_ADDSTRING, 0, LPARAM(curve.label.c_str()));
    }
    if (shell.graphCurveList.empty()) {
        shell.selectedGraphCurve = SIZE_MAX;
        shell.graphLayerLabel.clear();
        shell.graphKeys.clear();
        if (shell.modeStatus) InvalidateRect(shell.modeStatus, nullptr, FALSE);
        return;
    }
    selectSfmEditorGraphCurveCandidate(std::min(shell.selectedGraphCurve, shell.graphCurveList.size() - 1));
}

inline void paintSfmEditorTabCandidate(const DRAWITEMSTRUCT& draw) {
    const auto& shell = sfmEditorShellCandidate;
    const UINT id = draw.CtlID;
    const bool active = id == SFM_EDITOR_ANIMATION_TAB ? !shell.elementViewer :
        id == SFM_EDITOR_ELEMENT_TAB ? shell.elementViewer :
        id == SFM_EDITOR_MOTION_TAB ? shell.editorMode == 0 :
        id == SFM_EDITOR_GRAPH_TAB ? shell.editorMode == 1 : shell.editorMode == 2;
    RECT r = draw.rcItem;
    const COLORREF face = active ? RGB(57, 57, 57) : RGB(42, 42, 42);
    HBRUSH brush = CreateSolidBrush(face);
    FillRect(draw.hDC, &r, brush);
    DeleteObject(brush);
    HPEN light = CreatePen(PS_SOLID, 1, RGB(79, 79, 79));
    HPEN dark = CreatePen(PS_SOLID, 1, RGB(24, 24, 24));
    HGDIOBJ oldPen = SelectObject(draw.hDC, light);
    MoveToEx(draw.hDC, r.left, r.bottom - 1, nullptr);
    LineTo(draw.hDC, r.left, r.top);
    LineTo(draw.hDC, r.right - 1, r.top);
    SelectObject(draw.hDC, dark);
    LineTo(draw.hDC, r.right - 1, r.bottom - 1);
    if (!active) LineTo(draw.hDC, r.left, r.bottom - 1);
    if (active) {
        HPEN accent = CreatePen(PS_SOLID, 2, RGB(115, 170, 205));
        SelectObject(draw.hDC, accent);
        MoveToEx(draw.hDC, r.left + 2, r.bottom - 2, nullptr);
        LineTo(draw.hDC, r.right - 2, r.bottom - 2);
        DeleteObject(accent);
    }
    SelectObject(draw.hDC, oldPen);
    DeleteObject(light);
    DeleteObject(dark);
    char text[128]{};
    GetWindowTextA(draw.hwndItem, text, int(sizeof(text)));
    SetBkMode(draw.hDC, TRANSPARENT);
    SetTextColor(draw.hDC, active ? RGB(220, 220, 220) : RGB(165, 165, 165));
    HGDIOBJ oldFont = SelectObject(draw.hDC, reinterpret_cast<HGDIOBJ>(SendMessageA(draw.hwndItem, WM_GETFONT, 0, 0)));
    DrawTextA(draw.hDC, text, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(draw.hDC, oldFont);
}

inline HBRUSH sfmEditorDarkControlBrushCandidate() {
    static HBRUSH brush = CreateSolidBrush(RGB(39, 39, 39));
    return brush;
}

inline int timelineSliderMaximumCandidate(size_t count) {
    return int(count > 1000001 ? 1000000 : count ? count - 1 : 0);
}
inline size_t timelineSliderFrameCandidate(int position, size_t count) {
    if (!count) throw std::runtime_error("empty slider timeline");
    int maximum = timelineSliderMaximumCandidate(count);
    if (position < 0 || position > maximum) throw std::runtime_error("slider position outside range");
    if (!maximum) return 0;
    if (position == maximum) return count - 1;
    return size_t((static_cast<long double>(position) * (count - 1)) / maximum);
}
inline void updateTimelineSliderCandidate(size_t count, size_t frame) {
    if (!vulkanTimelineSlider || !count) return;
    const int maximum = timelineSliderMaximumCandidate(count);
    SendMessageA(vulkanTimelineSlider, TBM_SETRANGEMIN, FALSE, 0);
    SendMessageA(vulkanTimelineSlider, TBM_SETRANGEMAX, FALSE, maximum);
    const int position = frame == count - 1 ? maximum : count > 1
        ? int((static_cast<long double>(frame) * maximum) / (count - 1)) : 0;
    SendMessageA(vulkanTimelineSlider, TBM_SETPOS, TRUE, position);
}

inline std::string sfmEditorFoldCandidate(std::string text) {
    for (char& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}
inline bool sfmEditorAnimationPaneTypeCandidate(const std::string& type) {
    return type == "DmeAnimationSet" || type == "DmeGameModel" || type == "DmeCamera" ||
           type == "DmeLight" || type == "DmeParticleSystem" || type == "DmeBone" || type == "DmeDag";
}
inline void refreshSfmEditorRowsCandidate() {
    auto& shell = sfmEditorShellCandidate;
    if (!shell.elements) return;
    const auto oldSelectedCount = SendMessageA(shell.elements, LB_GETSELCOUNT, 0, 0);
    if (oldSelectedCount != LB_ERR) {
        std::vector<int> oldIndices(static_cast<size_t>(std::max<LRESULT>(0, oldSelectedCount)));
        const auto reported = oldSelectedCount > 0 ? SendMessageA(shell.elements, LB_GETSELITEMS,
            WPARAM(oldIndices.size()), LPARAM(oldIndices.data())) : 0;
        std::vector<uint32_t> ids;
        for (LRESULT i = 0; i < reported; ++i) {
            const int index = oldIndices[size_t(i)];
            if (index >= 0 && size_t(index) < shell.visibleRows.size()) {
                const uint32_t id = shell.rows[shell.visibleRows[size_t(index)]].index;
                if (std::find(ids.begin(), ids.end(), id) == ids.end()) ids.push_back(id);
            }
        }
        if (!ids.empty()) shell.selectedElements = std::move(ids);
        else if (!shell.selectedElements.empty()) {
            const bool anyVisible = std::any_of(shell.selectedElements.begin(), shell.selectedElements.end(), [&](uint32_t id) {
                return std::any_of(shell.visibleRows.begin(), shell.visibleRows.end(), [&](size_t row) { return shell.rows[row].index == id; });
            });
            if (anyVisible) shell.selectedElements.clear();
        }
    } else {
        const auto oldSelection = SendMessageA(shell.elements, LB_GETCURSEL, 0, 0);
        if (oldSelection >= 0 && size_t(oldSelection) < shell.visibleRows.size())
            shell.selectedElement = shell.rows[shell.visibleRows[size_t(oldSelection)]].index;
    }
    struct ListRedrawGuard {
        HWND list;
        explicit ListRedrawGuard(HWND control) : list(control) { SendMessageA(list, WM_SETREDRAW, FALSE, 0); }
        ~ListRedrawGuard() {
            SendMessageA(list, WM_SETREDRAW, TRUE, 0);
            InvalidateRect(list, nullptr, TRUE);
        }
    } redrawGuard(shell.elements);
    SendMessageA(shell.elements, LB_RESETCONTENT, 0, 0);
    shell.visibleRows.clear();
    const size_t reserveCount = shell.elementViewer ? shell.rows.size() :
        size_t(std::count_if(shell.rows.begin(), shell.rows.end(), [](const auto& row) {
            return sfmEditorAnimationPaneTypeCandidate(row.type);
        }));
    if (reserveCount)
        SendMessageA(shell.elements, LB_INITSTORAGE, WPARAM(reserveCount), LPARAM(reserveCount * 80));
    char filterText[256]{};
    if (shell.filter) GetWindowTextA(shell.filter, filterText, sizeof(filterText));
    const auto needle = sfmEditorFoldCandidate(filterText);
    for (size_t i = 0; i < shell.rows.size(); ++i) {
        const auto& row = shell.rows[i];
        const bool setMember = sfmEditorAnimationPaneTypeCandidate(row.type);
        // Element Viewer inspects the full DMX graph; only the animation-set pane is filtered.
        if (!shell.elementViewer && !setMember) continue;
        const std::string indent(std::min<uint32_t>(row.hierarchyDepth, 16) * 2, ' ');
        std::string label = indent + (shell.elementViewer
            ? row.type + "  " + row.name + "  [" + std::to_string(row.index) + "]"
            : row.name + "  (" + row.type + ")");
        if (!needle.empty() && sfmEditorFoldCandidate(label).find(needle) == std::string::npos) continue;
        if (SendMessageA(shell.elements, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str())) < 0) continue;
        shell.visibleRows.push_back(i);
    }
    if (!shell.visibleRows.empty()) {
        const bool extended = SendMessageA(shell.elements, LB_GETSELCOUNT, 0, 0) != LB_ERR;
        if (extended) {
            for (size_t visible = 0; visible < shell.visibleRows.size(); ++visible) {
                const uint32_t id = shell.rows[shell.visibleRows[visible]].index;
                if (std::find(shell.selectedElements.begin(), shell.selectedElements.end(), id) != shell.selectedElements.end())
                    SendMessageA(shell.elements, LB_SETSEL, TRUE, LPARAM(visible));
            }
        } else {
            auto selected = std::find_if(shell.visibleRows.begin(), shell.visibleRows.end(), [&](size_t row) {
                return shell.rows[row].index == shell.selectedElement;
            });
            const size_t visible = selected == shell.visibleRows.end() ? 0 : size_t(selected - shell.visibleRows.begin());
            SendMessageA(shell.elements, LB_SETCURSEL, visible, 0);
            shell.selectedElement = shell.rows[shell.visibleRows[visible]].index;
        }
        if (extended) {
            const auto selectedPrimary = std::find_if(shell.visibleRows.begin(), shell.visibleRows.end(), [&](size_t row) {
                return shell.rows[row].index == shell.selectedElement &&
                    std::find(shell.selectedElements.begin(), shell.selectedElements.end(), shell.selectedElement) != shell.selectedElements.end();
            });
            if (selectedPrimary == shell.visibleRows.end()) {
                const auto fallback = std::find_if(shell.visibleRows.begin(), shell.visibleRows.end(), [&](size_t row) {
                    return std::find(shell.selectedElements.begin(), shell.selectedElements.end(), shell.rows[row].index) != shell.selectedElements.end();
                });
                if (fallback != shell.visibleRows.end()) shell.selectedElement = shell.rows[*fallback].index;
                else if (!shell.visibleRows.empty()) {
                    shell.selectedElement = shell.rows[shell.visibleRows.front()].index;
                    shell.selectedElements = {shell.selectedElement};
                    SendMessageA(shell.elements, LB_SETSEL, TRUE, 0);
                }
            }
        }
        handleSfmEditorCommandCandidate(shell.parent, MAKEWPARAM(SFM_EDITOR_ELEMENTS, LBN_SELCHANGE), LPARAM(shell.elements));
    } else if (shell.details) {
        SetWindowTextA(shell.details, "No matching scene elements");
    }
}

inline void scheduleSfmEditorFilterRefreshCandidate(HWND parent) {
    if (!SetTimer(parent, SFM_EDITOR_FILTER_REFRESH_TIMER, SFM_EDITOR_FILTER_REFRESH_DELAY_MS, nullptr))
        refreshSfmEditorRowsCandidate();
}

inline bool dispatchSfmEditorFilterRefreshTimerCandidate(HWND parent, WPARAM timerId) {
    if (timerId != SFM_EDITOR_FILTER_REFRESH_TIMER) return false;
    KillTimer(parent, SFM_EDITOR_FILTER_REFRESH_TIMER);
    refreshSfmEditorRowsCandidate();
    return true;
}

inline void selectSfmEditorElementCandidate() {
    auto& shell = sfmEditorShellCandidate;
    if (!shell.elements || !shell.details) return;
    const auto selectedCount = SendMessageA(shell.elements, LB_GETSELCOUNT, 0, 0);
    LRESULT primary = selectedCount == LB_ERR ? SendMessageA(shell.elements, LB_GETCURSEL, 0, 0)
        : SendMessageA(shell.elements, LB_GETCARETINDEX, 0, 0);
    if (selectedCount > 0 && (primary < 0 || size_t(primary) >= shell.visibleRows.size() ||
        SendMessageA(shell.elements, LB_GETSEL, WPARAM(primary), 0) == 0)) {
        std::vector<int> firstSelected(static_cast<size_t>(selectedCount));
        if (SendMessageA(shell.elements, LB_GETSELITEMS, WPARAM(firstSelected.size()), LPARAM(firstSelected.data())) > 0)
            primary = firstSelected.front();
    }
    if (selectedCount == LB_ERR && (primary < 0 || size_t(primary) >= shell.visibleRows.size())) return;
    shell.selectedElements.clear();
    if (selectedCount > 0) {
        std::vector<int> indices(static_cast<size_t>(selectedCount));
        const auto reported = SendMessageA(shell.elements, LB_GETSELITEMS, WPARAM(indices.size()), LPARAM(indices.data()));
        for (LRESULT i = 0; i < reported; ++i) {
            const int visible = indices[size_t(i)];
            if (visible >= 0 && size_t(visible) < shell.visibleRows.size())
                shell.selectedElements.push_back(shell.rows[shell.visibleRows[size_t(visible)]].index);
        }
    }
    if (shell.selectedElements.empty()) {
        if (primary < 0 || size_t(primary) >= shell.visibleRows.size()) return;
        shell.selectedElements.push_back(shell.rows[shell.visibleRows[size_t(primary)]].index);
    }
    shell.selectedElement = primary >= 0 && size_t(primary) < shell.visibleRows.size()
        ? shell.rows[shell.visibleRows[size_t(primary)]].index : shell.selectedElements.front();
    const auto rowIt = std::find_if(shell.rows.begin(), shell.rows.end(), [&](const auto& candidate) {
        return candidate.index == shell.selectedElement;
    });
    if (rowIt == shell.rows.end()) return;
    const auto& row = *rowIt;
    const std::string details = "Name: " + row.name + "\r\nType: " + row.type +
        "\r\nElement: " + std::to_string(row.index) +
        "\r\nAttributes: " + std::to_string(row.attributeCount) +
        "\r\nSelected elements: " + std::to_string(shell.selectedElements.size()) +
        "\r\nParent: " + (row.parent.empty() ? std::string("(root or unlinked)") : row.parent) +
        (row.additionalParents ? "\r\nAdditional parent links: " + std::to_string(row.additionalParents) + " (showing first parent in this outline)" : "");
    std::string text = details;
    if (shell.elementViewer) {
        for (const auto& attribute : row.attributeDetails) {
            text += "\r\n\r\n";
            text += attribute;
        }
    }
    SetWindowTextA(shell.details, text.c_str());
}

inline bool selectSfmEditorRowAtPointCandidate(POINT point) {
    auto& shell = sfmEditorShellCandidate;
    if (!shell.elements) return false;
    const auto hit = SendMessageA(shell.elements, LB_ITEMFROMPOINT, 0,
        MAKELPARAM(WORD(point.x), WORD(point.y)));
    const auto index = LOWORD(hit);
    if (HIWORD(hit) || index >= shell.visibleRows.size()) return false;
    const bool clickedWasSelected = SendMessageA(shell.elements, LB_GETSEL, WPARAM(index), 0) != 0;
    if (!clickedWasSelected) SendMessageA(shell.elements, LB_SETSEL, FALSE, -1);
    SendMessageA(shell.elements, LB_SETSEL, TRUE, index);
    SendMessageA(shell.elements, LB_SETCARETINDEX, index, TRUE);
    shell.selectedElement = shell.rows[shell.visibleRows[index]].index;
    if (!clickedWasSelected) shell.selectedElements = {shell.selectedElement};
    handleSfmEditorCommandCandidate(shell.parent,
        MAKEWPARAM(SFM_EDITOR_ELEMENTS, LBN_SELCHANGE), LPARAM(shell.elements));
    return true;
}

inline HMENU createSfmEditorElementContextMenuCandidate(bool renameDag=false,bool renameAnimationSet=false,bool deleteEmptyDag=false) {
    HMENU menu = CreatePopupMenu();
    if (!menu) throw std::runtime_error("scene element context menu creation");
    bool populated=AppendMenuA(menu, MF_STRING, 41005, "Edit scalar property...") &&
        AppendMenuA(menu, MF_STRING, 41006, "Edit animation key...");
    if(populated&&renameDag)populated=AppendMenuA(menu,MF_STRING,41023,"Rename Dag");
    if(populated&&renameAnimationSet)populated=AppendMenuA(menu,MF_STRING,41023,"Rename AnimationSet");
    if(populated&&deleteEmptyDag)populated=AppendMenuA(menu,MF_STRING,41024,"Delete Empty Dag");
    if (!populated) {
        DestroyMenu(menu);
        throw std::runtime_error("scene element context menu population");
    }
    return menu;
}

inline void showSfmEditorElementContextMenuCandidate(HWND parent, LPARAM screenPosition) {
    auto& shell = sfmEditorShellCandidate;
    if (!shell.elements) return;
    POINT screen{short(LOWORD(screenPosition)), short(HIWORD(screenPosition))};
    if (screen.x == -1 && screen.y == -1) GetCursorPos(&screen);
    POINT client = screen;
    if (!ScreenToClient(shell.elements, &client) || !selectSfmEditorRowAtPointCandidate(client)) return;
    selectSfmEditorElementCandidate();
    const auto selectedRow=std::find_if(shell.rows.begin(),shell.rows.end(),[&](const auto&row){return row.index==shell.selectedElement;});
    if(selectedRow==shell.rows.end())return;
    HMENU menu = createSfmEditorElementContextMenuCandidate(selectedRow->dag,selectedRow->type=="DmeAnimationSet",selectedRow->dag&&selectedRow->childCount==0);
    const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
        screen.x, screen.y, 0, parent, nullptr);
    DestroyMenu(menu);
    if (command == 41005 || command == 41006 || command==41023 || command==41024)
        SendMessageA(parent, WM_COMMAND, MAKEWPARAM(command, 0), 0);
}

inline LRESULT CALLBACK sfmEditorElementsSubclassProcCandidate(HWND control, UINT message,
    WPARAM key, LPARAM value, UINT_PTR, DWORD_PTR) {
    if (message == WM_CONTEXTMENU) {
        try { showSfmEditorElementContextMenuCandidate(sfmEditorShellCandidate.parent, value); }
        catch (const std::exception& error) {
            MessageBoxA(sfmEditorShellCandidate.parent, error.what(), "Scene element menu failed", MB_OK | MB_ICONERROR);
        }
        return 0;
    }
    return DefSubclassProc(control, message, key, value);
}

inline void setSfmEditorElementRowsCandidate(std::vector<SfmEditorElementRowCandidate> rows, size_t totalElements) {
    auto& shell = sfmEditorShellCandidate;
    if (shell.elements) SendMessageA(shell.elements, LB_SETSEL, FALSE, -1);
    shell.visibleRows.clear();
    shell.selectedElement = 0xffffffffu;
    shell.selectedElements.clear();
    shell.rows = std::move(rows);
    shell.totalSceneElements = totalElements;
    refreshSfmEditorRowsCandidate();
}

template<class Graph>
inline std::vector<SfmEditorElementRowCandidate> buildSfmEditorElementRowsCandidate(const Graph& graph,
                                                                                   bool includeAllElements = false) {
    constexpr uint32_t noParent = 0xffffffffu;
    std::vector<uint32_t> parentByChild(graph.elements.size(), noParent);
    std::vector<uint32_t> lastParentByChild(graph.elements.size(), noParent);
    std::vector<uint32_t> additionalParentCount(graph.elements.size(), 0);
    for (uint32_t parent = 0; parent < graph.elements.size(); ++parent) {
        for (const auto& attribute : graph.elements[parent].attributes) {
            if (graph.strings.at(attribute.name) != "children") continue;
            if (attribute.type != 15) throw std::runtime_error("invalid editor hierarchy children type");
            for (const auto& value : attribute.values) {
                if (value.reference == 0xffffffffu) continue;
                if (value.reference >= graph.elements.size()) throw std::runtime_error("editor hierarchy child reference");
                auto& firstParent = parentByChild[value.reference];
                auto& lastParent = lastParentByChild[value.reference];
                if (firstParent == noParent) {
                    firstParent = parent;
                    lastParent = parent;
                } else if (firstParent != parent && lastParent != parent) {
                    ++additionalParentCount[value.reference];
                    lastParent = parent;
                }
            }
        }
    }
    std::vector<uint32_t> visitGeneration(graph.elements.size(), 0);
    std::vector<SfmEditorElementRowCandidate> rows;
    for (uint32_t i = 0; i < graph.elements.size(); ++i) {
        const auto& element = graph.elements[i];
        const auto& type = graph.strings.at(element.type);
        if (!includeAllElements && type != "DmeAnimationSet" && type != "DmeGameModel" && type != "DmeCamera" &&
            type != "DmeLight" && type != "DmeParticleSystem" && type != "DmeFilmClip" &&
            type != "DmeShot" && type != "DmeDag") continue;
        SfmEditorElementRowCandidate row;
        row.index = i;
        row.attributeCount = uint32_t(element.attributes.size());
        row.type = type;
        row.name = graph.strings.at(element.name);
        row.attributeDetails.reserve(element.attributes.size());
        for (const auto& attribute : element.attributes) {
            const auto& attributeName = graph.strings.at(attribute.name);
            const unsigned scalarType = attribute.type >= 15 ? attribute.type - 14 : attribute.type;
            static constexpr const char* typeNames[] = {"invalid", "element", "int", "float", "bool", "string", "binary", "time", "color", "vector2", "vector3", "vector4", "angle", "quaternion", "matrix"};
            const char* typeName = scalarType < std::size(typeNames) ? typeNames[scalarType] : "unknown";
            std::ostringstream summary;
            summary << attributeName << " : " << (attribute.type >= 15 ? "array<" : "") << typeName
                    << (attribute.type >= 15 ? ">" : "") << " (" << attribute.values.size() << " value(s))";
            if (attribute.type == 1 || attribute.type == 15) {
                for (size_t valueIndex = 0; valueIndex < attribute.values.size() && valueIndex < 4; ++valueIndex) {
                    const uint32_t reference = attribute.values[valueIndex].reference;
                    summary << (valueIndex ? ", " : ": ");
                    if (reference < graph.elements.size())
                        summary << graph.strings.at(graph.elements[reference].name) << " [" << reference << "]";
                    else if (reference == 0xffffffffu) summary << "null";
                    else if (reference == 0xfffffffeu) summary << "external";
                    else summary << "invalid-ref-" << reference;
                }
                if (attribute.values.size() > 4) summary << ", ...";
            } else if (scalarType == 5 && !attribute.values.empty()) {
                summary << ": \"" << attribute.values.front().text.substr(0, 72) << "\"";
                if (attribute.values.front().text.size() > 72) summary << "...";
            } else if (scalarType == 3 && attribute.type == 3 && attribute.values.size() == 1 && attribute.values[0].wire.size() == 4) {
                uint32_t bits = 0;
                for (unsigned byte = 0; byte < 4; ++byte)
                    bits |= uint32_t(static_cast<unsigned char>(attribute.values[0].wire[byte])) << (byte * 8);
                float value = 0;
                std::memcpy(&value, &bits, sizeof(value));
                summary << ": " << (std::isfinite(value) ? std::to_string(value) : "nonfinite");
            } else if (scalarType == 2 && attribute.type == 2 && attribute.values.size() == 1 && attribute.values[0].wire.size() == 4) {
                uint32_t bits = 0;
                for (unsigned byte = 0; byte < 4; ++byte)
                    bits |= uint32_t(static_cast<unsigned char>(attribute.values[0].wire[byte])) << (byte * 8);
                int32_t value = 0;
                std::memcpy(&value, &bits, sizeof(value));
                summary << ": " << value;
            } else {
                size_t bytes = 0;
                for (const auto& value : attribute.values) bytes += value.wire.size();
                summary << ": " << bytes << " wire byte(s)";
            }
            row.attributeDetails.push_back(summary.str());
        }
        row.dag=type=="DmeDag"||type=="DmeGameModel"||type=="DmeCamera"||type=="DmeLight"||type=="DmeParticleSystem";
        if(row.dag)for(const auto&attribute:element.attributes)if(graph.strings.at(attribute.name)=="children"&&attribute.type==15)for(const auto&child:attribute.values)if(child.reference!=0xffffffffu&&child.reference!=0xfffffffeu)++row.childCount;
        row.additionalParents = additionalParentCount[i];
        const uint32_t parent = parentByChild[i];
        if (parent != noParent) {
            row.parent = graph.strings.at(graph.elements[parent].name);
            const uint32_t generation = i + 1;
            visitGeneration[i] = generation;
            uint32_t current = parent;
            while (current != noParent) {
                if (visitGeneration[current] == generation) throw std::runtime_error("cyclic editor hierarchy");
                visitGeneration[current] = generation;
                if (row.hierarchyDepth == UINT32_MAX) throw std::runtime_error("editor hierarchy depth overflow");
                ++row.hierarchyDepth;
                current = parentByChild[current];
            }
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

inline void setSfmEditorTimelineCandidate(size_t count, size_t frame) {
    auto& shell = sfmEditorShellCandidate;
    shell.frameCount = count;
    shell.selectedFrame = count ? std::min(frame, count - 1) : 0;
    if (shell.timelinePanel) InvalidateRect(shell.timelinePanel, nullptr, FALSE);
    updateTimelineSliderCandidate(count, shell.selectedFrame);
    bool activeClip = false;
    for (size_t i = 0; i < shell.timelineSegments.size(); ++i) {
        const auto& segment = shell.timelineSegments[i];
        if (shell.selectedFrame >= segment.startFrame && shell.selectedFrame < segment.endFrame) {
            if (shell.clipShots && SendMessageA(shell.clipShots, CB_GETCURSEL, 0, 0) != LRESULT(i)) SendMessageA(shell.clipShots, CB_SETCURSEL, i, 0);
            activeClip = true;
            break;
        }
    }
    if (!activeClip && shell.clipShots) SendMessageA(shell.clipShots, CB_SETCURSEL, WPARAM(-1), 0);
    if (shell.modeStatus && shell.editorMode == 0) {
        char status[256]{};
        std::snprintf(status, sizeof(status), "Motion Editor\r\n\r\nFrame %zu of %zu\r\n\r\nUse the timeline slider or Left/Right and Space to scrub and play.",
            count ? shell.selectedFrame + 1 : 0, count);
        SetWindowTextA(shell.modeStatus, status);
    }
}

inline int clampSfmEditorTimelineScrollCandidate(int requestedOffset, int clientHeight);

inline void setSfmEditorTimelineSegmentsCandidate(std::vector<SfmEditorTimelineSegmentCandidate> segments) {
    sfmEditorShellCandidate.timelineSegments = std::move(segments);
    if (sfmEditorShellCandidate.clipShots) {
        SendMessageA(sfmEditorShellCandidate.clipShots, CB_RESETCONTENT, 0, 0);
        for (const auto& segment : sfmEditorShellCandidate.timelineSegments) {
            const auto label = segment.label + "  [" + std::to_string(segment.startFrame + 1) + "-" + std::to_string(segment.endFrame) + "]";
            SendMessageA(sfmEditorShellCandidate.clipShots, CB_ADDSTRING, 0, LPARAM(label.c_str()));
        }
    }
    if (sfmEditorShellCandidate.timelinePanel) InvalidateRect(sfmEditorShellCandidate.timelinePanel, nullptr, FALSE);
    if (sfmEditorShellCandidate.modeStatus) InvalidateRect(sfmEditorShellCandidate.modeStatus, nullptr, FALSE);
}

inline void setSfmEditorTimelineLanesCandidate(std::vector<SfmEditorTimelineLaneCandidate> lanes) {
    auto& shell = sfmEditorShellCandidate;
    shell.timelineLanes = std::move(lanes);
    shell.selectedTimelineClip = UINT32_MAX;
    shell.pendingTimelineClipMoveStartFrame = SIZE_MAX;
    if (shell.selectedTimelineLane >= shell.timelineLanes.size()) shell.selectedTimelineLane = SIZE_MAX;
    if (shell.timelinePanel) {
        RECT client{};
        if (GetClientRect(shell.timelinePanel, &client))
            shell.timelineScrollOffsetPixels = clampSfmEditorTimelineScrollCandidate(
                shell.timelineScrollOffsetPixels, client.bottom);
        InvalidateRect(shell.timelinePanel, nullptr, FALSE);
    }
}

inline void setSfmEditorBookmarkFramesCandidate(std::vector<size_t> frames) {
    auto& shell = sfmEditorShellCandidate;
    shell.bookmarkFrames = std::move(frames);
    if (shell.timelinePanel) InvalidateRect(shell.timelinePanel, nullptr, FALSE);
}

inline void setSfmEditorTimelineRateCandidate(double framesPerSecond) {
    if (!std::isfinite(framesPerSecond) || framesPerSecond <= 0) throw std::runtime_error("invalid timeline display rate");
    sfmEditorShellCandidate.framesPerSecond = framesPerSecond;
    if (sfmEditorShellCandidate.timelinePanel) InvalidateRect(sfmEditorShellCandidate.timelinePanel, nullptr, FALSE);
}

inline size_t sfmEditorTimelineLaneAtYCandidate(int y) {
    const auto& lanes = sfmEditorShellCandidate.timelineLanes;
    const auto scale = [](int value) { return sfmEditorScaleMetricCandidate(value); };
    int laneY = scale(54 + 38) - sfmEditorShellCandidate.timelineScrollOffsetPixels;
    std::string previousGroup;
    for (size_t i = 0; i < lanes.size(); ++i) {
        if (lanes[i].groupName != previousGroup) { laneY += scale(19); previousGroup = lanes[i].groupName; }
        if (y >= laneY && y < laneY + scale(24)) return i;
        laneY += scale(25);
    }
    return SIZE_MAX;
}

inline int sfmEditorTimelineScrollMaximumCandidate(int clientHeight) {
    const auto& shell = sfmEditorShellCandidate;
    const auto scale = [](int value) { return sfmEditorScaleMetricCandidate(value); };
    int contentHeight = 0;
    std::string previousGroup;
    for (const auto& lane : shell.timelineLanes) {
        if (lane.groupName != previousGroup) {
            contentHeight += scale(19);
            previousGroup = lane.groupName;
        }
        contentHeight += scale(25);
    }
    const int viewportHeight = std::max(0, clientHeight - scale(54 + 38 + 8));
    return std::max(0, contentHeight - viewportHeight);
}

inline int clampSfmEditorTimelineScrollCandidate(int requestedOffset, int clientHeight) {
    return std::clamp(requestedOffset, 0, sfmEditorTimelineScrollMaximumCandidate(clientHeight));
}

inline void seekSfmEditorTimelineAtPixelCandidate(int x) {
    auto& shell = sfmEditorShellCandidate;
    if (!shell.frameCount || !shell.timelinePanel) return;
    RECT client{};
    GetClientRect(shell.timelinePanel, &client);
    const int left = sfmEditorScaleMetricCandidate(148), right = std::max(left + 1, int(client.right) - sfmEditorScaleMetricCandidate(24));
    const auto frame = timelineFrameAtPixelCandidate(x, left, right, shell.frameCount);
    SendMessageA(shell.parent, SFM_EDITOR_SEEK_FRAME, 0, LPARAM(frame));
}

inline HMENU createSfmEditorTimelineContextMenuCandidate() {
    HMENU menu = CreatePopupMenu();
    if (!menu) throw std::runtime_error("timeline context menu creation");
    if (!AppendMenuA(menu, MF_STRING, 41001, "Trim clip start to playhead") ||
        !AppendMenuA(menu, MF_STRING, 41021, "Trim clip end to playhead") ||
        !AppendMenuA(menu, MF_STRING, 41028, "Duplicate clip at playhead") ||
        !AppendMenuA(menu, MF_STRING, 41026, "Delete clip") ||
        !AppendMenuA(menu, MF_STRING, 41025, "Add Bookmark at Playhead") ||
        !AppendMenuA(menu, MF_STRING, SFM_EDITOR_TRACK_GROUP_VISIBILITY, "Show/Hide Track Groups...")) {
        DestroyMenu(menu);
        throw std::runtime_error("timeline context menu population");
    }
    return menu;
}

inline HMENU createSfmEditorTimelineOptionsMenuCandidate(const SfmEditorShellCandidate& shell) {
    HMENU menu = CreatePopupMenu();
    if (!menu) throw std::runtime_error("timeline options menu creation");
    if (!AppendMenuA(menu, MF_STRING, SFM_EDITOR_TRACK_GROUP_VISIBILITY, "Show/Hide Track Groups...") ||
        !AppendMenuA(menu, MF_STRING | (shell.autoKeyBookmarks ? MF_CHECKED : MF_UNCHECKED),
            SFM_EDITOR_TIMELINE_AUTOKEY_BOOKMARKS, "AutoKey Bookmarks Mode") ||
        !AppendMenuA(menu, MF_STRING | (shell.engineSyncedToFixedFps ? MF_CHECKED : MF_UNCHECKED),
            SFM_EDITOR_TIMELINE_FIXED_FPS, "Engine Synced to Fixed FPS") ||
        !AppendMenuA(menu, MF_STRING, SFM_EDITOR_TIMELINE_HIDE_WAVEFORM,
            shell.hideWaveformOnSelect ? "Show Waveform on Select" : "Hide Waveform on Select") ||
        !AppendMenuA(menu, MF_SEPARATOR, 0, nullptr) ||
        !AppendMenuA(menu, MF_STRING, SFM_EDITOR_TIMELINE_SHOW_TOOLTIPS,
            shell.showTimelineToolTips ? "Hide ToolTips" : "Show ToolTips") ||
        !AppendMenuA(menu, MF_STRING, SFM_EDITOR_TIMELINE_SHOW_SNAP_MARKER,
            shell.showTimelineSnapMarker ? "Hide Snap Marker" : "Show Snap Marker") ||
        !AppendMenuA(menu, MF_STRING, SFM_EDITOR_TIMELINE_SHOW_OFF_FRAME_WARNING,
            shell.showTimelineOffFrameWarning ? "Hide Off Frame Warning" : "Show Off Frame Warning")) {
        DestroyMenu(menu);
        throw std::runtime_error("timeline options menu population");
    }
    return menu;
}

inline bool applySfmEditorTimelineOptionCandidate(SfmEditorShellCandidate& shell, UINT command) {
    bool* state = command == SFM_EDITOR_TIMELINE_SHOW_SNAP_MARKER ? &shell.showTimelineSnapMarker :
        command == SFM_EDITOR_TIMELINE_SHOW_TOOLTIPS ? &shell.showTimelineToolTips :
        command == SFM_EDITOR_TIMELINE_SHOW_OFF_FRAME_WARNING ? &shell.showTimelineOffFrameWarning :
        command == SFM_EDITOR_TIMELINE_AUTOKEY_BOOKMARKS ? &shell.autoKeyBookmarks :
        command == SFM_EDITOR_TIMELINE_FIXED_FPS ? &shell.engineSyncedToFixedFps :
        command == SFM_EDITOR_TIMELINE_HIDE_WAVEFORM ? &shell.hideWaveformOnSelect : nullptr;
    if (!state) return false;
    *state = !*state;
    return true;
}

inline HMENU createSfmEditorTimeDisplayModeSubmenuCandidate(SfmEditorTimeDisplayModeCandidate selected, UINT firstCommand) {
    HMENU menu = CreatePopupMenu();
    if (!menu) throw std::runtime_error("time display submenu creation");
    const std::array<SfmEditorTimeDisplayModeCandidate, 4> modes{{
        SfmEditorTimeDisplayModeCandidate::TotalSeconds,
        SfmEditorTimeDisplayModeCandidate::TotalFrames,
        SfmEditorTimeDisplayModeCandidate::TimecodeSeconds,
        SfmEditorTimeDisplayModeCandidate::TimecodeFrames,
    }};
    for (size_t i = 0; i < modes.size(); ++i) {
        const UINT flags = MF_STRING | (modes[i] == selected ? MF_CHECKED : MF_UNCHECKED);
        if (!AppendMenuA(menu, flags, firstCommand + UINT(i), sfmEditorTimeDisplayModeLabelCandidate(modes[i]))) {
            DestroyMenu(menu);
            throw std::runtime_error("time display submenu population");
        }
    }
    return menu;
}

inline HMENU createSfmEditorTimeDisplayMenuCandidate(SfmEditorTimeDisplayModeCandidate globalSelected,
                                                       SfmEditorTimeDisplayModeCandidate localSelected) {
    HMENU menu = CreatePopupMenu();
    if (!menu) throw std::runtime_error("time display menu creation");
    HMENU global = createSfmEditorTimeDisplayModeSubmenuCandidate(globalSelected, 41101);
    HMENU local = createSfmEditorTimeDisplayModeSubmenuCandidate(localSelected, 41111);
    if (!AppendMenuA(menu, MF_POPUP, UINT_PTR(global), "Global")) {
        DestroyMenu(global);
        DestroyMenu(local);
        DestroyMenu(menu);
        throw std::runtime_error("time display scope menu population");
    }
    if (!AppendMenuA(menu, MF_POPUP, UINT_PTR(local), "Local")) {
        DestroyMenu(local);
        DestroyMenu(menu);
        throw std::runtime_error("time display scope menu population");
    }
    return menu;
}

inline void showSfmEditorTimelineContextMenuCandidate(HWND control, LPARAM screenPosition) {
    auto& shell = sfmEditorShellCandidate;
    if (!shell.frameCount || !shell.timelinePanel) return;
    POINT screen{short(LOWORD(screenPosition)), short(HIWORD(screenPosition))};
    if (screen.x == -1 && screen.y == -1) GetCursorPos(&screen);
    POINT client = screen;
    if (!ScreenToClient(control, &client)) return;
    seekSfmEditorTimelineAtPixelCandidate(client.x);
    const size_t laneIndex = sfmEditorTimelineLaneAtYCandidate(client.y);
    if (laneIndex == SIZE_MAX || laneIndex >= shell.timelineLanes.size()) return;
    shell.selectedTimelineLane = laneIndex;
    const auto& lane = shell.timelineLanes[laneIndex];
    const auto clip = std::find_if(lane.segments.begin(), lane.segments.end(), [&](const auto& segment) {
        return shell.selectedFrame >= segment.startFrame && shell.selectedFrame < segment.endFrame;
    });
    if (clip == lane.segments.end()) { shell.selectedTimelineClip = UINT32_MAX; return; }
    shell.selectedTimelineClip = clip->element;
    InvalidateRect(control, nullptr, FALSE);
    HMENU menu = createSfmEditorTimelineContextMenuCandidate();
    const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
        screen.x, screen.y, 0, shell.parent, nullptr);
    DestroyMenu(menu);
    if (command == 41001 || command == 41021 || command == 41028 || command == 41026 || command == 41025 || command == SFM_EDITOR_TRACK_GROUP_VISIBILITY)
        SendMessageA(shell.parent, WM_COMMAND, MAKEWPARAM(command, 0), 0);
}

inline LRESULT CALLBACK sfmEditorTimelineSubclassProcCandidate(HWND control, UINT message,
    WPARAM key, LPARAM value, UINT_PTR, DWORD_PTR) {
    auto& shell = sfmEditorShellCandidate;
    if (message == WM_CONTEXTMENU) {
        POINT screen{short(LOWORD(value)), short(HIWORD(value))};
        if (screen.x == -1 && screen.y == -1) GetCursorPos(&screen);
        POINT client = screen;
        if (ScreenToClient(control, &client) && client.y < sfmEditorScaleMetricCandidate(34)) {
            HMENU menu = createSfmEditorTimeDisplayMenuCandidate(shell.globalTimeDisplayMode, shell.localTimeDisplayMode);
            const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
                screen.x, screen.y, 0, shell.parent, nullptr);
            DestroyMenu(menu);
            if (applySfmEditorTimeDisplayCommandCandidate(shell, command)) {
                InvalidateRect(shell.timelinePanel, nullptr, FALSE);
            }
            return 0;
        }
        try { showSfmEditorTimelineContextMenuCandidate(control, value); }
        catch (const std::exception& error) {
            MessageBoxA(shell.parent, error.what(), "Timeline menu failed", MB_OK | MB_ICONERROR);
        }
        return 0;
    }
    if (message == WM_LBUTTONDOWN) {
        POINT click{short(LOWORD(value)), short(HIWORD(value))};
        if (click.y < sfmEditorScaleMetricCandidate(34)) {
            RECT client{};
            GetClientRect(control, &client);
            if (click.x >= client.right - sfmEditorScaleMetricCandidate(34)) {
                POINT screen{click.x, click.y};
                ClientToScreen(control, &screen);
                HMENU menu = createSfmEditorTimelineOptionsMenuCandidate(shell);
                const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
                    screen.x, screen.y, 0, shell.parent, nullptr);
                DestroyMenu(menu);
                if (command == SFM_EDITOR_TRACK_GROUP_VISIBILITY) {
                    SendMessageA(shell.parent, WM_COMMAND, MAKEWPARAM(command, 0), 0);
                } else if (applySfmEditorTimelineOptionCandidate(shell, command)) {
                    InvalidateRect(control, nullptr, FALSE);
                }
            }
            SetFocus(control);
            return 0;
        }
        SetFocus(control);
        shell.selectedTimelineClip = UINT32_MAX;
        const size_t lane = sfmEditorTimelineLaneAtYCandidate(click.y);
        if (lane != SIZE_MAX && lane != shell.selectedTimelineLane) {
            shell.selectedTimelineLane = lane;
            InvalidateRect(control, nullptr, FALSE);
        }
        seekSfmEditorTimelineAtPixelCandidate(short(LOWORD(value)));
        bool hitClip = false;
        if (lane < shell.timelineLanes.size() && shell.selectedFrame < shell.frameCount) {
            const auto& segments = shell.timelineLanes[lane].segments;
            const auto selected = std::find_if(segments.begin(), segments.end(), [&](const auto& segment) {
                return shell.selectedFrame >= segment.startFrame && shell.selectedFrame < segment.endFrame;
            });
            if (selected != segments.end()) {
                shell.selectedTimelineClip = selected->element;
                shell.timelineDragPointerFrame = shell.selectedFrame;
                shell.timelineDragClipStartFrame = selected->startFrame;
                shell.pendingTimelineClipMoveStartFrame = selected->startFrame;
                shell.draggingTimelineClip = true;
                hitClip = true;
            }
        }
        shell.draggingTimeline = !hitClip;
        SetCapture(control);
        return 0;
    }
    if (message == WM_MOUSEWHEEL) {
        RECT client{};
        GetClientRect(control, &client);
        shell.timelineWheelDeltaRemainder += int(GET_WHEEL_DELTA_WPARAM(key));
        const int notches = shell.timelineWheelDeltaRemainder / WHEEL_DELTA;
        shell.timelineWheelDeltaRemainder -= notches * WHEEL_DELTA;
        const int step = sfmEditorScaleMetricCandidate(75);
        if (notches) {
            shell.timelineScrollOffsetPixels = clampSfmEditorTimelineScrollCandidate(
                shell.timelineScrollOffsetPixels - notches * step, client.bottom);
        }
        InvalidateRect(control, nullptr, FALSE);
        return 0;
    }
    if (message == WM_GETDLGCODE) return DefSubclassProc(control, message, key, value) | DLGC_WANTARROWS;
    if (message == WM_KEYDOWN && GetFocus() == control &&
        applySfmEditorTimeDisplayShortcutCandidate(shell, UINT(key), (GetKeyState(VK_SHIFT) & 0x8000) != 0,
                                                   (value & (LPARAM(1) << 30)) != 0)) {
        InvalidateRect(shell.timelinePanel, nullptr, FALSE);
        return 0;
    }
    if (message == WM_MOUSEMOVE && shell.draggingTimelineClip) {
        RECT client{};GetClientRect(control,&client);
        const int left=sfmEditorScaleMetricCandidate(148),right=std::max(left+1,int(client.right)-sfmEditorScaleMetricCandidate(24));
        const size_t frame=timelineFrameAtPixelCandidate(short(LOWORD(value)),left,right,shell.frameCount);
        const int64_t delta=int64_t(frame)-int64_t(shell.timelineDragPointerFrame);
        const int64_t moved=int64_t(shell.timelineDragClipStartFrame)+delta;
        shell.pendingTimelineClipMoveStartFrame=size_t(std::clamp<int64_t>(moved,0,shell.frameCount?int64_t(shell.frameCount-1):0));
        InvalidateRect(control,nullptr,FALSE);
        return 0;
    }
    if (message == WM_MOUSEMOVE && shell.draggingTimeline) {
        seekSfmEditorTimelineAtPixelCandidate(short(LOWORD(value)));
        return 0;
    }
    if (message == WM_LBUTTONUP && shell.draggingTimelineClip) {
        shell.draggingTimelineClip=false;
        if (GetCapture()==control) ReleaseCapture();
        if (shell.pendingTimelineClipMoveStartFrame!=SIZE_MAX&&shell.pendingTimelineClipMoveStartFrame!=shell.timelineDragClipStartFrame)
            SendMessageA(shell.parent,WM_COMMAND,41027,0);
        InvalidateRect(control,nullptr,FALSE);
        return 0;
    }
    if (message == WM_LBUTTONUP && shell.draggingTimeline) {
        seekSfmEditorTimelineAtPixelCandidate(short(LOWORD(value)));
        shell.draggingTimeline = false;
        if (GetCapture() == control) ReleaseCapture();
        return 0;
    }
    if (message == WM_CAPTURECHANGED) { shell.draggingTimeline = false; shell.draggingTimelineClip = false; }
    return DefSubclassProc(control, message, key, value);
}

inline void layoutSfmEditorShellCandidate(HWND parent) {
    auto& shell = sfmEditorShellCandidate;
    if (!shell.parent || shell.parent != parent) return;
    RECT client{};
    GetClientRect(parent, &client);
    const auto layout = sfmEditorDockLayoutCandidate(client.right, client.bottom,
        shell.rootSplitBasisPoints, shell.timeSplitBasisPoints, sfmEditorEffectiveScalePercentCandidate(shell));
    const int tabHeight = sfmEditorScaleMetricCandidate(30);
    const int detailHeight = sfmEditorScaleMetricCandidate(112);
    const int editorTop = sfmEditorScaleMetricCandidate(34);
    const int editorBottom = int(client.bottom) - layout.timelineHeight;
    const int editorHeight = std::max(100, editorBottom - editorTop);
    auto move = [](HWND window, int x, int y, int width, int height) {
        if (window) MoveWindow(window, x, y, std::max(0, width), std::max(0, height), TRUE);
    };
    const int edge = sfmEditorScaleMetricCandidate(6);
    move(shell.animationTab, sfmEditorScaleMetricCandidate(4), sfmEditorScaleMetricCandidate(2), layout.leftWidth / 2 - sfmEditorScaleMetricCandidate(3), tabHeight);
    move(shell.elementTab, layout.leftWidth / 2 + sfmEditorScaleMetricCandidate(1), sfmEditorScaleMetricCandidate(2), layout.leftWidth / 2 - sfmEditorScaleMetricCandidate(4), tabHeight);
    move(shell.filter, edge, editorTop, layout.leftWidth - edge * 2, sfmEditorScaleMetricCandidate(24));
    move(shell.elements, edge, editorTop + sfmEditorScaleMetricCandidate(28), layout.leftWidth - edge * 2, std::max(sfmEditorScaleMetricCandidate(55), editorHeight - detailHeight - sfmEditorScaleMetricCandidate(36)));
    move(shell.details, edge, editorBottom - detailHeight, layout.leftWidth - edge * 2, detailHeight - edge);
    const int motionX = layout.leftWidth;
    move(shell.motionTab, motionX + sfmEditorScaleMetricCandidate(4), sfmEditorScaleMetricCandidate(2), layout.motionWidth / 3 - sfmEditorScaleMetricCandidate(3), tabHeight);
    move(shell.graphTab, motionX + layout.motionWidth / 3 + sfmEditorScaleMetricCandidate(1), sfmEditorScaleMetricCandidate(2), layout.motionWidth / 3 - sfmEditorScaleMetricCandidate(3), tabHeight);
    move(shell.clipTab, motionX + 2 * layout.motionWidth / 3, sfmEditorScaleMetricCandidate(2), layout.motionWidth / 3 - sfmEditorScaleMetricCandidate(4), tabHeight);
    move(shell.modeStatus, motionX + sfmEditorScaleMetricCandidate(8), editorTop, layout.motionWidth - sfmEditorScaleMetricCandidate(16), editorHeight - sfmEditorScaleMetricCandidate(38));
    move(shell.graphCurves, motionX + sfmEditorScaleMetricCandidate(14), editorTop + sfmEditorScaleMetricCandidate(8), layout.motionWidth - sfmEditorScaleMetricCandidate(28), sfmEditorScaleMetricCandidate(240));
    if (shell.graphCurves) ShowWindow(shell.graphCurves, shell.editorMode == 1 ? SW_SHOW : SW_HIDE);
    move(shell.clipShots, motionX + sfmEditorScaleMetricCandidate(14), editorTop + sfmEditorScaleMetricCandidate(8), layout.motionWidth - sfmEditorScaleMetricCandidate(28), sfmEditorScaleMetricCandidate(240));
    if (shell.clipShots) ShowWindow(shell.clipShots, shell.editorMode == 2 ? SW_SHOW : SW_HIDE);
    move(shell.verticalDockSplitter, layout.leftWidth - sfmEditorScaleMetricCandidate(3), 0, sfmEditorScaleMetricCandidate(6), int(client.bottom));
    move(shell.horizontalDockSplitter, layout.timelineLeft, layout.timelineTop - sfmEditorScaleMetricCandidate(3),
         layout.timelineWidth, sfmEditorScaleMetricCandidate(6));
    move(shell.timelinePanel, layout.timelineLeft, layout.timelineTop, layout.timelineWidth, layout.timelineHeight);
    if (shell.timelinePanel) {
        RECT timelineClient{};
        if (GetClientRect(shell.timelinePanel, &timelineClient))
            shell.timelineScrollOffsetPixels = clampSfmEditorTimelineScrollCandidate(
                shell.timelineScrollOffsetPixels, timelineClient.bottom);
    }
    move(shell.primaryViewportControls, layout.viewportLeft, layout.timelineTop - sfmEditorScaleMetricCandidate(38),
         layout.viewportWidth, sfmEditorScaleMetricCandidate(36));
    if (shell.primaryViewportControls)
        ShowWindow(shell.primaryViewportControls, shell.primaryViewportControlsVisible ? SW_SHOW : SW_HIDE);
    if (vulkanTimelineSlider) MoveWindow(vulkanTimelineSlider, layout.timelineLeft + sfmEditorScaleMetricCandidate(8),
        int(client.bottom) - sfmEditorScaleMetricCandidate(36), std::max(0, layout.timelineWidth - sfmEditorScaleMetricCandidate(16)), sfmEditorScaleMetricCandidate(28), TRUE);
}

inline LRESULT CALLBACK sfmEditorDockSplitterSubclassProcCandidate(HWND control, UINT message,
    WPARAM key, LPARAM value, UINT_PTR subclassId, DWORD_PTR) {
    auto& shell = sfmEditorShellCandidate;
    const bool root = subclassId == 10;
    if (message == WM_SETCURSOR) {
        SetCursor(LoadCursorA(nullptr, root ? IDC_SIZEWE : IDC_SIZENS));
        return TRUE;
    }
    if (message == WM_ERASEBKGND) {
        RECT client{};
        GetClientRect(control, &client);
        HBRUSH brush = CreateSolidBrush(RGB(73, 76, 80));
        FillRect(reinterpret_cast<HDC>(key), &client, brush);
        DeleteObject(brush);
        return 1;
    }
    if (message == WM_LBUTTONDOWN) {
        if (root) shell.draggingRootSplitter = true;
        else shell.draggingTimeSplitter = true;
        SetCapture(control);
        return 0;
    }
    if (message == WM_MOUSEMOVE && ((root && shell.draggingRootSplitter) ||
                                    (!root && shell.draggingTimeSplitter))) {
        POINT point{short(LOWORD(value)), short(HIWORD(value))};
        MapWindowPoints(control, shell.parent, &point, 1);
        RECT client{};
        GetClientRect(shell.parent, &client);
        if (updateSfmEditorDockBoundaryCandidate(shell, root, root ? point.x : point.y,
                                                  client.right, client.bottom)) {
            layoutSfmEditorShellCandidate(shell.parent);
            InvalidateRect(shell.parent, nullptr, FALSE);
        }
        return 0;
    }
    if (message == WM_LBUTTONUP) {
        shell.draggingRootSplitter = false;
        shell.draggingTimeSplitter = false;
        if (!shell.dockSettingsPath.empty())
            saveSfmEditorDockRatiosCandidate(shell, shell.dockSettingsPath);
        if (GetCapture() == control) ReleaseCapture();
        return 0;
    }
    if (message == WM_CAPTURECHANGED) {
        shell.draggingRootSplitter = false;
        shell.draggingTimeSplitter = false;
        return 0;
    }
    return DefSubclassProc(control, message, key, value);
}

inline bool createSfmEditorShellCandidate(HWND parent) {
    auto& shell = sfmEditorShellCandidate;
    if (shell.parent) return false;
    INITCOMMONCONTROLSEX common{sizeof(common), ICC_BAR_CLASSES};
    if (!InitCommonControlsEx(&common)) return false;
    shell.parent = parent;
    if (shell.dockSettingsPath.empty()) shell.dockSettingsPath = defaultSfmEditorDockSettingsPathCandidate();
    loadSfmEditorDockRatiosCandidate(shell, shell.dockSettingsPath);
    auto make = [parent](DWORD ex, const char* klass, const char* title, DWORD style, UINT id) {
        return CreateWindowExA(ex, klass, title, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0,
                               parent, reinterpret_cast<HMENU>(UINT_PTR(id)), GetModuleHandleA(nullptr), nullptr);
    };
    shell.animationTab = make(0, "BUTTON", "Animation Set Editor", BS_OWNERDRAW | WS_TABSTOP, SFM_EDITOR_ANIMATION_TAB);
    shell.elementTab = make(0, "BUTTON", "Element Viewer", BS_OWNERDRAW | WS_TABSTOP, SFM_EDITOR_ELEMENT_TAB);
    shell.filter = make(WS_EX_CLIENTEDGE, "EDIT", "", ES_AUTOHSCROLL | WS_TABSTOP, SFM_EDITOR_FILTER);
    shell.elements = make(WS_EX_CLIENTEDGE, "LISTBOX", "", LBS_NOTIFY | LBS_EXTENDEDSEL | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_TABSTOP, SFM_EDITOR_ELEMENTS);
    shell.details = make(WS_EX_CLIENTEDGE, "EDIT", "No session or model data loaded",
        ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL | WS_TABSTOP, SFM_EDITOR_DETAILS);
    shell.motionTab = make(0, "BUTTON", "Motion", BS_OWNERDRAW | WS_TABSTOP, SFM_EDITOR_MOTION_TAB);
    shell.graphTab = make(0, "BUTTON", "Graph", BS_OWNERDRAW | WS_TABSTOP, SFM_EDITOR_GRAPH_TAB);
    shell.clipTab = make(0, "BUTTON", "Clip", BS_OWNERDRAW | WS_TABSTOP, SFM_EDITOR_CLIP_TAB);
    shell.modeStatus = make(0, "STATIC", "Motion Editor\r\n\r\nTimeline playback is ready.", SS_OWNERDRAW, SFM_EDITOR_MODE_STATUS);
    shell.graphCurves = make(0, "COMBOBOX", "", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, SFM_EDITOR_GRAPH_CURVES);
    shell.clipShots = make(0, "COMBOBOX", "", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, SFM_EDITOR_CLIP_SHOTS);
    shell.timelinePanel = make(0, "STATIC", "", SS_OWNERDRAW | SS_NOTIFY | WS_TABSTOP, 0);
    shell.primaryViewportControls = make(0, "STATIC", "", SS_OWNERDRAW | SS_NOTIFY | WS_TABSTOP, SFM_EDITOR_PRIMARY_VIEWPORT_CONTROLS);
    shell.verticalDockSplitter = make(0, "STATIC", "", SS_NOTIFY, 0);
    shell.horizontalDockSplitter = make(0, "STATIC", "", SS_NOTIFY, 0);
    if (!shell.animationTab || !shell.elementTab || !shell.filter || !shell.elements || !shell.details ||
        !shell.motionTab || !shell.graphTab || !shell.clipTab || !shell.modeStatus || !shell.graphCurves ||
        !shell.clipShots || !shell.timelinePanel || !shell.primaryViewportControls || !shell.verticalDockSplitter || !shell.horizontalDockSplitter) {
        DestroyWindow(shell.timelinePanel); DestroyWindow(shell.primaryViewportControls);
        for (HWND control : {shell.animationTab, shell.elementTab, shell.filter, shell.elements, shell.details,
                             shell.motionTab, shell.graphTab, shell.clipTab, shell.modeStatus, shell.graphCurves,
                             shell.clipShots, shell.verticalDockSplitter, shell.horizontalDockSplitter}) DestroyWindow(control);
        shell = {};
        return false;
    }
    if (!SetWindowSubclass(shell.elements, sfmEditorElementsSubclassProcCandidate, 1, 0)) {
        DestroyWindow(shell.timelinePanel); DestroyWindow(shell.primaryViewportControls);
        for (HWND control : {shell.animationTab, shell.elementTab, shell.filter, shell.elements, shell.details,
                             shell.motionTab, shell.graphTab, shell.clipTab, shell.modeStatus, shell.graphCurves,
                             shell.clipShots, shell.verticalDockSplitter, shell.horizontalDockSplitter}) DestroyWindow(control);
        shell = {};
        return false;
    }
    if (!SetWindowSubclass(shell.timelinePanel, sfmEditorTimelineSubclassProcCandidate, 2, 0)) {
        DestroyWindow(shell.timelinePanel); DestroyWindow(shell.primaryViewportControls);
        for (HWND control : {shell.animationTab, shell.elementTab, shell.filter, shell.elements, shell.details,
                             shell.motionTab, shell.graphTab, shell.clipTab, shell.modeStatus, shell.graphCurves,
                             shell.clipShots, shell.verticalDockSplitter, shell.horizontalDockSplitter}) DestroyWindow(control);
        shell = {};
        return false;
    }
    if (!SetWindowSubclass(shell.primaryViewportControls, sfmEditorPrimaryControlsSubclassProcCandidate, 4, 0)) {
        RemoveWindowSubclass(shell.timelinePanel, sfmEditorTimelineSubclassProcCandidate, 2);
        DestroyWindow(shell.timelinePanel); DestroyWindow(shell.primaryViewportControls);
        for (HWND control : {shell.animationTab, shell.elementTab, shell.filter, shell.elements, shell.details,
                             shell.motionTab, shell.graphTab, shell.clipTab, shell.modeStatus, shell.graphCurves,
                             shell.clipShots, shell.verticalDockSplitter, shell.horizontalDockSplitter}) DestroyWindow(control);
        shell = {};
        return false;
    }
    if (!SetWindowSubclass(shell.modeStatus, sfmEditorGraphSubclassProcCandidate, 3, 0)) {
        RemoveWindowSubclass(shell.timelinePanel, sfmEditorTimelineSubclassProcCandidate, 2);
        RemoveWindowSubclass(shell.primaryViewportControls, sfmEditorPrimaryControlsSubclassProcCandidate, 4);
        DestroyWindow(shell.timelinePanel); DestroyWindow(shell.primaryViewportControls);
        for (HWND control : {shell.animationTab, shell.elementTab, shell.filter, shell.elements, shell.details,
                             shell.motionTab, shell.graphTab, shell.clipTab, shell.modeStatus, shell.graphCurves,
                             shell.clipShots, shell.verticalDockSplitter, shell.horizontalDockSplitter}) DestroyWindow(control);
        shell = {};
        return false;
    }
    if (!SetWindowSubclass(shell.verticalDockSplitter, sfmEditorDockSplitterSubclassProcCandidate, 10, 0) ||
        !SetWindowSubclass(shell.horizontalDockSplitter, sfmEditorDockSplitterSubclassProcCandidate, 11, 0)) {
        RemoveWindowSubclass(shell.modeStatus, sfmEditorGraphSubclassProcCandidate, 3);
        RemoveWindowSubclass(shell.timelinePanel, sfmEditorTimelineSubclassProcCandidate, 2);
        RemoveWindowSubclass(shell.primaryViewportControls, sfmEditorPrimaryControlsSubclassProcCandidate, 4);
        DestroyWindow(shell.timelinePanel); DestroyWindow(shell.primaryViewportControls);
        for (HWND control : {shell.animationTab, shell.elementTab, shell.filter, shell.elements, shell.details,
                             shell.motionTab, shell.graphTab, shell.clipTab, shell.modeStatus, shell.graphCurves,
                             shell.clipShots, shell.verticalDockSplitter, shell.horizontalDockSplitter}) DestroyWindow(control);
        shell = {};
        return false;
    }
    SendMessageA(shell.filter, EM_SETLIMITTEXT, 255, 0);
    SendMessageA(shell.filter, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
    for (HWND control : {shell.animationTab, shell.elementTab, shell.elements, shell.details,
                         shell.motionTab, shell.graphTab, shell.clipTab, shell.modeStatus, shell.graphCurves,
                         shell.clipShots, shell.primaryViewportControls, shell.verticalDockSplitter, shell.horizontalDockSplitter})
        SendMessageA(control, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
    setSfmEditorGraphCurvesCandidate(shell.graphCurveList);
    setSfmEditorTimelineSegmentsCandidate(shell.timelineSegments);
    applySfmEditorUiScaleCandidate(parent);
    // Session data may be loaded before the editor controls are created.
    // Populate the hierarchy now that its list box exists.
    refreshSfmEditorRowsCandidate();
    return true;
}

inline void createTimelineSliderCandidate(HWND parent) {
    INITCOMMONCONTROLSEX common{sizeof(common), ICC_BAR_CLASSES};
    if (!InitCommonControlsEx(&common)) throw std::runtime_error("timeline controls initialization");
    vulkanTimelineSlider = CreateWindowExA(0, TRACKBAR_CLASSA, "Timeline",
        WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS, 8, 0, 100, 28,
        parent, nullptr, GetModuleHandleA(nullptr), nullptr);
    if (!vulkanTimelineSlider) throw std::runtime_error("timeline slider creation");
    if (sfmEditorShellCandidate.uiFont)
        SendMessageA(vulkanTimelineSlider, WM_SETFONT, reinterpret_cast<WPARAM>(sfmEditorShellCandidate.uiFont), TRUE);
    if (sfmEditorShellCandidate.parent) layoutSfmEditorShellCandidate(parent);
    else {
        RECT client{};
        GetClientRect(parent, &client);
        MoveWindow(vulkanTimelineSlider, 8, int(client.bottom) - 36, std::max(0, int(client.right) - 16), 28, TRUE);
    }
}

inline void handleSfmEditorCommandCandidate(HWND parent, WPARAM command, LPARAM source) {
    auto& shell = sfmEditorShellCandidate;
    const UINT id = LOWORD(command);
    if (id == SFM_EDITOR_ANIMATION_TAB || id == SFM_EDITOR_ELEMENT_TAB) {
        shell.elementViewer = id == SFM_EDITOR_ELEMENT_TAB;
        refreshSfmEditorRowsCandidate();
    } else if (id == SFM_EDITOR_FILTER && HIWORD(command) == EN_CHANGE) {
        scheduleSfmEditorFilterRefreshCandidate(parent);
    } else if (id == SFM_EDITOR_ELEMENTS && HIWORD(command) == LBN_SELCHANGE) {
        selectSfmEditorElementCandidate();
    } else if (id == SFM_EDITOR_GRAPH_CURVES && HIWORD(command) == CBN_SELCHANGE) {
        const auto selected = SendMessageA(shell.graphCurves, CB_GETCURSEL, 0, 0);
        if (selected != CB_ERR) selectSfmEditorGraphCurveCandidate(size_t(selected));
    } else if (id == SFM_EDITOR_CLIP_SHOTS && HIWORD(command) == CBN_SELCHANGE) {
        const auto selected = SendMessageA(shell.clipShots, CB_GETCURSEL, 0, 0);
        if (selected >= 0 && size_t(selected) < shell.timelineSegments.size())
            SendMessageA(parent, SFM_EDITOR_SEEK_FRAME, 0, LPARAM(shell.timelineSegments[size_t(selected)].startFrame));
    } else if (id >= SFM_EDITOR_MOTION_TAB && id <= SFM_EDITOR_CLIP_TAB) {
        shell.editorMode = id - SFM_EDITOR_MOTION_TAB;
        const char* title = shell.editorMode == 0 ? "Motion Editor" : shell.editorMode == 1 ? "Graph Editor" : "Clip Editor";
        char buffer[256]{};
        if (shell.editorMode == 0) {
            std::snprintf(buffer, sizeof(buffer), "%s\r\n\r\nFrame %zu of %zu\r\n\r\nUse the timeline slider or Left/Right and Space to scrub and play.",
                title, shell.frameCount ? shell.selectedFrame + 1 : 0, shell.frameCount);
        } else if (shell.editorMode == 1) {
            std::snprintf(buffer, sizeof(buffer), "%s", title);
        } else {
            std::snprintf(buffer, sizeof(buffer), "%s", title);
        }
        SetWindowTextA(shell.modeStatus, buffer);
        layoutSfmEditorShellCandidate(parent);
    }
    if (id == SFM_EDITOR_ANIMATION_TAB || id == SFM_EDITOR_ELEMENT_TAB ||
        (id >= SFM_EDITOR_MOTION_TAB && id <= SFM_EDITOR_CLIP_TAB)) {
        for (HWND tab : {shell.animationTab, shell.elementTab, shell.motionTab, shell.graphTab, shell.clipTab})
            if (tab) InvalidateRect(tab, nullptr, TRUE);
    }
    (void)parent;
    (void)source;
}

inline void paintSfmEditorTimelineCandidate(const DRAWITEMSTRUCT& draw, size_t count, size_t frame) {
    if (draw.CtlType != ODT_STATIC) return;
    const auto dc = draw.hDC;
    struct FontRestore { HDC dc; HGDIOBJ old; ~FontRestore() { if (old) SelectObject(dc, old); } };
    FontRestore fontRestore{dc, SelectObject(dc, sfmEditorShellCandidate.uiFont ? sfmEditorShellCandidate.uiFont : GetStockObject(DEFAULT_GUI_FONT))};
    const auto scale = [](int value) { return sfmEditorScaleMetricCandidate(value); };
    RECT r = draw.rcItem;
    HBRUSH background = CreateSolidBrush(RGB(49, 49, 49));
    FillRect(dc, &r, background);
    DeleteObject(background);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(220, 220, 220));
    TextOutA(dc, scale(12), r.top + scale(8), "Timeline", 8);
    std::string summary = count ? "Frame " + std::to_string(std::min(frame, count - 1) + 1) + " / " + std::to_string(count)
                                : "No animation frames loaded";
    if (count) summary = sfmEditorFormatTimelineTimeCandidate(sfmEditorShellCandidate.timeDisplayMode, frame, count,
                                                                sfmEditorShellCandidate.framesPerSecond);
    TextOutA(dc, scale(100), r.top + scale(8), summary.c_str(), int(summary.size()));
    const int gearCenterX = int(r.right) - scale(18);
    const int gearCenterY = r.top + scale(16);
    HPEN gearPen = CreatePen(PS_SOLID, scale(1), RGB(160, 165, 170));
    HGDIOBJ oldGearPen = SelectObject(dc, gearPen);
    HGDIOBJ oldGearBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    Ellipse(dc, gearCenterX - scale(6), gearCenterY - scale(6), gearCenterX + scale(6), gearCenterY + scale(6));
    for (int tooth = 0; tooth < 8; ++tooth) {
        const double angle = tooth * 3.14159265358979323846 / 4.0;
        const int x0 = gearCenterX + int(std::cos(angle) * scale(7));
        const int y0 = gearCenterY + int(std::sin(angle) * scale(7));
        const int x1 = gearCenterX + int(std::cos(angle) * scale(9));
        const int y1 = gearCenterY + int(std::sin(angle) * scale(9));
        MoveToEx(dc, x0, y0, nullptr); LineTo(dc, x1, y1);
    }
    SelectObject(dc, oldGearBrush);
    SelectObject(dc, oldGearPen);
    DeleteObject(gearPen);
    const int left = scale(148);
    const int right = std::max(left + 1, int(r.right) - scale(24));
    const int rulerY = r.top + scale(54);
    HPEN ruler = CreatePen(PS_SOLID, 1, RGB(112, 112, 112));
    auto oldPen = SelectObject(dc, ruler);
    MoveToEx(dc, left, rulerY, nullptr);
    LineTo(dc, right, rulerY);
    if (count) {
        const size_t steps = std::min<size_t>(count, 64);
        for (size_t i = 0; i < steps; ++i) {
            const int x = left + int((uint64_t(i) * uint64_t(right - left)) / std::max<size_t>(1, steps - 1));
            MoveToEx(dc, x, rulerY - scale(6), nullptr);
            LineTo(dc, x, rulerY + scale(8));
            if (steps <= 16) {
                const auto label = std::to_string(i + 1);
                TextOutA(dc, x - scale(3), rulerY + scale(11), label.c_str(), int(label.size()));
            }
        }
        const size_t selected = std::min(frame, count - 1);
        const int x = timelinePixelAtFrameCandidate(selected, left, right, count);
        HPEN bookmarkPen = CreatePen(PS_SOLID, 1, RGB(244, 190, 76));
        HGDIOBJ previousBookmarkPen = SelectObject(dc, bookmarkPen);
        for (const size_t bookmarkFrame : sfmEditorShellCandidate.bookmarkFrames) {
            if (bookmarkFrame >= count) continue;
            const int bookmarkX = timelinePixelAtFrameCandidate(bookmarkFrame, left, right, count);
            MoveToEx(dc, bookmarkX, rulerY - scale(10), nullptr);
            LineTo(dc, bookmarkX, rulerY + scale(30));
        }
        SelectObject(dc, previousBookmarkPen);
        DeleteObject(bookmarkPen);
        HPEN playhead = CreatePen(PS_SOLID, 2, RGB(220, 54, 54));
        SelectObject(dc, playhead);
        MoveToEx(dc, x, rulerY - 18, nullptr);
        LineTo(dc, x, r.bottom - 10);
        SelectObject(dc, ruler);
        DeleteObject(playhead);
    }
    int laneY = rulerY + scale(38) - sfmEditorShellCandidate.timelineScrollOffsetPixels;
    const int laneBottom = int(r.bottom) - scale(8);
    if (count && laneY < laneBottom && !sfmEditorShellCandidate.timelineLanes.empty()) {
        std::string previousGroup;
        for (size_t laneIndex = 0; laneIndex < sfmEditorShellCandidate.timelineLanes.size(); ++laneIndex) {
            const auto& lane = sfmEditorShellCandidate.timelineLanes[laneIndex];
            if (lane.groupName != previousGroup) {
                if (laneY + scale(19) > laneBottom) break;
                RECT groupHeader{scale(4), laneY, int(r.right) - scale(4), laneY + scale(18)};
                HBRUSH headerBrush = CreateSolidBrush(RGB(57, 59, 62));
                FillRect(dc, &groupHeader, headerBrush);
                DeleteObject(headerBrush);
                SetTextColor(dc, RGB(205, 205, 210));
                TextOutA(dc, scale(8), laneY + scale(1), lane.groupName.c_str(), int(lane.groupName.size()));
                laneY += scale(19);
                previousGroup = lane.groupName;
            }
            if (laneY + scale(25) > laneBottom) break;
            RECT row{scale(4), laneY, int(r.right) - scale(4), laneY + scale(24)};
            HBRUSH rowBrush = CreateSolidBrush(laneIndex == sfmEditorShellCandidate.selectedTimelineLane ? RGB(55, 75, 94) : RGB(39, 40, 42));
            FillRect(dc, &row, rowBrush);
            DeleteObject(rowBrush);
            SetTextColor(dc, lane.visible ? RGB(195, 198, 202) : RGB(105, 107, 110));
            RECT trackLabel{scale(8), row.top, left - scale(5), row.bottom};
            DrawTextA(dc, lane.trackName.c_str(), int(lane.trackName.size()), &trackLabel,
                DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
            for (const auto& segment : lane.segments) {
                size_t begin = std::min(segment.startFrame, count);
                size_t end = std::min(segment.endFrame, count);
                if (sfmEditorShellCandidate.draggingTimelineClip && segment.element == sfmEditorShellCandidate.selectedTimelineClip &&
                    sfmEditorShellCandidate.pendingTimelineClipMoveStartFrame != SIZE_MAX) {
                    const int64_t delta = int64_t(sfmEditorShellCandidate.pendingTimelineClipMoveStartFrame) -
                        int64_t(sfmEditorShellCandidate.timelineDragClipStartFrame);
                    begin = size_t(std::clamp<int64_t>(int64_t(begin) + delta, 0, int64_t(count)));
                    end = size_t(std::clamp<int64_t>(int64_t(end) + delta, int64_t(begin), int64_t(count)));
                }
                if (begin >= end) continue;
                const int x0 = left + int((uint64_t(begin) * uint64_t(right - left)) / count);
                const int x1 = left + int((uint64_t(end) * uint64_t(right - left)) / count);
                RECT clip{std::max(left, x0), row.top + scale(2), std::max(std::max(left, x0) + 1, x1), row.bottom - scale(2)};
                COLORREF color = lane.clipType == 1 ? RGB(75, 128, 84) : lane.clipType == 2 ? RGB(148, 100, 61) :
                    lane.clipType == 0 ? RGB(105, 83, 139) : RGB(68, 96, 139);
                if (frame >= begin && frame < end) color = RGB(66, 142, 200);
                if (!lane.visible) color = RGB(GetRValue(color) / 2, GetGValue(color) / 2, GetBValue(color) / 2);
                HBRUSH clipBrush = CreateSolidBrush(color);
                FillRect(dc, &clip, clipBrush);
                DeleteObject(clipBrush);
                if (clip.right - clip.left >= scale(48)) {
                    SetTextColor(dc, RGB(240, 240, 240));
                    RECT labelRect{clip.left + scale(4), clip.top, clip.right - scale(2), clip.bottom};
                    DrawTextA(dc, segment.label.c_str(), int(segment.label.size()), &labelRect,
                        DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
                }
            }
            laneY += scale(25);
        }
    } else if (count && laneY < laneBottom) {
        const int rowBottom = std::min(laneBottom, laneY + scale(24));
        for (size_t i = 0; i < sfmEditorShellCandidate.timelineSegments.size(); ++i) {
            const auto& segment = sfmEditorShellCandidate.timelineSegments[i];
            const size_t begin = std::min(segment.startFrame, count);
            const size_t end = std::min(segment.endFrame, count);
            if (begin >= end) continue;
            const int x0 = left + int((uint64_t(begin) * uint64_t(right - left)) / count);
            const int x1 = left + int((uint64_t(end) * uint64_t(right - left)) / count);
            RECT clip{std::max(left, x0), laneY, std::max(std::max(left, x0) + 1, x1), rowBottom};
            const bool active = frame >= begin && frame < end;
            HBRUSH brush = CreateSolidBrush(active ? RGB(66, 142, 200) : i % 2 ? RGB(91, 105, 119) : RGB(78, 91, 105));
            FillRect(dc, &clip, brush);
            DeleteObject(brush);
            if (clip.right - clip.left >= scale(64)) {
                SetTextColor(dc, RGB(240, 240, 240));
                RECT labelRect{clip.left + scale(4), clip.top, clip.right - scale(2), clip.bottom};
                DrawTextA(dc, segment.label.c_str(), int(segment.label.size()), &labelRect,
                    DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
            }
        }
    }
    SelectObject(dc, oldPen);
    DeleteObject(ruler);
}

inline RECT sfmEditorGraphPlotRectCandidate(int width, int height) {
    const auto scale = [](int value) { return sfmEditorScaleMetricCandidate(value); };
    // The curve selector is a sibling control at editorTop + 8 (relative y=42).
    // Keep the curve title and plot below that header so keys never paint under it.
    return {scale(48), scale(96), std::max(scale(49), width - scale(16)), std::max(scale(97), height - scale(38))};
}

inline POINT sfmEditorGraphPointCandidate(const std::vector<SfmEditorGraphKeyCandidate>& keys,
                                           size_t index, const RECT& plot, float overrideValue, int32_t overrideTick = 0,
                                           bool useOverride = false) {
    if (index >= keys.size() || keys.empty()) throw std::runtime_error("graph key point index outside curve");
    int32_t minTick = keys.front().tick, maxTick = minTick;
    float minValue = keys.front().value, maxValue = minValue;
    for (const auto& key : keys) { minTick = std::min(minTick, key.tick); maxTick = std::max(maxTick, key.tick); minValue = std::min(minValue, key.value); maxValue = std::max(maxValue, key.value); }
    if (minValue == maxValue) { minValue -= 1; maxValue += 1; }
    const int64_t tickSpan = std::max<int64_t>(1, int64_t(maxTick) - minTick);
    const float value = useOverride && index == sfmEditorShellCandidate.selectedGraphKey ? overrideValue : keys[index].value;
    const int32_t tick = useOverride && index == sfmEditorShellCandidate.selectedGraphKey ? overrideTick : keys[index].tick;
    const int x = plot.left + int((int64_t(tick) - minTick) * (plot.right - plot.left) / tickSpan);
    const int y = plot.bottom - int((value - minValue) * float(plot.bottom - plot.top) / (maxValue - minValue));
    return {x, y};
}

inline POINT sfmEditorGraphPointForValueCandidate(const std::vector<SfmEditorGraphKeyCandidate>& keys,
                                                   const RECT& plot, int32_t tick, float value) {
    if (keys.empty()) throw std::runtime_error("graph curve point outside empty curve");
    int32_t minTick = keys.front().tick, maxTick = minTick;
    float minValue = value, maxValue = value;
    for (const auto& key : keys) {
        minTick = std::min(minTick, key.tick); maxTick = std::max(maxTick, key.tick);
        minValue = std::min(minValue, key.value); maxValue = std::max(maxValue, key.value);
    }
    if (minValue == maxValue) { minValue -= 1; maxValue += 1; }
    const int64_t tickSpan = std::max<int64_t>(1, int64_t(maxTick) - minTick);
    const int x = plot.left + int((int64_t(tick) - minTick) * (plot.right - plot.left) / tickSpan);
    const int y = plot.bottom - int((value - minValue) * float(plot.bottom - plot.top) / (maxValue - minValue));
    return {x, y};
}

inline int32_t sfmEditorGraphTickAtPixelCandidate(const std::vector<SfmEditorGraphKeyCandidate>& keys,
                                                   const RECT& plot, int x, size_t index) {
    if (keys.empty() || index >= keys.size() || plot.right <= plot.left)
        throw std::runtime_error("graph key time mapping outside curve");
    int32_t minTick=keys.front().tick,maxTick=minTick;
    for(const auto&key:keys){minTick=std::min(minTick,key.tick);maxTick=std::max(maxTick,key.tick);}
    if(minTick==maxTick)return minTick;
    const int32_t previous=index?keys[index-1].tick:INT32_MIN;
    const int32_t next=index+1<keys.size()?keys[index+1].tick:INT32_MAX;
    const int boundedX=std::clamp(x,int(plot.left),int(plot.right));
    const double t=double(boundedX-plot.left)/double(plot.right-plot.left);
    const auto projected=static_cast<int64_t>(std::llround(double(minTick)+t*double(int64_t(maxTick)-minTick)));
    return int32_t(std::clamp<int64_t>(projected,previous,next));
}

inline std::optional<size_t> hitTestSfmEditorGraphKeyCandidate(const std::vector<SfmEditorGraphKeyCandidate>& keys,
                                                               const RECT& plot, int x, int y, int radius = 9) {
    std::optional<size_t> nearest;
    int64_t nearestDistance = int64_t(radius) * radius + 1;
    for (size_t i = 0; i < keys.size(); ++i) {
        auto p = sfmEditorGraphPointCandidate(keys, i, plot, 0);
        const int64_t dx = x - p.x, dy = y - p.y;
        const int64_t distance = dx * dx + dy * dy;
        if (distance <= int64_t(radius) * radius && distance < nearestDistance) { nearest = i; nearestDistance = distance; }
    }
    return nearest;
}

inline float sfmEditorGraphValueAtPixelCandidate(const std::vector<SfmEditorGraphKeyCandidate>& keys,
                                                  const RECT& plot, int y) {
    if (keys.empty() || plot.bottom <= plot.top) throw std::runtime_error("graph curve has no drawable range");
    float minValue = keys.front().value, maxValue = minValue;
    for (const auto& key : keys) { minValue = std::min(minValue, key.value); maxValue = std::max(maxValue, key.value); }
    if (minValue == maxValue) { minValue -= 1; maxValue += 1; }
    const float t = std::clamp(float(int(plot.bottom) - std::clamp(y, int(plot.top), int(plot.bottom))) / float(plot.bottom - plot.top), 0.0f, 1.0f);
    return minValue + t * (maxValue - minValue);
}

inline bool queueSfmEditorGraphKeyEditCandidate(const SfmEditorGraphKeyEditRequestCandidate& request) {
    auto& shell = sfmEditorShellCandidate;
    if (!shell.graphKeyQueue || shell.selectedGraphCurve >= shell.graphCurveList.size()) return false;
    auto& curve = shell.graphCurveList[shell.selectedGraphCurve];
    if (curve.element != request.element || request.index >= curve.keys.size() || !std::isfinite(request.value)) return false;
    if (!std::isfinite(request.value) || request.tick < (request.index ? curve.keys[request.index-1].tick : INT32_MIN) ||
        request.tick > (request.index+1<curve.keys.size() ? curve.keys[request.index+1].tick : INT32_MAX)) return false;
    if (!shell.graphKeyQueue(request.element, request.index, request.tick, request.value, curve.vectorComponent)) return false;
    // The playback thread applies this request later. Keep the displayed
    // source curve unchanged until its document transaction succeeds.
    return true;
}

inline LRESULT CALLBACK sfmEditorGraphSubclassProcCandidate(HWND control, UINT message, WPARAM key,
                                                              LPARAM value, UINT_PTR, DWORD_PTR) {
    auto& shell = sfmEditorShellCandidate;
    if (message == WM_GETDLGCODE) return DefSubclassProc(control, message, key, value) | DLGC_WANTALLKEYS;
    RECT client{}; GetClientRect(control, &client);
    const RECT plot = sfmEditorGraphPlotRectCandidate(client.right, client.bottom);
    if (shell.editorMode == 1 && message == WM_KEYDOWN && key == VK_DELETE &&
        GetFocus() == control && !shell.selectedGraphKeys.empty() &&
        shell.selectedGraphCurve < shell.graphCurveList.size() && shell.graphKeyDeleteQueue) {
        const auto& curve = shell.graphCurveList[shell.selectedGraphCurve];
        if (shell.graphKeyDeleteQueue(curve.element, shell.selectedGraphKeys)) {
            shell.selectedGraphKey = SIZE_MAX;
            shell.selectedGraphKeys.clear();
            InvalidateRect(control, nullptr, FALSE);
            return 0;
        }
    }
    if (shell.editorMode == 1 && message == WM_CONTEXTMENU) {
        POINT screen{short(LOWORD(value)), short(HIWORD(value))};
        if (screen.x == -1 && screen.y == -1) GetCursorPos(&screen);
        POINT local = screen;
        if (!ScreenToClient(control, &local)) return 0;
        if (local.x < plot.left || local.x > plot.right || local.y < plot.top || local.y > plot.bottom) return 0;
        auto selected = hitTestSfmEditorGraphKeyCandidate(shell.graphKeys, plot, local.x, local.y);
        if (!selected) return 0;
        SetFocus(control);
        if (std::find(shell.selectedGraphKeys.begin(), shell.selectedGraphKeys.end(), *selected) == shell.selectedGraphKeys.end()) {
            shell.selectedGraphKeys.assign(1, *selected);
        }
        shell.selectedGraphKey = *selected;
        if (shell.selectedGraphCurve >= shell.graphCurveList.size()) return 0;
        HMENU menu = createSfmEditorGraphKeyContextMenuCandidate(bool(shell.graphKeyDeleteQueue));
        const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
            screen.x, screen.y, 0, shell.parent, nullptr);
        DestroyMenu(menu);
        if (command == 41006 && shell.graphKeyDialog) {
            const auto& curve = shell.graphCurveList[shell.selectedGraphCurve];
            shell.graphKeyDialog(curve.element, *selected, curve.vectorComponent);
        }
        if (command == 41026 && shell.graphKeyDeleteQueue) {
            const auto& curve = shell.graphCurveList[shell.selectedGraphCurve];
            const auto& targets = std::find(shell.selectedGraphKeys.begin(), shell.selectedGraphKeys.end(), *selected) != shell.selectedGraphKeys.end()
                ? shell.selectedGraphKeys : std::vector<size_t>{*selected};
            shell.graphKeyDeleteQueue(curve.element, targets);
        }
        InvalidateRect(control, nullptr, FALSE);
        return 0;
    }
    if (shell.editorMode == 1 && message == WM_LBUTTONDOWN) {
        const int x = short(LOWORD(value)), y = short(HIWORD(value));
        if (x < plot.left || x > plot.right || y < plot.top || y > plot.bottom) return DefSubclassProc(control, message, key, value);
        SetFocus(control);
        auto selected = hitTestSfmEditorGraphKeyCandidate(shell.graphKeys, plot, x, y);
        if (selected) {
            const unsigned mode = sfmEditorGraphSelectionModeCandidate((GetKeyState(VK_CONTROL) & 0x8000) != 0,
                (GetKeyState(VK_MENU) & 0x8000) != 0);
            applySfmEditorGraphSelectionModeCandidate(shell.selectedGraphKeys, *selected, mode);
            shell.selectedGraphKey = *selected;
            if (mode != 0) { shell.draggingGraphKey = false; InvalidateRect(control, nullptr, FALSE); return 0; }
            shell.graphPreviewValue = shell.graphKeys[*selected].value;
            shell.graphPreviewTick = shell.graphKeys[*selected].tick;
            shell.draggingGraphKey = true;
            SetCapture(control); InvalidateRect(control, nullptr, FALSE); return 0;
        }
        shell.selectingGraphKeys = true;
        shell.graphSelectionStart = shell.graphSelectionEnd = POINT{x, y};
        shell.graphSelectionMode = sfmEditorGraphSelectionModeCandidate((GetKeyState(VK_CONTROL) & 0x8000) != 0,
            (GetKeyState(VK_MENU) & 0x8000) != 0);
        SetCapture(control); InvalidateRect(control, nullptr, FALSE); return 0;
    }
    if (message == WM_MOUSEMOVE && shell.selectingGraphKeys) {
        shell.graphSelectionEnd = POINT{short(LOWORD(value)), short(HIWORD(value))};
        InvalidateRect(control, nullptr, FALSE); return 0;
    }
    if (message == WM_LBUTTONUP && shell.selectingGraphKeys) {
        shell.graphSelectionEnd = POINT{short(LOWORD(value)), short(HIWORD(value))};
        const auto hits = sfmEditorGraphKeysInRectCandidate(shell.graphKeys, plot,
            normalizeSfmEditorGraphSelectionRectCandidate(shell.graphSelectionStart, shell.graphSelectionEnd));
        if (shell.graphSelectionMode == 0) shell.selectedGraphKeys.clear();
        for (size_t index : hits) applySfmEditorGraphSelectionModeCandidate(shell.selectedGraphKeys, index, shell.graphSelectionMode);
        shell.selectedGraphKey = shell.selectedGraphKeys.empty() ? SIZE_MAX : shell.selectedGraphKeys.back();
        shell.selectingGraphKeys = false;
        if (GetCapture() == control) ReleaseCapture();
        InvalidateRect(control, nullptr, FALSE); return 0;
    }
    if (message == WM_MOUSEMOVE && shell.draggingGraphKey && shell.selectedGraphKey < shell.graphKeys.size()) {
        shell.graphPreviewValue = sfmEditorGraphValueAtPixelCandidate(shell.graphKeys, plot, short(HIWORD(value)));
        shell.graphPreviewTick = sfmEditorGraphTickAtPixelCandidate(shell.graphKeys, plot, short(LOWORD(value)), shell.selectedGraphKey);
        InvalidateRect(control, nullptr, FALSE); return 0;
    }
    if (message == WM_LBUTTONUP && shell.draggingGraphKey) {
        shell.draggingGraphKey = false;
        if (GetCapture() == control) ReleaseCapture();
        if (shell.selectedGraphKey < shell.graphKeys.size() && shell.graphKeyQueue) {
            const auto& curve = shell.graphCurveList.at(shell.selectedGraphCurve);
            SfmEditorGraphKeyEditRequestCandidate request{curve.element, shell.selectedGraphKey, shell.graphPreviewValue, shell.graphPreviewTick, curve.vectorComponent};
            SendMessageA(shell.parent, SFM_EDITOR_GRAPH_KEY_EDIT, 0, LPARAM(&request));
        }
        InvalidateRect(control, nullptr, FALSE); return 0;
    }
    if (message == WM_CAPTURECHANGED) { shell.draggingGraphKey = false; shell.selectingGraphKeys = false; }
    return DefSubclassProc(control, message, key, value);
}

inline void paintSfmEditorGraphCandidate(const DRAWITEMSTRUCT& draw) {
    if (draw.CtlType != ODT_STATIC || draw.CtlID != SFM_EDITOR_MODE_STATUS) return;
    HDC dc = draw.hDC;
    struct FontRestore { HDC dc; HGDIOBJ old; ~FontRestore() { if (old) SelectObject(dc, old); } };
    FontRestore fontRestore{dc, SelectObject(dc, sfmEditorShellCandidate.uiFont ? sfmEditorShellCandidate.uiFont : GetStockObject(DEFAULT_GUI_FONT))};
    const auto scale = [](int value) { return sfmEditorScaleMetricCandidate(value); };
    RECT r = draw.rcItem;
    HBRUSH background = CreateSolidBrush(RGB(49, 49, 49));
    FillRect(dc, &r, background);
    DeleteObject(background);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(225, 225, 225));
    const auto& shell = sfmEditorShellCandidate;
    if (shell.editorMode != 1) {
        const char* label = shell.editorMode == 0 ? "Motion Editor" : "Clip Editor";
        TextOutA(dc, scale(12), scale(12), label, int(std::strlen(label)));
        char status[128]{};
        std::snprintf(status, sizeof(status), "Frame %zu / %zu", shell.frameCount ? shell.selectedFrame + 1 : 0, shell.frameCount);
        TextOutA(dc, scale(12), scale(38), status, int(std::strlen(status)));
        if(shell.editorMode==0&&!shell.sceneAssetWarning.empty()){
            SetTextColor(dc,RGB(255,190,100));
            const auto&warning=shell.sceneAssetWarning;TextOutA(dc,scale(12),scale(66),warning.c_str(),int(std::min<size_t>(warning.size(),180)));
            SetTextColor(dc,RGB(225,225,225));
        }
        if (shell.editorMode == 2) {
            auto clip = std::find_if(shell.timelineSegments.begin(), shell.timelineSegments.end(), [&](const auto& segment) {
                return shell.selectedFrame >= segment.startFrame && shell.selectedFrame < segment.endFrame;
            });
            if (clip != shell.timelineSegments.end()) {
                const std::string name = "Selected shot: " + clip->label;
                const std::string range = "Frames " + std::to_string(clip->startFrame + 1) + " to " + std::to_string(clip->endFrame) +
                    " (" + std::to_string(clip->endFrame - clip->startFrame) + " frames)";
                TextOutA(dc, scale(12), scale(76), name.c_str(), int(name.size()));
                TextOutA(dc, scale(12), scale(100), range.c_str(), int(range.size()));
                const char* hint = "Choose a shot above; right-click its timeline range to trim at the playhead.";
                TextOutA(dc, scale(12), scale(132), hint, int(std::strlen(hint)));
            } else TextOutA(dc, scale(12), scale(76), "No shot contains the current playhead.", 37);
        }
        return;
    }
    TextOutA(dc, scale(12), scale(12), "Graph Editor", 12);
    if (shell.graphKeys.empty()) {
        TextOutA(dc, scale(12), scale(42), "No float animation curve found in this session.", 44);
        return;
    }
    const std::string title = shell.graphLayerLabel + "  |  " + std::to_string(shell.graphKeys.size()) + " keys";
    TextOutA(dc, scale(12), scale(62), title.c_str(), int(title.size()));
    RECT plot = sfmEditorGraphPlotRectCandidate(int(r.right), int(r.bottom));
    HPEN grid = CreatePen(PS_SOLID, 1, RGB(83, 83, 83));
    HGDIOBJ oldPen = SelectObject(dc, grid);
    for (int i = 0; i <= 4; ++i) {
        int y = plot.top + (plot.bottom - plot.top) * i / 4;
        MoveToEx(dc, plot.left, y, nullptr); LineTo(dc, plot.right, y);
    }
    SelectObject(dc, oldPen);
    DeleteObject(grid);
    auto point = [&](size_t index) { return sfmEditorGraphPointCandidate(shell.graphKeys, index, plot, shell.graphPreviewValue, shell.graphPreviewTick, shell.draggingGraphKey); };
    HPEN curve = CreatePen(PS_SOLID, 2, RGB(84, 179, 235));
    oldPen = SelectObject(dc, curve);
    HBRUSH pointBrush = CreateSolidBrush(RGB(84, 179, 235));
    HBRUSH selectedBrush = CreateSolidBrush(RGB(255, 210, 90));
    for (size_t i = 0; i < shell.graphKeys.size(); ++i) {
        auto p = point(i);
        if (i == 0) MoveToEx(dc, p.x, p.y, nullptr); else LineTo(dc, p.x, p.y);
    }
    if (shell.selectingGraphKeys) {
        const RECT selection = normalizeSfmEditorGraphSelectionRectCandidate(shell.graphSelectionStart, shell.graphSelectionEnd);
        HPEN selectionPen = CreatePen(PS_DOT, 1, RGB(110, 185, 235));
        const int saved = SaveDC(dc);
        IntersectClipRect(dc, plot.left, plot.top, plot.right + 1, plot.bottom + 1);
        HGDIOBJ priorPen = SelectObject(dc, selectionPen);
        HGDIOBJ priorBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
        Rectangle(dc, selection.left, selection.top, selection.right + 1, selection.bottom + 1);
        SelectObject(dc, priorBrush); SelectObject(dc, priorPen); DeleteObject(selectionPen);
        if (saved) RestoreDC(dc, saved);
    }
    for (size_t i = 0; i < shell.graphKeys.size(); ++i) {
        auto p = point(i);
        HGDIOBJ oldBrush = SelectObject(dc, std::find(shell.selectedGraphKeys.begin(), shell.selectedGraphKeys.end(), i) != shell.selectedGraphKeys.end() ? selectedBrush : pointBrush);
        const int radius = scale(3);
        Ellipse(dc, p.x - radius, p.y - radius, p.x + radius + 1, p.y + radius + 1);
        SelectObject(dc, oldBrush);
    }
    SelectObject(dc, oldPen);
    DeleteObject(curve);
    DeleteObject(pointBrush);
    DeleteObject(selectedBrush);
    SetTextColor(dc, RGB(190, 190, 190));
    TextOutA(dc, plot.left, plot.bottom + scale(8), "time (ticks)", 12);
}
