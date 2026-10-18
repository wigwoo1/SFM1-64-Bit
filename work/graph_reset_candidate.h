#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
inline std::int32_t graphResetSigned(std::uint32_t n){std::int32_t v;std::memcpy(&v,&n,4);return v;}
template<class Adapter>void graphResetCandidate(Adapter& a){
 const auto state=a.state168();
 if(state==1){
  auto x=a.x19c(),y=a.y1a0(),xx=a.x1ac(),yy=a.y1b0();
  auto left=std::uint32_t(std::min(x,xx))-2,top=std::uint32_t(std::min(y,yy))-2;
  std::array<std::int32_t,4> rect{graphResetSigned(left),graphResetSigned(top),graphResetSigned(std::uint32_t(std::max(x,xx))+2-left),graphResetSigned(std::uint32_t(std::max(y,yy))-top+2)};
  a.setRect178(rect);a.finishRect100d3250(a.mode174());
 }else if(state==4 || state==5)a.finishState100c26b0();
 a.setState168(0);a.setMode174(0);a.setState16c(0);a.setByte170(1);a.finalImport();
}
