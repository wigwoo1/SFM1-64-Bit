#pragma once
#include <cstdint>
#include <cstring>
struct StudioScalarConstantSubmissionCandidate{
 unsigned ownerSlot,deviceSlot;uint32_t opcode,wordsPerRegister;
};
inline constexpr StudioScalarConstantSubmissionCandidate studioScalarConstantSubmissionsCandidate[]={
 {41,98,11,1},{42,96,12,4},{43,113,8,1},{44,111,9,4}
};
// Backend supplies live primary/secondary buffers and queue storage. Valid ranges
// and allocation ownership are caller responsibilities as in the recovered code.
template<class Backend>
bool studioScalarConstantSubmitCandidate(const StudioScalarConstantSubmissionCandidate& kind,
 uint32_t start,const uint32_t* source,uint32_t count,uint32_t force,Backend& backend){
 const uint32_t bytes=count*kind.wordsPerRegister*4;
 const uint32_t offset=start*kind.wordsPerRegister*4;
 if((force&255)==0){
  const auto* cache=reinterpret_cast<const unsigned char*>(backend.secondary(kind.ownerSlot))+offset;
  if(bytes==0||std::memcmp(cache,source,bytes)==0)return false;
 }
 if(backend.queueEnabled()){
  auto& queue=backend.queueState();const uint32_t words=count*kind.wordsPerRegister+3;
  if(queue.remaining<words)backend.refill(queue);
  queue.remaining-=words;queue.cursor[0]=kind.opcode;queue.cursor[1]=start;queue.cursor[2]=count;
  if(bytes)std::memmove(queue.cursor+3,source,bytes);
  queue.cursor+=static_cast<uint32_t>(bytes+12)/4;
 }else backend.direct(kind.deviceSlot,start,source,count);
 if(bytes){
  std::memmove(reinterpret_cast<unsigned char*>(backend.primary(kind.ownerSlot))+offset,source,bytes);
  std::memmove(reinterpret_cast<unsigned char*>(backend.secondary(kind.ownerSlot))+offset,source,bytes);
 }
 return true;
}
