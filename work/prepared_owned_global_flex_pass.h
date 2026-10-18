#pragma once
#include "session_fresh_flex_pass.h"
// Caller retains projection and invalidates/rebuilds on model/reference/name/
// metadata/structure changes. Native cache eviction/notification policy pending.
template<class Projection>class PreparedOwnedGlobalFlexPassCandidate {
 Projection&projection;DiskModelMetadataCacheCandidate cache;
 RecoveredGlobalFlexRegistryCandidate registry;std::vector<uint16_t>handles;
 OwnedGlobalFlexBindingPlanCandidate plan;bool valid=true;
 const uint64_t*bindingRevision=nullptr;uint64_t bindingRevisionAtPreparation=0;
 double preparationMilliseconds_=0;
public:
 // Optional revision belongs to caller and must outlive this pass. Owner adapters
 // advance it for binding edits; payload-only changes do not require a rebuild.
 PreparedOwnedGlobalFlexPassCandidate(Projection&value,const std::filesystem::path&contentRoot,const uint64_t*revision=nullptr):projection(value),bindingRevision(revision),bindingRevisionAtPreparation(revision?*revision:0){
 auto start=std::chrono::steady_clock::now();auto root=std::filesystem::weakly_canonical(contentRoot);
 plan=discoverOwnedGlobalFlexBindingsCandidate(projection,registry,[&](uint32_t model)->std::vector<MdlFlexControllerMetadata>*{
 const auto&e=projection.graph.elements.at(model);for(const auto&a:e.attributes)if(projection.graph.strings.at(a.name)=="modelName"&&a.type==5&&a.values.size()==1){
 std::filesystem::path relative=a.values[0].text;if(relative.is_absolute())throw std::runtime_error("absolute model asset");for(const auto&part:relative)if(part=="..")throw std::runtime_error("model traversal");auto path=std::filesystem::weakly_canonical(root/relative);auto inside=path.lexically_relative(root);if(inside.empty()||*inside.begin()=="..")throw std::runtime_error("model outside content root");auto h=cache.acquire(path);if(h!=0xffff)handles.push_back(h);return cache.controllers(h);
 }return nullptr;});
 if(plan.missingMetadata||plan.missingController||plan.outOfRange)throw std::runtime_error("fresh flex discovery incomplete or requires unsupported weight growth");

 validateFlexWeightWriterGroupsCandidate(projection,plan.bindings);
 preparationMilliseconds_=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
 }
 ~PreparedOwnedGlobalFlexPassCandidate(){for(auto handle:handles)cache.release(handle);}
 void invalidate(){valid=false;}
 double preparationMilliseconds()const{return preparationMilliseconds_;}
 SessionFreshFlexPassResultCandidate step(int phase=0){
 if(!valid||(bindingRevision&&*bindingRevision!=bindingRevisionAtPreparation))throw std::runtime_error("prepared flex invalidated; rebuild required");
 auto start=std::chrono::steady_clock::now();auto groups=validateFlexWeightWriterGroupsCandidate(projection,plan.bindings);
 SessionFreshFlexPassResultCandidate result{plan.bindings.size(),registry.names().size(),0,0};result.sharedSlots=groups.sharedSlots;result.sharedWriters=groups.sharedWriters;
 for(const auto&binding:plan.bindings){auto r=executeOwnedGlobalFlexCandidate(projection,binding,phase);if(r.dispatch!=RecoveredFlexExecuteDispatchCandidate::Array)throw std::runtime_error("prepared flex lost destination");if(r.changed)++result.changed;else ++result.equal;}
 result.executionMilliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();return result;
 }
};
