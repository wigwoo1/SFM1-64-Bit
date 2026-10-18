from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
old='if(!sceneTicks.empty()){auto previousIndices=renderInputs.indices;double scenePreparationMs=0;'
new='if(!sceneTicks.empty()){if(pending)throw std::runtime_error("scene geometry update while GPU submission pending");bool sceneIndexChanged=false;double scenePreparationMs=0;'
assert old in s;s=s.replace(old,new,1)
old='prepareStart=FrameClock::now();*frozenScene=std::move(next);'
new='prepareStart=FrameClock::now();sceneIndexChanged=next.upload.indices!=indices;*frozenScene=std::move(next);'
assert old in s;s=s.replace(old,new,1)
old='renderInputs.indices=frozenScene->upload.indices;materialScene.materials.clear();'
new='if(sceneIndexChanged)renderInputs.indices=frozenScene->upload.indices;materialScene.materials.clear();'
assert old in s;s=s.replace(old,new,1)
old='bool indexUpload=previousIndices!=indices;'
assert old in s;s=s.replace(old,'bool indexUpload=sceneIndexChanged;',1)
p.write_text(s)
