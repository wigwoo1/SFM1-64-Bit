from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h')
s=p.read_text()
s=s.replace('#pragma once','#pragma once\n#include "model_render_inputs_candidate.h"',1)
old='std::vector<uint32_t>indices;for(auto&m:model.meshes)for(auto&t:m.triangles)for(auto v:t)indices.push_back(v);'
new='auto mount=std::filesystem::path(base).parent_path();while(!mount.empty()&&vmtAsciiLowerCandidate(mount.filename().string())!="models"){auto parent=mount.parent_path();if(parent==mount)break;mount=parent;}if(vmtAsciiLowerCandidate(mount.filename().string())!="models")throw std::runtime_error("loose model mount unavailable");auto renderInputs=prepareModelRenderInputsCandidate(model,0,{mount.parent_path()},2);const auto&indices=renderInputs.indices;'
assert old in s
s=s.replace(old,new)
old='draw(command,uint32_t(indices.size()),1,0,0,0);'
assert old in s
s=s.replace(old,'for(const auto&range:renderInputs.ranges)draw(command,range.indexCount,1,range.firstIndex,0,0);')
p.write_text(s)
