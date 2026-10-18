#pragma once
#include <cstdint>
//100D4920 logical record+FA/+FC fields. Supplied argument points to a
//32-bit value; its public meaning is unresolved, so do not treat it as a pointer.
struct ModelRenderRecordAssignmentCandidate {uint16_t flags=0;uint32_t value=0;};
template<class Record>
inline void modelRenderRecordAssignmentCandidate(uint32_t argument,const uint32_t* supplied,Record record){
 uint16_t index=uint16_t(argument);if(index==0xffff)return;
 auto& destination=record(index);
 if(supplied){uint32_t value=*supplied;destination.flags=uint16_t(destination.flags|1u);destination.value=value;}
 else{destination.flags=uint16_t(destination.flags&0xfffeu);destination.value=0;}
}
