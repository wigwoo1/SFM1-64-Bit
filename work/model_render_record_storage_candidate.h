#pragma once
#include "model_render_record_grow_candidate.h"
#include <array>
#include <vector>
#include <stdexcept>
// Owned x64 storage: original byte-schema payloads and logical links are
// separate. Pointer-bearing native fields must be decoded before runtime use.
class ModelRenderRecordStorageCandidate {
 std::vector<std::array<unsigned char,264>> records_;
 std::vector<EngineShadowPoolLinkCandidate> links_;
public:
 void grow(ModelRenderRecordAllocationCandidate& state,int32_t growth,int32_t additional){
  modelRenderGrowCandidate(state.pool,growth,additional,[&](auto* old,int32_t count){
   if(old!=links_.data()&&!(links_.empty()&&old==nullptr))throw std::logic_error("foreign model storage");
   // Prepare both replacement buffers before publishing either one.
   auto records=records_;auto links=links_;records.resize(count);links.resize(count);
   records_.swap(records);links_.swap(links);return links_.data();
  });
  state.cachedStorage=state.pool.links;
 }
 unsigned char* record(uint16_t index){return records_.at(index).data();}
 size_t capacity()const{return records_.size();}
};
