#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include "float_component_write_sequence.h"
#include "ik_angle_quaternion.h"
// Logical record, not a native ABI layout. Find interprets logical name IDs.
template<size_t N>struct RecoveredFloatComponentsUndoPayloadCandidate {uint32_t owner=0xffffffffu,name=0xffffffffu;std::array<float,N>before{},after{};};
// Native vector3 undo10378AD0/redo10378B20. Normal setter owns type/phase/equality.
template<size_t N,class Resolve,class Find,class Flags,class Write>
bool replayRecoveredFloatComponentsUndoCandidate(const RecoveredFloatComponentsUndoPayloadCandidate<N>&record,bool reverse,Resolve resolve,Find find,Flags flags,Write write){auto owner=resolve(record.owner);if(!owner)return false;auto attribute=find(owner,record.name);if(!attribute||(flags(attribute)&0x20))return false;write(attribute,reverse?record.before:record.after);return true;}

// Constructor copy: vector 1036A452..1036A476 uses x87 stores; quaternion
// 10368209..1036822D uses movq. This models payload copies, not base metadata.
template<size_t N>void captureRecoveredFloatUndoValuesCandidate(RecoveredFloatComponentsUndoPayloadCandidate<N>&record,const std::array<float,N>&before,const std::array<float,N>&after){
 static_assert(N==3||N==4,"vector or quaternion only");
 std::memcpy(record.before.data(),before.data(),N*4);std::memcpy(record.after.data(),after.data(),N*4);
 if constexpr(N==3)for(auto*values:{&record.before,&record.after})for(auto&value:*values){uint32_t bits;std::memcpy(&bits,&value,4);bits=recoveredX87FloatStoreBitsCandidate(bits);std::memcpy(&value,&bits,4);}
}

// Constructor source selection. Quaternion type12 needs a separate Euler
// conversion adapter; do not silently substitute identity for that path.
template<size_t N>void captureRecoveredFloatUndoAttributeCandidate(RecoveredFloatComponentsUndoPayloadCandidate<N>&record,uint16_t flags,const std::array<float,N>&stored,const std::array<float,N>&incoming){
 const unsigned type=flags&31u;std::array<float,N>before{};
 if(type==(N==3?10u:13u))before=stored;
 else if constexpr(N==4){if(type==12)throw std::runtime_error("Euler undo capture requires conversion");before[3]=1.0f;}
 captureRecoveredFloatUndoValuesCandidate(record,before,incoming);
}

// Explicit Euler source avoids interpreting a three-component attribute as
// quaternion storage. Finite tested angle range; host trig is still a candidate.
inline void captureRecoveredEulerQuaternionUndoCandidate(RecoveredFloatComponentsUndoPayloadCandidate<4>&record,const std::array<float,3>&angles,const std::array<float,4>&incoming){
 captureRecoveredFloatUndoValuesCandidate(record,recoveredIkAngleQuaternionCandidate(angles),incoming);
}
