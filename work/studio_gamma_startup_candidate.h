#pragma once
#include <array>
#include <cstdint>
#include "studio_gamma_constants.h"
struct StudioGammaTablesCandidate {
 std::array<std::uint8_t,256> textureGamma{};
 std::array<std::int32_t,1024> linearToScreenGamma{},linearToTexture{};
 std::array<float,256> textureToLinear{},fixedPower0454{},fixedPower22{};
 std::array<float,4096> linearToVertex{};
 std::array<std::uint8_t,4096> linearToLightmap{};
};
inline std::int32_t studioGammaClampByte(std::int32_t x) {return x<0?0:x>255?255:x;}
// Operations explicitly distinguish SSE truncation from x87 rounding conversion.
// Provider arithmetic, exceptional inputs and floating environment need native comparison.
template<class Ops>
void studioGammaStartupCandidate(StudioGammaTablesCandidate& t,float gamma,float textureGamma,float brightness,int overbright,Ops& ops) {
 const auto& c=studioGammaConstants;
 float clampedGamma=gamma;
 if(gamma>c.gammaLimit) clampedGamma=c.gammaLimit;
 const float inverseGamma=static_cast<float>(c.oneDouble/static_cast<double>(clampedGamma));
 const float textureExponent=inverseGamma*textureGamma;
 float threshold;
 if(brightness<=0.0f) threshold=c.thresholdLow;
 else if(brightness>c.oneFloat) threshold=c.thresholdHigh;
 else {const float square=brightness*brightness; threshold=static_cast<float>(c.thresholdBase-static_cast<double>(square)*c.brightnessSquareScale);}
 for(int i=0;i<256;++i) {
  float x=static_cast<float>(i)*c.inverse255;
  x=ops.powerFloat(x,textureExponent)*c.byteScale;
  t.textureGamma[i]=static_cast<std::uint8_t>(studioGammaClampByte(ops.truncateFloat(x)));
 }
 for(int i=0;i<1024;++i) {
  float x=static_cast<float>(static_cast<double>(i)*c.inverse1023);
  if(brightness>c.oneFloat) x*=brightness;
  if(threshold>=x) {x/=threshold;x*=c.thresholdLow;}
  else {x-=threshold;float divisor=c.oneFloat-threshold;x/=divisor;x=static_cast<float>(static_cast<double>(x)*c.upperSlope+c.thresholdBase);}
  x=ops.powerFloat(x,inverseGamma)*c.byteScale;
  t.linearToScreenGamma[i]=studioGammaClampByte(ops.truncateFloat(x));
 }
 for(int i=0;i<256;++i) {
  float x=static_cast<float>(i)*c.inverse255;
  t.textureToLinear[i]=ops.powerFloat(x,textureGamma);
  t.fixedPower0454[i]=static_cast<float>(ops.powerDouble(static_cast<double>(x),c.fixedExponent0454));
  t.fixedPower22[i]=static_cast<float>(ops.powerDouble(static_cast<double>(x),c.fixedExponent22));
 }
 const double inverseTextureGamma=c.oneDouble/static_cast<double>(textureGamma);
 for(int i=0;i<1024;++i) {
  double x=static_cast<double>(i)*c.inverse1023;
  x=ops.powerDouble(x,inverseTextureGamma)*c.inverseScale;
  t.linearToTexture[i]=ops.truncateDouble(x);
 }
 float scale=c.oneFloat;
 if(overbright==2) scale=c.overbright2;
 else if(overbright==4) scale=c.overbright4;
 const double finalExponent=c.oneDouble/static_cast<double>(gamma);
 for(int i=0;i<4096;++i) {
  double x=static_cast<double>(i)*c.inverse1024;
  float p=static_cast<float>(ops.powerDouble(x,finalExponent));
  float vertex=p*scale;
  t.linearToVertex[i]=vertex>c.oneFloat?c.oneFloat:vertex;
  float lightmap=p*c.byteScale;
  lightmap*=scale;
  t.linearToLightmap[i]=static_cast<std::uint8_t>(studioGammaClampByte(ops.roundFloat(lightmap)));
 }
}

