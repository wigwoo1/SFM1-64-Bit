#pragma once
#include <array>
#include <cstdint>
// Logical x64 representation of callback1001f8a0, not the native 24-byte ABI.
// Valid borrowed records only: negative native sentinels remain unresolved.
struct StudioVertexTextureRecordCandidate {
 std::array<uint32_t,6> sampler{}; // Original DWORD offsets0..20; offset8 unused here.
 uint8_t resourceCount=1,activeResource=0;
 uintptr_t resource=0; // Direct resource or borrowed uintptr_t array when count!=1.
};
struct StudioVertexTextureCacheCandidate {
 StudioVertexTextureRecordCandidate* record=nullptr;
 std::array<uint32_t,6> sampler{};
};
template<class Ops>void studioVertexTextureSubmitCandidate(
 StudioVertexTextureRecordCandidate* const* current,StudioVertexTextureCacheCandidate* cache,
 int32_t count,uint32_t force,Ops& ops){
 constexpr unsigned fields[]={0,1,4,3,5};
 constexpr uint32_t types[]={1,2,6,5,7};
 for(int32_t i=0;i<count;++i){
  const uint32_t stage=257u+uint32_t(i);
  auto* const record=current[i];
  if(uint8_t(force)||cache[i].record!=record){
   cache[i].record=record;
   uintptr_t resource=0;
   if(record)resource=record->resourceCount==1?record->resource:
    reinterpret_cast<const uintptr_t*>(record->resource)[record->activeResource];
   if(ops.queueEnabled())ops.queueTexture(stage,resource);else ops.directTexture(stage,resource);
  }
  if(!record)continue;
  for(unsigned j=0;j<5;++j){
   const unsigned field=fields[j];
   const uint32_t value=record->sampler[field];
   if(!uint8_t(force)&&cache[i].sampler[field]==value)continue;
   cache[i].sampler[field]=value;
   if(ops.queueEnabled())ops.queueSampler(stage,types[j],value);else ops.directSampler(stage,types[j],value);
  }
 }
}
