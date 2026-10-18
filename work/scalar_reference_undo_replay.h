#pragma once
#include <cstdint>
// Logical name ID interpreted by Find, not a native pointer or binary layout.
struct RecoveredScalarReferenceUndoPayloadCandidate {uint32_t owner=0xffffffffu,name=0xffffffffu,before=0xffffffffu,after=0xffffffffu;};
// Native 1036C190/1036C1F0. Missing-name sentinel maps to empty name in Find.
// Normal scalar writer owns type, equality, class and phase checks.
template<class Resolve,class Find,class Flags,class Write>
bool replayRecoveredScalarReferenceUndoCandidate(const RecoveredScalarReferenceUndoPayloadCandidate&record,bool reverse,Resolve resolve,Find find,Flags flags,Write write){
 auto owner=resolve(record.owner);if(!owner)return false;auto attribute=find(owner,record.name);if(!attribute||(flags(attribute)&0x20))return false;write(attribute,reverse?record.before:record.after);return true;
}
