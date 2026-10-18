from pathlib import Path
p=Path('work/test_editor_current_time_fixture.h')
s=p.read_text().replace('#include "editor_current_time_protocol_candidate.h"','#include "editor_current_time_protocol_candidate.h"\n#include "editor_adjust_time_candidate.h"')
s=s.replace('return time+10;','return editorAdjustTimeCandidate(time,static_cast<unsigned>(mode),*this);')
s=s.replace('==466 && (f.events==std::vector<unsigned>{7,9,10,11})','==334 && (f.events==std::vector<unsigned>{7,9,10,11,1,2,3,4})')
# Add normal branch snapping with default rate and asymmetric negative boundary.
needle='    f.events.clear();f.override=true;'
s=s.replace(needle,'''    f.events.clear();f.attribute.rate=24;f.normalState.time684=209;
    assert(editorCurrentTimeCandidate(f)==417);
    f.events.clear();f.normalState.time684=-209;
    assert(editorCurrentTimeCandidate(f)==0);
    f.events.clear();f.normalState.time684=-210;
    assert(editorCurrentTimeCandidate(f)==-417);
    f.attribute.rate=29.97f;
'''+needle)
p.write_text(s)
