#pragma once
#include "model_render_inputs_candidate.h"
#include "material_find_cache_key_candidate.h"
#include "studio_proxy_prebind_selection_candidate.h"
#include <algorithm>
#include <set>

// Identity-only association. No borrowed parameter/snapshot pointers escape.
// A range has one MDL texture index and prebind visit, while a shared resource
// may have SEVERAL visits. This deliberately does not choose which value phase
// a native queued draw observes; per-visit values are not assumed equivalent to
// the final shared material state or a queued material snapshot.
struct ModelMaterialDrawMappingCandidate {
 struct Range {size_t range=0;uint32_t rendererMaterial=0,textureIndex=0;size_t visit=0,group=0;};
 struct Group {uintptr_t material=0;std::vector<size_t>visits,ranges;};
 uint32_t checksum=0,skinFamily=0,body=0;
 std::vector<Range>ranges;
 std::vector<Group>groups;
 std::vector<size_t>visitGroups;
 std::vector<std::vector<size_t>>rendererMaterialVisits;
 bool oneVisitPerMaterial()const noexcept{for(const auto&group:groups)if(group.visits.size()!=1)return false;return true;}
 bool oneVisitPerRendererMaterial()const noexcept{for(const auto&visits:rendererMaterialVisits)if(visits.size()!=1)return false;return true;}
};

// Logical identity from an already loaded VMT path and explicit mount profile.
// Lexical association only, never host-path authorization or native FindMaterial.
inline std::optional<std::vector<std::string>>modelRenderMaterialKeysCandidate(const ModelRenderInputsCandidate&inputs,const std::vector<std::filesystem::path>&mounts)noexcept{
 try{std::vector<std::string>keys;keys.reserve(inputs.materials.size());for(const auto&material:inputs.materials){std::string key;for(const auto&mount:mounts){auto relative=material.source.vmt.lexically_normal().lexically_relative((mount/"materials").lexically_normal());if(relative.empty()||relative.is_absolute())continue;bool outside=false;for(const auto&part:relative)if(part=="..")outside=true;if(outside)continue;if(materialFindCacheKeyCandidate(relative.generic_string(),key)&&!key.empty())break;key.clear();}if(key.empty())return {};keys.push_back(std::move(key));}return keys;}catch(...){return {};}
}

