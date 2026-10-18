#pragma once
#include "mutate_runtime_integer_array.h"
// Recovered 10384FB0; optional native undo allocation remains separate.
template<class Notify,class Suppressed>
bool removeRuntimeIntegerArray(SessionRuntime&r,MutationCoordinator&m,uint32_t owner,const std::string&name,int32_t index,uint32_t&dirty,bool preparationSuppressed,Notify notify,Suppressed suppressed){
 auto h=r.attributes.find(owner,name);auto identity=r.attributes.identity(r.graph,r.elements,h);auto resolve=[&](){return r.attributes.resolve(r.graph,r.elements,h);};auto a=resolve();
 if(!a||a->type!=16||index<0||static_cast<size_t>(index)>=a->values.size())return false;
 if(!m.begin(owner,identity,preparationSuppressed))return false;
 auto enabled=[&](){auto&o=m.owners.at(owner);return (o.attributeFlags.at(identity)&0x100)&&(o.state&4);};
 if(enabled())notify(8,h,index,index);
 a=resolve();if(!a||a->type!=16||static_cast<size_t>(index)>=a->values.size())throw std::runtime_error("integer removal invalidated by callback");
 a->values.erase(a->values.begin()+index);
 if(enabled())notify(0,h,0,0);
 propagateSessionReferences(r,m,owner,2,suppressed);dirty|=0x4000;return true;
}
