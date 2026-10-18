#pragma once
#include "session_runtime_handles.h"
#include <map>
#include <string>
struct SessionAttributeRecord{uint32_t ownerHandle;std::string name;};
// Independent handle domain; records remain stable across graph-vector growth.
// Native 0x1035B4C0 allocates via shared 0x1035AB20 on framework+C0EC;
// native 0x10359C80 releases through shared 0x10356E70 on that same table.
// Graph attribute removal must retire the handle before erasing storage.
struct SessionAttributeHandles{
 ElementHandleReleaseTable table;
 std::vector<std::unique_ptr<SessionAttributeRecord>> records;
 std::map<std::pair<uint32_t,std::string>,uint32_t> handles;
 std::map<uintptr_t,uint32_t> identityHandles;
 explicit SessionAttributeHandles(Graph&graph,const SessionRuntimeHandles&elements){
  for(uint32_t i=0;i<graph.elements.size();++i)for(const auto&a:graph.elements[i].attributes)bind(graph,elements,elements.graphHandles.at(i),graph.strings.at(a.name));
 }
 uint32_t bind(Graph&graph,const SessionRuntimeHandles&elements,uint32_t owner,const std::string&name){
  auto element=elements.resolve(graph,owner);if(!element)throw std::runtime_error("attribute owner is not live");
  bool found=false;for(const auto&a:element->attributes)if(graph.strings.at(a.name)==name){found=true;break;}
  if(!found)throw std::runtime_error("attribute storage missing");
  auto key=std::make_pair(owner,name);if(handles.count(key))throw std::runtime_error("attribute already bound");
  auto record=std::make_unique<SessionAttributeRecord>();record->ownerHandle=owner;record->name=name;
  uint32_t handle=table.allocate();auto identity=reinterpret_cast<uintptr_t>(record.get());table.publish(handle,identity);identityHandles.emplace(identity,handle);records.push_back(std::move(record));handles.emplace(std::move(key),handle);return handle;
 }
 uint32_t find(uint32_t owner,const std::string&name)const{auto it=handles.find({owner,name});return it==handles.end()?0xffffffff:it->second;}
 uintptr_t identity(Graph&graph,const SessionRuntimeHandles&elements,uint32_t handle)const{return resolve(graph,elements,handle)?table.resolve(handle):0;}
 Attribute* resolveIdentity(Graph&graph,const SessionRuntimeHandles&elements,uintptr_t identity)const{
  auto it=identityHandles.find(identity);if(it==identityHandles.end()||table.resolve(it->second)!=identity)return nullptr;
  return resolve(graph,elements,it->second);
 }
 Attribute* resolve(Graph&graph,const SessionRuntimeHandles&elements,uint32_t handle)const{
  auto record=reinterpret_cast<const SessionAttributeRecord*>(table.resolve(handle));if(!record)return nullptr;
  auto element=elements.resolve(graph,record->ownerHandle);if(!element)return nullptr;
  for(auto&a:element->attributes)if(graph.strings.at(a.name)==record->name)return &a;return nullptr;
 }
 bool retire(uint32_t handle){
  auto record=reinterpret_cast<const SessionAttributeRecord*>(table.resolve(handle));if(!record)return false;
  handles.erase({record->ownerHandle,record->name});return table.releaseIndex(handle);
 }
 template<class ReleaseReference> bool remove(Graph&graph,const SessionRuntimeHandles&elements,uint32_t owner,const std::string&name,ReleaseReference releaseReference){
  auto element=elements.resolve(graph,owner);if(!element)return false;
  for(auto it=element->attributes.begin();it!=element->attributes.end();++it)if(graph.strings.at(it->name)==name){
   auto handle=find(owner,name);if(handle==0xffffffff||!table.resolve(handle))return false;
   // 0x10383F30 unlinks scalar/array references before retiring the handle.
   if(it->type==1||it->type==15)for(const auto&value:it->values)releaseReference(value,handle);
   if(!retire(handle))return false;
   element->attributes.erase(it);return true;
  }
  return false;
 }
 bool remove(Graph&graph,const SessionRuntimeHandles&elements,uint32_t owner,const std::string&name){return remove(graph,elements,owner,name,[](const Value&,uint32_t){});}
};
