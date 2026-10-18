from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h')
s=p.read_text()
old='materialImages.create(renderInputs.materials,materialLayout);'
new='''struct MaterialAddress{size_t model;uint32_t material;};struct MaterialScene{std::vector<MaterialAddress>materials;}materialScene;for(size_t n=0;n<renderInputs.materials.size();++n){if(n>UINT32_MAX)throw std::runtime_error("scene material address overflow");materialScene.materials.push_back({0,uint32_t(n)});}struct SingleModelInputsView{const ModelRenderInputsCandidate&input;const ModelRenderInputsCandidate&at(size_t model)const{if(model)throw std::runtime_error("single model material address");return input;}}materialInputs{renderInputs};materialImages.createScene(materialScene,materialInputs,materialLayout);std::cout<<"material_upload_addressing=scene slots="<<materialScene.materials.size()<<" models=1\\n";'''
if s.count(old)!=1:raise RuntimeError('material call not unique')
p.write_text(s.replace(old,new))
