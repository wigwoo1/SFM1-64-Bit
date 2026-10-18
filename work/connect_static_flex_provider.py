from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h')
s=p.read_text(encoding='utf-8')
s=s.replace('#include "retained_session_scene_candidate.h"','#include "retained_session_scene_candidate.h"\n#include "retained_static_flex_provider_candidate.h"',1)
needle='std::unique_ptr<SessionTimelineDocumentCandidate>editorDocument;'
replacement='char staticFlexText[8]{};auto staticFlexSize=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_STATIC_FLEX",staticFlexText,sizeof(staticFlexText));if(staticFlexSize&&(staticFlexSize!=1||(staticFlexText[0]!=\'0\'&&staticFlexText[0]!=\'1\')))throw std::runtime_error("static flex option must be 0 or 1");const bool staticFlex=staticFlexSize&&staticFlexText[0]==\'1\';if(staticFlex&&!sceneRequested)throw std::runtime_error("static flex requires scene");'+needle
assert s.count(needle)==1
s=s.replace(needle,replacement,1)
needle='frozenSceneOwner=std::make_unique<RetainedSessionSceneCandidate>(std::move(sceneGraph),sceneRoot,materialMounts,geometryWorkers,true,true);'
replacement=needle+'if(staticFlex){auto provider=std::make_shared<RetainedStaticFlexProviderCandidate>(materialMounts);frozenSceneOwner->setModelDeformationCandidate([provider](const Graph&graph,const SessionRenderModelCandidate&instance,const SfmModelGeometryCandidate&value){return provider->sample(graph,instance,value);});std::cout<<"scene_static_flex=1 temporal_policy=identical_descriptor_arrays native_parity=unverified\\n";}'
assert s.count(needle)==1
s=s.replace(needle,replacement,1)
p.write_text(s,encoding='utf-8')
