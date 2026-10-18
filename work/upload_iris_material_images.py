from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
s=s.replace('VulkanMaterialImagesCandidate materialImages(library,dev,gpu);','VulkanMaterialImagesCandidate materialImages(library,dev,gpu),irisImages(library,dev,gpu);std::vector<size_t>irisMaterialIndices;')
old='materialImages.create(renderInputs.materials,materialLayout);'
new='''materialImages.create(renderInputs.materials,materialLayout);std::vector<ModelRenderMaterialCandidate>irisMaterials;for(size_t n=0;n<renderInputs.materials.size();++n){auto&iris=renderInputs.materials[n].irisImage;if(iris.rgba.empty())continue;ModelRenderMaterialCandidate material;material.image=std::move(iris);irisMaterialIndices.push_back(n);irisMaterials.push_back(std::move(material));}if(!irisMaterials.empty())irisImages.create(irisMaterials,materialLayout);std::cout<<"iris_gpu_images="<<irisMaterials.size()<<" iris_shader_sampling=unfinished\\n";'''
assert s.count(old)==1;s=s.replace(old,new).replace('if(!iteration)materialImages.upload(command);','if(!iteration){materialImages.upload(command);irisImages.upload(command);}')
p.write_text(s)
