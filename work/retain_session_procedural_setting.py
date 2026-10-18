from pathlib import Path
p=Path('work/session_render_scene_candidate.h');s=p.read_text().replace('#include "session_model_body_candidate.h"','#include "session_model_body_candidate.h"\n#include "session_model_procedural_candidate.h"').replace('SessionModelPlacementCandidate placement;','SessionModelPlacementCandidate placement;bool evaluateProcedural=true;')
s=s.replace('readSessionModelPlacementCandidate(graph,id,hierarchy)}','readSessionModelPlacementCandidate(graph,id,hierarchy),sessionModelEvaluateProceduralCandidate(graph,id)}')
p.write_text(s)
p=Path('work/inspect_scene_procedural_settings.cpp');s=p.read_text().replace('bool value=a->values[0].wire[0]!=0;','bool value=a->values[0].wire[0]!=0;if(model.evaluateProcedural!=value)throw std::runtime_error("scene dropped procedural setting");').replace('native_flag_semantics=unverified','scene_setting_retained=1 native_flag_semantics=unverified');p.write_text(s)
