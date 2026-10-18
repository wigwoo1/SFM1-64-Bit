from pathlib import Path
p=Path('work/translucent_range_order_candidate.h');s=p.read_text().replace('template<class Vertices,class Depth>','template<class Inputs,class Vertices,class Depth>').replace('const ModelRenderInputsCandidate&inputs','const Inputs&inputs');p.write_text(s)
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text().replace('struct PreparedDrawFrameCandidate {std::vector<float>materialAlpha;','struct PreparedDrawFrameCandidate {std::vector<bool>translucentMaterials;std::vector<float>materialAlpha;')
s=s.replace('if(result.materialAlpha[n]!=1&&!renderInputs.materials[n].translucent)throw std::runtime_error("opaque material alpha pipeline selection unfinished");','result.translucentMaterials.push_back(renderInputs.materials[n].translucent||result.materialAlpha[n]<1);')
old='result.transparentOrder=translucentRangeOrderCandidate(renderInputs,result.vertices,'
new='struct SortMaterial{bool translucent;};struct SortInputs{const std::vector<uint32_t>&indices;const std::vector<ModelRenderRangeCandidate>&ranges;std::vector<SortMaterial>materials;}sortInputs{renderInputs.indices,renderInputs.ranges,{}};for(bool value:result.translucentMaterials)sortInputs.materials.push_back({value});result.transparentOrder=translucentRangeOrderCandidate(sortInputs,result.vertices,'
if s.count(old)!=1:raise RuntimeError('frame sorting marker')
s=s.replace(old,new)
s=s.replace('std::vector<float>currentMaterialAlpha(renderInputs.materials.size(),1.0f);','std::vector<bool>currentTranslucentMaterials;for(const auto&material:renderInputs.materials)currentTranslucentMaterials.push_back(material.translucent);std::vector<float>currentMaterialAlpha(renderInputs.materials.size(),1.0f);')
s=s.replace('currentMaterialAlpha=std::move(prepared.materialAlpha);','currentMaterialAlpha=std::move(prepared.materialAlpha);currentTranslucentMaterials=std::move(prepared.translucentMaterials);')
s=s.replace('!renderInputs.materials.at(renderInputs.ranges[n].materialIndex).translucent','!currentTranslucentMaterials.at(renderInputs.ranges[n].materialIndex)')
p.write_text(s)
