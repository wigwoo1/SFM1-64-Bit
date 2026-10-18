#define NOMINMAX
#include "win32_sfm_editor_shell_candidate.h"
#include <iostream>

struct EditorRowValueFixture { uint32_t reference = 0xffffffffu; std::string text; std::vector<char> wire; };
struct EditorRowAttributeFixture { uint32_t name = 0, type = 0; std::vector<EditorRowValueFixture> values; };
struct EditorRowElementFixture { uint32_t type = 0, name = 0; std::vector<EditorRowAttributeFixture> attributes; };
struct EditorRowGraphFixture { std::vector<std::string> strings; std::vector<EditorRowElementFixture> elements; };
static size_t editorFixtureSeekFrame = SIZE_MAX;
static LRESULT CALLBACK editorFixtureWindowProc(HWND window, UINT message, WPARAM key, LPARAM value) {
    if (message == WM_TIMER && dispatchSfmEditorFilterRefreshTimerCandidate(window, key)) return 0;
    if (message == SFM_EDITOR_SEEK_FRAME) { editorFixtureSeekFrame = size_t(value); return 0; }
    if (message == WM_COMMAND) { handleSfmEditorCommandCandidate(window, key, value); return 0; }
    return DefWindowProcA(window, message, key, value);
}

int main() {
    try {
        std::vector<size_t> graphSelection;
        if (sfmEditorGraphSelectionModeCandidate(false, false) != 0 || sfmEditorGraphSelectionModeCandidate(true, true) != 1 ||
            sfmEditorGraphSelectionModeCandidate(false, true) != 2 || sfmEditorGraphSelectionModeCandidate(true, false) != 3)
            throw std::runtime_error("graph selection modifier mode mapping");
        applySfmEditorGraphSelectionModeCandidate(graphSelection, 1, 0);
        applySfmEditorGraphSelectionModeCandidate(graphSelection, 3, 1);
        if (graphSelection != std::vector<size_t>{1, 3}) throw std::runtime_error("graph additive selection");
        applySfmEditorGraphSelectionModeCandidate(graphSelection, 3, 1);
        applySfmEditorGraphSelectionModeCandidate(graphSelection, 1, 3);
        if (graphSelection != std::vector<size_t>{3}) throw std::runtime_error("graph control removal selection");
        applySfmEditorGraphSelectionModeCandidate(graphSelection, 3, 2);
        if (!graphSelection.empty()) throw std::runtime_error("graph alt toggle selection");
        applySfmEditorGraphSelectionModeCandidate(graphSelection, 2, 0);
        if (graphSelection != std::vector<size_t>{2}) throw std::runtime_error("graph replace selection");
        enableSfmEditorPerMonitorDpiAwarenessCandidate();
        const std::string dockSettingsPath = "work\\editor-dock-persistence-test.ini";
        DeleteFileA(dockSettingsPath.c_str());
        const auto layout1080 = sfmEditorDockLayoutCandidate(1920, 1080);
        if (layout1080.leftWidth != 457 || layout1080.timelineLeft != 457 || layout1080.timelineWidth != 1463 ||
            layout1080.timelineHeight != 359 ||
            layout1080.timelineTop != 721 || layout1080.viewportWidth < 300)
            throw std::runtime_error("installed SFM default layout proportions at 1080p");
        const auto layout4k = sfmEditorDockLayoutCandidate(3840, 2160);
        if (layout4k.leftWidth != 915 || layout4k.timelineLeft != 915 || layout4k.timelineWidth != 2925 ||
            layout4k.timelineHeight != 718 ||
            layout4k.timelineTop != 1442 || layout4k.viewportWidth < 300)
            throw std::runtime_error("installed SFM default layout proportions at 4K");
        bool undersizedRejected = false;
        try { (void)sfmEditorDockLayoutCandidate(819, 1080); }
        catch (const std::runtime_error&) { undersizedRejected = true; }
        if (!undersizedRejected) throw std::runtime_error("undersized editor dock accepted");
        EditorRowGraphFixture graph;
        graph.strings = {"DmeDag", "root", "DmeGameModel", "child", "grandchild", "other", "children", "unlisted"};
        graph.elements.resize(5);
        graph.elements[0] = {0, 1, {{6, 15, {{1}, {0xffffffffu}}}}};
        graph.elements[1] = {0, 3, {{6, 15, {{2}}}}};
        graph.elements[2] = {2, 3, {}};
        graph.elements[3] = {2, 4, {}};
        graph.elements[4] = {2, 5, {{7, 15, {{0xffffffffu}}}}};
        auto rows = buildSfmEditorElementRowsCandidate(graph);
        if (rows.size() != 5 || rows[1].index != 1 || rows[1].hierarchyDepth != 1 || rows[1].parent != "root" ||
            rows[2].index != 2 || rows[2].hierarchyDepth != 2 || rows[2].parent != "child" || rows[4].hierarchyDepth != 0)
            throw std::runtime_error("editor hierarchy projection");
        if (timelineFrameAtPixelCandidate(24, 24, 1000, 2276) != 0 ||
            timelineFrameAtPixelCandidate(1000, 24, 1000, 2276) != 2275 ||
            timelineFrameAtPixelCandidate(512, 24, 1000, 2276) != 1138 ||
            timelineFrameAtPixelCandidate(20, 24, 1000, 2276) != 0)
            throw std::runtime_error("timeline click-to-frame mapping");
        if (timelinePixelAtFrameCandidate(0, 24, 1000, 2276) != 24 ||
            timelinePixelAtFrameCandidate(2275, 24, 1000, 2276) != 1000 ||
            timelinePixelAtFrameCandidate(1138, 24, 1000, 2276) != 512 ||
            timelinePixelAtFrameCandidate(99, 24, 1000, 1) != 24)
            throw std::runtime_error("timeline frame-to-pixel bookmark mapping");
        auto& timelineScrollShell = sfmEditorShellCandidate;
        timelineScrollShell.timelineLanes.clear();
        for (size_t i = 0; i < 10; ++i) {
            SfmEditorTimelineLaneCandidate lane;
            lane.groupName = i < 5 ? "Group A" : "Group B";
            timelineScrollShell.timelineLanes.push_back(std::move(lane));
        }
        timelineScrollShell.timelineScrollOffsetPixels = 0;
        if (sfmEditorTimelineLaneAtYCandidate(112) != 0 ||
            clampSfmEditorTimelineScrollCandidate(-10, 200) != 0 ||
            clampSfmEditorTimelineScrollCandidate(500, 200) != 188)
            throw std::runtime_error("timeline lane hit testing or scroll bounds");
        timelineScrollShell.timelineScrollOffsetPixels = 25;
        if (sfmEditorTimelineLaneAtYCandidate(112) != 1)
            throw std::runtime_error("scrolled timeline click mapping");
        timelineScrollShell.timelineLanes.clear();
        timelineScrollShell.timelineScrollOffsetPixels = 0;
        graph.elements[0].attributes[0].values.push_back({1}); // Duplicate child edge from one parent is still one parent.
        graph.elements[2].attributes.push_back({6, 15, {{1}}});
        const auto multiParentRows = buildSfmEditorElementRowsCandidate(graph);
        const auto multiParentRow = std::find_if(multiParentRows.begin(), multiParentRows.end(),
            [](const auto& row) { return row.index == 1; });
        if (multiParentRow == multiParentRows.end() || multiParentRow->additionalParents != 1 ||
            multiParentRow->parent != "root")
            throw std::runtime_error("multi-parent DMX element was dropped or not marked in the editor outline");
        WNDCLASSA windowClass{};
        windowClass.lpfnWndProc = editorFixtureWindowProc;
        windowClass.hInstance = GetModuleHandleA(nullptr);
        windowClass.lpszClassName = "Sfm64EditorHierarchyFixture";
        RegisterClassA(&windowClass);
        auto parent = CreateWindowExA(0, windowClass.lpszClassName, "fixture", WS_POPUP,
            0, 0, 320, 240, nullptr, nullptr, windowClass.hInstance, nullptr);
        if (!parent) throw std::runtime_error("editor fixture parent window");
        auto& shell = sfmEditorShellCandidate;
        shell.parent = parent;
        shell.animationTab = CreateWindowExA(0, "BUTTON", "Animation Set Editor", WS_CHILD,
            0, 0, 220, 30, parent, nullptr, windowClass.hInstance, nullptr);
        shell.elementTab = CreateWindowExA(0, "BUTTON", "Element Viewer", WS_CHILD,
            0, 0, 220, 30, parent, nullptr, windowClass.hInstance, nullptr);
        shell.motionTab = CreateWindowExA(0, "BUTTON", "Motion", WS_CHILD,
            0, 0, 100, 30, parent, nullptr, windowClass.hInstance, nullptr);
        shell.graphTab = CreateWindowExA(0, "BUTTON", "Graph", WS_CHILD,
            0, 0, 100, 30, parent, nullptr, windowClass.hInstance, nullptr);
        shell.clipTab = CreateWindowExA(0, "BUTTON", "Clip", WS_CHILD,
            0, 0, 100, 30, parent, nullptr, windowClass.hInstance, nullptr);
        shell.graphCurves = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | CBS_DROPDOWNLIST,
            0, 0, 300, 180, parent, reinterpret_cast<HMENU>(UINT_PTR(SFM_EDITOR_GRAPH_CURVES)), windowClass.hInstance, nullptr);
        shell.clipShots = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | CBS_DROPDOWNLIST,
            0, 0, 300, 180, parent, reinterpret_cast<HMENU>(UINT_PTR(SFM_EDITOR_CLIP_SHOTS)), windowClass.hInstance, nullptr);
        shell.elements = CreateWindowExA(0, "LISTBOX", "", WS_CHILD | LBS_NOTIFY | LBS_EXTENDEDSEL,
            0, 0, 300, 180, parent, nullptr, windowClass.hInstance, nullptr);
        shell.details = CreateWindowExA(0, "STATIC", "", WS_CHILD,
            0, 0, 300, 40, parent, nullptr, windowClass.hInstance, nullptr);
        shell.filter = CreateWindowExA(0, "EDIT", "", WS_CHILD,
            0, 0, 300, 24, parent, nullptr, windowClass.hInstance, nullptr);
        shell.timelinePanel = CreateWindowExA(0, "STATIC", "", WS_CHILD | SS_OWNERDRAW | SS_NOTIFY,
            0, 0, 1000, 120, parent, nullptr, windowClass.hInstance, nullptr);
        if (!shell.animationTab || !shell.elementTab || !shell.motionTab || !shell.graphTab || !shell.clipTab ||
            !shell.graphCurves || !shell.clipShots || !shell.elements || !shell.details || !shell.filter || !shell.timelinePanel)
            throw std::runtime_error("editor fixture controls");
        setSfmEditorGraphCurvesCandidate({{1, "position x [1]", {{0, 1.0f}, {10, 2.0f}}},
                                          {2, "position y [2]", {{0, 3.0f}, {10, 4.0f}}}});
        SendMessageA(shell.graphCurves, CB_SETCURSEL, 1, 0);
        handleSfmEditorCommandCandidate(parent, MAKEWPARAM(SFM_EDITOR_GRAPH_CURVES, CBN_SELCHANGE), LPARAM(shell.graphCurves));
        if (shell.selectedGraphCurve != 1 || shell.graphLayerLabel != "position y [2]" ||
            shell.graphKeys.size() != 2 || shell.graphKeys.back().value != 4.0f)
            throw std::runtime_error("graph curve selection did not update plotted keys");
        const RECT graphPlot{48, 96, 148, 196};
        auto graphPoint = sfmEditorGraphPointCandidate(shell.graphKeys, 1, graphPlot, 0);
        if (graphPoint.x != 148 || graphPoint.y != 96 ||
            hitTestSfmEditorGraphKeyCandidate(shell.graphKeys, graphPlot, graphPoint.x, graphPoint.y) != 1 ||
            sfmEditorGraphValueAtPixelCandidate(shell.graphKeys, graphPlot, graphPlot.bottom) != 3.0f ||
            sfmEditorGraphValueAtPixelCandidate(shell.graphKeys, graphPlot, graphPlot.top) != 4.0f ||
            sfmEditorGraphTickAtPixelCandidate(shell.graphKeys, graphPlot, 98, 1) != 5 ||
            sfmEditorGraphTickAtPixelCandidate(shell.graphKeys, graphPlot, graphPlot.left, 1) != 0 ||
            sfmEditorGraphTickAtPixelCandidate(shell.graphKeys, graphPlot, graphPlot.right, 1) != 10 ||
            sfmEditorGraphPlotRectCandidate(800, 600).top <= sfmEditorScaleMetricCandidate(42))
            throw std::runtime_error("graph key hit testing/value mapping");
        const auto marquee = sfmEditorGraphKeysInRectCandidate(shell.graphKeys, graphPlot,
            normalizeSfmEditorGraphSelectionRectCandidate(POINT{160, 105}, POINT{140, 90}));
        if (marquee != std::vector<size_t>{1}) throw std::runtime_error("graph marquee bounds and key projection");
        bool graphEditQueued = false;
        shell.graphKeyQueue = [&](uint32_t element, size_t key, int32_t tick, float value, uint8_t component) {
            graphEditQueued = element == 2 && key == 1 && tick == 10 && value == 5.0f && component == 0xff;
            return graphEditQueued;
        };
        if (!queueSfmEditorGraphKeyEditCandidate({2, 1, 5.0f, 10}) || !graphEditQueued || shell.graphKeys.back().value != 4.0f ||
            shell.graphKeys.back().tick != 10 || queueSfmEditorGraphKeyEditCandidate({2, 1, 5.0f, 0}) ||
            queueSfmEditorGraphKeyEditCandidate({1, 0, 5.0f, 10}))
            throw std::runtime_error("graph key edit queue target or optimistic cache update");
        setSfmEditorGraphCurvesCandidate({{3, "vector x", {{0, 1.0f}, {10, 2.0f}}, 0}});
        bool vectorAxisQueued = false;
        shell.graphKeyQueue = [&](uint32_t element, size_t key, int32_t tick, float value, uint8_t component) {
            vectorAxisQueued = element == 3 && key == 1 && tick == 10 && value == 3.0f && component == 0;
            return vectorAxisQueued;
        };
        if (!queueSfmEditorGraphKeyEditCandidate({3, 1, 3.0f, 10, 0}) || !vectorAxisQueued ||
            shell.graphKeys.back().value != 2.0f || shell.graphKeys.back().tick != 10)
            throw std::runtime_error("vector component was lost or cache changed before commit");
        shell.frameCount = 2276;
        shell.rows = rows;
        shell.elementViewer = false;
        shell.selectedElement = 0xffffffffu;
        shell.selectedElements.clear();
        refreshSfmEditorRowsCandidate();
        if (std::none_of(shell.visibleRows.begin(), shell.visibleRows.end(), [&](size_t row) {
                return shell.rows[row].type == "DmeDag";
            }))
            throw std::runtime_error("Animation Set Editor omitted DmeDag hierarchy rows required by native Dag actions");
        const auto animationPaneElement = std::find_if(shell.visibleRows.begin(), shell.visibleRows.end(), [&](size_t row) {
            return shell.rows[row].index == 1;
        });
        if (animationPaneElement == shell.visibleRows.end())
            throw std::runtime_error("Animation Set Editor did not expose the selected fixture model");
        SendMessageA(shell.elements, LB_SETSEL, FALSE, -1);
        SendMessageA(shell.elements, LB_SETSEL, TRUE, LPARAM(animationPaneElement - shell.visibleRows.begin()));
        selectSfmEditorElementCandidate();
        shell.elementViewer = true;
        shell.selectedElement = 1;
        shell.selectedElements = {1};
        refreshSfmEditorRowsCandidate();
        if (SendMessageA(shell.elements, LB_GETSELCOUNT, 0, 0) != 1 || shell.selectedElement != 1 || shell.selectedElements != std::vector<uint32_t>{1})
            throw std::runtime_error("hierarchy refresh did not preserve single-selection state");
        SendMessageA(shell.elements, LB_SETSEL, FALSE, -1);
        SendMessageA(shell.elements, LB_SETSEL, TRUE, 1);
        SendMessageA(shell.elements, LB_SETSEL, TRUE, 2);
        selectSfmEditorElementCandidate();
        if (SendMessageA(shell.elements, LB_GETSELCOUNT, 0, 0) != 2)
            throw std::runtime_error("extended selection fixture did not select two rows");
        selectSfmEditorElementCandidate();
        if (shell.selectedElements.size() != 2 ||
            std::find(shell.selectedElements.begin(), shell.selectedElements.end(), uint32_t(1)) == shell.selectedElements.end() ||
            std::find(shell.selectedElements.begin(), shell.selectedElements.end(), uint32_t(2)) == shell.selectedElements.end())
            throw std::runtime_error("multi-selection did not map visible rows to scene element references");
        char selectionDetails[512]{};GetWindowTextA(shell.details,selectionDetails,sizeof(selectionDetails));
        if(std::string(selectionDetails).find("Selected elements: 2")==std::string::npos)
            throw std::runtime_error("hierarchy details did not show the multi-selection count");
        const int selectionFixtureRowHeight = int(SendMessageA(shell.elements, LB_GETITEMHEIGHT, 0, 0));
        ShowWindow(shell.elements, SW_SHOWNOACTIVATE);
        UpdateWindow(shell.elements);
        if (!selectSfmEditorRowAtPointCandidate({4, selectionFixtureRowHeight + 1}) || shell.selectedElement != 1 || shell.selectedElements.size() != 2)
            throw std::runtime_error("context hit on a selected row did not preserve multi-selection and set the clicked primary");
        SendMessageA(shell.elements, LB_SETSEL, FALSE, -1);
        SendMessageA(shell.elements, LB_SETSEL, TRUE, 1);
        SendMessageA(shell.elements, LB_SETSEL, TRUE, 2);
        if (!selectSfmEditorRowAtPointCandidate({4, selectionFixtureRowHeight * 3 + 1}) || shell.selectedElement != 3 || shell.selectedElements.size() != 1 || shell.selectedElements[0] != 3)
            throw std::runtime_error("context hit on an unselected row did not replace selection with the clicked element: primary=" + std::to_string(shell.selectedElement) + " count=" + std::to_string(shell.selectedElements.size()) + " first=" + (shell.selectedElements.empty() ? std::string("none") : std::to_string(shell.selectedElements[0])));
        GetWindowTextA(shell.details,selectionDetails,sizeof(selectionDetails));
        if(std::string(selectionDetails).find("Selected elements: 1")==std::string::npos)
            throw std::runtime_error("hierarchy details did not update after replacing the selection");
        SendMessageA(shell.elements, LB_SETSEL, TRUE, 1);
        SendMessageA(shell.elements, LB_SETSEL, TRUE, 2);
        selectSfmEditorElementCandidate();
        const auto rowsBeforeFilterTyping = shell.visibleRows.size();
        SendMessageA(shell.filter, WM_SETTEXT, 0, LPARAM("child"));
        handleSfmEditorCommandCandidate(parent, MAKEWPARAM(SFM_EDITOR_FILTER, EN_CHANGE), LPARAM(shell.filter));
        SendMessageA(shell.filter, WM_SETTEXT, 0, LPARAM("grandchild"));
        handleSfmEditorCommandCandidate(parent, MAKEWPARAM(SFM_EDITOR_FILTER, EN_CHANGE), LPARAM(shell.filter));
        if (shell.visibleRows.size() != rowsBeforeFilterTyping || SendMessageA(shell.elements, LB_GETCOUNT, 0, 0) != LRESULT(rowsBeforeFilterTyping))
            throw std::runtime_error("hierarchy filter rebuilt the visible list while typing");
        if (!dispatchSfmEditorFilterRefreshTimerCandidate(parent, SFM_EDITOR_FILTER_REFRESH_TIMER))
            throw std::runtime_error("deferred hierarchy filter refresh timer was not handled");
        SendMessageA(shell.filter, WM_SETTEXT, 0, LPARAM("grandchild"));
        selectSfmEditorElementCandidate();
        if (SendMessageA(shell.elements, LB_GETSELCOUNT, 0, 0) != 1 || shell.visibleRows.size() != 1 ||
            shell.selectedElements.size() != 1 || shell.selectedElements[0] != 3 ||
            shell.rows[shell.visibleRows[0]].name != "grandchild")
            throw std::runtime_error("filtered selection did not map to visible hierarchy element");
        SendMessageA(shell.filter, WM_SETTEXT, 0, LPARAM(""));
        handleSfmEditorCommandCandidate(parent, MAKEWPARAM(SFM_EDITOR_FILTER, EN_CHANGE), LPARAM(shell.filter));
        if (!dispatchSfmEditorFilterRefreshTimerCandidate(parent, SFM_EDITOR_FILTER_REFRESH_TIMER))
            throw std::runtime_error("hierarchy filter reset timer was not handled");
        selectSfmEditorElementCandidate();
        if (SendMessageA(shell.elements, LB_GETSELCOUNT, 0, 0) != 1 || shell.selectedElements.size() != 1 || shell.selectedElement != 3)
            throw std::runtime_error("hierarchy selection did not recover after filter reset");
        if (!SetWindowSubclass(shell.timelinePanel, sfmEditorTimelineSubclassProcCandidate, 2, 0))
            throw std::runtime_error("editor timeline subclass");
        SendMessageA(shell.timelinePanel, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(24, 60));
        SendMessageA(shell.timelinePanel, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(500, 60));
        SendMessageA(shell.timelinePanel, WM_LBUTTONUP, 0, MAKELPARAM(976, 60));
        if (editorFixtureSeekFrame != 2275 || shell.draggingTimeline || GetCapture() == shell.timelinePanel)
            throw std::runtime_error("timeline drag did not seek/release capture");
        for (size_t i = 0; i < 10; ++i) {
            SfmEditorTimelineLaneCandidate lane;
            lane.groupName = i < 5 ? "Group A" : "Group B";
            shell.timelineLanes.push_back(std::move(lane));
        }
        shell.timelineScrollOffsetPixels = 0;
        shell.timelineWheelDeltaRemainder = 0;
        SendMessageA(shell.timelinePanel, WM_MOUSEWHEEL, MAKEWPARAM(0, WORD(-WHEEL_DELTA / 2)), 0);
        if (shell.timelineScrollOffsetPixels != 0 || shell.timelineWheelDeltaRemainder != -WHEEL_DELTA / 2)
            throw std::runtime_error("partial timeline wheel delta was not retained");
        SendMessageA(shell.timelinePanel, WM_MOUSEWHEEL, MAKEWPARAM(0, WORD(-WHEEL_DELTA / 2)), 0);
        if (shell.timelineScrollOffsetPixels != 75 || sfmEditorTimelineLaneAtYCandidate(112) != 3)
            throw std::runtime_error("timeline wheel scrolling or scrolled lane hit test");
        SfmEditorTimelineLaneCandidate remainingLane;
        remainingLane.groupName = "Group A";
        shell.timelineScrollOffsetPixels = 75;
        setSfmEditorTimelineLanesCandidate({remainingLane});
        if (shell.timelineScrollOffsetPixels != 24)
            throw std::runtime_error("timeline scroll offset was not clamped after track data changed");
        setSfmEditorTimelineLanesCandidate({});
        if (shell.timelineScrollOffsetPixels != 0)
            throw std::runtime_error("empty timeline retained a stale scroll offset");
        shell.timelineWheelDeltaRemainder = 0;
        setSfmEditorTimelineSegmentsCandidate({{0, 3, 41, "slug"}, {3, 9, 42, "shot B"}});
        SendMessageA(shell.clipShots, CB_SETCURSEL, 1, 0);
        handleSfmEditorCommandCandidate(parent, MAKEWPARAM(SFM_EDITOR_CLIP_SHOTS, CBN_SELCHANGE), LPARAM(shell.clipShots));
        if (editorFixtureSeekFrame != 3 || SendMessageA(shell.clipShots, CB_GETCOUNT, 0, 0) != 2)
            throw std::runtime_error("clip selector failed to seek to selected shot");
        graph.elements[2].attributes.pop_back();
        shell.elementViewer = true;
        shell.rows = buildSfmEditorElementRowsCandidate(graph);
        refreshSfmEditorRowsCandidate();
        auto selected = std::find_if(shell.visibleRows.begin(), shell.visibleRows.end(), [&](size_t row) {
            return shell.rows[row].index == 2;
        });
        if (selected == shell.visibleRows.end()) throw std::runtime_error("editor fixture row missing");
        SendMessageA(shell.elements, LB_SETSEL, FALSE, -1);
        SendMessageA(shell.elements, LB_SETSEL, TRUE, LPARAM(selected - shell.visibleRows.begin()));
        shell.selectedElements = {2};
        selectSfmEditorElementCandidate();
        SetWindowTextA(shell.filter, "child");
        refreshSfmEditorRowsCandidate();
        auto preserved = std::find_if(shell.visibleRows.begin(), shell.visibleRows.end(), [&](size_t row) {
            return shell.rows[row].index == 2;
        });
        if (preserved == shell.visibleRows.end() || shell.selectedElement != 2 ||
            SendMessageA(shell.elements, LB_GETSEL, WPARAM(preserved - shell.visibleRows.begin()), 0) == 0)
            throw std::runtime_error("editor filter lost selection");
        const auto rowHeight = SendMessageA(shell.elements, LB_GETITEMHEIGHT, 0, 0);
        if (rowHeight <= 0 || !selectSfmEditorRowAtPointCandidate({4, int(rowHeight + 1)}))
            throw std::runtime_error("editor context hit selection");
        selectSfmEditorElementCandidate();
        if (shell.selectedElement != shell.rows[shell.visibleRows[1]].index)
            throw std::runtime_error("editor context selected wrong row");
        HMENU contextMenu = createSfmEditorElementContextMenuCandidate();
        const bool contextCommands = GetMenuItemCount(contextMenu) == 2 &&
            GetMenuItemID(contextMenu, 0) == 41005 && GetMenuItemID(contextMenu, 1) == 41006;
        DestroyMenu(contextMenu);
        if (!contextCommands) throw std::runtime_error("editor context action binding");
        HMENU dagMenu=createSfmEditorElementContextMenuCandidate(true,false);char dagLabel[64]{};GetMenuStringA(dagMenu,2,dagLabel,sizeof(dagLabel),MF_BYPOSITION);const bool dagRename=GetMenuItemCount(dagMenu)==3&&GetMenuItemID(dagMenu,2)==41023&&std::string(dagLabel)=="Rename Dag";DestroyMenu(dagMenu);if(!dagRename)throw std::runtime_error("DAG rename context command");
        HMENU deleteMenu=createSfmEditorElementContextMenuCandidate(true,false,true);char deleteLabel[64]{};GetMenuStringA(deleteMenu,3,deleteLabel,sizeof(deleteLabel),MF_BYPOSITION);const bool deleteCommand=GetMenuItemCount(deleteMenu)==4&&GetMenuItemID(deleteMenu,3)==41024&&std::string(deleteLabel)=="Delete Empty Dag";DestroyMenu(deleteMenu);if(!deleteCommand)throw std::runtime_error("empty DAG delete context command");
        HMENU setMenu=createSfmEditorElementContextMenuCandidate(false,true);char setLabel[64]{};GetMenuStringA(setMenu,2,setLabel,sizeof(setLabel),MF_BYPOSITION);const bool setRename=GetMenuItemCount(setMenu)==3&&GetMenuItemID(setMenu,2)==41023&&std::string(setLabel)=="Rename AnimationSet";DestroyMenu(setMenu);if(!setRename)throw std::runtime_error("animation set rename context command");
        HMENU graphMenu = createSfmEditorGraphKeyContextMenuCandidate(false);
        const bool graphContextCommands = GetMenuItemCount(graphMenu) == 1 && GetMenuItemID(graphMenu, 0) == 41006;
        DestroyMenu(graphMenu);
        if (!graphContextCommands) throw std::runtime_error("graph key context action binding");
        HMENU graphDeleteMenu = createSfmEditorGraphKeyContextMenuCandidate(true);
        const bool graphDeleteCommands = GetMenuItemCount(graphDeleteMenu) == 2 &&
            GetMenuItemID(graphDeleteMenu, 0) == 41006 && GetMenuItemID(graphDeleteMenu, 1) == 41026;
        DestroyMenu(graphDeleteMenu);
        if (!graphDeleteCommands) throw std::runtime_error("graph key delete context action binding");
        HWND graphControl = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
            0, 0, 640, 480, parent, nullptr, windowClass.hInstance, nullptr);
        if (!graphControl || !SetWindowSubclass(graphControl, sfmEditorGraphSubclassProcCandidate, 3, 0))
            throw std::runtime_error("graph plot input fixture");
        shell.editorMode = 1;
        shell.graphCurveList = {{71, "input fixture", {{0, 0.0f}, {100, 1.0f}}, 0xff}};
        shell.graphKeys = shell.graphCurveList.front().keys;
        shell.selectedGraphCurve = 0;
        shell.selectedGraphKey = 1;
        shell.selectedGraphKeys = {1};
        size_t graphDeleteCalls = 0;
        std::vector<size_t> graphDeletedIndices;
        shell.graphKeyDeleteQueue = [&](uint32_t element, const std::vector<size_t>& indices) {
            graphDeletedIndices = indices;
            if (element == 71 && indices == std::vector<size_t>{1, 2}) ++graphDeleteCalls;
            return true;
        };
        ShowWindow(parent, SW_SHOW);
        SetActiveWindow(parent);
        SetFocus(graphControl);
        const RECT livePlot = sfmEditorGraphPlotRectCandidate(640, 480);
        const POINT liveKey = sfmEditorGraphPointCandidate(shell.graphKeys, 1, livePlot, 0.0f, 0, false);
        SendMessageA(graphControl, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(liveKey.x - 17, liveKey.y));
        SendMessageA(graphControl, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(liveKey.x + 6, liveKey.y + 14));
        SendMessageA(graphControl, WM_LBUTTONUP, 0, MAKELPARAM(liveKey.x + 6, liveKey.y + 14));
        if (shell.selectingGraphKeys || shell.selectedGraphKeys != std::vector<size_t>{1})
            throw std::runtime_error("Graph Editor empty-space marquee did not select enclosed key: selecting=" + std::to_string(shell.selectingGraphKeys) + " count=" + std::to_string(shell.selectedGraphKeys.size()) + " key=" + std::to_string(liveKey.x) + "," + std::to_string(liveKey.y) + " plot=" + std::to_string(livePlot.left) + "," + std::to_string(livePlot.top) + "," + std::to_string(livePlot.right) + "," + std::to_string(livePlot.bottom));
        shell.selectedGraphKeys = {1, 2};
        SendMessageA(graphControl, WM_KEYDOWN, VK_DELETE, 0);
        if (GetFocus() != graphControl || graphDeleteCalls != 1 || shell.selectedGraphKey != SIZE_MAX || !shell.selectedGraphKeys.empty() || graphDeletedIndices != std::vector<size_t>{1, 2})
            throw std::runtime_error("focused Graph Editor Delete key dispatch");
        shell.graphKeyDeleteQueue = {};
        shell.graphKeys.clear();
        shell.graphCurveList.clear();
        shell.selectedGraphCurve = SIZE_MAX;
        shell.editorMode = 0;
        DestroyWindow(graphControl);
        HMENU timelineMenu = createSfmEditorTimelineContextMenuCandidate();
        const bool timelineCommands = GetMenuItemCount(timelineMenu) == 6 &&
            GetMenuItemID(timelineMenu, 0) == 41001 && GetMenuItemID(timelineMenu, 1) == 41021 &&
            GetMenuItemID(timelineMenu, 2) == 41028 && GetMenuItemID(timelineMenu, 3) == 41026 &&
            GetMenuItemID(timelineMenu, 4) == 41025 &&
            GetMenuItemID(timelineMenu, 5) == SFM_EDITOR_TRACK_GROUP_VISIBILITY;
        DestroyMenu(timelineMenu);
        if (!timelineCommands) throw std::runtime_error("timeline context action binding");
        if (shell.showTimelineSnapMarker || !shell.showTimelineToolTips || !shell.showTimelineOffFrameWarning)
            throw std::runtime_error("timeline settings did not start with SFM's recovered defaults");
        if (!shell.autoKeyBookmarks || !shell.engineSyncedToFixedFps || shell.hideWaveformOnSelect)
            throw std::runtime_error("timeline option defaults did not match the UI reference");
        HMENU timelineOptionsMenu = createSfmEditorTimelineOptionsMenuCandidate(shell);
        char snapMarkerLabel[64]{};
        char toolTipsLabel[64]{};
        char offFrameLabel[64]{};
        char autoKeyLabel[64]{};
        char fixedFpsLabel[64]{};
        char waveformLabel[64]{};
        const int menuItems = GetMenuItemCount(timelineOptionsMenu);
        const int autoKeyLength = GetMenuStringA(timelineOptionsMenu, 1, autoKeyLabel, sizeof(autoKeyLabel), MF_BYPOSITION);
        const int fixedFpsLength = GetMenuStringA(timelineOptionsMenu, 2, fixedFpsLabel, sizeof(fixedFpsLabel), MF_BYPOSITION);
        const int waveformLength = GetMenuStringA(timelineOptionsMenu, 3, waveformLabel, sizeof(waveformLabel), MF_BYPOSITION);
        const UINT separatorState = GetMenuState(timelineOptionsMenu, 4, MF_BYPOSITION);
        const int toolTipsLength = GetMenuStringA(timelineOptionsMenu, 5, toolTipsLabel, sizeof(toolTipsLabel), MF_BYPOSITION);
        const int snapMarkerLength = GetMenuStringA(timelineOptionsMenu, 6, snapMarkerLabel, sizeof(snapMarkerLabel), MF_BYPOSITION);
        const int offFrameLength = GetMenuStringA(timelineOptionsMenu, 7, offFrameLabel, sizeof(offFrameLabel), MF_BYPOSITION);
        const bool timelineOptionLabels = menuItems == 8 &&
            GetMenuItemID(timelineOptionsMenu, 0) == SFM_EDITOR_TRACK_GROUP_VISIBILITY &&
            GetMenuItemID(timelineOptionsMenu, 1) == SFM_EDITOR_TIMELINE_AUTOKEY_BOOKMARKS &&
            GetMenuItemID(timelineOptionsMenu, 2) == SFM_EDITOR_TIMELINE_FIXED_FPS &&
            GetMenuItemID(timelineOptionsMenu, 3) == SFM_EDITOR_TIMELINE_HIDE_WAVEFORM &&
            (separatorState & MF_SEPARATOR) != 0 &&
            GetMenuItemID(timelineOptionsMenu, 5) == SFM_EDITOR_TIMELINE_SHOW_TOOLTIPS &&
            GetMenuItemID(timelineOptionsMenu, 6) == SFM_EDITOR_TIMELINE_SHOW_SNAP_MARKER &&
            GetMenuItemID(timelineOptionsMenu, 7) == SFM_EDITOR_TIMELINE_SHOW_OFF_FRAME_WARNING &&
            autoKeyLength > 0 && fixedFpsLength > 0 && waveformLength > 0 &&
            toolTipsLength > 0 && snapMarkerLength > 0 && offFrameLength > 0 &&
            std::string(autoKeyLabel) == "AutoKey Bookmarks Mode" &&
            std::string(fixedFpsLabel) == "Engine Synced to Fixed FPS" &&
            std::string(waveformLabel) == "Hide Waveform on Select" &&
            (GetMenuState(timelineOptionsMenu, 1, MF_BYPOSITION) & MF_CHECKED) != 0 &&
            (GetMenuState(timelineOptionsMenu, 2, MF_BYPOSITION) & MF_CHECKED) != 0 &&
            (GetMenuState(timelineOptionsMenu, 3, MF_BYPOSITION) & MF_CHECKED) == 0 &&
            std::string(toolTipsLabel) == "Hide ToolTips" &&
            std::string(snapMarkerLabel) == "Show Snap Marker" &&
            std::string(offFrameLabel) == "Hide Off Frame Warning";
        if (!timelineOptionLabels) {
            const std::string diagnostic = "timeline options menu did not match recovered labels: count=" +
            std::to_string(GetMenuItemCount(timelineOptionsMenu)) + " ids=" +
            std::to_string(GetMenuItemID(timelineOptionsMenu, 0)) + "," +
            std::to_string(GetMenuItemID(timelineOptionsMenu, 1)) + "," +
            std::to_string(GetMenuItemID(timelineOptionsMenu, 2)) + "," +
            std::to_string(GetMenuItemID(timelineOptionsMenu, 3)) + "," +
            std::to_string(GetMenuItemID(timelineOptionsMenu, 4)) + " labels=" +
            std::string(toolTipsLabel) + "," + std::string(snapMarkerLabel) + "," + std::string(offFrameLabel);
            DestroyMenu(timelineOptionsMenu);
            throw std::runtime_error(diagnostic);
        }
        DestroyMenu(timelineOptionsMenu);
        shell.hideWaveformOnSelect = true;
        timelineOptionsMenu = createSfmEditorTimelineOptionsMenuCandidate(shell);
        waveformLabel[0] = '\0';
        const int hiddenWaveformLabelLength = GetMenuStringA(timelineOptionsMenu, 3, waveformLabel, sizeof(waveformLabel), MF_BYPOSITION);
        if (hiddenWaveformLabelLength <= 0 || std::string(waveformLabel) != "Show Waveform on Select" ||
            (GetMenuState(timelineOptionsMenu, 3, MF_BYPOSITION) & MF_CHECKED) != 0) {
            DestroyMenu(timelineOptionsMenu);
            throw std::runtime_error("waveform setting did not use SFM's inverse dynamic action label");
        }
        DestroyMenu(timelineOptionsMenu);
        shell.hideWaveformOnSelect = false;
        if (!applySfmEditorTimelineOptionCandidate(shell, SFM_EDITOR_TIMELINE_SHOW_SNAP_MARKER) ||
            !shell.showTimelineSnapMarker || !shell.showTimelineToolTips || !shell.showTimelineOffFrameWarning ||
            !applySfmEditorTimelineOptionCandidate(shell, SFM_EDITOR_TIMELINE_SHOW_SNAP_MARKER) ||
            shell.showTimelineSnapMarker || applySfmEditorTimelineOptionCandidate(shell, 41399))
            throw std::runtime_error("snap marker visibility state was not independent and reversible");
        if (!applySfmEditorTimelineOptionCandidate(shell, SFM_EDITOR_TIMELINE_AUTOKEY_BOOKMARKS) ||
            shell.autoKeyBookmarks || !applySfmEditorTimelineOptionCandidate(shell, SFM_EDITOR_TIMELINE_FIXED_FPS) ||
            shell.engineSyncedToFixedFps || !applySfmEditorTimelineOptionCandidate(shell, SFM_EDITOR_TIMELINE_HIDE_WAVEFORM) ||
            !shell.hideWaveformOnSelect || shell.showTimelineToolTips != true || shell.showTimelineOffFrameWarning != true)
            throw std::runtime_error("timeline option states were not independently reversible");
        MoveWindow(parent, 80, 80, 1920, 1080, FALSE);
        ShowWindow(parent, SW_SHOWNOACTIVATE);
        UpdateWindow(parent);
        RECT parentClient{};
        GetClientRect(parent, &parentClient);
        const int clientWidth = parentClient.right;
        const int clientHeight = parentClient.bottom;
        shell.verticalDockSplitter = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_NOTIFY,
            0, 0, 6, clientHeight, parent, nullptr, windowClass.hInstance, nullptr);
        shell.horizontalDockSplitter = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_NOTIFY,
            0, 0, clientWidth, 6, parent, nullptr, windowClass.hInstance, nullptr);
        if (!shell.verticalDockSplitter || !shell.horizontalDockSplitter ||
            !SetWindowSubclass(shell.verticalDockSplitter, sfmEditorDockSplitterSubclassProcCandidate, 10, 0) ||
            !SetWindowSubclass(shell.horizontalDockSplitter, sfmEditorDockSplitterSubclassProcCandidate, 11, 0))
            throw std::runtime_error("dock splitter fixture controls");
        layoutSfmEditorShellCandidate(parent);
        const auto initial = sfmEditorDockLayoutCandidate(clientWidth, clientHeight,
            shell.rootSplitBasisPoints, shell.timeSplitBasisPoints);
        const int rootTarget = initial.leftWidth + 150;
        POINT rootDown{initial.leftWidth, 40};
        MapWindowPoints(parent, shell.verticalDockSplitter, &rootDown, 1);
        SendMessageA(shell.verticalDockSplitter, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(rootDown.x, rootDown.y));
        POINT rootMove{rootTarget, 40};
        MapWindowPoints(parent, shell.verticalDockSplitter, &rootMove, 1);
        SendMessageA(shell.verticalDockSplitter, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(rootMove.x, rootMove.y));
        SendMessageA(shell.verticalDockSplitter, WM_LBUTTONUP, 0, MAKELPARAM(rootMove.x, rootMove.y));
        if (shell.draggingRootSplitter || GetCapture() == shell.verticalDockSplitter ||
            sfmEditorDockLayoutCandidate(clientWidth, clientHeight, shell.rootSplitBasisPoints,
                                         shell.timeSplitBasisPoints).leftWidth != rootTarget)
            throw std::runtime_error("root Win32 dock splitter drag did not resize/release");
        const auto beforeTimeDrag = sfmEditorDockLayoutCandidate(clientWidth, clientHeight,
            shell.rootSplitBasisPoints, shell.timeSplitBasisPoints);
        const int timeTargetTop = beforeTimeDrag.timelineTop - 100;
        POINT timeDown{beforeTimeDrag.timelineLeft + 5, beforeTimeDrag.timelineTop};
        MapWindowPoints(parent, shell.horizontalDockSplitter, &timeDown, 1);
        SendMessageA(shell.horizontalDockSplitter, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(timeDown.x, timeDown.y));
        POINT timeMove{beforeTimeDrag.timelineLeft + 5, timeTargetTop};
        MapWindowPoints(parent, shell.horizontalDockSplitter, &timeMove, 1);
        SendMessageA(shell.horizontalDockSplitter, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(timeMove.x, timeMove.y));
        SendMessageA(shell.horizontalDockSplitter, WM_LBUTTONUP, 0, MAKELPARAM(timeMove.x, timeMove.y));
        if (shell.draggingTimeSplitter || GetCapture() == shell.horizontalDockSplitter ||
            sfmEditorDockLayoutCandidate(clientWidth, clientHeight, shell.rootSplitBasisPoints,
                                         shell.timeSplitBasisPoints).timelineTop != timeTargetTop)
            throw std::runtime_error("Timestrip Win32 dock splitter drag did not resize/release");
        shell.uiScalePercent = 150;
        applySfmEditorUiScaleCandidate(parent);
        RECT filterRect{};
        GetWindowRect(shell.filter, &filterRect);
        MapWindowPoints(nullptr, parent, reinterpret_cast<POINT*>(&filterRect), 2);
        LOGFONTA scaledUiFont{}, compactUiFont{};
        if (GetObjectA(shell.uiFont, sizeof(scaledUiFont), &scaledUiFont) != sizeof(scaledUiFont) ||
            GetObjectA(shell.compactTabFont, sizeof(compactUiFont), &compactUiFont) != sizeof(compactUiFont))
            throw std::runtime_error("editor scaled-font metrics unavailable");
        if (filterRect.top != sfmEditorScaleMetricCandidate(34) || SendMessageA(shell.filter, WM_GETFONT, 0, 0) != LPARAM(shell.uiFont) ||
            SendMessageA(shell.animationTab, WM_GETFONT, 0, 0) != LPARAM(shell.compactTabFont) ||
            SendMessageA(shell.motionTab, WM_GETFONT, 0, 0) != LPARAM(shell.compactTabFont) ||
            compactUiFont.lfHeight != int((int64_t(scaledUiFont.lfHeight) * 65) / 100) ||
            sfmEditorGraphPlotRectCandidate(800, 600).left != sfmEditorScaleMetricCandidate(48))
            throw std::runtime_error("150% UI scaling did not resize controls, keep compact dock tabs, or scale the graph HUD: top=" + std::to_string(filterRect.top) +
                " expected=" + std::to_string(sfmEditorScaleMetricCandidate(51)) + " graph=" +
                std::to_string(sfmEditorGraphPlotRectCandidate(800, 600).left) + " expected_graph=" +
                std::to_string(sfmEditorScaleMetricCandidate(48)) + " dpi=" + std::to_string(shell.dpiScalePercent) +
                " tabFont=" + std::to_string(SendMessageA(shell.animationTab, WM_GETFONT, 0, 0)) +
                " expectedTabFont=" + std::to_string(reinterpret_cast<LPARAM>(shell.compactTabFont)) +
                " modeTabFont=" + std::to_string(SendMessageA(shell.motionTab, WM_GETFONT, 0, 0)) +
                " mainFont=" + std::to_string(reinterpret_cast<LPARAM>(shell.uiFont)) +
                " heights=" + std::to_string(compactUiFont.lfHeight) + "/" +
                std::to_string(int((int64_t(scaledUiFont.lfHeight) * 65) / 100)));
        SfmEditorShellCandidate dpiProbe;
        if (!setSfmEditorDpiScaleCandidate(dpiProbe, 144) || dpiProbe.dpiScalePercent != 150 ||
            sfmEditorEffectiveScalePercentCandidate(dpiProbe) != 150)
            throw std::runtime_error("150% monitor DPI was not incorporated into interface scale");
        dpiProbe.uiScalePercent = 125;
        if (sfmEditorEffectiveScalePercentCandidate(dpiProbe) != 188 ||
            !setSfmEditorDpiScaleCandidate(dpiProbe, 192) ||
            sfmEditorEffectiveScalePercentCandidate(dpiProbe) != 250 ||
            setSfmEditorDpiScaleCandidate(dpiProbe, 900) || dpiProbe.dpiScalePercent != 200)
            throw std::runtime_error("monitor DPI scaling, user scale composition, or invalid-DPI rejection failed");
        const auto fourKAt150Dpi = sfmEditorDockLayoutCandidate(3840, 2160, 23819, 33233, 225);
        if (fourKAt150Dpi.leftWidth < 540 || fourKAt150Dpi.timelineHeight < 326)
            throw std::runtime_error("high-DPI 4K layout did not scale dock minimums");
        shell.uiScalePercent = 250;
        applySfmEditorUiScaleCandidate(parent);
        RECT scaledClient{};
        GetClientRect(parent, &scaledClient);
        if (scaledClient.right < sfmEditorScaleMetricCandidate(780) || scaledClient.bottom < sfmEditorScaleMetricCandidate(420) ||
            sfmEditorGraphPlotRectCandidate(800, 600).left != sfmEditorScaleMetricCandidate(48))
            throw std::runtime_error("250% UI scaling did not grow shell and graph HUD");
        shell.uiScalePercent = 150;
        applySfmEditorUiScaleCandidate(parent);
        shell.autoKeyBookmarks = false;
        shell.engineSyncedToFixedFps = false;
        shell.hideWaveformOnSelect = true;
        shell.showTimelineToolTips = false;
        shell.showTimelineSnapMarker = true;
        shell.showTimelineOffFrameWarning = false;
        shell.dockSettingsPath = dockSettingsPath;
        if (!saveSfmEditorDockRatiosCandidate(shell, shell.dockSettingsPath))
            throw std::runtime_error("dock settings save failed");
        SfmEditorShellCandidate restored;
        if (!loadSfmEditorDockRatiosCandidate(restored, shell.dockSettingsPath) ||
            restored.rootSplitBasisPoints != shell.rootSplitBasisPoints ||
            restored.timeSplitBasisPoints != shell.timeSplitBasisPoints ||
            restored.uiScalePercent != shell.uiScalePercent ||
            restored.autoKeyBookmarks != shell.autoKeyBookmarks ||
            restored.engineSyncedToFixedFps != shell.engineSyncedToFixedFps ||
            restored.hideWaveformOnSelect != shell.hideWaveformOnSelect ||
            restored.showTimelineToolTips != shell.showTimelineToolTips ||
            restored.showTimelineSnapMarker != shell.showTimelineSnapMarker ||
            restored.showTimelineOffFrameWarning != shell.showTimelineOffFrameWarning ||
            sfmEditorDockLayoutCandidate(clientWidth, clientHeight, restored.rootSplitBasisPoints,
                                         restored.timeSplitBasisPoints).timelineTop != timeTargetTop)
            throw std::runtime_error("saved dock settings did not restore layout");
        {
            std::ofstream legacy(dockSettingsPath, std::ios::binary | std::ios::trunc);
            legacy << "[EditorDock]\nRootSplitBasisPoints=23819\nTimeSplitBasisPoints=33233\n";
        }
        SfmEditorShellCandidate legacyRestored;
        if (!loadSfmEditorDockRatiosCandidate(legacyRestored, dockSettingsPath) ||
            !legacyRestored.autoKeyBookmarks || !legacyRestored.engineSyncedToFixedFps ||
            legacyRestored.hideWaveformOnSelect || legacyRestored.showTimelineSnapMarker ||
            !legacyRestored.showTimelineToolTips || !legacyRestored.showTimelineOffFrameWarning)
            throw std::runtime_error("legacy dock settings did not retain Timeline defaults");
        {
            std::ofstream malformed(dockSettingsPath, std::ios::binary | std::ios::trunc);
            malformed << "[EditorDock]\\nRootSplitBasisPoints=99999\\nTimeSplitBasisPoints=33233\\n";
        }
        SfmEditorShellCandidate fallback;
        if (loadSfmEditorDockRatiosCandidate(fallback, dockSettingsPath) ||
            fallback.rootSplitBasisPoints != 23819 || fallback.timeSplitBasisPoints != 33233 || fallback.uiScalePercent != 100)
            throw std::runtime_error("invalid dock settings were not rejected safely");
        DeleteFileA(dockSettingsPath.c_str());
        destroySfmEditorUiFontCandidate();
        shell.uiScalePercent = 100;
        SfmEditorShellCandidate resized;
        const auto rootBeforeClamp = resized.rootSplitBasisPoints;
        if (updateSfmEditorDockBoundaryCandidate(resized, true, -100, 1920, 1080) ||
            resized.rootSplitBasisPoints != rootBeforeClamp)
            throw std::runtime_error("root dock split accepted an invalid ratio");
        DestroyWindow(parent);
        shell = {};
        std::cout << "editor hierarchy rows passed graph_selection_modes=1 graph_batch_delete=1 parent_mapping=1 depth=1 unlinked=1 multiparent_outline_retained=1 dag_rows_visible=1 filter_selection_preserved=1 extended_multi_selection_reference_mapping=1 filtered_selection_mapping=1 deferred_filter_refresh=1 context_click_preserves_or_replaces_selection=1 context_actions_bound=1 timeline_context_trim_actions=1 timeline_click_mapping=1 timeline_vertical_scroll_wheel=1 live_root_splitter_drag=1 live_timestrip_splitter_drag=1 dock_settings_save_restore=1 invalid_settings_fallback=1 ui_scale_150_geometry_font_graph=1 per_monitor_dpi_scale_composition=1 dpi_4k_layout_metrics=1";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
