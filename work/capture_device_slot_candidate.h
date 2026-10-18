#pragma once
#include <cstdint>
//1025f610: encoded slot bit15 selects dynamic table, low15 is table index.
template<class Count,class Encoding,class Fixed,class Dynamic>
auto captureDeviceSlotCandidate(std::int32_t index,Count count,Encoding encoding,
 Fixed fixed,Dynamic dynamic)->decltype(fixed(0u)){
 if(index<0||index>=count())return nullptr;
 const std::uint32_t slot=encoding(index);
 return (slot&0x8000u)?dynamic(slot&0x7fffu):fixed(slot&0x7fffu);
}
//1025f760: slot accessor uses cached count; refresh after failed comparison.
// Callbacks must preserve table/record storage validity during each lookup.
template<class Element,class Count,class Slot,class Handle,class Resolve,class IsAnimationSet>
std::int32_t findCaptureAnimationSetCandidate(Element* sought,Count count,Slot slot,
 Handle handle,Resolve resolve,IsAnimationSet isAnimationSet){
 if(!sought)return -1;
 auto currentCount=count();
 if(currentCount<=0)return -1;
 std::int32_t index=0;
 do{
  Element* candidate=nullptr;
  if(index>=0&&index<currentCount){
   auto* record=slot(index);
   if(record){auto* element=resolve(handle(record));
    if(element&&isAnimationSet(element))candidate=element;
   }
  }
  if(candidate==sought)return index;
  currentCount=count();
  ++index;
 }while(index<currentCount);
 return -1;
}
