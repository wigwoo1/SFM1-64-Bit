#pragma once
#include "global_flex_binding_lifecycle.h"
struct RecoveredFlexSourceValueCandidate{uint16_t flags;uint32_t bits;};
enum class RecoveredFlexExecuteDispatchCandidate{MissingDestination,UnsetArrayIndex,Array,Scalar};
// 105C24D0 resolves destination first. All types15..28 use indexed dispatch.
// Type conversion and mutation permission belong to destination setters.
template<class Resolve,class ReadSource,class ArrayWrite,class ScalarWrite>
RecoveredFlexExecuteDispatchCandidate recoveredFlexExecuteCandidate(const RecoveredFlexBindingCache&cache,Resolve resolve,ReadSource readSource,ArrayWrite arrayWrite,ScalarWrite scalarWrite){
 auto*destination=resolve(cache.destination);if(!destination)return RecoveredFlexExecuteDispatchCandidate::MissingDestination;
 auto source=readSource();uint32_t type=destination->flags&31;
 if(type>=15&&type<=28){if(cache.index==0xffffffffu)return RecoveredFlexExecuteDispatchCandidate::UnsetArrayIndex;arrayWrite(destination,cache.index,source.flags&31,source.bits);return RecoveredFlexExecuteDispatchCandidate::Array;}
 scalarWrite(destination,source.flags&31,source.bits);return RecoveredFlexExecuteDispatchCandidate::Scalar;
}
