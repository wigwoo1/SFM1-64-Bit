#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
#include <xmmintrin.h>

// Independently authored installed per-instance opcodes11/15/18 only.
// These are not ordinary texture/shader commands. No renderer/cache/queue
// ownership is implied. Callers select the FP environment and actual HDR scale.
inline std::uint32_t sourceModulationBitsCandidate(float x) {
    std::uint32_t b; std::memcpy(&b,&x,4); return b;
}
inline float sourceModulationFloatCandidate(std::uint32_t b) {
    float x; std::memcpy(&x,&b,4); return x;
}
inline float sourceModulationMultiplyCandidate(float a,float b) {
    // Preserve native SSE operand order and separate rounding of each multiply.
    return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));
}
inline std::array<std::uint32_t,18> sourceLightmapInstanceCommandsCandidate(float scale) {
    const auto one=sourceModulationBitsCandidate(1.f),s=sourceModulationBitsCandidate(scale);
    return {11,21,one,one,one,15,12,one,one,one,s,18,47,one,one,one,s,0};
}
struct SourceInstanceModulationConstantCandidate {
    bool vertex=false;
    std::uint32_t index=0;
    std::array<float,4> value{};
};
struct SourceInstanceModulationPlanCandidate {
    std::vector<SourceInstanceModulationConstantCandidate> constants;
};
// A checked desired-constant planner, not the native submission cache.
// Refusal leaves output unchanged. Allocation exceptions propagate before commit.
// Byte49==2 deliberately overrides RGB with1; its wider configuration meaning
// is not inferred here. Alpha semantics differ between opcodes15 and18.
inline bool sourceInstanceModulationPlanCandidate(SourceInstanceModulationPlanCandidate& output,
    const std::uint32_t* words,std::size_t count,const std::array<float,4>& instance,
    std::uint8_t configurationByte49,std::uint32_t pixelLimit,std::uint32_t vertexLimit) {
    if(!words||!count||count>128)return false;
    SourceInstanceModulationPlanCandidate next;std::size_t at=0;
    while(at<count) {
        const auto op=words[at++];
        if(op==0){if(at!=count)return false;output.constants.swap(next.constants);return true;}
        if(op!=11&&op!=15&&op!=18)return false;
        const std::size_t operands=op==11?4:5;
        if(count-at<operands)return false;
        SourceInstanceModulationConstantCandidate p;p.vertex=op==18;p.index=words[at++];
        if(p.index>=(p.vertex?vertexLimit:pixelLimit))return false;
        std::array<float,3> tint{};for(auto&v:tint)v=sourceModulationFloatCandidate(words[at++]);
        const float scale=op==11?1.f:sourceModulationFloatCandidate(words[at++]);
        for(std::size_t k=0;k<3;++k) {
            if(configurationByte49==2){p.value[k]=1.f;continue;}
            const bool tintFirst=k==0&&op!=15;
            float value=tintFirst?sourceModulationMultiplyCandidate(tint[k],instance[k]):sourceModulationMultiplyCandidate(instance[k],tint[k]);
            if(op!=11)value=sourceModulationMultiplyCandidate(value,scale);
            p.value[k]=value;
        }
        p.value[3]=op==15?scale:instance[3];next.constants.push_back(p);
    }
    return false;
}
