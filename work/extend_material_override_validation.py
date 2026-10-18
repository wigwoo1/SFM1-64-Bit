from pathlib import Path
p=Path('work/test_session_material_overrides.cpp')
s=p.read_text().replace('#include "session_material_overrides_candidate.h"','#include "session_material_overrides_candidate.h"\n#include "apply_session_material_overrides_candidate.h"\n#include "mounted_model_material_candidate.h"')
needle='auto key=sessionMaterialOverrideKeyCandidate(records);'
insert='''for(const auto&record:records){auto found=record.parameters.find("$basetexture");if(found==record.parameters.end())continue;auto syntax=parseVmtCandidate("VertexLitGeneric { \\"$basetexture\\" \\"old\\" }");auto applied=applySessionMaterialOverridesCandidate(syntax,record);auto values=vmtValuesCandidate(applied,"$basetexture");if(values.size()!=1||values[0]!=found->second.values[0].text)throw std::runtime_error("base texture override application");std::vector<std::filesystem::path>mounts{R"(D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\usermod)"};auto texture=findMountedMaterialCandidate(mounts,materialRelativePathCandidate(values[0],".vtf"));if(texture.empty())throw std::runtime_error("saved override texture missing");std::cout<<"override_texture="<<texture.string()<<"\\n";}'''
if s.count(needle)!=1:raise RuntimeError('key marker')
s=s.replace(needle,insert+needle).replace('scoped_binding=1','override_merge=1 override_texture_lookup=1 scoped_binding=1')
p.write_text(s)
