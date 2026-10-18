#pragma once
#include "studio_obj_snapshot_candidate.h"
#include "session_render_root_candidate.h"
inline constexpr const char* objSessionSnapshotNameCandidate="sfm64ImportedObjSnapshotV1";
// Prepare every allocating/validating step before committing attributes/strings.
inline void studioEmbedObjSessionCandidate(Graph&graph,const std::shared_ptr<const RetainedObjSceneCandidate>&owner){
 auto&attributes=graph.elements.at(sessionRenderRootCandidate(graph)).attributes;
 size_t existing=0;for(const auto&a:attributes)if(graph.strings.at(a.name)==objSessionSnapshotNameCandidate)++existing;
 if(existing>1)throw std::runtime_error("Duplicate session OBJ snapshots");
 std::optional<Attribute> replacement;
 if(owner){auto bytes=studioEncodeObjSnapshotCandidate(*owner);(void)studioDecodeObjSnapshotCandidate(bytes);Attribute attr{};attr.type=6;Value value;value.wire.assign(bytes.begin(),bytes.end());attr.values.push_back(std::move(value));replacement=std::move(attr);}
 std::vector<Attribute> staged;staged.reserve(attributes.size()-existing+(replacement?1:0));
 for(const auto&a:attributes)if(graph.strings.at(a.name)!=objSessionSnapshotNameCandidate)staged.push_back(a);
 if(replacement){staged.push_back(std::move(*replacement));auto name=internString(graph,objSessionSnapshotNameCandidate);staged.back().name=name;}
 attributes.swap(staged);
}
inline std::shared_ptr<const RetainedObjSceneCandidate> studioReadObjSessionCandidate(const Graph&graph){const Attribute*found=nullptr;for(const auto&a:graph.elements.at(sessionRenderRootCandidate(graph)).attributes)if(graph.strings.at(a.name)==objSessionSnapshotNameCandidate){if(found)throw std::runtime_error("Duplicate session OBJ snapshots");found=&a;}if(!found)return {};if(found->type!=6||found->values.size()!=1)throw std::runtime_error("Invalid session OBJ snapshot attribute");const auto&wire=found->values[0].wire;return studioDecodeObjSnapshotCandidate(std::vector<uint8_t>(wire.begin(),wire.end()));}
