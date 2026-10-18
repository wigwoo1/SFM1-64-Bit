#pragma once
#include "reference_owned_attributes.h"
#include "reference_element_removal.h"
#include "reference_update_queues.h"
#include "resolve_queue_dispatch.h"
#include "reference_cleanup_pass.h"
#include "reference_live_handle_iteration.h"
#include "reference_root_mark.h"
#include "reference_outgoing_registration.h"
#include "reference_registration_update.h"
#include "reference_dirty_state.h"
#include "attribute_mutation_gate.h"
#include "element_mutation_state.h"
#include "scalar_reference_change_guard.h"
#include "scalar_reference_storage_sequence.h"
#include "scalar_reference_write_sequence.h"
#include "array_reference_range_assignment.h"
#include "scalar_reference_undo_replay.h"
#include "attribute_attachment_listener_dispatch.h"
#include <functional>
using OwnedAttributeAttachmentListenerCandidate=std::function<void(RecoveredOwnedReferenceAttributeCandidate&)>;
struct RecoveredOwnedSceneElementCandidate:RecoveredReferenceRuntimeElementCandidate {
 RecoveredReferenceRegistrationStateCandidate registration;uint32_t flags=0;RecoveredOwnedReferenceAttributeCandidate* attributes=nullptr;
 std::vector<OwnedAttributeAttachmentListenerCandidate*> attachmentListeners;
};
// Logical ownedscene binding. Native derived-class/factory callbacks still adapters.
class RecoveredOwnedSceneCandidate {
 std::unordered_map<RecoveredOwnedSceneElementCandidate*,std::unique_ptr<RecoveredOwnedSceneElementCandidate>> elements;
 std::unordered_map<uint32_t,RecoveredOwnedSceneElementCandidate*> owners;
public:
 RecoveredReferenceIdentityRuntimeCandidate references;
 RecoveredOwnedReferenceAttributesCandidate attributes;
 RecoveredReferenceUpdateQueuesCandidate queues;
 uint32_t mode=1;
 bool suppressAttachmentListeners=false;
 // Caller retains callback objects until all active snapshot dispatches finish.
 void addAttachmentListener(RecoveredOwnedSceneElementCandidate&owner,OwnedAttributeAttachmentListenerCandidate&listener){if(!elements.count(&owner))throw std::invalid_argument("unowned listener owner");if(!listener)throw std::invalid_argument("empty attachment listener");addAttributeAttachmentListenerCandidate(owner.attachmentListeners,&listener);}
 void removeAttachmentListener(RecoveredOwnedSceneElementCandidate&owner,OwnedAttributeAttachmentListenerCandidate&listener){if(!elements.count(&owner))throw std::invalid_argument("unowned listener owner");removeAttributeAttachmentListenerCandidate(owner.attachmentListeners,&listener);}
 size_t size()const{return elements.size();}
 // Native first callback queue; derived behavior supplied by the class registry.
 template<class Callback>void dispatchResolveQueue(Callback callback){recoveredResolveQueueDispatchCandidate(queues.registration.handles,[&](uint32_t h){return resolve(h);},callback,[](RecoveredOwnedSceneElementCandidate*e){e->registration.state&=0xfe;for(auto*a=e->attributes;a;a=a->next)a->flags&=0xff7f;});}

