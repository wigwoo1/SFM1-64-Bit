#pragma once
#include <cstdint>
// 0x1035F0D0 jump table: 0=all, 1=exclude 0x1000,
// 2=require 0x800 and exclude 0x1000, 3=no traversal.
inline bool referenceMarkTraverses(uint16_t flags,uint32_t mode){
 if(mode>2)return false;
 if(mode==2&&!(flags&0x800))return false;
 if(mode>=1&&(flags&0x1000))return false;
 const auto type=flags&0x1f;return type==1||type==15;
}