// Ordinary LOD0 mapping for caller-authenticated immutable geometry and render
// inputs. Validates selected body/skin/mesh identities, exact triangle-index
// transport, material keys, and COMPLETE unique-texture prebind visit order.
// This validates association only: shader parameters, eye semantics, queued
// lifetime, per-mesh immediate binds and GPU admission require other evidence.
inline bool modelMaterialDrawMappingCandidate(const SfmModelGeometryCandidate&geometry,uint32_t skin,uint32_t body,
 const ModelRenderInputsCandidate&inputs,const std::vector<std::string>&materialKeys,const StudioProxyPrebindPlanCandidate&selection,ModelMaterialDrawMappingCandidate&out)noexcept{
 try{
  if(skin>INT32_MAX||body>INT32_MAX||geometry.materials.skins.empty()||geometry.materials.names.size()>32768||geometry.bodyModelCounts.size()!=geometry.bodyBases.size()||geometry.bodyBases.size()>4096||geometry.meshes.size()>(1u<<20)||inputs.materials.size()>32768||inputs.materials.size()!=materialKeys.size()||inputs.indices.size()>UINT32_MAX)return false;
  for(const auto&key:materialKeys){std::string canonical;if(!materialFindCacheKeyCandidate(key,canonical)||canonical.empty()||canonical!=key)return false;}
  const auto family=skin>0&&skin<geometry.materials.skins.size()?skin:0u;const auto&remap=geometry.materials.skins[family];
  std::vector<uint32_t>models;size_t modelCount=0;for(size_t i=0;i<geometry.bodyBases.size();++i){const auto count=geometry.bodyModelCounts[i],base=geometry.bodyBases[i];if(!count||count>4096||!base||base>INT32_MAX||count>65536-modelCount)return false;modelCount+=count;models.push_back((body/base)%count);}
  using Key=std::tuple<uint32_t,uint32_t,uint32_t>;std::map<Key,const SfmModelMeshGeometryCandidate*>all,selected;
  for(const auto&mesh:geometry.meshes){if(mesh.body>=models.size()||mesh.model>=geometry.bodyModelCounts[mesh.body]||mesh.mesh>=(1u<<20)||mesh.material<0||!all.emplace(Key{mesh.body,mesh.model,mesh.mesh},&mesh).second)return false;}
  std::optional<std::pair<uint32_t,uint32_t>>part;uint32_t ordinal=0;
  for(const auto&item:all){const auto&mesh=*item.second;auto current=std::make_pair(mesh.body,mesh.model);if(!part||current!=*part){part=current;ordinal=0;}if(mesh.mesh!=ordinal++)return false;if(mesh.model==models[mesh.body])selected.emplace(item);}
  if(inputs.ranges.size()!=selected.size())return false;
  std::vector<uint32_t>textureOrder;std::set<uint32_t>seenTextures;
  for(const auto&item:selected){const auto&mesh=*item.second;if(size_t(mesh.material)>=remap.size())return false;const auto texture=remap[size_t(mesh.material)];if(texture>=geometry.materials.names.size())return false;if(seenTextures.insert(texture).second)textureOrder.push_back(texture);}
  if(textureOrder.size()!=selection.visits.size())return false;
  ModelMaterialDrawMappingCandidate next;next.checksum=geometry.checksum;next.skinFamily=family;next.body=body;next.rendererMaterialVisits.resize(inputs.materials.size());std::map<uint32_t,size_t>textureVisits;std::map<uintptr_t,size_t>resourceGroups;
  for(size_t i=0;i<textureOrder.size();++i){const auto&visit=selection.visits[i];if(!visit.textureIndex||*visit.textureIndex!=textureOrder[i]||!visit.material)return false;textureVisits.emplace(textureOrder[i],i);auto group=resourceGroups.find(visit.material);if(group==resourceGroups.end()){size_t index=next.groups.size();group=resourceGroups.emplace(visit.material,index).first;next.groups.push_back({visit.material,{},{}});}next.visitGroups.push_back(group->second);next.groups[group->second].visits.push_back(i);}
  std::set<Key>mapped;uint64_t offset=0;
  for(size_t i=0;i<inputs.ranges.size();++i){const auto&range=inputs.ranges[i];const Key key{range.body,range.model,range.mesh};auto found=selected.find(key);if(found==selected.end()||!mapped.insert(key).second||range.materialIndex>=inputs.materials.size())return false;const auto&mesh=*found->second;const auto texture=remap[size_t(mesh.material)];const auto visit=textureVisits.at(texture);const auto group=next.visitGroups[visit];
   bool identity=false;for(const auto&directory:geometry.materials.directories){auto relative=materialRelativePathCandidate(directory+geometry.materials.names[texture],".vmt");std::string canonical;if(materialFindCacheKeyCandidate(relative.generic_string(),canonical)&&canonical==materialKeys[range.materialIndex])identity=true;}if(!identity)return false;
   const uint64_t count=uint64_t(mesh.triangles.size())*3;if(count>UINT32_MAX||offset>UINT32_MAX-count||range.firstIndex!=offset||range.indexCount!=count||offset+count>inputs.indices.size())return false;
   for(const auto&triangle:mesh.triangles)for(auto vertex:triangle){if(vertex>=geometry.geometry.vertices.size()||inputs.indices[size_t(offset++)]!=vertex)return false;}
   next.ranges.push_back({i,range.materialIndex,texture,visit,group});next.groups[group].ranges.push_back(i);auto&visits=next.rendererMaterialVisits[range.materialIndex];if(std::find(visits.begin(),visits.end(),visit)==visits.end())visits.push_back(visit);
  }
  if(offset!=inputs.indices.size())return false;for(const auto&visits:next.rendererMaterialVisits)if(visits.empty())return false;
  out=std::move(next);return true;
 }catch(...){return false;}
}
