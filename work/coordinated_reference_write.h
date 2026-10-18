#pragma once
#include "session_runtime.h"
#include "mutation_coordinator.h"
// Metadata is explicitly registered in the coordinator, not guessed from DMX.
// Caller resolves the recovered schema symbol and implements native slot-0x10
// type ancestry. Null targets bypass the type check as in 0x10366000.
struct RuntimeReferenceNoCapture{void operator()()const{}};
template<class Accept,class Capture=RuntimeReferenceNoCapture> bool coordinatedScalarReferenceWriteChecked(SessionRuntime&runtime,MutationCoordinator&mutation,uint32_t owner,const std::string&name,uint32_t target,bool suppressed,Accept accept,Capture capture={}){
 auto handle=runtime.attributes.find(owner,name);auto identity=runtime.attributes.identity(runtime.graph,runtime.elements,handle);
 auto attribute=runtime.attributes.resolve(runtime.graph,runtime.elements,handle);
 if(!attribute||attribute->type!=1||attribute->values.size()!=1)return false;
 if(target!=0xffffffff&&!runtime.elements.resolve(runtime.graph,target))return false;
 auto&value=attribute->values[0];if(value.reference==0xfffffffe)throw std::runtime_error("external checked reference requires resolver");
 auto previous=runtime.elements.fromReference(value);if(previous==target)return false;
 if(target!=0xffffffff&&!accept(target))return false;
 if(previous!=0xffffffff&&(!runtime.elements.resolve(runtime.graph,previous)||!runtime.references.occurrences(previous,handle)))throw std::runtime_error("checked reference ledger inconsistent");
 if(!mutation.begin(owner,identity,suppressed))return false;
 capture();
 return runtime.setReference(owner,name,0,target);
}
inline bool coordinatedReferenceWrite(SessionRuntime&runtime,MutationCoordinator&mutation,uint32_t owner,const std::string&name,size_t index,uint32_t target,bool suppressed){
 auto handle=runtime.attributes.find(owner,name);auto identity=runtime.attributes.identity(runtime.graph,runtime.elements,handle);
 auto attribute=runtime.attributes.resolve(runtime.graph,runtime.elements,handle);
 if(!attribute||(attribute->type!=1&&attribute->type!=15)||index>=attribute->values.size()||(attribute->type==1&&index!=0))return false;
 if(target!=0xffffffff&&!runtime.elements.resolve(runtime.graph,target))return false;
 auto&value=attribute->values[index];if(value.reference==0xfffffffe)throw std::runtime_error("external coordinated reference requires resolver");
 auto previous=runtime.elements.fromReference(value);if(previous==target)return false;
 if(previous!=0xffffffff&&(!runtime.elements.resolve(runtime.graph,previous)||!runtime.references.occurrences(previous,handle)))throw std::runtime_error("coordinated reference ledger inconsistent");
 if(!mutation.begin(owner,identity,suppressed))return false;
 return runtime.setReference(owner,name,index,target);
}
template<class Notify> bool coordinatedReferenceRange(SessionRuntime&runtime,MutationCoordinator&mutation,uint32_t owner,const std::string&name,size_t start,const std::vector<uint32_t>&targets,bool suppressed,Notify notify){
 auto handle=runtime.attributes.find(owner,name);auto identity=runtime.attributes.identity(runtime.graph,runtime.elements,handle);auto attribute=runtime.attributes.resolve(runtime.graph,runtime.elements,handle);
 if(!attribute||attribute->type!=15||targets.empty()||start>attribute->values.size()||targets.size()>attribute->values.size()-start)return false;
 bool changed=false;std::map<uint32_t,size_t> required;
 for(size_t i=0;i<targets.size();++i){if(targets[i]!=0xffffffff&&!runtime.elements.resolve(runtime.graph,targets[i]))return false;
  auto&value=attribute->values[start+i];if(value.reference==0xfffffffe)throw std::runtime_error("external coordinated range requires resolver");auto old=runtime.elements.fromReference(value);changed|=old!=targets[i];if(old!=0xffffffff)++required[old];
 }
 if(!changed)return false;
 for(auto entry:required)if(!runtime.elements.resolve(runtime.graph,entry.first)||runtime.references.occurrences(entry.first,handle)<entry.second)throw std::runtime_error("coordinated range ledger inconsistent");
 if(!mutation.begin(owner,identity,suppressed))return false;
 auto&state=mutation.owners.at(owner);auto enabled=[&](){return (state.attributeFlags.at(identity)&0x100)&&(state.state&4);};
 return runtime.setReferenceRangeControlled(owner,name,start,targets,enabled,notify);
}

