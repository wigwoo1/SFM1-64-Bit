#define NOMINMAX
#include <windows.h>
#define SFM64_RETAINED_HDR2_PROFILE_NO_MAIN
#include "test_retained_hdr2_profile.cpp"
#include "editor_hdr2_profile_candidate.h"
#include <iomanip>

// Dump only generated, prepared CPU scene data. No GPU or original SFM runtime.
int main(int argc, char** argv) {
    try {
        static_assert(sizeof(void*) == 8, "native x64 fixture inspection");
        check(argc == 3, "owned content directory and profile required");
        const std::filesystem::path root = argv[1];
        std::ifstream input(argv[2]);
        auto profile = readEditorHdr2ProfileCandidate(input);
        RetainedSessionSceneCandidate owner(loadGraph((root / "owned.dmx").string().c_str()),
            1, {root}, 1, false, false, profile);
        const auto scene = owner.sample(0, 512, 512);
        std::cout << std::setprecision(9) << "camera";
        for (float value : scene.camera.rows) std::cout << ' ' << value;
        std::cout << '\n';
        for (size_t index = 0; index < scene.upload.vertices.size(); ++index) {
            std::cout << "vertex " << index;
            for (float value : scene.upload.vertices[index].position) std::cout << ' ' << value;
            std::cout << '\n';
        }
        for (const auto* draws : {&scene.upload.opaque, &scene.upload.translucent})
            for (const auto& draw : *draws)
                for (uint32_t offset = 0; offset < draw.indexCount; offset += 3) {
                    std::cout << "triangle";
                    for (uint32_t index = 0; index < 3; ++index)
                        std::cout << ' ' << scene.upload.indices.at(draw.firstIndex + offset + index);
                    std::cout << '\n';
                }
        std::cout << "PASS fixture_geometry_only=1 gpu_execution=0 original_runtime=ABSENT\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
