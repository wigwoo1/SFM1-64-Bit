from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h')
s=p.read_text()
s='#include "vulkan_scene_commands_candidate.h"\n'+s
start='for(unsigned phase=0;modelVisible&&phase<2;++phase)'
prefix='''if(!eyeCandidate){struct SceneView{const std::vector<Vertex>&vertices;const std::vector<uint32_t>&indices;const std::vector<MaterialAddress>&materials;std::vector<SceneGpuDrawCandidate>opaque,translucent;}sceneView{vertices,indices,materialScene.materials,{},{}};auto appendDraw=[&](size_t n,auto&target){const auto&r=renderInputs.ranges.at(n);target.push_back({0,n,r.firstIndex,r.indexCount,r.materialIndex,r.materialIndex});};if(modelVisible){for(size_t n=0;n<renderInputs.ranges.size();++n)if(!renderInputs.materials.at(renderInputs.ranges[n].materialIndex).translucent)appendDraw(n,sceneView.opaque);for(auto n:transparentOrder)appendDraw(n,sceneView.translucent);}std::vector<VkDescriptorSet>sets;sets.reserve(materialImages.entries.size());for(const auto&entry:materialImages.entries)sets.push_back(entry.set);recordVulkanSceneCommandsCandidate({bindPipeline,bindDescriptors,bindVertices,bindIndices,draw},command,sceneView,vertexBuffer,indexBuffer,pipeline,translucentPipeline,layout,sets);if(!iteration)std::cout<<"scene_command_recorder=active models=1\\n";}else{'''
if s.count(start)!=1 or s.count('}}endPass(command);')!=1:raise RuntimeError('command block not unique')
s=s.replace(start,prefix+start).replace('}}endPass(command);','}}}endPass(command);')
p.write_text(s)
