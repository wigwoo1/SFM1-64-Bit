#pragma once
#include "reference_attribute_destruction.h"
#include "reference_element_finalization.h"
#include "reference_handle_table.h"
#include "reference_identity_runtime.h"
#include <memory>
#include <unordered_map>
struct RecoveredOwnedReferenceAttributeCandidate {
 uint16_t flags=0;uint32_t token=0xffffffffu;uintptr_t storage=0;
 RecoveredOwnedReferenceAttributeCandidate* next=nullptr;
 std::unique_ptr<std::vector<uint32_t>> owned;
 const std::vector<uint32_t>* borrowed=nullptr;
 const std::vector<uint32_t>& references()const{return owned?*owned:*borrowed;}
};
// Owns scalar/array reference attributes only; other typedpayloads still pending.
class RecoveredOwnedReferenceAttributesCandidate {
 std::unordered_map<RecoveredOwnedReferenceAttributeCandidate*,std::unique_ptr<RecoveredOwnedReferenceAttributeCandidate>> values;
public:
 RecoveredReferenceHandleTableCandidate handles;
 size_t size()const{return values.size();}
 RecoveredOwnedReferenceAttributeCandidate* resolve(uint32_t token)const{return reinterpret_cast<RecoveredOwnedReferenceAttributeCandidate*>(handles.resolve(token));}
 RecoveredOwnedReferenceAttributeCandidate* create(uint16_t flags,std::vector<uint32_t> references,const std::vector<uint32_t>* borrowed=nullptr){
 const auto type=flags&31;if(type!=1&&type!=15)throw std::invalid_argument("reference attribute type required");
 if(bool(flags&0x200)!=(borrowed!=nullptr))throw std::invalid_argument("external storage ownership mismatch");
 if(type==1&&(borrowed?borrowed->size():references.size())!=1)throw std::invalid_argument("scalar requires one handle");
 auto value=std::make_unique<RecoveredOwnedReferenceAttributeCandidate>();value->flags=flags;
 if(borrowed)value->borrowed=borrowed;else value->owned=std::make_unique<std::vector<uint32_t>>(std::move(references));
 value->storage=reinterpret_cast<uintptr_t>(borrowed?borrowed:value->owned.get());auto*ptr=value.get();
 auto inserted=values.emplace(ptr,std::move(value));try{ptr->token=handles.allocate();if(ptr->token==0xffffffffu)throw std::length_error("attribute handles exhausted");handles.bind(ptr->token,reinterpret_cast<uintptr_t>(ptr));}catch(...){values.erase(inserted.first);throw;}return ptr;
 }
 template<class Trace>void destroy(RecoveredOwnedReferenceAttributeCandidate* attribute,RecoveredReferenceIdentityRuntimeCandidate& runtime,Trace trace){
 if(!attribute)return;if(!values.count(attribute))throw std::invalid_argument("unowned attribute");
 destroyRecoveredReferenceAttributeCandidate(attribute,[](auto&a,auto visit){for(auto handle:a.references())visit(handle);},[&](uint32_t target,auto&a){trace(1,target);runtime.mutate(target,a.token,true,true);},[&](uint32_t token){handles.retire(token);if((handles.slots()[token&0x3fffffu].state&0x7fffffffu)<511)trace(5,token&0x3fffffu);},[&](uint32_t type,uintptr_t){trace(3,type);attribute->owned.reset();},[&](auto*ptr){trace(4,0);values.erase(ptr);});
 }
 template<class GetMode,class SetMode,class Dirty>void finalize(RecoveredOwnedReferenceAttributeCandidate*& head,RecoveredReferenceIdentityRuntimeCandidate& runtime,GetMode getMode,SetMode setMode,Dirty dirty){finalizeRecoveredReferenceElementCandidate(head,runtime.frameworkFlags,getMode,setMode,[](auto*a){return a->next;},[&](auto*a){destroy(a,runtime);},dirty);}
 void destroy(RecoveredOwnedReferenceAttributeCandidate* attribute,RecoveredReferenceIdentityRuntimeCandidate& runtime){destroy(attribute,runtime,[](uint32_t,uint32_t){});}
};
