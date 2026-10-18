#pragma once
#include <cstdint>
//Role9 branch1007f992. Packed byte order RGBA in original x86 storage.
// Owning-group callback must follow105cc060 controls/same-owner incoming lookup.
template<class Group,class GroupColor,class Control,class OwningGroup,class ControlColor>
std::uint32_t controlModelForegroundCandidate(std::int32_t kind,bool inheritedFlag,
 Group group,GroupColor groupColor,Control control,OwningGroup owningGroup,
 ControlColor controlColor){
 std::uint32_t color=0xffffffffu;
 if(kind==1)color=0xff808080u;
 else{
  auto* g=group();
  if(g)color=groupColor(g);
  else{
   auto* c=control();
   if(c){auto* owner=owningGroup(c);
    if(owner)color=controlColor(owner);
    else color=(color&0x00ffffffu)|0x80000000u;
   }
  }
 }
 if(inheritedFlag)color=(color&0x00ffffffu)|0x40000000u;
 return color;
}
