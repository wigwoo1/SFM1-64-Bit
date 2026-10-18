#pragma once
#include "session_attribute_handles.h"
// Tracks one incoming occurrence per reference value, including duplicates.
// link/unlink order matches8192 native insertion/removal snapshots.
// Constructor registration/restore traversal order remains unverified.
struct SessionReferenceLedger{
 std::map<uint32_t,std::vector<uint32_t>> incoming;
 size_t externalUnresolved=0;
 SessionReferenceLedger(const Graph&g,const SessionRuntimeHandles&e,const SessionAttributeHandles&a){
  for(uint32_t i=0;i<g.elements.size();++i)for(const auto&attribute:g.elements[i].attributes)if(attribute.type==1||attribute.type==15){
   auto handle=a.find(e.graphHandles.at(i),g.strings.at(attribute.name));
   for(const auto&value:attribute.values){if(value.reference==0xfffffffe){++externalUnresolved;continue;}if(value.reference==0xffffffff)continue;link(e.graphHandles.at(value.reference),handle);}
  }
 }
 void link(uint32_t target,uint32_t attribute){auto&list=incoming[target];list.insert(list.begin(),attribute);}
 bool unlink(uint32_t target,uint32_t attribute,bool keepEmpty=false){
  auto entry=incoming.find(target);if(entry==incoming.end())return false;auto&list=entry->second;
  for(auto it=list.begin();it!=list.end();++it)if(*it==attribute){list.erase(it);if(list.empty()&&!keepEmpty)incoming.erase(entry);return true;}return false;
 }
 size_t occurrences(uint32_t target,uint32_t attribute)const{
  auto it=incoming.find(target);if(it==incoming.end())return 0;size_t count=0;for(auto value:it->second)if(value==attribute)++count;return count;
 }
};
