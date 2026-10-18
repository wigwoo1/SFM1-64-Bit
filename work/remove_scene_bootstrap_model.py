from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
old='auto model=readSfmModelGeometryCandidate(read(base+".mdl"),read(base+".vvd"),read(base+".dx90.vtx"));'
new='bool sceneRequested=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK",nullptr,0)!=0;SfmModelGeometryCandidate model;if(!sceneRequested)model=readSfmModelGeometryCandidate(read(base+".mdl"),read(base+".vvd"),read(base+".dx90.vtx"));else std::cout<<"scene_bootstrap_model_loaded=0\\n";'
assert old in s;s=s.replace(old,new,1)
old='auto mount=std::filesystem::path(base).parent_path();while('
new='char contentDirectory[32768]{};auto contentSize=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_CONTENT_DIRECTORY",contentDirectory,sizeof(contentDirectory));if(contentSize>=sizeof(contentDirectory))throw std::runtime_error("content directory length");if(contentSize&&!sceneRequested)throw std::runtime_error("content directory option requires scene");auto mount=contentSize?std::filesystem::path(contentDirectory)/"models":std::filesystem::path(base).parent_path();while('
assert old in s;s=s.replace(old,new,1)
old='auto renderInputs=prepareModelRenderInputsCandidate(model,selectedSkin,materialMounts,2,modelBodySelectionCandidate(selectedBody,model.bodyModelCounts,model.bodyBases),frameProvider.applyMaterialOverrides);'
new='ModelRenderInputsCandidate renderInputs;if(!sceneRequested)renderInputs=prepareModelRenderInputsCandidate(model,selectedSkin,materialMounts,2,modelBodySelectionCandidate(selectedBody,model.bodyModelCounts,model.bodyBases),frameProvider.applyMaterialOverrides);'
assert old in s;s=s.replace(old,new,1);p.write_text(s)
p=Path('work/vulkan_model_swapchain_probe_candidate.h');s=p.read_text()
old='if(!size)return;if(size>=sizeof(base))'
new='if(!size&&!GetEnvironmentVariableA("SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK",nullptr,0))return;if(size>=sizeof(base))'
assert old in s;s=s.replace(old,new,1);p.write_text(s)
