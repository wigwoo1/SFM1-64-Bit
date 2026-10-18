#pragma once
#include <array>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <stdexcept>
inline double sineBitsDouble(uint64_t bits){double d;std::memcpy(&d,&bits,8);return d;}
inline constexpr std::array<std::array<double,2>,64> legacySinTable{{{0x0.0p+0,0x1.0000000000000p+0},{0x1.917a6bc29b42cp-4,0x1.fd88da3d12526p-1},{0x1.8f8b83c69a60bp-3,0x1.f6297cff75cb0p-1},{0x1.294062ed59f06p-2,0x1.e9f4156c62ddap-1},{0x1.87de2a6aea963p-2,0x1.d906bcf328d46p-1},{0x1.e2b5d3806f63bp-2,0x1.c38b2f180bdb1p-1},{0x1.1c73b39ae68c8p-1,0x1.a9b66290ea1a3p-1},{0x1.44cf325091dd6p-1,0x1.8bc806b151741p-1},{0x1.6a09e667f3bcdp-1,0x1.6a09e667f3bcdp-1},{0x1.8bc806b151741p-1,0x1.44cf325091dd6p-1},{0x1.a9b66290ea1a3p-1,0x1.1c73b39ae68c8p-1},{0x1.c38b2f180bdb1p-1,0x1.e2b5d3806f63bp-2},{0x1.d906bcf328d46p-1,0x1.87de2a6aea963p-2},{0x1.e9f4156c62ddap-1,0x1.294062ed59f06p-2},{0x1.f6297cff75cb0p-1,0x1.8f8b83c69a60bp-3},{0x1.fd88da3d12526p-1,0x1.917a6bc29b42cp-4},{0x1.0000000000000p+0,0x0.0p+0},{0x1.fd88da3d12526p-1,-0x1.917a6bc29b42cp-4},{0x1.f6297cff75cb0p-1,-0x1.8f8b83c69a60bp-3},{0x1.e9f4156c62ddap-1,-0x1.294062ed59f06p-2},{0x1.d906bcf328d46p-1,-0x1.87de2a6aea963p-2},{0x1.c38b2f180bdb1p-1,-0x1.e2b5d3806f63bp-2},{0x1.a9b66290ea1a3p-1,-0x1.1c73b39ae68c8p-1},{0x1.8bc806b151741p-1,-0x1.44cf325091dd6p-1},{0x1.6a09e667f3bcdp-1,-0x1.6a09e667f3bcdp-1},{0x1.44cf325091dd6p-1,-0x1.8bc806b151741p-1},{0x1.1c73b39ae68c8p-1,-0x1.a9b66290ea1a3p-1},{0x1.e2b5d3806f63bp-2,-0x1.c38b2f180bdb1p-1},{0x1.87de2a6aea963p-2,-0x1.d906bcf328d46p-1},{0x1.294062ed59f06p-2,-0x1.e9f4156c62ddap-1},{0x1.8f8b83c69a60bp-3,-0x1.f6297cff75cb0p-1},{0x1.917a6bc29b42cp-4,-0x1.fd88da3d12526p-1},{0x0.0p+0,-0x1.0000000000000p+0},{-0x1.917a6bc29b42cp-4,-0x1.fd88da3d12526p-1},{-0x1.8f8b83c69a60bp-3,-0x1.f6297cff75cb0p-1},{-0x1.294062ed59f06p-2,-0x1.e9f4156c62ddap-1},{-0x1.87de2a6aea963p-2,-0x1.d906bcf328d46p-1},{-0x1.e2b5d3806f63bp-2,-0x1.c38b2f180bdb1p-1},{-0x1.1c73b39ae68c8p-1,-0x1.a9b66290ea1a3p-1},{-0x1.44cf325091dd6p-1,-0x1.8bc806b151741p-1},{-0x1.6a09e667f3bcdp-1,-0x1.6a09e667f3bcdp-1},{-0x1.8bc806b151741p-1,-0x1.44cf325091dd6p-1},{-0x1.a9b66290ea1a3p-1,-0x1.1c73b39ae68c8p-1},{-0x1.c38b2f180bdb1p-1,-0x1.e2b5d3806f63bp-2},{-0x1.d906bcf328d46p-1,-0x1.87de2a6aea963p-2},{-0x1.e9f4156c62ddap-1,-0x1.294062ed59f06p-2},{-0x1.f6297cff75cb0p-1,-0x1.8f8b83c69a60bp-3},{-0x1.fd88da3d12526p-1,-0x1.917a6bc29b42cp-4},{-0x1.0000000000000p+0,0x0.0p+0},{-0x1.fd88da3d12526p-1,0x1.917a6bc29b42cp-4},{-0x1.f6297cff75cb0p-1,0x1.8f8b83c69a60bp-3},{-0x1.e9f4156c62ddap-1,0x1.294062ed59f06p-2},{-0x1.d906bcf328d46p-1,0x1.87de2a6aea963p-2},{-0x1.c38b2f180bdb1p-1,0x1.e2b5d3806f63bp-2},{-0x1.a9b66290ea1a3p-1,0x1.1c73b39ae68c8p-1},{-0x1.8bc806b151741p-1,0x1.44cf325091dd6p-1},{-0x1.6a09e667f3bcdp-1,0x1.6a09e667f3bcdp-1},{-0x1.44cf325091dd6p-1,0x1.8bc806b151741p-1},{-0x1.1c73b39ae68c8p-1,0x1.a9b66290ea1a3p-1},{-0x1.e2b5d3806f63bp-2,0x1.c38b2f180bdb1p-1},{-0x1.87de2a6aea963p-2,0x1.d906bcf328d46p-1},{-0x1.294062ed59f06p-2,0x1.e9f4156c62ddap-1},{-0x1.8f8b83c69a60bp-3,0x1.f6297cff75cb0p-1},{-0x1.917a6bc29b42cp-4,0x1.fd88da3d12526p-1}}};
inline float recoveredLegacySinFinite(float x){
 if(!std::isfinite(x)||std::fabs(x)>100.125f)throw std::runtime_error("legacy sine range not implemented");
 uint32_t xb;std::memcpy(&xb,&x,4);
 if((xb&0x7fffffff)<0x00800000)return float(double(x)*0x1.fffffff800000p-1);
 uint64_t reductionBits=0x4122f983ull;
 uint32_t rb=uint32_t(reductionBits);float multiplier;std::memcpy(&multiplier,&rb,4);
 float reduction=x*multiplier;const int32_t index=int32_t(std::nearbyint(reduction));
 const float magic=0x1.8000000000000p+23f;
 reduction=(reduction+magic)-magic;
 std::memcpy(&rb,&reduction,4);reductionBits=(reductionBits&0xffffffff00000000ull)|rb;
 const double n=sineBitsDouble(reductionBits<<29);
 uint64_t sign=uint64_t(xb)&0x80000000ull;
 sign=(sign<<32)^(sign<<29);
 const double main=sineBitsDouble(0x77b921fb54444000ull^sign);
 const double tail=sineBitsDouble(sign^0x7532e7b9676733afull);
 const double r=double(x)-main*n;
 const double corrected=tail*n+r;
 const double r2=r*r,r4=r2*r2;
 const double c=(-0x1.0000000000000p-1*r2+0x1.0000000000000p+0)+0x1.5555555555555p-5*r4;
 const double p=(-0x1.5555555555555p-3*r2+0x1.0000000000000p+0)+0x1.1111111111111p-7*r4;
 const auto& table=legacySinTable[uint32_t(index+0x1C7600)&63];
 const double a=table[0]*c;
 const double b=(corrected*table[1])*p;
 return float(a+b);
}
