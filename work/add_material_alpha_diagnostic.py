from pathlib import Path
p=Path('work/live_session_frame_provider_candidate.h');s=p.read_text().replace('#include "session_material_alpha_candidate.h"','#include "session_material_alpha_candidate.h"\n#include <cstdlib>')
needle='if(ticks.empty())throw std::runtime_error("live session ticks empty");'
insert='''const char*alphaOption=std::getenv("SFM64_DIAGNOSTIC_MATERIAL_ALPHA_MODE");std::string alphaMode=alphaOption?alphaOption:"live";if(alphaMode!="live"&&alphaMode!="one"&&alphaMode!="zero")throw std::runtime_error("invalid material alpha diagnostic mode");std::cout<<"material_alpha_mode="<<alphaMode<<"\\n";'''
if s.count(needle)!=1:raise RuntimeError('initial marker')
s=s.replace(needle,insert+needle).replace('initialBody,initialMaterialKey]','initialBody,initialMaterialKey,alphaMode]')
s=s.replace('state.materialAlpha.push_back({value.material,value.alpha});','state.materialAlpha.push_back({value.material,alphaMode=="one"?1.0f:alphaMode=="zero"?0.0f:value.alpha});')
p.write_text(s)
