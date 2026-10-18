#pragma once
#include <cstdint>
// Shared operator activity slot 0x105DEBA0 (expression primary slot zero).
inline bool forcedOperatorActive(uint8_t state42){return (state42&2)!=0;}
// 0x104CE280 channel override. Cached +0xC4 meaning remains unresolved.
inline bool channelActive(uint8_t state42,int32_t mode,uint32_t cachedC4,bool sourceResolved,uint16_t sourceFlags){
 if(forcedOperatorActive(state42))return true;if(mode==3)return true;if(mode==2&&cachedC4!=0xffffffff)return true;
 return (mode==1||mode==2)&&sourceResolved&&(sourceFlags&0x4000);
}

// Native preparation slot 1, 0x105DEBB0, consumes forced activity.
inline void prepareChannelActivity(uint8_t&state42){state42&=0xfd;}

// state42 is base element owner state +0x3E (primary operator base +4).
// Activity bit is mutation bit 1; production integration must share that state.
// Explicit transient state, not fields inferred from serialized DMX.
struct ChannelActivityState{
 uint8_t state42=0;int32_t mode=0;uint32_t cachedC4=0xffffffff;
 bool active(bool sourceResolved,uint16_t sourceFlags)const{return channelActive(state42,mode,cachedC4,sourceResolved,sourceFlags);}
 void prepare(){prepareChannelActivity(state42);}
};
