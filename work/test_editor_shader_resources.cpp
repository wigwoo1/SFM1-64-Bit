#include "editor_shader_resources_candidate.h"
#include <iostream>

int main(int argc, char** argv) {
    try {
        static_assert(sizeof(void*) == 8, "x64 required");
        if (argc != 2) throw std::runtime_error("foreign working directory required");
        const std::filesystem::path legacy("build/session-core/sfm64-diagnostic-model.spv");
        if (editorShaderResourcePathCandidate(legacy) != legacy)
            throw std::runtime_error("standalone probe path contract changed");
        initializeEditorShaderResourcesCandidate();
        const auto expected = *editorShaderDirectoryCandidate / legacy.filename();
        const auto original = std::filesystem::current_path();
        std::filesystem::current_path(argv[1]);
        if (editorShaderResourcePathCandidate(legacy) != expected || !expected.is_absolute())
            throw std::runtime_error("editor shader path depends on current directory");
        bool rejected = false;
        try { (void)editorShaderResourcePathCandidate("unlisted.dll"); }
        catch (const std::runtime_error&) { rejected = true; }
        if (!rejected) throw std::runtime_error("invalid resource extension accepted");
        std::filesystem::current_path(original);
        std::cout << "PASS executable_relative_shader_path=1 standalone_probe_path=preserved invalid_extension=rejected\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
