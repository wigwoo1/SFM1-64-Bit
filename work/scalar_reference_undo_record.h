#pragma once
#include "runtime_undo_retention.h"
#include "mutation_coordinator.h"
// Native 103659C0 payload; target retention ledger must outlive this record.
struct RuntimeScalarReferenceUndoRecord{
 uint32_t owner;std::string name;RuntimeUndoRetainedTarget before,after;
 RuntimeScalarReferenceUndoRecord(RuntimeUndoRetention&l,uint32_t h,std::string n,uint32_t oldTarget,uint32_t newTarget):owner(h),name(std::move(n)),before(l),after(l){before.assign(oldTarget);after.assign(newTarget);}
 RuntimeScalarReferenceUndoRecord(RuntimeScalarReferenceUndoRecord&&)=default;
};
inline RuntimeScalarReferenceUndoRecord captureScalarReferenceRecord(SessionRuntime&r,RuntimeUndoRetention&l,uint32_t owner,const std::string&name,uint32_t next){
 if(&l.runtime!=&r)throw std::runtime_error("scalar undo retention runtime mismatch");
 auto a=r.attributes.resolve(r.graph,r.elements,r.attributes.find(owner,name));if(!a||a->type!=1||a->values.size()!=1)throw std::runtime_error("invalid scalar undo capture");
 return {l,owner,name,r.elements.fromReference(a->values[0]),next};
}
// Invoke class-specific normal writer, preserving notifications and schema gates.
template<class Write>
bool replayScalarReferenceRecord(SessionRuntime&r,MutationCoordinator&m,const RuntimeScalarReferenceUndoRecord&record,bool reverse,Write write){
 auto h=r.attributes.find(record.owner,record.name);auto a=r.attributes.resolve(r.graph,r.elements,h);if(!a||a->type!=1||a->values.size()!=1)return false;
 auto it=m.owners.find(record.owner);if(it==m.owners.end())return false;auto flags=it->second.attributeFlags.find(r.attributes.identity(r.graph,r.elements,h));if(flags==it->second.attributeFlags.end()||(flags->second&0x20))return false;
 write(record.owner,record.name,reverse?record.before.target:record.after.target);return true;
}
