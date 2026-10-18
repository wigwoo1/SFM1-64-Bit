#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_timeline_document_candidate.h"
#include "win32_vulkan_playback_candidate.h"
#include <iostream>

int main() {
    try {
        WNDCLASSA cls{};
        cls.hInstance = GetModuleHandleA(nullptr);
        cls.lpfnWndProc = vulkanPlaybackWindowProc;
        cls.lpszClassName = "Sfm64DeleteEmptyDagDispatchTest";
        if (!RegisterClassA(&cls) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            throw std::runtime_error("window class registration");
        HWND window = CreateWindowExA(0, cls.lpszClassName, "test", WS_OVERLAPPED,
            0, 0, 640, 480, nullptr, nullptr, cls.hInstance, nullptr);
        if (!window) throw std::runtime_error("test window creation");

        HMENU menu = createSfmEditorElementContextMenuCandidate(true, false, true);
        if (GetMenuItemCount(menu) != 4 || GetMenuItemID(menu, 3) != 41024) {
            DestroyMenu(menu);
            throw std::runtime_error("Delete Empty Dag context command missing");
        }
        DestroyMenu(menu);

        size_t calls = 0;
        uint32_t receivedElement = 0xffffffffu;
        size_t receivedFrame = SIZE_MAX;
        playbackDeleteEmptyDagHandler = [&](uint32_t element, size_t frame) {
            ++calls;
            receivedElement = element;
            receivedFrame = frame;
            return true;
        };
        sfmEditorShellCandidate.selectedElement = 13285;
        vulkanPlaybackState.count = 100;
        vulkanPlaybackState.playing = true;

        SendMessageA(window, WM_COMMAND, MAKEWPARAM(41024, 0), 0);
        if (calls != 0 || playbackEditorQueue.size() != 1 || vulkanPlaybackState.playing)
            throw std::runtime_error("command must pause playback and defer the edit");
        if (!drainPlaybackEditorActionsCandidate(7) || calls != 1 ||
            receivedElement != 13285 || receivedFrame != 7 || !playbackEditorQueue.empty())
            throw std::runtime_error("queued deletion did not reach the bound handler at frame boundary");

        sfmEditorShellCandidate.selectedElement = 0xffffffffu;
        SendMessageA(window, WM_COMMAND, MAKEWPARAM(41024, 0), 0);
        if (calls != 1 || !playbackEditorQueue.empty())
            throw std::runtime_error("deletion command queued without a selected element");

        playbackDeleteEmptyDagHandler = {};
        DestroyWindow(window);
        std::cout << "delete_empty_dag_menu_dispatch=passed command=41024 deferred=1 selected_element=13285 frame_boundary=7 missing_selection_rejected=1\n";
        return 0;
    } catch (const std::exception& error) {
        playbackDeleteEmptyDagHandler = {};
        std::cerr << error.what() << '\n';
        return 1;
    }
}
