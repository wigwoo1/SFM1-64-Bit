#pragma once
#include "material_element_binding_candidate.h"
#include "material_object_store_candidate.h"
#include "material_find_cache_key_candidate.h"

// Owner-thread session-element cache owner. Store MUST outlive this object;
// neither owner may be destroyed from a callback. No Graph factory/GC or GPU.
// Acquisition is persistent cache work, separate from scoped parameter rollback.
// Explicit full Find service returns a borrowed actual store ID,
// actual0 (known no resource), or nullopt (unavailable). Without it, only existing
// filename-category cache entries are supported; a miss is UNAVAILABLE, not0.
class MaterialSessionStoreBindingsCandidate {
public:
 using Store=MaterialObjectStoreCandidate;using Id=Store::Id;
 using Lookup=std::function<std::optional<Id>(std::string_view)>;
 struct Source{uint32_t element=0;std::vector<MaterialSessionOverrideInputCandidate>rows;};
 enum class Status{Refused,UnavailableBinding,ValuesRefused,Observed};
 struct Attempt{Status status=Status::Refused;size_t acquiredPositions=0;};
 struct Counts{size_t lookups=0,retains=0,releases=0,staleRetirements=0;};
private:
 Store&store;Lookup lookup;std::map<uint32_t,MaterialElementBindingCandidate>elements;
 bool busy=false;Counts counts;
 struct Guard{MaterialSessionStoreBindingsCandidate&owner;explicit Guard(MaterialSessionStoreBindingsCandidate&o)noexcept:owner(o){owner.busy=true;}~Guard(){owner.busy=false;}Guard(const Guard&)=delete;Guard&operator=(const Guard&)=delete;};
 Guard guard()noexcept{return Guard(*this);}
 bool retainable(Id id)const{
  // Native cached Find77 does not initialize parameters. A valid resource may
  // remain uninitialized; the separate value scope will refuse that state.
  const auto*p=store.get(id);return p&&!p->initializationPending&&!p->releasePending&&!p->removalPending&&!p->evaluationPending&&!p->sessionPending&&p->references.load()<std::numeric_limits<int32_t>::max();
 }
 bool releasable(Id id)const{const auto*p=store.get(id);return p&&!p->sessionPending&&p->references.load()>0;}
 struct Services:MaterialElementBindingServicesCandidate{
  MaterialSessionStoreBindingsCandidate&owner;
  explicit Services(MaterialSessionStoreBindingsCandidate&o):owner(o){}
  std::optional<uintptr_t>findMaterial(std::string_view name)override{
   ++owner.counts.lookups;
   if(owner.lookup)return owner.lookup(name);
   std::string key;if(!materialFindCacheKeyCandidate(name,key))return {};
   auto id=owner.store.find(key,false);if(!id)return {};return id;
  }
  void addReference(uintptr_t id)override{
   if(!owner.retainable(id)||!owner.store.increment(id))throw std::runtime_error("session resource retain unavailable");++owner.counts.retains;
  }
  void releaseReference(uintptr_t id)override{
   if(!owner.releasable(id)||!owner.store.decrement(id))throw std::runtime_error("session resource release unavailable");++owner.counts.releases;
  }
 };
 std::optional<Id>acquireInside(uint32_t element){
  auto i=elements.find(element);if(i==elements.end())return {};auto&state=i->second;
  if(state.binding.resource){if(!retainable(state.binding.resource)||!releasable(state.binding.resource))return {};return state.binding.resource;}
  Services services(*this);
  // Original stores cache before AddRef. The owned retain may refuse a stale or
  // busy ID: restore this empty cache on that failure, without claiming lookup
  // or registry side effects were rolled back. No allocation follows AddRef.
  try{return acquireMaterialSessionResourceCandidate(state.binding,services);}
  catch(...){state.binding.resource=0;return {};}
 }
public:
 explicit MaterialSessionStoreBindingsCandidate(Store&s,Lookup find={}):store(s),lookup(std::move(find)){}
 ~MaterialSessionStoreBindingsCandidate(){
  // Caller contract forbids destruction while busy. Forced store deletion or
  // refcount edits are not undone; retire dead bindings without a false release.
  for(auto&entry:elements){auto id=entry.second.binding.resource;if(!id)continue;if(releasable(id)&&store.decrement(id))++counts.releases;else ++counts.staleRetirements;}
 }
 MaterialSessionStoreBindingsCandidate(const MaterialSessionStoreBindingsCandidate&)=delete;
 MaterialSessionStoreBindingsCandidate&operator=(const MaterialSessionStoreBindingsCandidate&)=delete;
 const MaterialElementBindingCandidate*get(uint32_t element)const{auto i=elements.find(element);return i==elements.end()?nullptr:&i->second;}
 const Counts&statistics()const{return counts;}
 size_t size()const{return elements.size();}
 bool define(uint32_t element){if(busy||!element||elements.size()>=4096||elements.count(element))return false;try{elements.emplace(element,MaterialElementBindingCandidate{});return true;}catch(...){return false;}}
 bool setName(uint32_t element,const std::string&name,int phase=0){
  if(busy)return false;auto i=elements.find(element);if(i==elements.end())return false;
  try{auto plan=i->second;if(!setMaterialElementNameCandidate(plan,name,phase))return false;i->second=std::move(plan);return true;}catch(...){return false;}
 }
 // Explicit DataModel metadata boundary, not automatic dirty clearing/undo.
 bool setNameAttributeFlags(uint32_t element,uint16_t flags){if(busy||(flags&31)!=5)return false;auto i=elements.find(element);if(i==elements.end())return false;i->second.nameFlags=flags;return true;}
 bool clearDirty(uint32_t element){if(busy)return false;auto i=elements.find(element);if(i==elements.end())return false;i->second.nameFlags=uint16_t(i->second.nameFlags&~0x80u);return true;}
 bool resolve(uint32_t element,bool materialSystemAvailable=true){
  if(busy)return false;auto i=elements.find(element);if(i==elements.end())return false;
  try{auto pending=guard();Services services(*this);resolveMaterialElementBindingCandidate(i->second,materialSystemAvailable,services);return true;}catch(...){return false;}
 }
 std::optional<Id>acquire(uint32_t element){if(busy)return {};try{auto pending=guard();return acquireInside(element);}catch(...){return {};}}
 bool erase(uint32_t element){
  if(busy)return false;auto i=elements.find(element);if(i==elements.end())return false;auto id=i->second.binding.resource;
  if(id&&(!releasable(id)||!store.decrement(id)))return false;if(id)++counts.releases;elements.erase(i);return true;
 }
 bool withCollection(const std::vector<Source>&source,const std::function<bool(const Store::SessionCollectionView&)>&observer,int phase=0,const Store::Permit&permit={},Attempt*attempt=nullptr){
  if(attempt)*attempt={};if(busy||!observer||source.size()>4096)return false;
  try{
   auto pending=guard();auto input=source;const auto use=observer;const auto policy=permit;
   std::vector<Store::SessionEntry>entries(input.size());for(size_t i=0;i<input.size();++i)entries[i].rows=std::move(input[i].rows);
   for(size_t i=0;i<input.size();++i){
    // An explicit element handle0 is the known unresolved/skipped handle. A
    // missing positive definition is unavailable, not a fabricated native skip.
    auto id=input[i].element?acquireInside(input[i].element):std::optional<Id>(0);
    if(!id){if(attempt)attempt->status=Status::UnavailableBinding;return false;}
    entries[i].material=id;if(attempt)++attempt->acquiredPositions;
   }
   const bool ok=store.withSessionCollection(entries,use,phase,policy);
   if(attempt)attempt->status=ok?Status::Observed:Status::ValuesRefused;return ok;
  }catch(...){return false;}
 }
};
