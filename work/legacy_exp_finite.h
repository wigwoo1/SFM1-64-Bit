#pragma once
#include <array>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <stdexcept>
inline constexpr std::array<double,16> expLow{0x1.0000000000000p+0,0x1.0002c5cc37da9p+0,0x1.00058ba01fba0p+0,0x1.0008517bb7b38p+0,0x1.000b175effdc7p+0,0x1.000ddd49f84a3p+0,0x1.0010a33ca1120p+0,0x1.00136936fa493p+0,0x1.00162f3904052p+0,0x1.0018f542be5b1p+0,0x1.001bbb5429606p+0,0x1.001e816d452a6p+0,0x1.0021478e11ce6p+0,0x1.00240db68f61cp+0,0x1.0026d3e6bdf9bp+0,0x1.00299a1e9dabbp+0};
inline constexpr std::array<double,32> expMiddle{0x1.0000000000000p+0,0x1.002c605e2e8cfp+0,0x1.0058c86da1c0ap+0,0x1.0085382faef83p+0,0x1.00b1afa5abcbfp+0,0x1.00de2ed0ee0f5p+0,0x1.010ab5b2cbd11p+0,0x1.0137444c9b5b5p+0,0x1.0163da9fb3335p+0,0x1.019078ad6a19fp+0,0x1.01bd1e77170b4p+0,0x1.01e9cbfe113efp+0,0x1.02168143b0281p+0,0x1.02433e494b755p+0,0x1.027003103b10ep+0,0x1.029ccf99d720ap+0,0x1.02c9a3e778061p+0,0x1.02f67ffa765e6p+0,0x1.032363d42b027p+0,0x1.03504f75ef071p+0,0x1.037d42e11bbccp+0,0x1.03aa3e170aafep+0,0x1.03d7411915a8ap+0,0x1.04044be896ab6p+0,0x1.04315e86e7f85p+0,0x1.045e78f5640b9p+0,0x1.048b9b35659d8p+0,0x1.04b8c54847a28p+0,0x1.04e5f72f654b1p+0,0x1.051330ec1a03fp+0,0x1.0540727fc1762p+0,0x1.056dbbebb786bp+0};
inline constexpr std::array<double,32> expHigh{0x1.0000000000000p+0,0x1.059b0d3158574p+0,0x1.0b5586cf9890fp+0,0x1.11301d0125b51p+0,0x1.172b83c7d517bp+0,0x1.1d4873168b9aap+0,0x1.2387a6e756238p+0,0x1.29e9df51fdee1p+0,0x1.306fe0a31b715p+0,0x1.371a7373aa9cbp+0,0x1.3dea64c123422p+0,0x1.44e086061892dp+0,0x1.4bfdad5362a27p+0,0x1.5342b569d4f82p+0,0x1.5ab07dd485429p+0,0x1.6247eb03a5585p+0,0x1.6a09e667f3bcdp+0,0x1.71f75e8ec5f74p+0,0x1.7a11473eb0187p+0,0x1.82589994cce13p+0,0x1.8ace5422aa0dbp+0,0x1.93737b0cdc5e5p+0,0x1.9c49182a3f090p+0,0x1.a5503b23e255dp+0,0x1.ae89f995ad3adp+0,0x1.b7f76f2fb5e47p+0,0x1.c199bdd85529cp+0,0x1.cb720dcef9069p+0,0x1.d5818dcfba487p+0,0x1.dfc97337b9b5fp+0,0x1.ea4afa2a490dap+0,0x1.f50765b6e4540p+0};
inline float recoveredLegacyExpFinite(float x){
 if(!std::isfinite(x)||std::fabs(x)>87.25f)throw std::runtime_error("legacy exp range not implemented");
 if(std::fabs(x)<=0x1p-28f)return 1.0f;
 float multiplier;uint32_t multiplierBits=0x46B8AA3B;std::memcpy(&multiplier,&multiplierBits,4);
 const float scaled=x*multiplier;
 const int32_t n=int32_t(std::nearbyint(scaled));
 const unsigned low=uint32_t(n)&15u,middle=(uint32_t(n)>>4)&31u,high=(uint32_t(n)>>9)&31u;
 const double reduced=double(x)*0x1.71547652b82fep+14-double(n);
 const double slope=0x1.62e42fefa39efp-15*expLow[low];
 double scale=expMiddle[middle]*expHigh[high];
 uint64_t bits;std::memcpy(&bits,&scale,8);
 bits+=uint64_t(uint32_t(n>>14))<<52;std::memcpy(&scale,&bits,8);
 const double adjustment=slope*reduced;
 const double base=expLow[low]+adjustment;
 return float(base*scale);
}
