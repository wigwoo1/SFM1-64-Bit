"""Deterministic terminal-state race checks; no fake full-render success claim."""
import json
import uuid
from pathlib import Path
from sfm_cycles_bridge_candidate import SfmCyclesBridgeCandidate
from sfm_cycles_conversion_candidate import file_digest
class TerminalController:
    def __init__(self,state):self.state=state;self.start_calls=0
    def cancel(self,identifier):return False
    def poll(self,identifier):return dict(state=self.state,job_id=identifier,result={'retained':True})
    def start(self,*args):self.start_calls+=1;raise AssertionError('Unexpected render start')
bridge=SfmCyclesBridgeCandidate.__new__(SfmCyclesBridgeCandidate)
bridge.conversions=TerminalController('complete')
bridge.renders=TerminalController('complete')
bridge.jobs={'conversion':dict(state='converting')}
assert bridge.cancel('conversion')
assert bridge.poll('conversion')['state']=='cancelled'
assert bridge.renders.start_calls==0
for state in ('complete','failed'):
    bridge.renders=TerminalController(state)
    bridge.jobs['render']=dict(state='rendering',render_id='child',conversion={'retained':True})
    assert not bridge.cancel('render')
    outcome=bridge.poll('render')
    assert outcome['state']==state and outcome['render']['result']['retained']
    assert outcome['conversion']['retained']
root=Path(__file__).resolve().parent.parent
source=root/'outputs/cycles-bridge'/('freshness-'+str(uuid.uuid4())+'.dmx')
source.write_bytes(b'captured session fixture')
digest=file_digest(source)
def render_record():
    return dict(state='rendering',render_id='child',conversion={'retained':True},
                current_session=str(source),session_sha256=digest)
bridge.renders=TerminalController('complete')
bridge.jobs['fresh']=render_record()
assert bridge.poll('fresh')['state']=='complete'
source.write_bytes(b'edited session fixture')
bridge.jobs['stale']=render_record()
stale=bridge.poll('stale')
assert stale['state']=='failed' and stale['render']['state']=='failed'
assert stale['rejected_render']['state']=='complete'
bridge.jobs['late_cancel_stale']=render_record()
assert not bridge.cancel('late_cancel_stale')
assert bridge.poll('late_cancel_stale')['state']=='failed'
source.unlink()
bridge.jobs['missing']=render_record()
assert bridge.poll('missing')['state']=='failed'
(root/'outputs/cycles-bridge/cancel-transition-validation.json').write_text(json.dumps(dict(
    terminal_conversion_does_not_start_render_after_cancel=True,
    completed_render_retained=True,failed_render_retained=True,
    unchanged_session_accepted=True,stale_session_rejected=True,
    stale_late_cancel_rejected=True,missing_session_rejected=True,
    scope='Deterministic state-machine race checks; actual Blender worker tests are separate.'),indent=2))
print('bridge_cancel_transition_checks_passed')
