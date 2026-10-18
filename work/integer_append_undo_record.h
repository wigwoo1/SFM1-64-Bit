#pragma once
#include "insert_runtime_integer_array.h"
#include "remove_runtime_integer_array.h"
struct RuntimeIntegerAppendRecord{uint32_t owner=0xffffffff;std::string name;int32_t index=0,value=0;};
template<class Notify,class Suppressed>
bool replayIntegerAppendRecord(SessionRuntime&r,MutationCoordinator&m,const RuntimeIntegerAppendRecord&record,bool reverse,uint32_t&dirty,bool preparationSuppressed,Notify notify,Suppressed suppressed){
 auto a=r.attributes.resolve(r.graph,r.elements,r.attributes.find(record.owner,record.name));if(!a||a->type!=16)return false;
 if(reverse)removeRuntimeIntegerArray(r,m,record.owner,record.name,record.index,dirty,preparationSuppressed,notify,suppressed);
 else insertRuntimeIntegerArray(r,m,record.owner,record.name,record.index,record.value,dirty,preparationSuppressed,notify,suppressed);
 return true; // Native record methods ignore mutation operation results.
}
