#pragma once
#include "session_render_root_candidate.h"
#include "session_movie_dimensions_candidate.h"
#include "session_preview_dimensions_candidate.h"
#include <charconv>

inline VkExtent2D vulkanSceneOutputExtentCandidate() {
    char mode[32]{};
    auto size = GetEnvironmentVariableA("SFM64_DIAGNOSTIC_SCENE_OUTPUT", mode, sizeof(mode));
    char preview[8]{};
    const auto previewSize = GetEnvironmentVariableA("SFM64_DIAGNOSTIC_SCENE_PREVIEW", preview, sizeof(preview));
    if (previewSize && (previewSize != 1 || (preview[0] != '0' && preview[0] != '1')))
        throw std::runtime_error("scene preview mode invalid");
    const bool sessionPreview = previewSize && preview[0] == '1';
    if (!size && !sessionPreview) return {512, 512};
    if (size && (size >= sizeof(mode) || std::string(mode) != "movie"))
        throw std::runtime_error("scene output mode invalid");

    // Read the selected provider's session without activating another provider.
    char path[32768]{};
    auto liveSize = GetEnvironmentVariableA("SFM64_DIAGNOSTIC_LIVE_SESSION", path, sizeof(path));
    const bool live = liveSize != 0;
    const bool retained = GetEnvironmentVariableA("SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK", nullptr, 0) != 0;
    if (live && retained) throw std::runtime_error("movie output has conflicting scene providers");
    if (live) {
        if (liveSize >= sizeof(path)) throw std::runtime_error("live movie session path length");
    } else {
        auto pathSize = GetEnvironmentVariableA("SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION", path, sizeof(path));
        if (!pathSize || pathSize >= sizeof(path) || !retained)
            throw std::runtime_error("movie output requires scene session");
    }

    std::optional<uint32_t> root;
    char rootText[32]{};
    const char* rootOption = live ? "SFM64_DIAGNOSTIC_LIVE_ROOT" : "SFM64_DIAGNOSTIC_SCENE_ROOT";
    auto rootSize = GetEnvironmentVariableA(rootOption, rootText, sizeof(rootText));
    if (live && !rootSize) throw std::runtime_error("live movie output requires root");
    if (rootSize) {
        if (rootSize >= sizeof(rootText)) throw std::runtime_error("scene root length");
        uint32_t value = 0;
        auto result = std::from_chars(rootText, rootText + rootSize, value);
        if (result.ec != std::errc{} || result.ptr != rootText + rootSize)
            throw std::runtime_error("scene root invalid");
        root = value;
    }
    auto graph = loadGraph(path);
    const auto dimensions = readSessionMovieDimensionsCandidate(graph, sessionRenderRootCandidate(graph, root), size != 0);
    if (!dimensions) return {512, 512};
    if (!size) {
        const auto extent = sessionPreviewDimensionsCandidate(dimensions->width, dimensions->height);
        return {extent[0], extent[1]};
    }
    return {dimensions->width, dimensions->height};
}
