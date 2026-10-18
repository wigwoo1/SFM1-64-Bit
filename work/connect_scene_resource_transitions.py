from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
old='for(size_t n=0;n<next.materialInputs.size();++n)if(&next.materialInputs.at(n)!=&frozenScene->materialInputs.at(n))throw std::runtime_error("animated scene resources changed; GPU rebuild required");'
assert old in s;s=s.replace(old,'',1)
old='check(begin(command,&beginInfo));if(!iteration){sceneBridge.recordMaterialUploads(command);irisImages.upload(command);}'
new='check(begin(command,&beginInfo));if(frozenScene){initialMaterialSets=materialCache.descriptors(frozenScene->upload,frozenScene->materialInputs);sceneBridge.recordMaterialUploads(command);}else if(!iteration)sceneBridge.recordMaterialUploads(command);if(!iteration)irisImages.upload(command);'
assert old in s;s=s.replace(old,new,1)
p.write_text(s)
