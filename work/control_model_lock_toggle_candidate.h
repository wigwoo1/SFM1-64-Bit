#pragma once
#include <cstdint>
#include <optional>
// Recovered column1 branch1007f24a; object/scope adapters remain unresolved.
// Names independently established from104c3e30 attribute lookups.
struct ControlModelLockValuesCandidate { bool overridePos; bool overrideRot; };
inline std::optional<int32_t> controlModelLockCheckStateCandidate(int32_t kind,
 bool resolvedTransformDag,const ControlModelLockValuesCandidate& values){
 if(!resolvedTransformDag)return std::nullopt;
 switch(kind){
 case 4: return values.overridePos==values.overrideRot?(values.overridePos?2:0):1;
 case 5: return values.overridePos?2:0;
 case 9: return values.overrideRot?2:0;
 default: return std::nullopt;
 }
}
// Selector104c3ed0/104c3f60: fallback implementation provided by model adapter.
// Preserve lazy fallback invocation; do not guess the relation resolver behavior.
template<class Parent,class ResolveFallback>
Parent* selectControlLockParentCandidate(bool receiverByte64,bool overrideEnabled,
 Parent* resolvedOverrideParent,const ResolveFallback& fallback){
 if(!receiverByte64&&overrideEnabled&&resolvedOverrideParent)return resolvedOverrideParent;
 return fallback();
}
inline bool toggleControlModelLockValuesCandidate(int32_t kind,
 ControlModelLockValuesCandidate& values){
 switch(kind){
 case 4: values.overridePos=!values.overridePos; values.overrideRot=!values.overrideRot; return true;
 case 5: values.overridePos=!values.overridePos; return true;
 case 9: values.overrideRot=!values.overrideRot; return true;
 default: return false;
 }
}
