#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <string>
#include "win32_vulkan_playback_candidate.h"

static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

static void verifyTimeDisplayMenu() {
    const auto menu = createSfmEditorTimeDisplayMenuCandidate(
        SfmEditorTimeDisplayModeCandidate::TimecodeFrames,
        SfmEditorTimeDisplayModeCandidate::TotalSeconds);
    require(menu != nullptr, "time display menu creation");
    require(GetMenuItemCount(menu) == 2, "global/local menu groups");
    const auto global = GetSubMenu(menu, 0);
    const auto local = GetSubMenu(menu, 1);
    require(global && local, "global/local submenu handles");
    require(GetMenuItemCount(global) == 4 && GetMenuItemCount(local) == 4, "four modes per scope");
    char label[64]{};
    require(GetMenuStringA(menu, 0, label, sizeof(label), MF_BYPOSITION) > 0 && std::string(label) == "Global",
            "global menu label");
    require(GetMenuStringA(menu, 1, label, sizeof(label), MF_BYPOSITION) > 0 && std::string(label) == "Local",
            "local menu label");
    MENUITEMINFOA item{sizeof(item)};
    item.fMask = MIIM_STATE;
    require(GetMenuItemInfoA(global, 3, TRUE, &item) && (item.fState & MFS_CHECKED), "global checked mode");
    item.fState = 0;
    require(GetMenuItemInfoA(local, 0, TRUE, &item) && (item.fState & MFS_CHECKED), "local checked mode");
    DestroyMenu(menu);
}

int main() {
    try {
        vulkanPlaybackState.count = 9;
        vulkanPlaybackState.selected = 4;
        vulkanPlaybackState.playing = true;
        vulkanPlaybackWindowProc(nullptr, WM_KEYDOWN, VK_END, 0);
        require(vulkanPlaybackState.selected == 8 && !vulkanPlaybackState.playing, "End key state");
        vulkanPlaybackWindowProc(nullptr, WM_KEYDOWN, VK_HOME, 0);
        require(vulkanPlaybackState.selected == 0 && !vulkanPlaybackState.playing, "Home key state");
        vulkanPlaybackWindowProc(nullptr, WM_KEYDOWN, VK_SPACE, 0);
        const auto anchor = vulkanPlaybackState.clock.anchor;
        vulkanPlaybackWindowProc(nullptr, WM_KEYDOWN, VK_SPACE, LPARAM(1) << 30);
        require(vulkanPlaybackState.playing && vulkanPlaybackState.clock.anchor == anchor, "Space repeat handling");
        require(playbackWindowTitleCandidate() == "SFM64 prototype - Frame 1 / 9 - Playing", "frame title");
        vulkanPlaybackState.sessionName = "mtt_soldier";
        require(playbackWindowTitleCandidate() == "SFM64 prototype - mtt_soldier - Frame 1 / 9 - Playing", "session title");
        vulkanPlaybackState.sessionName.clear();
        vulkanPlaybackState.count = 1;
        vulkanPlaybackWindowProc(nullptr, WM_KEYDOWN, VK_END, 0);
        require(vulkanPlaybackState.selected == 0, "single-frame End bound");

        auto& shell = sfmEditorShellCandidate;
        shell.frameCount = 240;
        shell.framesPerSecond = 24.0;
        require(std::string(sfmEditorTimeDisplayModeLabelCandidate(SfmEditorTimeDisplayModeCandidate::TotalSeconds)) == "Total Seconds" &&
                std::string(sfmEditorTimeDisplayModeLabelCandidate(SfmEditorTimeDisplayModeCandidate::TotalFrames)) == "Total Frames" &&
                std::string(sfmEditorTimeDisplayModeLabelCandidate(SfmEditorTimeDisplayModeCandidate::TimecodeSeconds)) == "Timecode:Seconds" &&
                std::string(sfmEditorTimeDisplayModeLabelCandidate(SfmEditorTimeDisplayModeCandidate::TimecodeFrames)) == "Timecode:Frames",
                "native time display labels");
        shell.globalTimeDisplayMode = SfmEditorTimeDisplayModeCandidate::TotalFrames;
        shell.localTimeDisplayMode = SfmEditorTimeDisplayModeCandidate::TotalSeconds;
        require(applySfmEditorTimeDisplayCommandCandidate(shell, 41104) &&
                shell.globalTimeDisplayMode == SfmEditorTimeDisplayModeCandidate::TimecodeFrames &&
                shell.timeDisplayMode == SfmEditorTimeDisplayModeCandidate::TimecodeFrames, "global mode action");
        require(applySfmEditorTimeDisplayCommandCandidate(shell, 41112) &&
                shell.localTimeDisplayMode == SfmEditorTimeDisplayModeCandidate::TotalFrames &&
                shell.timeDisplayMode == SfmEditorTimeDisplayModeCandidate::TotalFrames, "local mode action");
        require(!applySfmEditorTimeDisplayCommandCandidate(shell, 41109), "invalid mode action");
        require(applySfmEditorTimeDisplayShortcutCandidate(shell, VK_LEFT, false, false) &&
                shell.globalTimeDisplayMode == SfmEditorTimeDisplayModeCandidate::TotalSeconds &&
                shell.timeDisplayMode == SfmEditorTimeDisplayModeCandidate::TotalSeconds, "global Left shortcut");
        require(applySfmEditorTimeDisplayShortcutCandidate(shell, VK_LEFT, true, false) &&
                shell.localTimeDisplayMode == SfmEditorTimeDisplayModeCandidate::TimecodeSeconds &&
                shell.timeDisplayMode == SfmEditorTimeDisplayModeCandidate::TimecodeSeconds, "local Shift+Left shortcut");
        require(!applySfmEditorTimeDisplayShortcutCandidate(shell, VK_LEFT, false, true) &&
                shell.globalTimeDisplayMode == SfmEditorTimeDisplayModeCandidate::TotalSeconds, "shortcut key repeat ignored");
        require(!applySfmEditorTimeDisplayShortcutCandidate(shell, VK_RIGHT, false, false), "unmapped shortcut ignored");
        require(sfmEditorFormatTimelineTimeCandidate(SfmEditorTimeDisplayModeCandidate::TotalFrames, 0, 240, 24) == "1 frames" &&
                sfmEditorFormatTimelineTimeCandidate(SfmEditorTimeDisplayModeCandidate::TotalSeconds, 23, 240, 24) == "1.000 s" &&
                sfmEditorFormatTimelineTimeCandidate(SfmEditorTimeDisplayModeCandidate::TimecodeSeconds, 23, 240, 24) == "00:00:01.000" &&
                sfmEditorFormatTimelineTimeCandidate(SfmEditorTimeDisplayModeCandidate::TimecodeFrames, 23, 240, 24) == "00:00:01:00",
                "timeline format results");
        require(sfmEditorFormatTimelineTimeCandidate(SfmEditorTimeDisplayModeCandidate::TotalFrames, 999, 240, 24) == "240 frames" &&
                sfmEditorFormatTimelineTimeCandidate(SfmEditorTimeDisplayModeCandidate::TotalSeconds, 0, 0, 24) == "No animation frames loaded",
                "timeline format bounds");
        verifyTimeDisplayMenu();
        std::cout << "timeline keys passed playback_bounds=1 session_title=1 time_display_modes=4 global_local_actions=1 "
                     "native_menu_tree_and_checked_state=1 shortcut_cycle=1 formatting=1 native_formatter_parity=unverified\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
