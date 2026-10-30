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
         "SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK", "SFM64_DIAGNOSTIC_SCENE_ROOT",
         "SFM64_DIAGNOSTIC_SCENE_PREVIEW"}) setOption(name, nullptr);
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
        for (auto dimensions : {std::array<uint32_t, 4>{1920,1080,512,288},
             {3840,2160,512,288}, {1080,1920,288,512}, {1024,1024,512,512},
             {640,480,512,384}, {320,180,320,180}, {854,480,512,288},
             {1001,1000,512,511}, {UINT32_MAX,UINT32_MAX,512,512}}) {
            const auto preview = sessionPreviewDimensionsCandidate(dimensions[0], dimensions[1]);
            if (preview[0] != dimensions[2] || preview[1] != dimensions[3])
                throw std::runtime_error("preview aspect/size mismatch");
        }
        for (auto dimensions : {std::array<uint32_t, 3>{0,720,512},
             {1280,0,512}, {1280,720,0}, {4096,1,512}}) {
            bool rejected = false;
            try { (void)sessionPreviewDimensionsCandidate(dimensions[0], dimensions[1], dimensions[2]); }
            catch (const std::runtime_error&) { rejected = true; }
            if (!rejected) throw std::runtime_error("invalid preview extent accepted");
        }
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
        auto portrait = makeFixture(720, 1280, "live-720x1280.dmx");
        auto invalid = makeFixture(0, 720, "invalid-zero-width.dmx");
        auto missingGraph = graph;
        bool removedMovieSettings = false;
        for (auto& element : missingGraph.elements) {
            auto& attributes = element.attributes;
            const auto before = attributes.size();
            attributes.erase(std::remove_if(attributes.begin(), attributes.end(), [&](const Attribute& a) {
                return missingGraph.strings.at(a.name) == "movieSettings";
            }), attributes.end());
            removedMovieSettings |= before != attributes.size();
        }
        if (!removedMovieSettings || readSessionMovieDimensionsCandidate(missingGraph, root, false))
            throw std::runtime_error("absent preview settings not optional");
        const auto missing = (directory / "missing-movie-settings.dmx").string();
        saveGraph(missingGraph, missing.c_str());
        auto malformed = graph;
        for (auto& element : malformed.elements)
            for (auto& a : element.attributes)
                if (malformed.strings.at(a.name) == "movieSettings") a.type = 2;
        bool malformedRejected = false;
        try { (void)readSessionMovieDimensionsCandidate(malformed, root, false); }
        catch (const std::runtime_error&) { malformedRejected = true; }
        if (!malformedRejected) throw std::runtime_error("malformed optional settings accepted");
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
        setOption("SFM64_DIAGNOSTIC_SCENE_PREVIEW", "1"); expectRejected();
        setOption("SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION", wide.c_str());
        setOption("SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK", "870833");
        expectExtent(512, 288);
        setOption("SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION", square.c_str()); expectExtent(512, 384);
        setOption("SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION", portrait.c_str()); expectExtent(288, 512);
        setOption("SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION", invalid.c_str()); expectRejected();
        setOption("SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION", missing.c_str()); expectExtent(512, 512);
        setOption("SFM64_DIAGNOSTIC_SCENE_OUTPUT", "movie"); expectRejected();
        setOption("SFM64_DIAGNOSTIC_SCENE_OUTPUT", nullptr);
        setOption("SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION", wide.c_str());
        setOption("SFM64_DIAGNOSTIC_SCENE_OUTPUT", "movie"); expectExtent(1280, 720);
        setOption("SFM64_DIAGNOSTIC_SCENE_OUTPUT", nullptr);
        setOption("SFM64_DIAGNOSTIC_SCENE_PREVIEW", "0"); expectExtent(512, 512);
        setOption("SFM64_DIAGNOSTIC_SCENE_PREVIEW", "invalid"); expectRejected();
        clearOptions();
        std::cout << "output sizing passed default=512x512 live=640x480,1280x720 retained="
                  << dimensions.width << "x" << dimensions.height << " retained_roots=explicit,saved guards=8"
                  << " preview_sizes=9 preview_guards=4 preview_modes=retained,movie,disabled invalid_preview_rejected=1"
                  << " missing_settings_preview=preserved missing_settings_movie=rejected malformed_settings=rejected\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
