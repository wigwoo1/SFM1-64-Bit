#include "live_session_frame_provider_candidate.h"
#include "sfm_model_geometry.h"
#include <iostream>

static std::vector<uint8_t> readBytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot read fixture asset: " + path.string());
    return {std::istreambuf_iterator<char>(file), {}};
}
static bool sameCamera(const RenderCameraCandidate& a, const RenderCameraCandidate& b) {
    return std::memcmp(a.rows.data(), b.rows.data(), sizeof(a.rows)) == 0;
}
int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("session, root, model base required");
        const auto rootValue = std::stoul(argv[2]);
        if (rootValue > UINT32_MAX) throw std::runtime_error("root id range");
        auto originalGraph = loadGraph(argv[1]);
        const int32_t tick = 870833;
        auto selected = selectFilmShotCandidate(originalGraph, static_cast<uint32_t>(rootValue), tick);
        auto binding = readFilmSceneRenderBindingCandidate(originalGraph, selected.clip);
        if (binding.models.size() != 1) throw std::runtime_error("fixture is not a single-model shot");
        auto model = readSfmModelGeometryCandidate(readBytes(std::string(argv[3]) + ".mdl"),
                                                    readBytes(std::string(argv[3]) + ".vvd"),
                                                    readBytes(std::string(argv[3]) + ".dx90.vtx"));
        auto assertSize = [&](uint32_t width, uint32_t height, bool useDefaults) {
            auto graph = loadGraph(argv[1]);
            auto provider = useDefaults
                ? liveSessionFrameProviderCandidate(std::move(graph), static_cast<uint32_t>(rootValue), {tick}, argv[3])
                : liveSessionFrameProviderCandidate(std::move(graph), static_cast<uint32_t>(rootValue), {tick}, argv[3], width, height);
            auto state = provider.sample(0, model.bones);
            auto evaluatedGraph = originalGraph;
            RetainedShotTimelineCandidate expectedTimeline(evaluatedGraph, static_cast<uint32_t>(rootValue));
            auto evaluated = expectedTimeline.step(tick, 2, 2);
            const auto& frameGraph = expectedTimeline.graph();
            auto frameBinding = readFilmSceneRenderBindingCandidate(frameGraph, evaluated.shot.clip);
            auto expected = sessionCameraViewProjectionCandidate(frameGraph, frameBinding.camera,
                static_cast<int>(width), static_cast<int>(height), true);
            if (!sameCamera(state.camera, expected)) throw std::runtime_error("sample camera ignored requested output size");
            if (provider.initialSkin != sessionModelSkinCandidate(originalGraph, binding.models.front()) ||
                provider.initialBody != sessionModelBodyCandidate(originalGraph, binding.models.front()))
                throw std::runtime_error("single-model skin/body selection changed");
        };
        assertSize(1024, 768, false);
        assertSize(512, 512, false);
        assertSize(1280, 720, false);
        assertSize(512, 512, true);
        for (auto [width, height] : {std::pair<uint32_t,uint32_t>{0,720}, {1280,0}}) {
            bool rejected = false;
            try { auto graph = loadGraph(argv[1]); (void)liveSessionFrameProviderCandidate(
                    std::move(graph), static_cast<uint32_t>(rootValue), {tick}, argv[3], width, height); }
            catch (const std::runtime_error& e) { rejected = std::string(e.what()).find("dimensions") != std::string::npos; }
            if (!rejected) throw std::runtime_error("zero output dimension was not rejected");
        }
        std::cout << "single-model live provider dimensions passed fixture_shot=" << selected.clip
                  << " model_count=1 aspect_cases=4 zero_dimension_guards=2 skin_body_selection=preserved\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
