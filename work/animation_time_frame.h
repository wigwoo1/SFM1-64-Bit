#pragma once
#include "time_selection.h"
// Graph declarations and animation_keys.h must be included first.
struct AnimationTimeFrame {
 int32_t start=0,duration=0,offset=0;float scale=1;
 explicit AnimationTimeFrame(const Graph&g,const Element&e){
  if(g.strings.at(e.type)!="DmeTimeFrame")throw std::runtime_error("expected DmeTimeFrame");
  unsigned found=0;
  for(auto&a:e.attributes){auto&n=g.strings.at(a.name);unsigned bit=n=="start"?1:n=="duration"?2:n=="offset"?4:n=="scale"?8:0;
   if(!bit)continue;if(found&bit)throw std::runtime_error("duplicate time-frame field");found|=bit;
   if(a.values.size()!=1||a.type!=(bit==8?3:7))throw std::runtime_error("invalid time-frame field");
   if(bit==1)start=wireInt(a.values[0]);else if(bit==2)duration=wireInt(a.values[0]);else if(bit==4)offset=wireInt(a.values[0]);else scale=wireFloat(a.values[0]);
  }
  if(found!=15)throw std::runtime_error("missing time-frame field");
 }
 int32_t childTime(int32_t parentTick,bool clamp)const{return timeToChild(parentTick,start,duration,offset,scale,clamp);}
 const Value* exactKey(const KeyTrack&track,int32_t parentTick,bool clamp)const{return track.exact(childTime(parentTick,clamp));}
};
