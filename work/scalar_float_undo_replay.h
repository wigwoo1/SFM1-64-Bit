#pragma once
#include "float_component_write_sequence.h"
#include <utility>
// Logical fields, not the native record ABI at 0x10367B20.
struct RecoveredScalarFloatUndoPayloadCandidate{uint32_t owner=0xffffffffu,name=0xffffffffu;float before=0,after=0;};
inline void captureRecoveredScalarFloatUndoCandidate(RecoveredScalarFloatUndoPayloadCandidate&record,float before,float after){
 for(auto pair:{std::pair<float*,float>{&record.before,before},{&record.after,after}}){uint32_t bits;std::memcpy(&bits,&pair.second,4);bits=recoveredX87FloatStoreBitsCandidate(bits);std::memcpy(pair.first,&bits,4);}
}
// Native undo0x10377E40/redo0x10377E90. Write owns equality and phase checks.
template<class Resolve,class Find,class Flags,class Write>
bool replayRecoveredScalarFloatUndoCandidate(const RecoveredScalarFloatUndoPayloadCandidate&record,bool reverse,Resolve resolve,Find find,Flags flags,Write write){auto owner=resolve(record.owner);if(!owner)return false;auto attribute=find(owner,record.name);if(!attribute||(flags(attribute)&0x20))return false;write(attribute,reverse?record.before:record.after);return true;}
