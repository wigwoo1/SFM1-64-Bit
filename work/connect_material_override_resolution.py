from pathlib import Path
p=Path('work/mounted_model_material_candidate.h')
s=p.read_text().replace('#include <fstream>','#include <fstream>\n#include <functional>')
old='const std::vector<std::filesystem::path>&mounts){\n const auto&name=metadata.resolve'
new='const std::vector<std::filesystem::path>&mounts,const std::function<void(const std::filesystem::path&,VmtNodeCandidate&)>&applyOverrides={}){\n const auto&name=metadata.resolve'
if s.count(old)!=1:raise RuntimeError('resolver signature')
s=s.replace(old,new).replace('result.syntax=parseVmtCandidate(source);','result.syntax=parseVmtCandidate(source);if(applyOverrides)applyOverrides(result.vmt,result.syntax);')
p.write_text(s)
p=Path('work/model_render_inputs_candidate.h')
s=p.read_text().replace('const std::vector<uint32_t>&bodySelection={}){','const std::vector<uint32_t>&bodySelection={},const std::function<void(const std::filesystem::path&,VmtNodeCandidate&)>&applyOverrides={}){')
s=s.replace('resolveMountedModelMaterialCandidate(model.materials,skin,mesh.material,mounts)','resolveMountedModelMaterialCandidate(model.materials,skin,mesh.material,mounts,applyOverrides)')
p.write_text(s)
p=Path('work/test_session_material_overrides.cpp')
s=p.read_text()
needle='++resolved;std::cout<<"override_texture="'
insert='''++resolved;ModelMaterialMetadataCandidate metadata;metadata.names={record.material};metadata.directories={""};metadata.skins={{0}};auto loaded=resolveMountedModelMaterialCandidate(metadata,0,0,mounts,[&](const auto&,auto&syntax){syntax=applySessionMaterialOverridesCandidate(std::move(syntax),record);});if(loaded.baseTexture!=texture)throw std::runtime_error("resolver ignored override");std::cout<<"override_texture="'''
if s.count(needle)!=1:raise RuntimeError('test insertion')
s=s.replace(needle,insert).replace('override_merge=1','resolver_applies_before_lookup=1 override_merge=1')
p.write_text(s)