 RecoveredOwnedSceneElementCandidate* resolve(uint32_t handle)const{auto*p=reinterpret_cast<RecoveredOwnedSceneElementCandidate*>(references.resolve(handle));return p&&elements.count(p)?p:nullptr;}
 RecoveredOwnedSceneElementCandidate& create(const RecoveredPendingIdentityCandidate& identity){auto value=std::make_unique<RecoveredOwnedSceneElementCandidate>();value->identity=identity;value->cleanupContext=0;auto*ptr=value.get();elements.emplace(ptr,std::move(value));references.registerExistingElement(*ptr);return *ptr;}
 RecoveredOwnedReferenceAttributeCandidate& attach(RecoveredOwnedSceneElementCandidate& owner,uint16_t flags,std::vector<uint32_t> targets){
 if(!elements.count(&owner))throw std::invalid_argument("unowned element");auto*a=attributes.create(flags,std::move(targets));a->next=owner.attributes;owner.attributes=a;owners.emplace(a->token,&owner);owner.registration.state|=3;
 for(auto target:a->references())references.mutate(target,a->token,true,false);
 dispatchAttributeAttachmentListenersCandidate(suppressAttachmentListeners,owner.attachmentListeners,[&](auto*listener){(*listener)(*a);});return *a;
 }
 // No-cache owner adaptation: valid until cacheconstruction is connected.
 void invalidate(uint32_t token,int phase){auto*a=attributes.resolve(token);auto it=owners.find(token);if(!a||it==owners.end())return;auto&owner=*it->second;beginAttributeMutation(a->flags,phase,[&](){queues.cache.append(owner.record.handle);},[&](){markElementMutation(owner.registration.state,owner.record.handle,[&](uint32_t h){queues.registration.append(h);});});}
 // Recovered scalar guard/storage sequence; class checking remains caller supplied.
 // Borrowed payloads are read-only in this owned runtime.
 template<class Matches>bool setScalarReference(uint32_t token,uint32_t incoming,int phase,uint32_t requiredClass,Matches matches){
 auto*a=attributes.resolve(token);auto it=owners.find(token);if(!a||it==owners.end()||!a->owned||(a->flags&31)!=1||a->owned->size()!=1)return false;auto&owner=*it->second;
 auto resolveClass=[&](uint32_t h){return references.resolve(h)?h:0xffffffffu;};
 if(!recoveredScalarReferenceChangeAllowedCandidate(1,a->owned->front(),incoming,requiredClass,resolveClass,matches,[&](){return beginAttributeMutation(a->flags,phase,[&](){queues.cache.append(owner.record.handle);},[&](){markElementMutation(owner.registration.state,owner.record.handle,[&](uint32_t h){queues.registration.append(h);});});}))return false;
 writeRecoveredScalarReferenceCandidate(a->owned->front(),incoming,[](){},[&](uint32_t h){references.mutate(h,token,true,true);},[&](uint32_t h){references.mutate(h,token,true,false);},[](){},[&](){dirty(owner,3);references.frameworkFlags|=0x1000;});return true;
 }
 // Fixed-size array range. Input copy protects callers that alias owned storage.
 // Native undo and derived owner callbacks are not yet connected.
 template<class Matches>bool setArrayReferenceRange(uint32_t token,int start,std::vector<uint32_t> incoming,int phase,uint32_t requiredClass,Matches matches){
 auto*a=attributes.resolve(token);auto it=owners.find(token);if(!a||it==owners.end()||!a->owned||(a->flags&31)!=15)return false;auto&owner=*it->second;
 return assignRecoveredReferenceArrayRangeCandidate(*a->owned,start,incoming,requiredClass,[&](){return beginAttributeMutation(a->flags,phase,[&](){queues.cache.append(owner.record.handle);},[&](){markElementMutation(owner.registration.state,owner.record.handle,[&](uint32_t h){queues.registration.append(h);});});},[](size_t,size_t){},[&](uint32_t h){references.mutate(h,token,true,true);},[&](uint32_t h){return references.resolve(h)?uint64_t(h)+1:0ull;},[&](uint64_t target,uint32_t cls){return matches(uint32_t(target-1),cls);},[](size_t,size_t){},[&](uint32_t h){references.mutate(h,token,true,false);},[&](){dirty(owner,3);references.frameworkFlags|=0x1000;});
 }
 // Returns whether normal writer was dispatched, not whether it changed value.
 // Caller supplies logical name lookup and must retain history targets separately.
 template<class FindName,class Matches>bool replayScalarReference(const RecoveredScalarReferenceUndoPayloadCandidate&record,bool reverse,FindName findName,int phase,uint32_t requiredClass,Matches matches){
 return replayRecoveredScalarReferenceUndoCandidate(record,reverse,[&](uint32_t h){return resolve(h);},[&](auto*owner,uint32_t name){auto token=findName(*owner,name);auto it=owners.find(token);return it!=owners.end()&&it->second==owner?attributes.resolve(token):nullptr;},[](auto*a){return a->flags;},[&](auto*a,uint32_t target){setScalarReference(a->token,target,phase,requiredClass,matches);});
 }
 
