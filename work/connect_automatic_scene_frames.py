from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text(encoding='utf-8');anchor='#include "scene_tick_sequence_candidate.h"';assert anchor in s;s=s.replace(anchor,anchor,1)
# Graph is available only after the live session provider includes its declarations.
anchor=' std::vector<int32_t>sceneTicks;';assert anchor in s;s=s.replace(anchor,' SceneFrameSourceCandidate sceneTicks;',1)
anchor=' std::vector<std::string>sceneSessionFiles;';assert anchor in s
auto=' char automaticText[8]{};auto automaticSize=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_SESSION_TIMELINE",automaticText,sizeof(automaticText));if(automaticSize){if(automaticSize!=1||automaticText[0]!=\'1\'||sceneTickPathSize||!frozenScene||hasFrameInputs||cameraPathSize||posePathSize||modelMatrixSize||!timelineBlanks)throw std::runtime_error("automatic session timeline conflict");char path[32768]{};auto length=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION",path,sizeof(path));if(!length||length>=sizeof(path))throw std::runtime_error("automatic timeline session path");auto graph=loadGraph(path);sceneTicks.setTimeline(sessionTimelineFramesCandidate(graph,sessionRenderRootCandidate(graph,explicitSceneRoot)));int32_t first=0;std::from_chars(frozenTickText,frozenTickText+frozenTickSize,first);if(sceneTicks.front()!=first)throw std::runtime_error("automatic timeline initial tick mismatch");frameCount=sceneTicks.size();hasFrameInputs=true;std::cout<<"scene_frame_source=saved_timeline count="<<frameCount<<" tick_array_bytes=0\\n";}\n'
s=s.replace(anchor,auto+anchor,1)
# Put new include after retained scene header, where Graph and time utilities exist.
anchor='#include "retained_session_scene_candidate.h"';assert anchor in s;s=s.replace(anchor,anchor+'\n#include "scene_frame_source_candidate.h"',1)
p.write_text(s,encoding='utf-8')
