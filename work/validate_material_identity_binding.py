from pathlib import Path
p=Path('work/test_session_material_overrides.cpp')
s=p.read_text().replace('#include "mounted_model_material_candidate.h"','#include "mounted_model_material_candidate.h"\n#include "session_material_binding_candidate.h"')
old='[&](const auto&,auto&syntax){syntax=applySessionMaterialOverridesCandidate(std::move(syntax),record);}'
if s.count(old)!=1:raise RuntimeError('callback marker')
s=s.replace(old,'sessionMaterialBindingCandidate(records,mounts)')
s=s.replace('resolver_applies_before_lookup=1','material_identity_match=1 resolver_applies_before_lookup=1')
p.write_text(s)