 void dirty(RecoveredOwnedSceneElementCandidate& e,uint32_t request){
 if(!dirtyRecoveredReferenceStateCandidate(e.registration.state,e.flags,request,false,false,[&](){queues.dirty.append(e.record.handle);},[](){}))return;
 for(auto token:references.nodes.tokens(e.record)){auto*a=attributes.resolve(token);auto it=owners.find(token);if(a&&!(a->flags&0x1000)&&it!=owners.end())dirty(*it->second,request&~2u);}
 }
 // Native derived-class hooks are supplied by caller; caches/observers currently empty.
 template<class Notify,class BeforeDestroy>
 void cleanup(const std::vector<uint32_t>& roots,Notify notify,BeforeDestroy beforeDestroy){
 const auto rawCount=references.handles.slots().size();
 auto raw=[&](auto visit){for(size_t i=0;i<rawCount;++i){const auto&slot=references.handles.slots()[i];if(i<=0x3fffffu&&slot.element&&(slot.state&0x7fffffffu)<=511){auto*e=reinterpret_cast<RecoveredOwnedSceneElementCandidate*>(slot.element);if(elements.count(e))visit(*e);}}};
 auto enumerate=[&](auto visit){auto resolveHandle=[&](uint32_t h){return resolve(h);};auto handle=firstRecoveredReferenceLiveHandleCandidate(references.handles.slots(),resolveHandle);while(handle!=0xffffffffu){visit(handle);handle=nextRecoveredReferenceLiveHandleCandidate(references.handles.slots(),handle,resolveHandle);}};
 auto resolveElement=[&](uint32_t h){return resolve(h);};auto registration=[](auto&e)->RecoveredReferenceRegistrationStateCandidate&{return e.registration;};
 auto state=[](auto*e)->RecoveredReferenceRegistrationStateCandidate&{return e->registration;};
 auto attached=[](auto*e,auto visit){for(auto*a=e->attributes;a;a=a->next)visit(a->flags,a->references());};
 auto mark=[&](auto&e,uint32_t enabled,uint32_t mode,uint32_t context){markRecoveredReferenceRootCandidate(&e,enabled,mode,context,state,attached,resolveElement,[](auto*){return true;});};
 auto outgoing=[](auto*e,auto visit){for(auto*a=e->attributes;a;a=a->next)visit(a->token,a->flags,a->references());};
 auto registerReferences=[&](auto&e,uint32_t context,bool removal){registerRecoveredOutgoingReferencesCandidate(e,context,removal,outgoing,resolveElement,[](auto*){return true;},registration,[&](auto&target,uint32_t token,bool remove){if(remove)references.nodes.remove(target.record,token);else references.nodes.add(target.record,token);});};
 auto update=[&](auto&e,bool enabled){updateRecoveredReferenceRegistrationCandidate(e.registration.state,e.flags,enabled,[](){},[&](){queues.registration.append(e.record.handle);},[&](uint32_t request){dirty(e,request);},[](uint32_t){});};
 recoveredReferenceCleanupPassCandidate(raw,registration,mark,[&](auto visit){for(auto h:roots)visit(h);},enumerate,resolveElement,registerReferences,update,notify,[&](uint32_t h,uint32_t){removeModeZero(h,0,[](auto&){},[&](auto&e,uint32_t request){dirty(e,request);},beforeDestroy);});
 }

 template<class Observe,class Dirty,class BeforeDestroy>
 bool removeModeZero(uint32_t handle,int phase,Observe observe,Dirty dirty,BeforeDestroy beforeDestroy){
 return removeRecoveredReferenceElementModeZeroCandidate(handle,references.frameworkFlags,[&](){return phase;},[&](uint32_t h){if(auto*r=references.record(h))for(auto token:references.nodes.tokens(*r))invalidate(token,phase);},[&](uint32_t h){return resolve(h);},observe,[&](auto&e,bool){dirty(e,3);},[&](auto&e){auto position=references.live.findPosition(e.identity,[&](uint32_t h)->const RecoveredPendingIdentityCandidate*{auto*p=references.resolve(h);return p?&p->identity:nullptr;});if(position!=0xffffffffu)references.live.erase(position);},beforeDestroy,[&](auto&e){auto*head=e.attributes;for(auto*a=head;a;a=a->next)owners.erase(a->token);attributes.finalize(e.attributes,references,[&](){return mode;},[&](uint32_t value){mode=value;},[&](uint32_t request){dirty(e,request);});},[&](uint32_t h){auto*e=resolve(h);if(!e)throw std::runtime_error("lost removal element");for(auto token:references.nodes.tokens(e->record))references.nodes.remove(e->record,token);elements.erase(e);},[&](uint32_t h){references.handles.retire(h);});
 }
};
