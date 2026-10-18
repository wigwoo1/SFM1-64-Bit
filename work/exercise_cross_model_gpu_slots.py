from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
old='if(!eyeCandidate){struct SceneView'
new='if(!eyeCandidate){auto commandMaterialAddresses=materialScene.materials;if(blendMode==6)for(size_t n=0;n<commandMaterialAddresses.size();++n)commandMaterialAddresses[n]={n,0};struct SceneView'
if s.count(old)!=1:raise RuntimeError('scene command marker')
s=s.replace(old,new).replace('sceneView{vertices,indices,materialScene.materials,{},{}}','sceneView{vertices,indices,commandMaterialAddresses,{},{}}')
old='target.push_back({0,n,r.firstIndex,r.indexCount,r.materialIndex,r.materialIndex});'
new='const auto&address=commandMaterialAddresses.at(r.materialIndex);target.push_back({address.model,n,r.firstIndex,r.indexCount,address.material,r.materialIndex});'
if s.count(old)!=1:raise RuntimeError('draw address marker')
s=s.replace(old,new).replace('scene_command_recorder=active models=1\\n','scene_command_recorder=active cross_model_slots="<<(blendMode==6?3:1)<<" geometry_source="<<(blendMode==6?"controlled_layers":"single_asset")<<"\\n')
p.write_text(s)
