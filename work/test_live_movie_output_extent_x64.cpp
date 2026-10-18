#define NOMINMAX
#include <windows.h>
#include <vulkan/vulkan.h>
#include <charconv>
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "vulkan_scene_output_extent_candidate.h"
#include <iostream>

static void setOption(const char* name, const char* value) {
    if (!SetEnvironmentVariableA(name, value)) throw std::runtime_error("test environment write");
}
static void clearOptions() {
    for (auto name : {"SFM64_DIAGNOSTIC_SCENE_OUTPUT", "SFM64_DIAGNOSTIC_LIVE_SESSION",
         "SFM64_DIAGNOSTIC_LIVE_ROOT", "SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION",
         "SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK", "SFM64_DIAGNOSTIC_SCENE_ROOT"}) setOption(name, nullptr);
}
static void expectExtent(uint32_t width, uint32_t height) {
    auto extent = vulkanSceneOutputExtentCandidate();
    if (extent.width != width || extent.height != height) throw std::runtime_error("wrong output extent");
}
static void expectRejected() {
    bool rejected = false;
    try { (void)vulkanSceneOutputExtentCandidate(); }
    catch (const std::runtime_error&) { rejected = true; }
    if (!rejected) throw std::runtime_error("invalid output configuration accepted");
}
int main(int argc, char** argv) {
    try {
        static_assert(sizeof(void*) == 8, "x64 required");
        if (argc != 3) throw std::runtime_error("input fixture and fresh output directory required");
        auto directory = std::filesystem::path(argv[2]);
        if (std::filesystem::exists(directory)) throw std::runtime_error("refuse fixture directory replacement");
        std::filesystem::create_directories(directory);
        auto graph = loadGraph(argv[1]);
        const auto root = sessionRenderRootCandidate(graph);
        auto dimensions = sessionMovieDimensionsCandidate(graph, root);
        auto makeFixture = [&](uint32_t width, uint32_t height, const char* name) {
            auto copy = graph;
            for (auto& attribute : copy.elements.at(dimensions.settings).attributes) {
                auto key = copy.strings.at(attribute.name);
                if (key == "width") setWireU32(attribute.values.at(0), width);
                if (key == "height") setWireU32(attribute.values.at(0), height);
            }
            auto path = (directory / name).string();
            saveGraph(copy, path.c_str());
            return path;
        };
        auto square = makeFixture(640, 480, "live-640x480.dmx");
        auto wide = makeFixture(1280, 720, "live-1280x720.dmx");
        auto invalid = makeFixture(0, 720, "invalid-zero-width.dmx");
        auto rootText = std::to_string(root);
        clearOptions(); expectExtent(512, 512);
        setOption("SFM64_DIAGNOSTIC_SCENE_OUTPUT", "movie");
        setOption("SFM64_DIAGNOSTIC_LIVE_SESSION", square.c_str());
        setOption("SFM64_DIAGNOSTIC_LIVE_ROOT", rootText.c_str());
        expectExtent(640, 480);
        setOption("SFM64_DIAGNOSTIC_LIVE_SESSION", wide.c_str()); expectExtent(1280, 720);
        setOption("SFM64_DIAGNOSTIC_LIVE_SESSION", invalid.c_str()); expectRejected();
        setOption("SFM64_DIAGNOSTIC_LIVE_SESSION", wide.c_str());
        setOption("SFM64_DIAGNOSTIC_LIVE_ROOT", nullptr); expectRejected();
        setOption("SFM64_DIAGNOSTIC_LIVE_ROOT", "2237junk"); expectRejected();
        setOption("SFM64_DIAGNOSTIC_LIVE_ROOT", "4294967295"); expectRejected();
        setOption("SFM64_DIAGNOSTIC_LIVE_ROOT", rootText.c_str());
        setOption("SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK", "870833");
        setOption("SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION", argv[1]); expectRejected();
        setOption("SFM64_DIAGNOSTIC_LIVE_SESSION", nullptr);
        setOption("SFM64_DIAGNOSTIC_SCENE_ROOT", rootText.c_str());
        expectExtent(dimensions.width, dimensions.height);
        setOption("SFM64_DIAGNOSTIC_SCENE_ROOT", nullptr); expectExtent(dimensions.width, dimensions.height);
        setOption("SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK", nullptr); expectRejected();
        setOption("SFM64_DIAGNOSTIC_SCENE_OUTPUT", "unsupported"); expectRejected();
        clearOptions();
        setOption("SFM64_DIAGNOSTIC_SCENE_OUTPUT", "movie"); expectRejected();
        clearOptions();
        std::cout << "output sizing passed default=512x512 live=640x480,1280x720 retained="
                  << dimensions.width << "x" << dimensions.height << " retained_roots=explicit,saved guards=8\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
