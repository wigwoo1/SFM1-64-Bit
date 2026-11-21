#pragma once

#include "material_texture_file_resolver_candidate.h"
#include "mounted_model_material_candidate.h"
#include "read_binary_asset_candidate.h"
#include "source_particle_definition_candidate.h"
#include "vtf_particle_sheet_candidate.h"

// The replay borrows metadata from this owner-scoped, read-only cache. Resolve
// the logical material name and its base texture through the existing ordered
// GAME mounts; only the sheet metadata is kept between samples.
class SourceParticleSheetCacheCandidate {
    const std::vector<std::filesystem::path>& mounts_;
    std::map<std::filesystem::path, std::optional<VtfParticleSheetCandidate>> sheets_;

    std::optional<VtfParticleSheetCandidate> load(const std::filesystem::path& relative) const {
        const auto material = findLooseParticleFileCandidate(mounts_, relative);
        if (material.empty()) return {};
        Reader reader(material);
        if (reader.size > 1024u * 1024u) throw std::runtime_error("particle material is too large");
        std::string text(static_cast<size_t>(reader.size), '\0');
        reader.bytes(text.data(), text.size());
        const auto syntax = parseVmtCandidate(text);
        const auto textures = vmtValuesCandidate(syntax, "$basetexture");
        if (vmtAsciiLowerCandidate(syntax.key) != "spritecard" || textures.size() != 1) return {};
        MaterialTextureFileRequestCandidate request;
        if (!materialTextureFileRequestCandidate(textures.front(), request)) return {};
        const auto texture = findLooseParticleFileCandidate(mounts_, request.file);
        if (texture.empty()) return {};
        if (std::filesystem::file_size(texture) > (uint64_t{1} << 29))
            throw std::runtime_error("particle texture is too large");
        return readVtfParticleSheetCandidate(readBinaryAssetCandidate(texture));
    }

public:
    explicit SourceParticleSheetCacheCandidate(const std::vector<std::filesystem::path>& mounts)
        : mounts_(mounts) {}

    const VtfParticleSheetCandidate* resolve(const std::string& material) {
        const auto relative = "materials" / materialRelativePathCandidate(vmtAsciiLowerCandidate(material), ".vmt");
        const auto [entry, inserted] = sheets_.try_emplace(relative);
        if (inserted) {
            try {
                entry->second = load(relative);
            } catch (const std::runtime_error&) {
                // Preserve the existing emission-only path for unusable assets.
            }
        }
        return entry->second ? &*entry->second : nullptr;
    }
};
