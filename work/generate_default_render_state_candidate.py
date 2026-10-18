from pathlib import Path
import json
r=json.loads(Path('outputs/sfm64-default-render-state-sequence.json').read_text());steps=r['ordered_state_calls'];conditional=[i for i,s in enumerate(steps) if s['predicate']!='always'];assert len(conditional)==2
h='#pragma once\n#include <cstdint>\nstruct StudioDefaultRenderStateEntryCandidate{uint32_t state,value;};\ninline constexpr StudioDefaultRenderStateEntryCandidate studioDefaultRenderStateEntriesCandidate[]={\n'+',\n'.join('{'+str(s['state'])+'u,'+str(s['value_bits'])+'u}' for s in steps)+'\n};\n'
a,b=conditional
h+='''template<class Services>void studioDefaultRenderStateCandidate(Services& services){
'''+f' for(unsigned i=0;i<{a};++i)services.submit(studioDefaultRenderStateEntriesCandidate[i].state,studioDefaultRenderStateEntriesCandidate[i].value);\n'+''' auto* capability=services.capability();
 if((services.flags(capability)&0x10u)&&services.vendor(capability)==0x1002u){
'''+f'  services.submit(studioDefaultRenderStateEntriesCandidate[{a}].state,studioDefaultRenderStateEntriesCandidate[{a}].value);\n'+'''  capability=services.capability();
 }
'''+f' if(services.flags(capability)&0x20u)services.submit(studioDefaultRenderStateEntriesCandidate[{b}].state,studioDefaultRenderStateEntriesCandidate[{b}].value);\n'+f' for(unsigned i={b+1};i<91;++i)services.submit(studioDefaultRenderStateEntriesCandidate[i].state,studioDefaultRenderStateEntriesCandidate[i].value);\n'+'}\n'
assert b==a+1
Path('work/studio_default_render_state_candidate.h').write_text(h)
print('conditional_indices='+str(conditional))
