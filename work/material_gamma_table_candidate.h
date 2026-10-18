#pragma once
#include <array>
#include <cmath>
#include <immintrin.h>
#include <stdexcept>
#include "studio_gamma_constants.h"
// Native fixedPower22 startup sequence, verified under nearest SSE rounding.
// Toolchain/libm changes require repeating the original-table bit comparison.
inline std::array<float,256> materialGammaTableCandidate(){
 if((_mm_getcsr()&0x6000u)!=0)
  throw std::runtime_error("material gamma table requires nearest SSE rounding");
 std::array<float,256> table;
 for(unsigned i=0;i<table.size();++i){
  float x=float(i)*studioGammaConstants.inverse255;
  table[i]=float(std::pow(double(x),studioGammaConstants.fixedExponent22));
  if(!std::isfinite(table[i])||table[i]<0||table[i]>1)
   throw std::runtime_error("invalid generated material gamma value");
 }
 return table;
}
inline const std::array<float,256>& materialGammaTableSharedCandidate(){
 // Validate each use as well as initialization; never reuse silently under a
 // floating-point environment outside the tested generation policy.
 if((_mm_getcsr()&0x6000u)!=0)
  throw std::runtime_error("shared material gamma table requires nearest SSE rounding");
 static const auto table=materialGammaTableCandidate();
 return table;
}
