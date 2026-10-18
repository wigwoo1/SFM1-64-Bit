#pragma once
#include <cmath>
#include <stdexcept>
// Explicit x87 FISTP rounding policy. Multiplication still follows the caller's
// SSE floating-point environment, as does the recovered native MULSS.
enum class NativeGammaIndexRounding { nearestEven, down, up, truncate };
inline int nativeGammaIndexCandidate(float value, NativeGammaIndexRounding rounding){
 if(!std::isfinite(value)||value<0||value>=.95f)
  throw std::runtime_error("gamma index input outside lookup branch");
 float scaled=value*255.f;
 int lower=static_cast<int>(scaled); // Positive, bounded; truncation is exact.
 float fraction=scaled-float(lower);
 switch(rounding){
 case NativeGammaIndexRounding::nearestEven:
  return lower+int(fraction>.5f||(fraction==.5f&&(lower&1)));
 case NativeGammaIndexRounding::down:
 case NativeGammaIndexRounding::truncate:return lower;
 case NativeGammaIndexRounding::up:return lower+int(fraction>0);
 }
 throw std::runtime_error("invalid gamma index rounding policy");
}
