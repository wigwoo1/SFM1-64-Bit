#pragma once
#include "session_runtime.h"
#include "mutation_coordinator.h"
// Recovered append ordering at 0x1036D810. Callbacks must not throw or
// mutate graph/handle lifetimes. Schema acceptance is validated by the caller;
// undo capture and native propagation/global dirty flags remain separate.
template<class Notify,class Propagate>
bool appendRuntimeReference(SessionRuntime&r,MutationCoordinator&m,uint32_t owner,const std::string&name,uint32_t target,bool preparationSuppressed,Notify notify,Propagate propagate){
 auto attributeHandle=r.attributes.find(owner,name);
 auto attribute=r.attributes.resolve(r.graph,r.elements,attributeHandle);
 if(!attribute||attribute->type!=15)return false;
 if(target!=0xffffffff&&!r.elements.resolve(r.graph,target))return false;
 auto identity=r.attributes.identity(r.graph,r.elements,attributeHandle);
 auto index=attribute->values.size();attribute->values.reserve(index+1);
 if(target!=0xffffffff){auto&list=r.references.incoming[target];list.reserve(list.size()+1);}
 if(!m.begin(owner,identity,preparationSuppressed))return false;
 Value value;value.reference=target==0xffffffff?0xffffffff:reinterpret_cast<const SessionElementRecord*>(r.elements.table.resolve(target))->graphIndex;
 attribute->values.push_back(std::move(value));
 auto enabled=[&](){auto&state=m.owners.at(owner);return (state.attributeFlags.at(identity)&0x100)&&(state.state&4);};
 if(enabled())notify(1,attributeHandle,index,index);
 if(target!=0xffffffff)r.references.link(target,attributeHandle);
 if(enabled())notify(2,attributeHandle,index,index);
 propagate(owner,true);return true;
}
