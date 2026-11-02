#include "source_plain_lightmap_sampling_candidate.h"
#include <cassert>
#include <cstring>
#include <limits>
#include <iostream>
#include <vector>

static SourcePlainLightmapSamplingInputCandidate validInput() {
    SourcePlainLightmapSamplingInputCandidate in{};
    in.axes[0] = {1, 0, 0, 0};
    in.axes[1] = {0, 1, 0, 0};
    in.minima = {-1, 2}; in.size = {4, 3}; in.pageOffset = {2, 1};
    in.pageDimensions = {10, 8}; in.position = {1, 5, 0};
    return in;
}
static bool sameBits(float a, float b) {
    uint32_t x, y; std::memcpy(&x, &a, 4); std::memcpy(&y, &b, 4); return x == y;
}
static float fromBits(uint32_t bits) { float value; std::memcpy(&value,&bits,4); return value; }
static bool sameInput(const SourcePlainLightmapSamplingInputCandidate& a,
                      const SourcePlainLightmapSamplingInputCandidate& b) {
    for (size_t i=0;i<2;++i) {
        if (a.minima[i]!=b.minima[i] || a.size[i]!=b.size[i] ||
            a.pageOffset[i]!=b.pageOffset[i] || a.pageDimensions[i]!=b.pageDimensions[i]) return false;
        for(size_t j=0;j<4;++j) if(!sameBits(a.axes[i][j],b.axes[i][j])) return false;
    }
    for(size_t i=0;i<3;++i) if(!sameBits(a.position[i],b.position[i])) return false;
    return a.surfaceFlags==b.surfaceFlags;
}
static unsigned refusalCase=0;
static void refuseUnchanged(const SourcePlainLightmapSamplingInputCandidate& in) {
    const auto saved = in;
    std::array<float,2> out{ -0.0f, 123.25f }, before=out;
    const uint32_t controlBefore=_mm_getcsr();
    if(sourcePlainLightmapSamplingCandidate(out,in)) { std::cerr << "unexpectedly accepted refusal case " << refusalCase << "\n"; std::abort(); }
    ++refusalCase;
    assert(sameBits(out[0],before[0]) && sameBits(out[1],before[1]));
    assert(sameInput(in,saved));
    assert((_mm_getcsr()&~0x3fu)==(controlBefore&~0x3fu));
}
int main() {
    auto in=validInput(); std::array<float,2> out{};
    assert(sourcePlainLightmapSamplingCandidate(out,in));
    assert(sameBits(out[0],.45f) && sameBits(out[1],.5625f));

    // Non-power-of-two rectangular page, distinct offsets and zero-S branch.
    in.pageDimensions={7,5}; in.size={0,2}; in.pageOffset={1,2};
    assert(sourcePlainLightmapSamplingCandidate(out,in));
    assert(sameBits(out[0],fromBits(0x3e5b6db8u)) && sameBits(out[1],.5f));
    // A zero T extent by itself stays on the ordinary projection path.
    in=validInput(); in.size[1]=0; in.position={1,2,0};
    assert(sourcePlainLightmapSamplingCandidate(out,in));
    assert(sameBits(out[0],.45f) && sameBits(out[1],.1875f));
    // Outside-face coordinates clamp to the full page edges.
    in=validInput(); in.position={-100,100,0};
    assert(sourcePlainLightmapSamplingCandidate(out,in));
    assert(sameBits(out[0],0.0f) && sameBits(out[1],1.0f));
    // Default-zero input refuses safely.
    refuseUnchanged(SourcePlainLightmapSamplingInputCandidate{});

    unsigned refusals=0;
    auto reject=[&](auto edit){auto bad=validInput();edit(bad);refuseUnchanged(bad);++refusals;};
    reject([](auto&x){x.surfaceFlags=1;});
    for(size_t a=0;a<2;++a){
        reject([=](auto&x){x.pageDimensions[a]=0;});
        reject([=](auto&x){x.pageDimensions[a]=32769;});
        reject([=](auto&x){x.size[a]=-1;});
        reject([=](auto&x){x.size[a]=32768;});
        reject([=](auto&x){x.minima[a]=-32769;});
        reject([=](auto&x){x.minima[a]=32768;});
        reject([=](auto&x){x.pageOffset[a]=-1;});
        reject([=](auto&x){x.pageOffset[a]=32768;});
        reject([=](auto&x){x.size[a]=2;x.pageOffset[a]=int32_t(x.pageDimensions[a])-2;});
    }
    for(size_t a=0;a<2;++a)for(size_t j=0;j<4;++j) {
        reject([=](auto&x){x.axes[a][j]=std::numeric_limits<float>::infinity();});
        if(a==1&&j==3)reject([](auto&x){x.axes[0][0]=std::numeric_limits<float>::max();x.position[0]=std::numeric_limits<float>::max();});
        else reject([=](auto&x){x.axes[a][j]=std::numeric_limits<float>::quiet_NaN();});
    }
    for(size_t j=0;j<3;++j)
        reject([=](auto&x){x.position[j]=std::numeric_limits<float>::quiet_NaN();});
    assert(refusals==38);

    const uint32_t original=_mm_getcsr();
    for(uint32_t bit : {0x2000u,0x4000u,0x40u,0x8000u,0x80u,0x100u,0x200u,0x400u,0x800u,0x1000u}) {
        _mm_setcsr(original ^ bit); refuseUnchanged(validInput());
    }
    _mm_setcsr(original);
    std::cout << "PASS literal branches; 38 refusal fixtures; MXCSR controls; preservation\n";
}
