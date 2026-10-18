from pathlib import Path
p=Path('work/sfm_model_geometry.h');s=p.read_text().replace('#include "model_material_metadata.h"','#include "model_material_metadata.h"\n#include "model_eyeball_metadata_candidate.h"').replace('std::vector<uint32_t>bodyModelCounts,bodyBases;', 'std::vector<uint32_t>bodyModelCounts,bodyBases;std::vector<ModelEyeballCandidate>eyeballs;')
s=s.replace('size_t model=models.first+size_t(mi)*148;', 'size_t model=models.first+size_t(mi)*148;auto eyes=table(model,100,104,172);for(uint32_t eye=0;eye<eyes.second;++eye)result.eyeballs.push_back(readModelEyeballCandidate(mdl,eyes.first+size_t(eye)*172,bi,mi,result.bones.size()));')
p.write_text(s)
