"""Editor-side validation must reject malformed requests before worker launch."""
import json
from pathlib import Path
from sfm_cycles_bridge_candidate import SfmCyclesBridgeCandidate
from cycles_settings_candidate import validate_settings

root=Path(__file__).resolve().parent.parent
schema=json.loads((root/'outputs/cycles-worker/dynamic-enum-result.json').read_text())['cycles_schema']
validate_settings(schema,{name:definition['default'] for name,definition in schema['properties'].items()})
class ConversionSpy:
    def __init__(self):self.starts=0
    def start(self,*args):self.starts+=1;return 'candidate'
bridge=SfmCyclesBridgeCandidate.__new__(SfmCyclesBridgeCandidate)
bridge.conversions=ConversionSpy();bridge.jobs={}
prepared={'job':{'session_sha256':'test fixture'}}
good={'resolution':[1920,1080],'cycles':{'samples':8,'denoiser':'OPENIMAGEDENOISE'}}
bad=[dict(good,renderer='source'),dict(good,renderer='vulkan'),dict(good,resolution=[True,1080]),dict(good,resolution=[1920]),
     dict(good,resolution=[0,1080]),dict(good,frame=True),dict(good,scene=''),
     dict(good,cycles={'samples':True}),dict(good,cycles={'adaptive_threshold':float('nan')}),
     dict(good,cycles={'denoiser':'INVALID'}),dict(good,cycles={'unknown':1}),
     dict(good,cycles=[]),[]]
for settings in bad:
    try:bridge.start(prepared,settings,cycles_schema=schema)
    except (ValueError,TypeError):pass
    else:raise AssertionError(settings)
assert bridge.conversions.starts==0 and not bridge.jobs
for resolution in ([1920,1080],[2560,1440],[3840,2160]):
    bridge.start(prepared,dict(good,resolution=resolution),cycles_schema=schema)
assert bridge.conversions.starts==3
good['cycles']['samples']=123
assert bridge.jobs['candidate']['settings']['cycles']['samples']==8
report=dict(installed_defaults_validated=len(schema['properties']),
    malformed_requests_rejected_before_launch=len(bad),resolution_presets_validated=3,
    owned_settings_snapshot=True,scope='Preflight only; native dialog and export integration unfinished.')
(root/'outputs/cycles-bridge/preflight-validation.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report))
