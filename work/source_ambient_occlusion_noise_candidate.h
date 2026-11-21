#pragma once
#include "ambient_occlusion_view_candidate.h"
#include "mounted_model_material_candidate.h"
#include "read_binary_asset_candidate.h"
#include "vtf_block_decode.h"

inline constexpr uint32_t sourceAmbientOcclusionNoiseExtentCandidate = 32;

inline std::vector<AmbientOcclusionViewCandidate::Row> readSourceAmbientOcclusionNoiseCandidate(
        const std::vector<std::filesystem::path>& mounts) {
    const auto path = findMountedMaterialCandidate(mounts,"engine/ssaoreflectionvectors.vtf");
    if (path.empty())
        throw std::runtime_error("Source AO noise texture not found on the mounted content paths");
    const auto bytes = readBinaryAssetCandidate(path);
    const auto layout = readVtfTextureLayoutCandidate(bytes);
    const auto& mip = layout.mips.front();
    constexpr auto extent = sourceAmbientOcclusionNoiseExtentCandidate;
    // The inspected stock asset is a single-frame, point-filtered, repeating
    // BGR888 texture with no mipmaps. Decode it as linear UNORM data.
    if (layout.format != 3 || layout.frameCount != 1 || layout.mips.size() != 1 ||
        mip.width != extent || mip.height != extent || (layout.flags & 0x30fu) != 0x301u)
        throw std::runtime_error("Unsupported Source AO noise texture format or sampling flags");
    const auto image = decodeVtfMipCandidate(bytes,0);
    std::vector<AmbientOcclusionViewCandidate::Row> pixels(extent*extent);
    for (size_t i=0;i<pixels.size();++i)
        for (size_t channel=0;channel<pixels[i].size();++channel)
            pixels[i][channel] = float(image.rgba[4*i+channel])/255.f;
    return pixels;
}
