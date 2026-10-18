from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text();s='#include "session_render_root_candidate.h"\n'+s
# Graph must precede this helper's include; the live provider already owns it.
s=s.replace('#include "session_render_root_candidate.h"\n','',1)
s=s.replace('#include "retained_session_scene_candidate.h"','#include "retained_session_scene_candidate.h"\n#include "session_render_root_candidate.h"',1)
old='}std::unique_ptr<RetainedSessionSceneCandidate>frozenSceneOwner;'
new='}std::optional<uint32_t>explicitSceneRoot;char rootText[32]{};auto rootSize=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_SCENE_ROOT",rootText,sizeof(rootText));if(rootSize){if(!sceneRequested||rootSize>=sizeof(rootText))throw std::runtime_error("scene root option conflict or length");uint32_t root=0;auto parsed=std::from_chars(rootText,rootText+rootSize,root);if(parsed.ec!=std::errc{}||parsed.ptr!=rootText+rootSize)throw std::runtime_error("scene root invalid");explicitSceneRoot=root;}std::unique_ptr<RetainedSessionSceneCandidate>frozenSceneOwner;'
assert old in s;s=s.replace(old,new,1)
old='frozenSceneOwner=std::make_unique<RetainedSessionSceneCandidate>(loadGraph(sessionPath),2237,materialMounts,geometryWorkers,true,true);'
new='auto sceneGraph=loadGraph(sessionPath);auto sceneRoot=sessionRenderRootCandidate(sceneGraph,explicitSceneRoot);std::cout<<"scene_root_source="<<(explicitSceneRoot?"explicit":"activeClip")<<" root="<<sceneRoot<<"\\n";frozenSceneOwner=std::make_unique<RetainedSessionSceneCandidate>(std::move(sceneGraph),sceneRoot,materialMounts,geometryWorkers,true,true);'
assert old in s;s=s.replace(old,new,1)
old='if(!sceneSessionFiles.empty()&&frame&&sceneSessionFiles.at(frame)!=sceneSessionFiles.at(frame-1))frozenSceneOwner->replaceSession(loadGraph(sceneSessionFiles.at(frame).c_str()),2237);'
new='if(!sceneSessionFiles.empty()&&frame&&sceneSessionFiles.at(frame)!=sceneSessionFiles.at(frame-1)){auto graph=loadGraph(sceneSessionFiles.at(frame).c_str());auto root=sessionRenderRootCandidate(graph,explicitSceneRoot);frozenSceneOwner->replaceSession(std::move(graph),root);}'
assert old in s;s=s.replace(old,new,1);p.write_text(s)
