#pragma once
#include "session_reference_propagation.h"
// Verified types 1,3,4,5,15,21 restore hooks 0x10363DC0/0x103652C0/0x10365340.
// This replays notifications/propagation; external resolution is separate.
template<class Notify,class Suppressed>
bool restoreKnownAttribute(SessionRuntime&r,MutationCoordinator&m,uint32_t owner,const std::string&name,uint32_t&dirty,Notify notify,Suppressed suppressed){
 auto handle=r.attributes.find(owner,name);auto attribute=r.attributes.resolve(r.graph,r.elements,handle);auto identity=r.attributes.identity(r.graph,r.elements,handle);
 if(!attribute||(attribute->type!=1&&attribute->type!=3&&attribute->type!=4&&attribute->type!=5&&attribute->type!=15&&attribute->type!=21))return false;
 auto metadata=m.owners.find(owner);if(metadata==m.owners.end()||!metadata->second.attributeFlags.count(identity))return false;
 auto enabled=[&](){auto&state=m.owners.at(owner);return (state.attributeFlags.at(identity)&0x100)&&(state.state&4);};
 if((attribute->type==15||attribute->type==21)&&!attribute->values.empty()&&enabled())notify(1,handle,0,attribute->values.size()-1);
 if(enabled())notify(2,handle,0,0);
 propagateSessionReferences(r,m,owner,3,suppressed);dirty|=0x1000;return true;
}

