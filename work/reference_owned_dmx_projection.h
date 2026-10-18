#pragma once
#include "reference_owned_scene.h"
#include <cstring>
#include <cctype>
#include <unordered_map>
#include "float_mutation.h"
#include "float_component_write_sequence.h"
#include "float_components_undo_replay.h"
#include "float_array_write_sequence.h"
#include "float_array_owner_notification.h"
#include "scalar_float_undo_replay.h"
// Retains completeDMXgraph; onlyreferencepayloads currently bound toownedruntime.
// External reference resolution remains an explicit caller-supplied adapter.
template<class Graph>
class RecoveredOwnedDmxReferenceProjectionCandidate {
public:
 Graph graph;RecoveredOwnedSceneCandidate scene;
 std::vector<uint32_t> elementHandles;
 std::vector<uint32_t> opaqueExternalElementHandles;
 std::unordered_map<std::string,uint32_t> opaqueExternalHandlesByUuid;
 std::unordered_map<uint32_t,std::string> opaqueExternalUuidsByHandle;
 std::unordered_map<uint32_t,std::pair<uint32_t,uint32_t>> attributeLocations;
 // Logical flags for nonreference payloads; native class defaults still pending.
 std::vector<std::vector<uint16_t>> typedAttributeFlags;
 static RecoveredPendingIdentityCandidate identityFromUuid(const std::string&text){
 std::array<uint8_t,16>canonical{};size_t count=0;int high=-1;
 for(unsigned char c:text){if(c=='-')continue;int digit=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;if(digit<0)throw std::runtime_error("invalid external DMX UUID");if(high<0)high=digit;else{if(count>=canonical.size())throw std::runtime_error("invalid external DMX UUID length");canonical[count++]=uint8_t((high<<4)|digit);high=-1;}}
 if(count!=canonical.size()||high>=0)throw std::runtime_error("invalid external DMX UUID length");
 const std::array<uint8_t,16>wire={canonical[3],canonical[2],canonical[1],canonical[0],canonical[5],canonical[4],canonical[7],canonical[6],canonical[8],canonical[9],canonical[10],canonical[11],canonical[12],canonical[13],canonical[14],canonical[15]};
 RecoveredPendingIdentityCandidate identity{};std::memcpy(identity.data(),wire.data(),wire.size());return identity;
 }
 uint32_t preserveOpaqueExternalReference(const std::string&uuid){
 auto existing=opaqueExternalHandlesByUuid.find(uuid);if(existing!=opaqueExternalHandlesByUuid.end())return existing->second;
 const auto identity=identityFromUuid(uuid);
 for(auto handle:elementHandles){auto*element=scene.resolve(handle);if(element&&element->identity==identity)throw std::runtime_error("external DMX UUID collides with a local element identity");}
 auto&element=scene.create(identity);element.record.flags|=1;const auto handle=element.record.handle;
 opaqueExternalHandlesByUuid.emplace(uuid,handle);opaqueExternalUuidsByHandle.emplace(handle,uuid);opaqueExternalElementHandles.push_back(handle);return handle;
 }
 const std::string&opaqueExternalUuid(uint32_t handle)const{auto found=opaqueExternalUuidsByHandle.find(handle);if(found==opaqueExternalUuidsByHandle.end())throw std::runtime_error("reference is not an opaque external DMX UUID");return found->second;}
 template<class External>RecoveredOwnedDmxReferenceProjectionCandidate(Graph source,External external,bool preserveOpaqueExternal=false):graph(std::move(source)){
 elementHandles.reserve(graph.elements.size());for(const auto&element:graph.elements){RecoveredPendingIdentityCandidate identity{};std::memcpy(identity.data(),element.id.data(),16);elementHandles.push_back(scene.create(identity).record.handle);}
 for(uint32_t i=0;i<graph.elements.size();++i){auto*owner=scene.resolve(elementHandles[i]);for(uint32_t j=0;j<graph.elements[i].attributes.size();++j){const auto&a=graph.elements[i].attributes[j];if(a.type!=1&&a.type!=15)continue;std::vector<uint32_t>targets;for(const auto&value:a.values){uint32_t h;if(value.reference==0xffffffffu)h=0xffffffffu;else if(value.reference==0xfffffffeu)h=preserveOpaqueExternal?preserveOpaqueExternalReference(value.text):external(value.text,scene.references);else{if(value.reference>=elementHandles.size())throw std::runtime_error("DMX reference index");h=elementHandles[value.reference];}targets.push_back(h);}auto&bound=scene.attach(*owner,a.type,std::move(targets));attributeLocations.emplace(bound.token,std::make_pair(i,j));}}
 for(auto h:elementHandles)scene.resolve(h)->record.flags|=1;
 for(const auto&e:graph.elements){std::vector<uint16_t>flags;for(const auto&a:e.attributes)flags.push_back(a.type);typedAttributeFlags.push_back(std::move(flags));}
 }
 // Native float branch 0x1036D040; storage helper 0x10362736 uses x87.
 // Optional explicit undo capture; native recording gate/journal still unbound.
 bool setScalarFloat(uint32_t element,const std::string&name,float incoming,int phase,RecoveredScalarFloatUndoPayloadCandidate*undo=nullptr){
 if(element>=elementHandles.size())return false;auto*owner=scene.resolve(elementHandles[element]);if(!owner)return false;auto&e=graph.elements.at(element);
 for(size_t j=0;j<e.attributes.size();++j){auto&a=e.attributes[j];if(graph.strings.at(a.name)!=name)continue;if(a.type!=3||a.values.size()!=1||a.values[0].wire.size()!=4)return false;auto&flags=typedAttributeFlags.at(element).at(j);if((flags&31)!=3)throw std::runtime_error("scalar float flags mismatch");auto&wire=a.values[0].wire;float current;std::memcpy(&current,wire.data(),4);if(!floatMutationNeeded(current,incoming))return false;
 if(!beginAttributeMutation(flags,phase,[&](){scene.queues.cache.append(owner->record.handle);},[&](){markElementMutation(owner->registration.state,owner->record.handle,[&](uint32_t h){scene.queues.registration.append(h);});}))return false;
 if(undo){undo->owner=owner->record.handle;undo->name=a.name;captureRecoveredScalarFloatUndoCandidate(*undo,current,incoming);}
 uint32_t bits;std::memcpy(&bits,&incoming,4);bits=recoveredX87FloatStoreBitsCandidate(bits);std::memcpy(wire.data(),&bits,4);scene.dirty(*owner,1);scene.references.frameworkFlags|=0x2000;return true;
 }return false;
 }
 // Returns whether the normal setter changed payload; equal/rejected writes
 // return false. Replay does not create another undo record.
 bool replayScalarFloatUndo(const RecoveredScalarFloatUndoPayloadCandidate&record,bool reverse,int phase){
 if(!scene.resolve(record.owner)||record.name>=graph.strings.size())return false;
 for(uint32_t i=0;i<elementHandles.size();++i)if(elementHandles[i]==record.owner)return setScalarFloat(i,graph.strings.at(record.name),reverse?record.before:record.after,phase);
 return false;
 }
 template<size_t N>bool setFloatComponents(uint32_t element,const std::string&name,const std::array<float,N>&incoming,int phase){
 static_assert(N==3||N==4,"vector or quaternion only");if(element>=elementHandles.size())return false;auto*owner=scene.resolve(elementHandles[element]);if(!owner)return false;auto&e=graph.elements.at(element);
 for(size_t j=0;j<e.attributes.size();++j){auto&a=e.attributes[j];if(graph.strings.at(a.name)!=name)continue;if(a.type!=(N==3?10:13)||a.values.size()!=1||a.values[0].wire.size()!=N*4)return false;auto&flags=typedAttributeFlags.at(element).at(j);if((flags&31)!=a.type)throw std::runtime_error("typed attribute flags mismatch");auto&wire=a.values[0].wire;std::array<float,N>storage{};std::memcpy(storage.data(),wire.data(),N*4);
 return writeRecoveredFloatComponentsCandidate(storage,incoming,[&](){return beginAttributeMutation(flags,phase,[&](){scene.queues.cache.append(owner->record.handle);},[&](){markElementMutation(owner->registration.state,owner->record.handle,[&](uint32_t h){scene.queues.registration.append(h);});});},[](){},[&](){std::memcpy(wire.data(),storage.data(),N*4);},[&](){scene.dirty(*owner,1);scene.references.frameworkFlags|=0x2000;});
 }return false;
 }
 // Indexed type17 writes. Callbacks may change flags, but must not change graph
 // structure/storage or recursively write this attribute. Native undo is pending.
 template<class Before,class After>bool setFloatArrayElement(uint32_t element,const std::string&name,int32_t index,uint32_t incoming,int phase,Before before,After after){
 if(element>=elementHandles.size())return false;auto*owner=scene.resolve(elementHandles[element]);if(!owner)return false;
 auto&e=graph.elements.at(element);for(size_t j=0;j<e.attributes.size();++j){auto&a=e.attributes[j];if(graph.strings.at(a.name)!=name)continue;
 if(a.type!=17||index<0||size_t(index)>=a.values.size())return false;
 auto&flags=typedAttributeFlags.at(element).at(j);if((flags&31)!=17)throw std::runtime_error("float array flags mismatch");
 for(const auto&v:a.values)if(v.wire.size()!=4)throw std::runtime_error("float array wire size");uint32_t storage;std::memcpy(&storage,a.values[size_t(index)].wire.data(),4);
 return writeRecoveredFloatArrayElementCandidate(storage,index,incoming,[&](){return beginAttributeMutation(flags,phase,[&](){scene.queues.cache.append(owner->record.handle);},[&](){markElementMutation(owner->registration.state,owner->record.handle,[&](uint32_t h){scene.queues.registration.append(h);});});},[](){},
 [&](int32_t first,int32_t last){recoveredFloatArrayOwnerNotificationCandidate(flags,owner->registration.state,first,last,before);},
 [&](int32_t first,int32_t last,int){std::memcpy(a.values[size_t(index)].wire.data(),&storage,4);recoveredFloatArrayOwnerNotificationCandidate(flags,owner->registration.state,first,last,after);},
 [&](){scene.dirty(*owner,1);scene.references.frameworkFlags|=0x2000;});
 }return false;
 }
 bool setFloatArrayElement(uint32_t element,const std::string&name,int32_t index,uint32_t incoming,int phase){return setFloatArrayElement(element,name,index,incoming,phase,[](int32_t,int32_t){},[](int32_t,int32_t){});}
 // Logical capture; native constructor metadata/history grouping still pending.
 template<size_t N>RecoveredFloatComponentsUndoPayloadCandidate<N> captureFloatUndo(uint32_t element,const std::string&name,const std::array<float,N>&incoming)const{
 static_assert(N==3||N==4,"vector or quaternion only");if(element>=elementHandles.size()||!scene.resolve(elementHandles[element]))throw std::runtime_error("missing typed undo owner");for(const auto&a:graph.elements.at(element).attributes)if(graph.strings.at(a.name)==name){if(a.type!=(N==3?10:13)||a.values.size()!=1||a.values[0].wire.size()!=N*4)throw std::runtime_error("invalid typed undo attribute");RecoveredFloatComponentsUndoPayloadCandidate<N>r;r.owner=elementHandles[element];r.name=a.name;std::array<float,N>before;std::memcpy(before.data(),a.values[0].wire.data(),N*4);captureRecoveredFloatUndoValuesCandidate(r,before,incoming);return r;}throw std::runtime_error("missing typed undo attribute");
 }
 // Returns dispatch status, even if normal setter rejects phase or equal value.
 template<size_t N>bool replayFloatUndo(const RecoveredFloatComponentsUndoPayloadCandidate<N>&record,bool reverse,int phase){
 uint32_t element=0xffffffffu;return replayRecoveredFloatComponentsUndoCandidate(record,reverse,[&](uint32_t h){return scene.resolve(h);},[&](auto*,uint32_t name)->uint16_t*{for(uint32_t i=0;i<elementHandles.size();++i)if(elementHandles[i]==record.owner){element=i;for(size_t j=0;j<graph.elements[i].attributes.size();++j)if(graph.elements[i].attributes[j].name==name)return &typedAttributeFlags.at(i).at(j);break;}return nullptr;},[](uint16_t*flags){return *flags;},[&](uint16_t*,const auto&value){setFloatComponents(element,graph.strings.at(record.name),value,phase);});
 }
 auto captureVectorUndo(uint32_t element,const std::string&name,const std::array<float,3>&incoming)const{return captureFloatUndo(element,name,incoming);}
 auto captureQuaternionUndo(uint32_t element,const std::string&name,const std::array<float,4>&incoming)const{return captureFloatUndo(element,name,incoming);}
 bool replayVectorUndo(const RecoveredFloatComponentsUndoPayloadCandidate<3>&r,bool reverse,int phase){return replayFloatUndo(r,reverse,phase);}
 bool replayQuaternionUndo(const RecoveredFloatComponentsUndoPayloadCandidate<4>&r,bool reverse,int phase){return replayFloatUndo(r,reverse,phase);}
 // Export a snapshot only while the imported element/attribute structure survives.
 // Deletion, insertion and external UUID serialization require explicit adapters.
 template<class External>Graph exportReferences(External external)const{
 Graph result=graph;std::unordered_map<uint32_t,uint32_t> indices;
 for(uint32_t i=0;i<elementHandles.size();++i){if(!scene.resolve(elementHandles[i]))throw std::runtime_error("deleted DMX element requires structural export");indices.emplace(elementHandles[i],i);}
 if(scene.size()!=elementHandles.size()+opaqueExternalElementHandles.size()||scene.attributes.size()!=attributeLocations.size())throw std::runtime_error("changed DMX structure requires structural export");
 for(const auto&item:attributeLocations){auto*bound=scene.attributes.resolve(item.first);if(!bound)throw std::runtime_error("deleted DMX attribute requires structural export");auto&a=result.elements.at(item.second.first).attributes.at(item.second.second);if((bound->flags&31)!=a.type)throw std::runtime_error("changed DMX attribute type");const auto&targets=bound->references();if(a.type==1&&targets.size()!=1)throw std::runtime_error("invalid scalar reference");a.values.resize(targets.size());for(size_t i=0;i<targets.size();++i){auto&value=a.values[i];value.wire.clear();value.text.clear();value.stringIndex=0xffffffffu;auto found=indices.find(targets[i]);if(targets[i]==0xffffffffu)value.reference=0xffffffffu;else if(found!=indices.end())value.reference=found->second;else{value.reference=0xfffffffeu;value.text=external(targets[i],scene.references);}}}
 return result;
 }
};
