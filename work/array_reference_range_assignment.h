#pragma once
#include <cstdint>
#include <vector>
// Native 1036D480, fixed-size reference array range. Undo/owner notifications
// and dirty dispatch are adapters. Rejected classes retain the old slot value.
template<class Permission,class BeforeRemove,class Remove,class Resolve,class Matches,class AfterWrite,class Add,class Finish>
bool assignRecoveredReferenceArrayRangeCandidate(std::vector<uint32_t>&storage,int start,const std::vector<uint32_t>&incoming,uint32_t requiredClass,Permission permission,BeforeRemove beforeRemove,Remove remove,Resolve resolve,Matches matches,AfterWrite afterWrite,Add add,Finish finish){
 if(start<0||incoming.empty()||size_t(start)>storage.size()||incoming.size()>storage.size()-size_t(start))return false;
 bool different=false;for(size_t i=0;i<incoming.size();++i)if(storage[size_t(start)+i]!=incoming[i]){different=true;break;}if(!different||!permission())return false;
 auto last=size_t(start)+incoming.size()-1;beforeRemove(size_t(start),last);
 for(size_t i=size_t(start);i<=last;++i)remove(storage[i]);
 for(size_t i=0;i<incoming.size();++i){auto target=resolve(incoming[i]);if(!target||matches(target,requiredClass))storage[size_t(start)+i]=incoming[i];}
 afterWrite(size_t(start),last);for(size_t i=size_t(start);i<=last;++i)add(storage[i]);finish();return true;
}
