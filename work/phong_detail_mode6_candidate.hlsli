#ifndef SFM64_PHONG_DETAIL_MODE6
#define SFM64_PHONG_DETAIL_MODE6

// Linear RGB contribution of the ordinary Phong detail blend mode 6.
// Apply the draw's exposure and fog after adding this to the lit RGB.
float3 phongDetailMode6Candidate(float3 texel, float factor) {
    return factor <= 0.5
        ? (4 * factor) * max(texel - 0.5, 0)
        : saturate((texel - (1 - factor)) / factor);
}

#endif
