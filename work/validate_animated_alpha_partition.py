from pathlib import Path
p=Path('work/inspect_animated_material_overrides.cpp');s=p.read_text().replace('#include <iostream>','#include <iostream>\n#include "session_material_alpha_candidate.h"')
s=s.replace('size_t events=0;','size_t events=0;std::string resourceKey;')
needle='std::map<std::string,std::string>current;'
insert='''auto partition=partitionSessionMaterialAlphaCandidate(overrides);auto key=sessionMaterialOverrideKeyCandidate(partition.resources);if(frame&&resourceKey!=key)throw std::runtime_error("texture resource override changed");resourceKey=key;if(partition.alpha.size()!=1||partition.alpha[0].material!="models/anno/mamagen/visor")throw std::runtime_error("dynamic alpha coverage");'''
if s.count(needle)!=1:raise RuntimeError('audit marker')
s=s.replace(needle,insert+needle).replace('native_shader_behavior=unverified','resource_key_stable=1 alpha_partition_checked=1 native_shader_behavior=unverified')
p.write_text(s)
