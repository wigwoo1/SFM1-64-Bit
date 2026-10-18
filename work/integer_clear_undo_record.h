#pragma once
#include "mutate_runtime_integer_array.h"
// Recovered clear record payload and operation slots 0/4. Recording and the
// global undo scheduler remain separate. Supports native clear's flag=false.
struct RuntimeIntegerClearRecord{
 uint32_t owner=0xffffffff;std::string name;std::vector<int32_t>before,after;
};
inline RuntimeIntegerClearRecord captureIntegerClearRecord(SessionRuntime&r,uint32_t owner,const std::string&name){
 auto a=r.attributes.resolve(r.graph,r.elements,r.attributes.find(owner,name));if(!a||a->type!=16)throw std::runtime_error("invalid clear record attribute");
 RuntimeIntegerClearRecord record;record.owner=owner;record.name=name;for(const auto&v:a->values)record.before.push_back(wireInt(v));return record;
}
template<class Notify,class Suppressed>
bool replayIntegerClearRecord(SessionRuntime&r,MutationCoordinator&m,const RuntimeIntegerClearRecord&record,bool reverse,uint32_t&dirty,bool preparationSuppressed,Notify notify,Suppressed suppressed){
 // Native lookup is performed anew by stable owner handle plus attribute name.
 auto a=r.attributes.resolve(r.graph,r.elements,r.attributes.find(record.owner,record.name));if(!a||a->type!=16)return false;
 clearRuntimeIntegerArray(r,m,record.owner,record.name,dirty,preparationSuppressed,notify,suppressed);
 const auto&values=reverse?record.before:record.after;
 for(auto value:values)appendRuntimeIntegerArray(r,m,record.owner,record.name,value,dirty,preparationSuppressed,notify,suppressed);
 return true; // Native operation ignores individual mutation-gate failures.
}
