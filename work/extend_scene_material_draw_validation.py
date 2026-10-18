from pathlib import Path
p=Path('work/test_scene_material_inputs.cpp');s=p.read_text().replace('#include "scene_material_inputs_candidate.h"','#include "scene_material_inputs_candidate.h"\n#include "scene_material_draw_inputs_candidate.h"')
needle='float alpha=inputs.alpha[0][0].alpha;'
insert='''auto drawInputs=prepareSceneMaterialDrawInputsCandidate(inputs,mounts);if(drawInputs.size()!=1||&drawInputs[0].indices!=&inputs[0].indices||&drawInputs[0].ranges!=&inputs[0].ranges)throw std::runtime_error("scene draw input copied topology");size_t visor=0;for(size_t n=0;n<inputs[0].materials.size();++n)if(inputs[0].materials[n].source.vmt.stem().string()=="visor"){++visor;if(drawInputs[0].alpha[n]!=inputs.alpha[0][0].alpha||drawInputs[0].materials[n].translucent!=(inputs[0].materials[n].translucent||inputs.alpha[0][0].alpha<1))throw std::runtime_error("scene visor draw alpha");}if(visor!=1)throw std::runtime_error("scene visor material coverage");'''
if s.count(needle)!=1:raise RuntimeError('alpha marker')
s=s.replace(needle,insert+needle).replace('evaluated_overrides=1','per_draw_alpha=1 dynamic_translucency=1 topology_references_retained=1 evaluated_overrides=1')
p.write_text(s)
