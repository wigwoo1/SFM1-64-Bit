#pragma once
#include "session_reference_ledger.h"
#include <chrono>
// Runtime owner for graph storage and its independent live handle domains.
struct BorrowedSessionGraphCandidate{};
struct SessionRuntime{
 std::unique_ptr<Graph> ownedGraph;
 Graph&graph;
 uint64_t bindingRevision=0;
 // Owner/metadata adapters call this for edits outside tracked setters.
 void invalidateBindings(){++bindingRevision;}
 std::chrono::steady_clock::time_point setupStarted=std::chrono::steady_clock::now();
 SessionRuntimeHandles elements;
 std::chrono::steady_clock::time_point elementsReady=std::chrono::steady_clock::now();
 SessionAttributeHandles attributes;
 std::chrono::steady_clock::time_point attributesReady=std::chrono::steady_clock::now();
 SessionReferenceLedger references;
 std::chrono::steady_clock::time_point referencesReady=std::chrono::steady_clock::now();
 explicit SessionRuntime(Graph loaded):ownedGraph(std::make_unique<Graph>(std::move(loaded))),graph(*ownedGraph),elements(graph),attributes(graph,elements),references(graph,elements,attributes){}
 // Borrower must die before graph owner. Only payload evaluation is supported;
 // reference/structural edits belong to owner and require plan invalidation.
 SessionRuntime(Graph&borrowed,BorrowedSessionGraphCandidate):graph(borrowed),elements(graph),attributes(graph,elements),references(graph,elements,attributes){}
 SessionRuntime(const SessionRuntime&)=delete;
 SessionRuntime&operator=(const SessionRuntime&)=delete;
 // Hooks must not mutate the graph or throw. Notification enablement is explicit
 // until native attribute flags and owner state are represented by this runtime.
 template<class Enabled,class Notify> bool setReferenceRangeControlled(uint32_t owner,const std::string&name,size_t start,const std::vector<uint32_t>&targets,Enabled enabled,Notify callback){
  if(!ownedGraph)throw std::runtime_error("borrowed runtime reference edit requires owner adapter");
  auto handle=attributes.find(owner,name);auto attribute=attributes.resolve(graph,elements,handle);
  if(!attribute||attribute->type!=15||targets.empty()||start>attribute->values.size()||targets.size()>attribute->values.size()-start)return false;
  std::vector<uint32_t> old;std::map<uint32_t,size_t> removed,added;bool changed=false;
  for(size_t i=0;i<targets.size();++i){auto target=targets[i];if(target!=0xffffffff&&!elements.resolve(graph,target))return false;
   const auto&value=attribute->values[start+i];if(value.reference==0xfffffffe)throw std::runtime_error("external range edit requires resolver");
   auto previous=elements.fromReference(value);old.push_back(previous);changed|=previous!=target;
   if(previous!=0xffffffff)++removed[previous];if(target!=0xffffffff)++added[target];
  }
  if(!changed)return false;
  for(auto item:removed)if(!elements.resolve(graph,item.first)||references.occurrences(item.first,handle)<item.second)throw std::runtime_error("range reference ledger inconsistent");
  for(auto item:added){auto&list=references.incoming[item.first];list.reserve(list.size()+item.second);}
  invalidateBindings();auto end=start+targets.size()-1;if(enabled())callback(0,handle,start,end);
  for(auto previous:old)if(previous!=0xffffffff)references.unlink(previous,handle,true);
  for(size_t i=0;i<targets.size();++i)attribute->values[start+i].reference=targets[i]==0xffffffff?0xffffffff:reinterpret_cast<const SessionElementRecord*>(elements.table.resolve(targets[i]))->graphIndex;
  if(enabled())callback(1,handle,start,end);
  for(auto target:targets)if(target!=0xffffffff)references.link(target,handle);
  for(auto item:removed){auto it=references.incoming.find(item.first);if(it!=references.incoming.end()&&it->second.empty())references.incoming.erase(it);}
  if(enabled())callback(2,handle,start,end);return true;
 }
 template<class Notify> bool setReferenceRange(uint32_t owner,const std::string&name,size_t start,const std::vector<uint32_t>&targets,bool notify,Notify callback){return setReferenceRangeControlled(owner,name,start,targets,[=](){return notify;},callback);}
 bool setReference(uint32_t owner,const std::string&name,size_t index,uint32_t target){
  if(!ownedGraph)throw std::runtime_error("borrowed runtime reference edit requires owner adapter");
  auto handle=attributes.find(owner,name);auto attribute=attributes.resolve(graph,elements,handle);
  if(!attribute||(attribute->type!=1&&attribute->type!=15)||index>=attribute->values.size()||(attribute->type==1&&index!=0))return false;
  if(target!=0xffffffff&&!elements.resolve(graph,target))return false;
  auto&value=attribute->values[index];if(value.reference==0xfffffffe)throw std::runtime_error("external reference edits require resolver");
  auto previous=elements.fromReference(value);if(previous==target)return false;
  if(previous!=0xffffffff&&(!elements.resolve(graph,previous)||!references.occurrences(previous,handle)))throw std::runtime_error("reference ledger inconsistent before edit");
  // Reserve insertion storage before mutation, then follow native scalar
  // setter 0x10365F90: unlink old -> write value -> link new.
  if(target!=0xffffffff){auto&list=references.incoming[target];list.reserve(list.size()+1);}
  invalidateBindings();if(previous!=0xffffffff)references.unlink(previous,handle);
  value.reference=target==0xffffffff?0xffffffff:reinterpret_cast<const SessionElementRecord*>(elements.table.resolve(target))->graphIndex;
  if(target!=0xffffffff)references.link(target,handle);
  return true;
 }
 bool removeAttribute(uint32_t owner,const std::string&name){
  if(!ownedGraph)throw std::runtime_error("borrowed runtime structural edit requires owner adapter");
  auto element=elements.resolve(graph,owner);if(!element)return false;
  auto handle=attributes.find(owner,name);auto attribute=attributes.resolve(graph,elements,handle);if(!attribute)return false;
  std::map<uint32_t,size_t> required;size_t external=0;
  if(attribute->type==1||attribute->type==15)for(const auto&value:attribute->values){
   if(value.reference==0xffffffff)continue;
   if(value.reference==0xfffffffe){++external;continue;}
   auto target=elements.fromReference(value);if(target==0xffffffff||!elements.resolve(graph,target))throw std::runtime_error("removed reference target is not live");++required[target];
  }
  // Check all occurrences before changing the ledger, avoiding partial removal
  // on an inconsistent runtime graph.
  for(auto entry:required)if(references.occurrences(entry.first,handle)!=entry.second)throw std::runtime_error("incoming ledger inconsistent before removal");
  if(external>references.externalUnresolved)throw std::runtime_error("external reference ledger inconsistent");
  auto removed=attributes.remove(graph,elements,owner,name,[&](const Value&value,uint32_t live){
   if(value.reference==0xffffffff)return;
   if(value.reference==0xfffffffe){--references.externalUnresolved;return;}
   if(!references.unlink(elements.fromReference(value),live))throw std::runtime_error("incoming reference unlink failed");
  });if(removed)invalidateBindings();return removed;
 }
};
