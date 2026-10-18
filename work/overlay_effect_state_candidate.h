#pragma once
#include <array>
#include <string>
#include <cstdint>
// Include dmx_graph.cpp declarations first. Evidence: overlay-effect-*-recovery.json.
// Construction defaults plus serialized overrides; no drawing or UI implementation.
struct OverlayEffectStateCandidate {
 std::string material;
 uint32_t color=0xffffffff;
 int32_t left=0,top=0,width=1,height=1;
 bool fullscreen=true,useSubRect=false;
 float movementAngle=0,movementSpeed=0;
 int32_t subRectLeft=0,subRectTop=0,subRectWidth=0,subRectHeight=0;
 OverlayEffectStateCandidate()=default;
 OverlayEffectStateCandidate(const Graph&g,const Element&e){
  if(g.strings.at(e.type)!="DmeMaterialOverlayFXClip")throw std::runtime_error("expected material overlay effect");
  constexpr std::array<const char*,14> names={"material","overlaycolor","left","top","width","height","fullscreen","useSubRect","movementAngle","movementSpeed","subRectLeft","subRectTop","subRectWidth","subRectHeight"};
  constexpr std::array<uint8_t,14> types={5,8,2,2,2,2,4,4,3,3,2,2,2,2};
  uint32_t seen=0;
  for(const auto&a:e.attributes){
   size_t i=0;while(i<names.size()&&g.strings.at(a.name)!=names[i])++i;
   if(i==names.size())continue;
   const uint32_t bit=uint32_t(1)<<i;
   if(seen&bit)throw std::runtime_error("duplicate overlay field");seen|=bit;
   if(a.type!=types[i]||a.values.size()!=1)throw std::runtime_error("invalid overlay field");
   const auto&v=a.values[0];
   switch(i){
    case 0:material=v.text;break;
    case 1:color=wireU32(v);break;
    case 2:left=wireInt(v);break;case 3:top=wireInt(v);break;
    case 4:width=wireInt(v);break;case 5:height=wireInt(v);break;
    case 6:case 7:
     if(v.wire.size()!=1)throw std::runtime_error("invalid overlay bool");
     (i==6?fullscreen:useSubRect)=v.wire[0]!=0;break;
    case 8:movementAngle=wireFloat(v);break;case 9:movementSpeed=wireFloat(v);break;
    case 10:subRectLeft=wireInt(v);break;case 11:subRectTop=wireInt(v);break;
    case 12:subRectWidth=wireInt(v);break;case 13:subRectHeight=wireInt(v);break;
   }
  }
 }
};
