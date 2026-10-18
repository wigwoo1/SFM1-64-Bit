#pragma once
#include <cstdint>
#include <cstring>
// Logical1000f1b0. Ops owns both float arrays and bounds, and exposes their
// live pointers. submit chooses native opcode10 or direct COM94.
template<class Ops>void studioVertexConstantRunsCandidate(uint32_t start,int32_t count,uint32_t force,Ops& ops){
 if(count<=0)return;
 uint32_t first=start,pending=0;
 auto flush=[&](){
  if(!pending)return;
  ops.submit(first,pending,ops.current()+size_t(first)*4);
  auto source=ops.current()+size_t(first)*4;
  auto destination=ops.cache()+size_t(first)*4;
  std::memcpy(destination,source,size_t(pending)*16);
  pending=0;
 };
 for(int32_t i=0;i<count;++i){
  const uint32_t index=start+static_cast<uint32_t>(i);
  bool changed=uint8_t(force)!=0;
  if(!changed){
   auto current=ops.current()+size_t(index)*4;
   auto cached=ops.cache()+size_t(index)*4;
   for(unsigned word=0;word<4;++word)if(!(current[word]==cached[word])){changed=true;break;}
  }
  if(changed)++pending;
  else{flush();first=index+1;}
 }
 flush();
}
