#pragma once
#include "element_handle_release.h"
#include <memory>
struct SessionElementRecord{uint32_t graphIndex;};
struct SessionRuntimeHandles{
 ElementHandleReleaseTable table;
 std::vector<std::unique_ptr<SessionElementRecord>> records;
 std::vector<uint32_t> graphHandles;
 explicit SessionRuntimeHandles(const Graph&graph){for(uint32_t i=0;i<graph.elements.size();++i)bind(graph,i);}
 uint32_t bind(const Graph&graph,uint32_t index){
  (void)graph.elements.at(index);if(graphHandles.size()<=index)graphHandles.resize(size_t(index)+1,0xffffffff);
  if(graphHandles[index]!=0xffffffff)throw std::runtime_error("graph element already bound");
  auto record=std::make_unique<SessionElementRecord>();record->graphIndex=index;
  uint32_t handle=table.allocate();table.publish(handle,reinterpret_cast<uintptr_t>(record.get()));records.push_back(std::move(record));graphHandles[index]=handle;return handle;
 }
 uint32_t fromReference(const Value&value)const{
  if(value.reference==0xffffffff)return 0xffffffff;
  if(value.reference==0xfffffffe)throw std::runtime_error("external element reference requires resolver");
  return graphHandles.at(value.reference);
 }
 Element* resolve(Graph&graph,uint32_t handle)const{
  auto record=reinterpret_cast<const SessionElementRecord*>(table.resolve(handle));return record?&graph.elements.at(record->graphIndex):nullptr;
 }
 bool retire(uint32_t handle){
  auto record=reinterpret_cast<const SessionElementRecord*>(table.resolve(handle));if(!record)return false;
  graphHandles.at(record->graphIndex)=0xffffffff;return table.releaseIndex(handle);
 }
};
