#pragma once
#include "scalar_reference_storage_sequence.h"
// Native 10366F90 after scalar guard succeeds. Undo and owner notification
// supplied as adapters; finish performs dirty dispatch before framework flags.
template<class Undo,class Remove,class Add,class Notify,class Finish>
void writeRecoveredScalarReferenceCandidate(uint32_t&storage,uint32_t incoming,Undo undo,Remove remove,Add add,Notify notify,Finish finish){
 undo();assignRecoveredScalarReferenceStorageCandidate(storage,incoming,remove,add);notify();finish();
}
