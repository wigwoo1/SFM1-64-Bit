#pragma once
#include <cstdint>
struct FlexExecuteFixture{uint16_t destinationFlags,sourceFlags;uint32_t index,bits,handle;bool present;};
inline FlexExecuteFixture flexExecuteFixture(unsigned seed){
 const uint32_t indices[]={0xffffffffu,0,7,0x80000000u};
 return {uint16_t((seed%32)|((seed/1024)%2?0x4080:0)),uint16_t(((seed/32)%32)|((seed/2048)%2?0x2100:0)),indices[(seed/4096)%4],0x7f812345u^(seed*7919u),0x12340000u+seed,((seed/16384)%2)!=0};
}
