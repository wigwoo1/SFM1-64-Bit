#pragma once
// Independently authored CPU lightmap accumulation stage. This is normalized
// TexLightLinear, not vector-color units, packed GPU storage or tone mapping.
// Native reference evidence is kept privately outside the published work tree.
#include <array>
#include <vector>
#include <cstdint>
#include <cmath>
#include <xmmintrin.h>

enum class SourceLightmapAccumulationUnitsCandidate { TexLightLinear };
using SourceLightmapLinearPixelCandidate=std::array<float,4>;
using SourceLightmapRawSampleCandidate=std::array<std::uint8_t,4>;

inline bool sourceLightmapAccumulationCandidate(
    std::vector<SourceLightmapLinearPixelCandidate>& destination,
    const std::vector<SourceLightmapRawSampleCandidate>& samples,float scale,
    SourceLightmapAccumulationUnitsCandidate units){
    // Refuse unsupported arithmetic environments rather than change the host's
    // rounding mode, denormal handling or exception masks. Status flags may rise
    // as with the original scalar SSE arithmetic; control bits remain unchanged.
    const auto control=_mm_getcsr();
    if(units!=SourceLightmapAccumulationUnitsCandidate::TexLightLinear||
       samples.size()!=destination.size()||samples.size()>16384||
       !std::isfinite(scale)||scale<0.f||scale>16.f||
       (control&0xe040u)!=0||(control&0x1f80u)!=0x1f80u)return false;
    auto staged=destination;
    for(size_t i=0;i<samples.size();++i){
        const auto& raw=samples[i];
        const int exponent=raw[3]<128?int(raw[3]):int(raw[3])-256;
        // The table entry is rounded before multiplication, independently of
        // the channel. Compute this owned value, do not embed original bytes.
        const float factor=static_cast<float>(std::ldexp(1.0,exponent)/255.0);
        for(size_t c=0;c<3;++c){
            if(!std::isfinite(destination[i][c])||destination[i][c]<0.f)return false;
            const auto decoded=_mm_mul_ss(_mm_set_ss(float(raw[c])),_mm_set_ss(factor));
            const auto scaled=_mm_mul_ss(decoded,_mm_set_ss(scale));
            const float sum=_mm_cvtss_f32(_mm_add_ss(scaled,_mm_set_ss(destination[i][c])));
            if(!std::isfinite(sum)||sum<0.f)return false;
            staged[i][c]=sum;
        }
        // This RGB stage does not read or write alpha, including its bit pattern.
    }
    destination.swap(staged);
    return true;
}
