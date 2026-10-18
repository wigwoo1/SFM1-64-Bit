#pragma once
#include "studio_vertex_texture_submit_candidate.h"
struct StudioTextureBindingRecordCandidate:StudioVertexTextureRecordCandidate {
 uint32_t* usageCounter=nullptr;
 uint32_t bytes=0,epoch=0,maximumUses=0,uses=0;
};
struct StudioTextureBindingOwnerCandidate {uint32_t bytes=0,epoch=0;uint8_t suppressUseIncrement=0;};
struct StudioTextureBindingStageCandidate {
 StudioTextureBindingRecordCandidate* record=nullptr;
 std::array<uint32_t,6> sampler{};
 uint8_t enabled=0;
};
// Unsigned arithmetic preserves native wrapping; xor maps signed comparison order.
inline bool studioTextureSignedGreaterCandidate(uint32_t a,uint32_t b){return (a^0x80000000u)>(b^0x80000000u);}
template<class Ops>void studioTextureBindingCandidate(uint32_t stage,StudioTextureBindingRecordCandidate* record,uint32_t force,
 StudioTextureBindingStageCandidate& cache,StudioTextureBindingOwnerCandidate& owner,Ops& ops){
 if(cache.record==record&&!uint8_t(force))return;
 if(record){
  const uint32_t budget=ops.budget();
  if(!(budget&0x80000000u)&&record->epoch!=owner.epoch&&studioTextureSignedGreaterCandidate(owner.bytes+record->bytes,budget<<10)){
   ops.texture(stage,0);return;
  }
 }else{ops.texture(stage,0);return;}
 cache.record=record;
 if(!cache.enabled)return;
 const uintptr_t resource=record->resourceCount==1?record->resource:
  reinterpret_cast<const uintptr_t*>(record->resource)[record->activeResource];
 ops.texture(stage,resource);
 if(record->epoch!=owner.epoch){
  record->epoch=owner.epoch;record->uses=0;
  if(record->usageCounter)*record->usageCounter+=record->bytes;
  owner.bytes+=record->bytes;
 }
 if(!owner.suppressUseIncrement)++record->uses;
 if(studioTextureSignedGreaterCandidate(record->uses,record->maximumUses))record->maximumUses=record->uses;
 auto sampler=[&](unsigned field,uint32_t type,uint32_t value){
  if(cache.sampler[field]==value)return;
  cache.sampler[field]=value;ops.sampler(stage,type,value);
 };
 sampler(0,1,record->sampler[0]);sampler(1,2,record->sampler[1]);sampler(2,3,record->sampler[2]);
 const bool disableFiltering=!ops.filteringEnabled();
 sampler(4,6,disableFiltering?0:record->sampler[4]);
 sampler(3,5,disableFiltering?0:record->sampler[3]);
 const bool mipEnabled=ops.mipEnabled();
 const uint32_t mip=record->sampler[5];
 sampler(5,7,!mipEnabled?0:disableFiltering?1:mip);
}
