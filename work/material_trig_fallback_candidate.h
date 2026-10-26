#pragma once
#include "material_trig_fast_candidate.h"
#include <emmintrin.h>

extern "C" float materialTrigX87Candidate(float radians,int cosine);
extern "C" uint32_t materialTrigX87EnvironmentCandidate();

struct MaterialTrigDoubleCellCandidate {double high,low,residual,power;};
inline MaterialTrigDoubleCellCandidate materialTrigDoubleCellCandidate(unsigned index) {
    // Independent high-precision sin(k*pi/32). Fields: binary64 value,
    // binary32-rounded low residual, binary64 residual from nearest power of
    // two, and that power. Regeneration/checking lives in recover_material_trig_fallback.py.
    constexpr MaterialTrigDoubleCellCandidate quarter[] = {
        {0x0p+0,0x0p+0,0x0p+0,0x0p+0},
        {0x1.917a6bc29b42cp-4,-0x1.e2718ep-60,-0x1.ba1650f592f50p-6,0x1p-3},
        {0x1.8f8b83c69a60bp-3,-0x1.26d19cp-57,-0x1.c1d1f0e5967d5p-5,0x1p-2},
        {0x1.294062ed59f06p-2,-0x1.5d28dap-56,0x1.4a03176acf82dp-5,0x1p-2},
        {0x1.87de2a6aea963p-2,-0x1.72cedep-57,-0x1.e087565455a75p-4,0x1p-1},
        {0x1.e2b5d3806f63bp-2,0x1.e0d892p-58,-0x1.d4a2c7f909c4ep-6,0x1p-1},
        {0x1.1c73b39ae68c8p-1,0x1.b25dd2p-55,0x1.c73b39ae68c87p-5,0x1p-1},
        {0x1.44cf325091dd6p-1,0x1.8076a2p-57,0x1.133cc94247758p-3,0x1p-1},
        {0x1.6a09e667f3bcdp-1,-0x1.bdd342p-55,0x1.a827999fcef32p-3,0x1p-1},
        {0x1.8bc806b151741p-1,-0x1.2c5e12p-55,-0x1.d0dfe53aba2fdp-3,0x1p+0},
        {0x1.a9b66290ea1a3p-1,0x1.9f630ep-60,-0x1.592675bc57974p-3,0x1p+0},
        {0x1.c38b2f180bdb1p-1,-0x1.6e0b18p-56,-0x1.e3a6873fa1279p-4,0x1p+0},
        {0x1.d906bcf328d46p-1,0x1.457e62p-56,-0x1.37ca1866b95cfp-4,0x1p+0},
        {0x1.e9f4156c62ddap-1,0x1.760b1ep-55,-0x1.60bea939d225ap-5,0x1p+0},
        {0x1.f6297cff75cb0p-1,0x1.562172p-56,-0x1.3ad06011469fbp-6,0x1p+0},
        {0x1.fd88da3d12526p-1,-0x1.87df64p-55,-0x1.3b92e176d6d31p-8,0x1p+0},
        {0x1p+0,0x0p+0,0x0p+0,0x1p+0}
    };
    index &= 63;
    const unsigned reflected = index<=16 ? index : index<=32 ? 32-index : index<=48 ? index-32 : 64-index;
    auto cell=quarter[reflected];
    if(index>32) {
        // Installed table axis zeroes are positive even in negative quadrants.
        if(cell.high!=0)cell.high=-cell.high;
        if(cell.low!=0)cell.low=-cell.low;
        if(cell.residual!=0)cell.residual=-cell.residual;
        if(cell.power!=0)cell.power=-cell.power;
    }
    return cell;
}

inline float materialTrigDoubleModerateCandidate(float radians,bool cosine) {
    // Reachable double-SSE2 branch of the float wrapper: 8192<=abs(x)<90112.
    // Express both packed lanes with explicit scalar operation order; no FMA.
    const double x=radians;
    const int n=_mm_cvtsd_si32(_mm_mul_sd(_mm_set_sd(x),_mm_set_sd(0x1.45f306dc9c883p+3)));
    const double dn=n;
    const double firstProduct=dn*0x1.921fb544p-4;
    const double secondProduct=dn*0x1.0b4611a6p-38;
    const double preReduced=x-firstProduct;
    const double reduced=preReduced-secondProduct;
    const double reductionError=(preReduced-reduced)-secondProduct;
    const double tail=dn*0x1.3198a2e037073p-73-reductionError;
    const double squared=reduced*reduced;
    const double fourth=squared*squared;
    const double sineHighTerm=((1.0/362880.0)*preReduced)*reduced;
    const double cosineHighTerm=((1.0/40320.0)*preReduced)*reduced;
    const double sinePolynomial=((1.0/120.0)*squared-1.0/6.0)+(sineHighTerm-1.0/5040.0)*fourth;
    const double cosinePolynomial=((1.0/24.0)*squared-0.5)+(cosineHighTerm-1.0/720.0)*fourth;
    const unsigned index=(static_cast<unsigned>(n)+(cosine?16u:0u))&63;
    const auto s=materialTrigDoubleCellCandidate(index),c=materialTrigDoubleCellCandidate(index+16);
    const double cosHigh=c.residual+c.power;
    const double derivative=s.high*reduced-cosHigh;
    const double cosineTimesReduced=cosHigh*reduced;
    const double powerTimesReduced=c.power*reduced;
    const double residualTimesReduced=c.residual*reduced;
    const double partial=powerTimesReduced+s.high;
    const double leading=residualTimesReduced+partial;
    const double errorA=(s.high-partial)+powerTimesReduced;
    const double errorB=(partial-leading)+residualTimesReduced;
    double correction=tail*derivative;
    correction+=s.low;
    correction+=errorA;
    correction+=errorB;
    correction+=sinePolynomial*(cosineTimesReduced*squared);
    correction+=cosinePolynomial*(s.high*squared);
    return static_cast<float>(leading+correction);
}

inline float materialTrigCandidate(float radians,bool cosine,bool sse2=false) {
    float value=0;
    if(materialTrigFastCandidate(radians,cosine,value))return value;
    // Default flag0 describes the controlled legacy fixture, not a discovered
    // live-engine setting. Callers must explicitly choose their dispatch state.
    uint32_t bits=0;std::memcpy(&bits,&radians,4);
    if(sse2&&(bits&0x7fffffffu)<0x47b00000u)return materialTrigDoubleModerateCandidate(radians,cosine);
    return materialTrigX87Candidate(radians,cosine?1:0);
}
