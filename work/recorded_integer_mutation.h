#pragma once
#include "integer_undo_journal.h"
#include "undo_recording_gate.h"
// Explicit native file ownership; absent ownership never enables recording.
struct RuntimeIntegerUndoCapture{
 RuntimeUndoRetention*retention=nullptr;RuntimeIntegerUndoJournal journal;std::map<uint32_t,uint32_t>fileIds;
 bool enabled(SessionRuntime&r,uint32_t owner)const{auto it=fileIds.find(owner);return recoveredUndoRecordingGate(journal.flags,r.elements.resolve(r.graph,owner)!=nullptr,it==fileIds.end()?0xffffffff:it->second);}
};
template<class Notify,class Suppressed>
bool clearRecordedIntegerArray(SessionRuntime&r,MutationCoordinator&m,RuntimeIntegerUndoCapture&capture,uint32_t owner,const std::string&name,uint32_t&dirty,bool preparationSuppressed,Notify notify,Suppressed suppressed){
 return clearRuntimeIntegerArray(r,m,owner,name,dirty,preparationSuppressed,notify,suppressed,[&](){if(capture.enabled(r,owner))capture.journal.append(captureIntegerClearRecord(r,owner,name));});
}
template<class Notify,class Suppressed>
int32_t appendRecordedIntegerArray(SessionRuntime&r,MutationCoordinator&m,RuntimeIntegerUndoCapture&capture,uint32_t owner,const std::string&name,int32_t value,uint32_t&dirty,bool preparationSuppressed,Notify notify,Suppressed suppressed){
 return appendRuntimeIntegerArray(r,m,owner,name,value,dirty,preparationSuppressed,notify,suppressed,[&](){if(capture.enabled(r,owner)){auto a=r.attributes.resolve(r.graph,r.elements,r.attributes.find(owner,name));capture.journal.append(RuntimeIntegerAppendRecord{owner,name,static_cast<int32_t>(a->values.size()),value});}});
}

