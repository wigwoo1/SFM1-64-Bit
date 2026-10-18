#pragma once
#include "mutate_runtime_integer_array.h"
// Recovered 1036E470 for valid positions. Invalid positions safely rejected.
template<class Notify,class Suppressed>
int32_t insertRuntimeIntegerArray(SessionRuntime&r,MutationCoordinator&m,uint32_t owner,const std::string&name,int32_t index,int32_t value,uint32_t&dirty,bool preparationSuppressed,Notify notify,Suppressed suppressed){
 auto h=r.attributes.find(owner,name);auto identity=r.attributes.identity(r.graph,r.elements,h);auto a=r.attributes.resolve(r.graph,r.elements,h);if(!a||a->type!=16||index<0||static_cast<size_t>(index)>a->values.size())return -1;
 if(a->values.size()>=0x7fffffff)throw std::runtime_error("integer array exceeds native signed range");
 if(!m.begin(owner,identity,preparationSuppressed))return -1;
 Value v;v.wire.resize(4);setWireU32(v,static_cast<uint32_t>(value));a->values.insert(a->values.begin()+index,std::move(v));
 auto enabled=[&](){auto&o=m.owners.at(owner);return (o.attributeFlags.at(identity)&0x100)&&(o.state&4);};
 if(enabled())notify(4,h,index,index);
 if(enabled())notify(0,h,0,0);
 propagateSessionReferences(r,m,owner,2,suppressed);dirty|=0x4000;return index;
}

