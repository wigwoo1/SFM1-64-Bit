from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text().replace('#include "vulkan_prepared_scene_candidate.h"','#include "vulkan_prepared_scene_candidate.h"\n#include "retained_scene_material_gpu_cache_candidate.h"')
old='VulkanMaterialImagesCandidate materialImages(library,dev,gpu),irisImages(library,dev,gpu);'
new='RetainedSceneMaterialGpuCacheCandidate<VulkanMaterialImagesCandidate>materialCache([&](const ModelRenderInputsCandidate&input){auto images=std::make_unique<VulkanMaterialImagesCandidate>(library,dev,gpu);images->create(input.materials,materialLayout);return images;});auto&materialImages=materialCache.get(renderInputs);VulkanMaterialImagesCandidate irisImages(library,dev,gpu);'
if s.count(old)!=1:raise RuntimeError('resource declaration')
s=s.replace(old,new)
s=s.replace('materialImages.createScene(materialScene,materialInputs,materialLayout);','auto initialMaterialSets=materialCache.descriptors(materialScene,materialInputs);if(initialMaterialSets.size()!=materialScene.materials.size())throw std::runtime_error("retained GPU descriptor coverage");')
s=s.replace('material.image=std::move(iris);','material.image=iris;')
s=s.replace('materialImages.upload(command);irisImages.upload(command);','materialCache.recordUploads(command);irisImages.upload(command);')
s=s.replace('check(submit(graphics,1,&submission,frameFence));pending=true;','check(submit(graphics,1,&submission,frameFence));materialCache.submitted();pending=true;')
s=s.replace('std::cout<<"eye_candidate_enabled="','std::cout<<"retained_gpu_material_cache_entries="<<materialCache.size()<<" upload_state=submitted_same_queue\\n";std::cout<<"eye_candidate_enabled="')
p.write_text(s)
