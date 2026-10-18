#pragma once
#include <cstdint>
#include <vector>
struct LogLayerBounds {bool leftInfinite=false,rightInfinite=false;std::vector<int32_t> times;};
// Recovered float-log selector 0x10516CB0. Runtime flag bindings pending.
inline int32_t selectLogLayer(const std::vector<LogLayerBounds>&layers,int32_t tick){
 for(size_t n=layers.size();n>0;--n){auto&l=layers[n-1];int32_t start,end;
  if(l.leftInfinite)start=-2147483647;else{if(l.times.empty()||l.times.front()==INT32_MIN)continue;start=l.times.front();}
  if(l.rightInfinite)end=INT32_MAX;else{if(l.times.empty()||l.times.back()==INT32_MIN)continue;end=l.times.back();}
  if(tick>=start&&tick<=end)return static_cast<int32_t>(n-1);
 }return layers.empty()?-1:0;
}
