from pathlib import Path
p=Path('work/vulkan_material_images_candidate.h')
s=p.read_text()
old=' void create(const std::vector<ModelRenderMaterialCandidate>&materials,VkDescriptorSetLayout layout){\n  entries.resize(materials.size());'
new=''' void create(const std::vector<ModelRenderMaterialCandidate>&materials,VkDescriptorSetLayout layout){
  createFrom(materials.size(),[&](size_t n)->const ModelRenderMaterialCandidate&{return materials.at(n);},layout);
 }
 template<class Scene,class Inputs>
 void createScene(const Scene&scene,const Inputs&inputs,VkDescriptorSetLayout layout){
  createFrom(scene.materials.size(),[&](size_t n)->const ModelRenderMaterialCandidate&{const auto&address=scene.materials.at(n);return inputs.at(address.model).materials.at(address.material);},layout);
 }
 template<class Material>
 void createFrom(size_t materialCount,Material material,VkDescriptorSetLayout layout){
  if(pool||sampler||!entries.empty())throw std::runtime_error("material images already created");
  if(materialCount>UINT32_MAX)throw std::runtime_error("material image count overflow");
  for(size_t n=0;n<materialCount;++n){const auto&image=material(n).image;if(!image.width||!image.height||uint64_t(image.width)*image.height>SIZE_MAX/4||image.rgba.size()!=uint64_t(image.width)*image.height*4)throw std::runtime_error("material image dimensions or bytes");}
  if(!materialCount)return;
  entries.resize(materialCount);'''
if old not in s:raise RuntimeError('create signature not found')
s=s.replace(old,new).replace('auto&image=materials[n].image;','const auto&image=material(n).image;')
p.write_text(s)
