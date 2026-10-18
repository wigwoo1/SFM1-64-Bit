#pragma once
#include <cstdint>
//1010B9D0 attachment portion after filter and item/branch dispatch.
// Captured item record, live count/base/table providers; no null guard inferred.
template<class Count,class Base,class AttachmentIndex,class Object,class Skip,class Prepare,class Gate>
inline bool engineProjectionAttachmentLoopCandidate(uint8_t branchFlag,
 Count count,Base base,AttachmentIndex attachmentIndex,Object object,
 Skip skip,Prepare prepare,Gate gate){
 int32_t i=0;uint16_t limit=count();
 if(limit==0)return true;
 do{
  uint32_t slot=uint32_t(base())+uint32_t(i);
  auto current=object(attachmentIndex(slot));
  if(uint8_t(skip(current))==0){prepare(current);gate(current,branchFlag);}
  limit=count();++i;
 }while(i<int32_t(limit));
 return true;
}
