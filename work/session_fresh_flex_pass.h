#pragma once
#include "owned_global_flex_binding_plan.h"
#include "disk_model_metadata_cache.h"
#include "flex_weight_writer_groups.h"
#include <chrono>
struct SessionFreshFlexPassResultCandidate{size_t bindings=0,names=0,changed=0,equal=0,sharedSlots=0,sharedWriters=0;double discoveryMilliseconds=0,executionMilliseconds=0;};
// Isolated current-value pass. Owns fresh registry/cache together; no timeline.
template<class Projection>
SessionFreshFlexPassResultCandidate executeSessionFreshFlexPassCandidate(Projection&projection,const std::filesystem::path&contentRoot){
 auto start=std::chrono::steady_clock::now();DiskModelMetadataCacheCandidate cache;RecoveredGlobalFlexRegistryCandidate registry;std::vector<uint16_t>handles;auto root=std::filesystem::weakly_canonical(contentRoot);
 auto plan=discoverOwnedGlobalFlexBindingsCandidate(projection,registry,[&](uint32_t model)->std::vector<MdlFlexControllerMetadata>*{
 const auto&e=projection.graph.elements.at(model);for(const auto&a:e.attributes)if(projection.graph.strings.at(a.name)=="modelName"&&a.type==5&&a.values.size()==1){
 std::filesystem::path relative=a.values[0].text;if(relative.is_absolute())throw std::runtime_error("absolute model asset");for(const auto&part:relative)if(part=="..")throw std::runtime_error("model traversal");auto path=std::filesystem::weakly_canonical(root/relative);auto inside=path.lexically_relative(root);if(inside.empty()||*inside.begin()=="..")throw std::runtime_error("model outside content root");auto h=cache.acquire(path);if(h!=0xffff)handles.push_back(h);return cache.controllers(h);
 }return nullptr;});
 if(plan.missingMetadata||plan.missingController||plan.outOfRange)throw std::runtime_error("fresh flex discovery incomplete or requires unsupported weight growth");
 auto writerGroups=validateFlexWeightWriterGroupsCandidate(projection,plan.bindings);
 SessionFreshFlexPassResultCandidate result{plan.bindings.size(),registry.names().size(),0,0};
 result.sharedSlots=writerGroups.sharedSlots;result.sharedWriters=writerGroups.sharedWriters;
 auto executionStart=std::chrono::steady_clock::now();result.discoveryMilliseconds=std::chrono::duration<double,std::milli>(executionStart-start).count();
 for(const auto&binding:plan.bindings){auto r=executeOwnedGlobalFlexCandidate(projection,binding,0);if(r.dispatch!=RecoveredFlexExecuteDispatchCandidate::Array)throw std::runtime_error("fresh flex binding lost destination");if(r.changed)++result.changed;else ++result.equal;}
 auto executionEnd=std::chrono::steady_clock::now();result.executionMilliseconds=std::chrono::duration<double,std::milli>(executionEnd-executionStart).count();for(auto handle:handles)cache.release(handle);return result;
}
