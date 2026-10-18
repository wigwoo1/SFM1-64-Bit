#pragma once
#include <cstdint>
#include <vector>
// Installed 0x105166E0: last key <= tick, retaining a forward-search cache.
struct CachedKeySearch {
 int32_t cached=-1;
 int32_t find(const std::vector<int32_t>&times,int32_t tick){
  auto end=static_cast<int32_t>(times.size());
  if(cached>=0&&cached<end){
   if(tick>=times[cached]){while(cached<end-1&&tick>=times[cached+1])++cached;return cached;}
   end=cached;
  }
  for(int32_t i=end-1;i>=0;--i)if(tick>=times[i]){cached=i;return i;}
  return -1;
 }
};
