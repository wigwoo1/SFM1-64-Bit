#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <vector>

// The application sets an executable-relative directory before renderer setup.
// Standalone historical component probes retain their explicit input paths.
inline std::optional<std::filesystem::path> editorShaderDirectoryCandidate;

inline void initializeEditorShaderResourcesCandidate() {
    std::vector<wchar_t> buffer(260);
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), DWORD(buffer.size()));
        if (!length) throw std::runtime_error("cannot resolve editor executable directory");
        if (length < buffer.size()) {
            editorShaderDirectoryCandidate = std::filesystem::path(
                std::wstring(buffer.data(), length)).parent_path() / "shaders";
            return;
        }
        if (buffer.size() >= 32768) throw std::runtime_error("editor executable path too long");
        buffer.resize(buffer.size() * 2 > 32768 ? 32768 : buffer.size() * 2);
    }
}

inline std::filesystem::path editorShaderResourcePathCandidate(const std::filesystem::path& input) {
    if (!editorShaderDirectoryCandidate) return input;
    if (input.extension() != L".spv") throw std::runtime_error("invalid editor shader resource name");
    return *editorShaderDirectoryCandidate / input.filename();
}
