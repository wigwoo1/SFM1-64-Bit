from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
old='for(const auto&address:frozenScene->upload.materials)renderInputs.materials.push_back(frozenScene->materialInputs.at(address.model).materials.at(address.material));'
new='for(const auto&address:frozenScene->upload.materials){const auto&source=frozenScene->materialInputs.at(address.model).materials.at(address.material);ModelRenderMaterialCandidate metadata;metadata.source=source.source;metadata.translucent=source.translucent;renderInputs.materials.push_back(std::move(metadata));}'
assert old in s;s=s.replace(old,new,1)
old='auto&materialImages=materialCache.get(renderInputs);'
assert old in s;s=s.replace(old,'',1)
old='auto initialMaterialSets=materialCache.descriptors(materialScene,materialInputs);'
new='if(frozenScene)for(size_t n=0;n<materialScene.materials.size();++n){auto a=frozenScene->upload.materials[n];materialScene.materials[n]={a.model,a.material};}auto initialMaterialSets=frozenScene?materialCache.descriptors(materialScene,frozenScene->materialInputs):materialCache.descriptors(materialScene,materialInputs);'
assert old in s;s=s.replace(old,new,1)
old='<<" models=1\\n";std::vector<ModelRenderMaterialCandidate>irisMaterials;'
new='<<" models="<<(frozenScene?frozenScene->materialInputs.size():1)<<"\\n";if(frozenScene){size_t copiedBytes=0;for(const auto&material:renderInputs.materials)copiedBytes+=material.image.rgba.size()+material.irisImage.rgba.size();if(copiedBytes)throw std::runtime_error("scene flattened texture copies remain");std::cout<<"scene_material_cache=per_model flattened_texture_bytes="<<copiedBytes<<"\\n";}std::vector<ModelRenderMaterialCandidate>irisMaterials;'
assert old in s;s=s.replace(old,new,1)
old='std::vector<VkDescriptorSet>sets;sets.reserve(materialImages.entries.size());for(const auto&entry:materialImages.entries)sets.push_back(entry.set);if(blendMode==6||frozenScene)'
new='auto sets=initialMaterialSets;if(blendMode==6)'
assert old in s;s=s.replace(old,new,1)
old='else{struct PreparedView{const SceneView&upload;const RenderCameraCandidate&camera;const std::vector<float>&materialAlpha;const SingleModelInputsView&materialInputs;}preparedView{sceneView,camera,currentMaterialAlpha,materialInputs};auto resources=sceneBridge.resources(preparedView,vertexBuffer,indexBuffer,vertices.size()*sizeof(Vertex),indices.size()*sizeof(uint32_t),pipeline,translucentPipeline,layout);sceneBridge.record(preparedView,{bindPipeline,bindDescriptors,bindVertices,bindIndices,draw},load<PFN_vkCmdPushConstants>(library,"vkCmdPushConstants"),command,resources);if(!iteration)std::cout<<"prepared_scene_gpu_bridge=active model_inputs=1\\n";}'
new='else{auto recordPrepared=[&](auto&inputs){struct PreparedView{const SceneView&upload;const RenderCameraCandidate&camera;const std::vector<float>&materialAlpha;decltype(inputs) materialInputs;}preparedView{sceneView,camera,currentMaterialAlpha,inputs};auto resources=sceneBridge.resources(preparedView,vertexBuffer,indexBuffer,vertices.size()*sizeof(Vertex),indices.size()*sizeof(uint32_t),pipeline,translucentPipeline,layout);sceneBridge.record(preparedView,{bindPipeline,bindDescriptors,bindVertices,bindIndices,draw},load<PFN_vkCmdPushConstants>(library,"vkCmdPushConstants"),command,resources);};if(frozenScene)recordPrepared(frozenScene->materialInputs);else recordPrepared(materialInputs);if(!iteration)std::cout<<"prepared_scene_gpu_bridge=active model_inputs="<<(frozenScene?frozenScene->materialInputs.size():1)<<"\\n";}'
assert old in s;s=s.replace(old,new,1)
old='auto set=materialImages.entries.at(range.materialIndex).set;'
assert old in s;s=s.replace(old,'auto set=initialMaterialSets.at(range.materialIndex);',1)
p.write_text(s)
