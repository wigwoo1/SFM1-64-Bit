#pragma once
#include "session_runtime.h"
#include "mutation_coordinator.h"
#include "session_reference_propagation.h"
struct RuntimeIntegerNoCapture{void operator()()const{}};
// Native integer clear/append (1036E6E0/1036E470), excluding undo records.
// Notify receives native owner vtable slot byte offset, attribute handle,
// and signed inclusive bounds (clear on an empty array uses last=-1).
template<class Notify,class Suppressed,class Capture=RuntimeIntegerNoCapture>
bool clearRuntimeIntegerArray(SessionRuntime&r,MutationCoordinator&m,uint32_t owner,const std::string&name,uint32_t&dirty,bool preparationSuppressed,Notify notify,Suppressed suppressed,Capture capture={}){
 auto h=r.attributes.find(owner,name);auto identity=r.attributes.identity(r.graph,r.elements,h);
 auto resolve=[&](){return r.attributes.resolve(r.graph,r.elements,h);};auto a=resolve();if(!a||a->type!=16)return false;
 if(a->values.size()>0x7fffffff)throw std::runtime_error("integer array exceeds native signed range");
 if(!m.begin(owner,identity,preparationSuppressed))return false;
 capture();
 auto enabled=[&](){auto&o=m.owners.at(owner);return (o.attributeFlags.at(identity)&0x100)&&(o.state&4);};
 auto last=static_cast<int32_t>(a->values.size())-1;
 if(enabled())notify(8,h,0,last);
 a=resolve();if(!a||a->type!=16)throw std::runtime_error("integer array invalidated during clear callback");a->values.clear();
 if(enabled())notify(0,h,0,0);
 propagateSessionReferences(r,m,owner,2,suppressed);dirty|=0x4000;return true;
}
template<class Notify,class Suppressed,class Capture=RuntimeIntegerNoCapture>
int32_t appendRuntimeIntegerArray(SessionRuntime&r,MutationCoordinator&m,uint32_t owner,const std::string&name,int32_t value,uint32_t&dirty,bool preparationSuppressed,Notify notify,Suppressed suppressed,Capture capture={}){
 auto h=r.attributes.find(owner,name);auto identity=r.attributes.identity(r.graph,r.elements,h);auto a=r.attributes.resolve(r.graph,r.elements,h);if(!a||a->type!=16)return -1;
 if(a->values.size()>=0x7fffffff)throw std::runtime_error("integer array exceeds native signed range");
 if(!m.begin(owner,identity,preparationSuppressed))return -1;
 capture();
 const auto index=static_cast<int32_t>(a->values.size());Value v;v.wire.resize(4);setWireU32(v,static_cast<uint32_t>(value));a->values.push_back(std::move(v));
 auto enabled=[&](){auto&o=m.owners.at(owner);return (o.attributeFlags.at(identity)&0x100)&&(o.state&4);};
 if(enabled())notify(4,h,index,index);
 if(enabled())notify(0,h,0,0);
 propagateSessionReferences(r,m,owner,2,suppressed);dirty|=0x4000;return index;
}

