from pathlib import Path
root=Path(__file__).parent
s=(root/'session_fresh_flex_pass.h').read_text()
start=s.index(' auto start=std::chrono::steady_clock::now();')
end=s.index(' auto writerGroups=')
prepare=s[start:end]
prepare=prepare.replace('DiskModelMetadataCacheCandidate cache;RecoveredGlobalFlexRegistryCandidate registry;std::vector<uint16_t>handles;','')
prepare=prepare.replace('auto plan=discoverOwnedGlobalFlexBindingsCandidate','plan=discoverOwnedGlobalFlexBindingsCandidate',1)
prefix='''#pragma once
#include "session_fresh_flex_pass.h"
// Caller retains projection and invalidates/rebuilds on model/reference/name/
// metadata/structure changes. Native cache eviction/notification policy pending.
template<class Projection>class PreparedOwnedGlobalFlexPassCandidate {
 Projection&projection;DiskModelMetadataCacheCandidate cache;
 RecoveredGlobalFlexRegistryCandidate registry;std::vector<uint16_t>handles;
 OwnedGlobalFlexBindingPlanCandidate plan;bool valid=true;
 double preparationMilliseconds_=0;
public:
 PreparedOwnedGlobalFlexPassCandidate(Projection&value,const std::filesystem::path&contentRoot):projection(value){
'''
suffix='''
 validateFlexWeightWriterGroupsCandidate(projection,plan.bindings);
 preparationMilliseconds_=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
 }
 ~PreparedOwnedGlobalFlexPassCandidate(){for(auto handle:handles)cache.release(handle);}
 void invalidate(){valid=false;}
 double preparationMilliseconds()const{return preparationMilliseconds_;}
 SessionFreshFlexPassResultCandidate step(int phase=0){
 if(!valid)throw std::runtime_error("prepared flex invalidated; rebuild required");
 auto start=std::chrono::steady_clock::now();auto groups=validateFlexWeightWriterGroupsCandidate(projection,plan.bindings);
 SessionFreshFlexPassResultCandidate result{plan.bindings.size(),registry.names().size(),0,0};result.sharedSlots=groups.sharedSlots;result.sharedWriters=groups.sharedWriters;
 for(const auto&binding:plan.bindings){auto r=executeOwnedGlobalFlexCandidate(projection,binding,phase);if(r.dispatch!=RecoveredFlexExecuteDispatchCandidate::Array)throw std::runtime_error("prepared flex lost destination");if(r.changed)++result.changed;else ++result.equal;}
 result.executionMilliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();return result;
 }
};
'''
p=root/'prepared_owned_global_flex_pass.h';assert not p.exists();p.write_text(prefix+prepare+suffix)
