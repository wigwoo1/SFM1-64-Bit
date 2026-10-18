#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <cassert>
#include <fstream>
#include "win32_sfm_editor_shell_candidate.h"

static UINT receivedCommand = 0;
static LRESULT CALLBACK parentProc(HWND window, UINT message, WPARAM key, LPARAM value) {
    if (message == WM_COMMAND) {
        receivedCommand = LOWORD(key);
        return 0;
    }
    return DefWindowProcA(window, message, key, value);
}

int main() {
    const auto commandAt = [](int x) { return sfmEditorPrimaryControlCommandAtCandidate(x, 6, 82, 3, 44); };
    assert(commandAt(-1) == 0);
    assert(commandAt(5) == 0);
    assert(commandAt(6) == SFM_EDITOR_VIEWPORT_MODE_MENU);
    assert(commandAt(88) == 0);
    assert(commandAt(89) == 0);
    assert(commandAt(91) == SFM_EDITOR_VIEWPORT_HOME);
    assert(commandAt(135) == SFM_EDITOR_VIEWPORT_PREVIOUS);
    assert(commandAt(179) == SFM_EDITOR_VIEWPORT_PLAY);
    assert(commandAt(223) == SFM_EDITOR_VIEWPORT_NEXT);
    assert(commandAt(267) == SFM_EDITOR_VIEWPORT_END);
    assert(commandAt(311) == 0);
    assert(sfmEditorPrimaryControlCommandAtCandidate(6, 6, 82, 3, 0) == 0);
    SfmEditorShellCandidate scaledShell{};
    scaledShell.uiScalePercent = 150;
    const int scaledInset = sfmEditorScaleMetricCandidate(scaledShell, 6);
    const int scaledModeWidth = sfmEditorScaleMetricCandidate(scaledShell, 82);
    const int scaledGap = sfmEditorScaleMetricCandidate(scaledShell, 3);
    const int scaledButtonWidth = sfmEditorScaleMetricCandidate(scaledShell, 44);
    assert(sfmEditorPrimaryControlCommandAtCandidate(scaledInset, scaledInset, scaledModeWidth, scaledGap,
        scaledButtonWidth) == SFM_EDITOR_VIEWPORT_MODE_MENU);
    assert(sfmEditorPrimaryControlCommandAtCandidate(scaledInset + scaledModeWidth + scaledGap, scaledInset,
        scaledModeWidth, scaledGap, scaledButtonWidth) == SFM_EDITOR_VIEWPORT_HOME);
    assert(SFM_EDITOR_VIEW_FULLSCREEN != SFM_EDITOR_VIEW_PRIMARY_CONTROLS);
    assert(std::string(sfmEditorFullscreenMenuLabelCandidate(false)) == "Show Fullscreen");
    assert(std::string(sfmEditorFullscreenMenuLabelCandidate(true)) == "Show Normal");
    for (UINT tool = 0; tool < 6; ++tool) {
        UINT recovered = 99;
        assert(sfmEditorViewportModeFromCommandCandidate(SFM_EDITOR_VIEWPORT_MODE_SELECTION + tool, recovered));
        assert(recovered == tool);
    }
    UINT invalidTool = 0;
    assert(!sfmEditorViewportModeFromCommandCandidate(SFM_EDITOR_VIEWPORT_MODE_SELECTION - 1, invalidTool));
    assert(!sfmEditorViewportModeFromCommandCandidate(SFM_EDITOR_VIEWPORT_MODE_SCREEN + 1, invalidTool));
    SfmEditorShellCandidate modeShell{};
    for (UINT mode = 0; mode < 6; ++mode) {
        assert(applySfmEditorViewportModeCommandCandidate(modeShell, SFM_EDITOR_VIEWPORT_MODE_SELECTION + mode));
        assert(modeShell.activeViewportToolMode == mode);
    }
    assert(!applySfmEditorViewportModeCommandCandidate(modeShell, SFM_EDITOR_VIEWPORT_MODE_SCREEN + 1));
    assert(modeShell.activeViewportToolMode == 5);
    HMENU modeMenu = createSfmEditorViewportToolModeMenuCandidate(4);
    assert(modeMenu);
    const char* modeLabels[] = {"Selection\tQ", "Translation\tW", "Global Translation\tShift+W", "Rotation\tE", "Global Rotation\tShift+E", "Screen\tR"};
    for (UINT i = 0; i < 6; ++i) {
        char label[64]{};
        MENUITEMINFOA item{sizeof(item)};
        item.fMask = MIIM_STRING | MIIM_STATE;
        item.dwTypeData = label;
        item.cch = sizeof(label);
        assert(GetMenuItemInfoA(modeMenu, SFM_EDITOR_VIEWPORT_MODE_SELECTION + i, FALSE, &item));
        assert(std::string(label) == modeLabels[i]);
        assert(bool(item.fState & MFS_CHECKED) == (i == 4));
    }
    DestroyMenu(modeMenu);

    SfmEditorShellCandidate displayShell{};
    HMENU viewportMenu = createSfmEditorViewportContextMenuCandidate(displayShell);
    assert(viewportMenu);
    HMENU displayMenu = GetSubMenu(viewportMenu, 0);
    assert(displayMenu);
    const UINT displayCommands[] = {SFM_EDITOR_VIEWPORT_DISPLAY_FOCAL_PLANE,
        SFM_EDITOR_VIEWPORT_DISPLAY_CAMERA_FRUSTUM, SFM_EDITOR_VIEWPORT_DISPLAY_VIEW_TARGETS};
    const char* displayLabels[] = {"Show Focal Plane", "Show Camera Frustum", "Show View Targets"};
    for (size_t i = 0; i < std::size(displayCommands); ++i) {
        char label[64]{};
        MENUITEMINFOA item{sizeof(item)};
        item.fMask = MIIM_STRING | MIIM_STATE;
        item.dwTypeData = label;
        item.cch = sizeof(label);
        assert(GetMenuItemInfoA(displayMenu, displayCommands[i], FALSE, &item));
        assert(std::string(label) == displayLabels[i]);
        assert(bool(item.fState & MFS_CHECKED));
    }
    assert(applySfmEditorViewportDisplayCommandCandidate(displayShell, displayCommands[0]));
    assert(!displayShell.showFocalPlane);
    assert(applySfmEditorViewportDisplayCommandCandidate(displayShell, displayCommands[1]));
    assert(!displayShell.showCameraFrustum);
    assert(applySfmEditorViewportDisplayCommandCandidate(displayShell, displayCommands[2]));
    assert(!displayShell.showViewTargets);
    assert(!applySfmEditorViewportDisplayCommandCandidate(displayShell, displayCommands[2] + 1));
    displayShell.primaryViewportControlsVisible = false;
    char settingsDirectory[MAX_PATH]{};
    char settingsPath[MAX_PATH]{};
    assert(GetTempPathA(MAX_PATH, settingsDirectory));
    assert(GetTempFileNameA(settingsDirectory, "sfm", 0, settingsPath));
    assert(saveSfmEditorDockRatiosCandidate(displayShell, settingsPath));
    SfmEditorShellCandidate restoredDisplayShell{};
    assert(loadSfmEditorDockRatiosCandidate(restoredDisplayShell, settingsPath));
    assert(!restoredDisplayShell.showFocalPlane && !restoredDisplayShell.showCameraFrustum && !restoredDisplayShell.showViewTargets);
    assert(!restoredDisplayShell.primaryViewportControlsVisible);
    assert(DeleteFileA(settingsPath));
    assert(GetTempFileNameA(settingsDirectory, "sfm", 0, settingsPath));
    {
        std::ofstream legacy(settingsPath, std::ios::binary | std::ios::trunc);
        legacy << "[EditorDock]\nRootSplitBasisPoints=23819\nTimeSplitBasisPoints=33233\nUiScalePercent=100\n";
        assert(legacy.good());
    }
    SfmEditorShellCandidate legacyDisplayShell{};
    assert(loadSfmEditorDockRatiosCandidate(legacyDisplayShell, settingsPath));
    assert(legacyDisplayShell.showFocalPlane && legacyDisplayShell.showCameraFrustum && legacyDisplayShell.showViewTargets);
    assert(legacyDisplayShell.primaryViewportControlsVisible);
    assert(DeleteFileA(settingsPath));
    DestroyMenu(viewportMenu);
    const RECT viewBounds = sfmEditorViewportClientRectCandidate(1920, 1080, displayShell);
    assert(viewBounds.left > 0 && viewBounds.right == 1920);
    assert(viewBounds.top == 0 && viewBounds.bottom > 0 && viewBounds.bottom < 1080);
    assert(PtInRect(&viewBounds, POINT{viewBounds.left, viewBounds.top}));
    assert(!PtInRect(&viewBounds, POINT{viewBounds.left - 1, viewBounds.top}));
    assert(!PtInRect(&viewBounds, POINT{viewBounds.right, viewBounds.bottom}));

    INITCOMMONCONTROLSEX common{sizeof(common), ICC_STANDARD_CLASSES};
    (void)InitCommonControlsEx(&common);
    WNDCLASSA klass{};
    klass.lpfnWndProc = parentProc;
    klass.hInstance = GetModuleHandleA(nullptr);
    klass.lpszClassName = "SfmPrimaryControlsInteractionTest";
    assert(RegisterClassA(&klass));
    HWND parent = CreateWindowExA(0, klass.lpszClassName, "", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, nullptr, nullptr, klass.hInstance, nullptr);
    assert(parent);
    HACCEL editorAccelerators = createSfmEditorAcceleratorTableCandidate();
    assert(editorAccelerators);
    ACCEL acceleratorRows[20]{};
    const int acceleratorCount = CopyAcceleratorTableA(editorAccelerators, acceleratorRows, int(std::size(acceleratorRows)));
    assert(acceleratorCount == 15);
    const auto hasAccelerator = [&](BYTE key, BYTE modifiers, UINT command) {
        return std::any_of(acceleratorRows, acceleratorRows + acceleratorCount, [&](const ACCEL& entry) {
            return entry.key == key && (entry.fVirt & (FSHIFT | FCONTROL | FALT)) == modifiers && entry.cmd == command;
        });
    };
    assert(hasAccelerator('Q', 0, SFM_EDITOR_VIEWPORT_MODE_SELECTION));
    assert(hasAccelerator('W', 0, SFM_EDITOR_VIEWPORT_MODE_TRANSLATION));
    assert(hasAccelerator('W', FSHIFT, SFM_EDITOR_VIEWPORT_MODE_GLOBAL_TRANSLATION));
    assert(hasAccelerator('E', 0, SFM_EDITOR_VIEWPORT_MODE_ROTATION));
    assert(hasAccelerator('E', FSHIFT, SFM_EDITOR_VIEWPORT_MODE_GLOBAL_ROTATION));
    assert(hasAccelerator('R', 0, SFM_EDITOR_VIEWPORT_MODE_SCREEN));
    DestroyAcceleratorTable(editorAccelerators);
    const LONG_PTR originalStyle = GetWindowLongPtrA(parent, GWL_STYLE);
    WINDOWPLACEMENT originalPlacement{sizeof(WINDOWPLACEMENT)};
    assert(GetWindowPlacement(parent, &originalPlacement));
    SfmEditorFullscreenStateCandidate fullscreen{};
    MONITORINFO expectedMonitor{sizeof(MONITORINFO)};
    const HMONITOR monitor = MonitorFromWindow(parent, MONITOR_DEFAULTTONEAREST);
    assert(monitor && GetMonitorInfoA(monitor, &expectedMonitor));
    assert(setSfmEditorFullscreenCandidate(parent, fullscreen, true));
    assert(fullscreen.active);
    assert((GetWindowLongPtrA(parent, GWL_STYLE) & WS_OVERLAPPEDWINDOW) == 0);
    RECT fullscreenRect{};
    assert(GetWindowRect(parent, &fullscreenRect));
    assert(fullscreenRect.left == expectedMonitor.rcMonitor.left);
    assert(fullscreenRect.top == expectedMonitor.rcMonitor.top);
    assert(fullscreenRect.right == expectedMonitor.rcMonitor.right);
    assert(fullscreenRect.bottom == expectedMonitor.rcMonitor.bottom);
    assert(setSfmEditorFullscreenCandidate(parent, fullscreen, false));
    assert(!fullscreen.active);
    if (GetWindowLongPtrA(parent, GWL_STYLE) != originalStyle) {
        std::fprintf(stderr, "fullscreen style before=%llx after=%llx\n",
            static_cast<unsigned long long>(originalStyle),
            static_cast<unsigned long long>(GetWindowLongPtrA(parent, GWL_STYLE)));
        return 2;
    }
    WINDOWPLACEMENT restoredPlacement{sizeof(WINDOWPLACEMENT)};
    assert(GetWindowPlacement(parent, &restoredPlacement));
    assert(restoredPlacement.showCmd == originalPlacement.showCmd);
    assert(restoredPlacement.rcNormalPosition.left == originalPlacement.rcNormalPosition.left);
    assert(restoredPlacement.rcNormalPosition.top == originalPlacement.rcNormalPosition.top);
    ShowWindow(parent, SW_SHOW);
    UpdateWindow(parent);
    HWND controls = CreateWindowExA(0, "STATIC", "", WS_CHILD | SS_OWNERDRAW | SS_NOTIFY,
        0, 0, 300, 36, parent, reinterpret_cast<HMENU>(UINT_PTR(SFM_EDITOR_PRIMARY_VIEWPORT_CONTROLS)),
        klass.hInstance, nullptr);
    assert(controls);
    assert(SetWindowSubclass(controls, sfmEditorPrimaryControlsSubclassProcCandidate, 4, 0));
    SfmEditorShellCandidate shell{};
    shell.primaryViewportControls = controls;
    setSfmEditorPrimaryControlsVisibleCandidate(shell, false);
    assert(!shell.primaryViewportControlsVisible);
    assert(!IsWindowVisible(controls));
    setSfmEditorPrimaryControlsVisibleCandidate(shell, true);
    assert(shell.primaryViewportControlsVisible);
    assert(IsWindowVisible(controls));
    SendMessageA(controls, WM_LBUTTONUP, 0, MAKELPARAM(179, 10));
    assert(receivedCommand == SFM_EDITOR_VIEWPORT_PLAY);
    RemoveWindowSubclass(controls, sfmEditorPrimaryControlsSubclassProcCandidate, 4);
    DestroyWindow(parent);
    return 0;
}
