from pathlib import Path
import json
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
startup=region(0x10028960,0x10028a5a)
wrapper=region(0x10027c00,0x10027c44)
thunk=region(0x100dbe10,0x100dbe19)
getter=region(0x100d6060,0x100d6066)
lookup=region(0x100d5fe0,0x100d604e)
assert bytes.fromhex(thunk['instructions'][-1]['bytes'])==bytes.fromhex('e9478effff')
assert im.word(0x100d6061)==0x100d5fe0
assert any(x['target']=='0x100dbe10' for x in startup['direct_calls'])
assert any(x['address']=='0x10028974' and bytes.fromhex(x['bytes'])==bytes.fromhex('c74508007c0210') for x in startup['instructions'])
report=dict(sha256=sha,startup=startup,wrapper_factory=wrapper,connector_thunk=thunk,local_factory_getter=getter,local_lookup=lookup,
finding='Startup10028960 saves suppliedfactory stack8 into10183b48, replacesstack8 withwrapper10027c00 andconnectsonefactory via100dbe10 tail100d4c60. Wrapper initializes optionalstatus0, calls savedexternalfactory(name,status), returnsnon-null immediately; nullfallsbackto localfactory100d5fe0 obtainedvia100d6060. Bothnullforcesstatus1. Startuplaterclears temporaryfactory10183b48. Tracker localfactory isverifiedno-opprovider, so thispath selects externaltracker whenprovided else localfallback unlessbindingalreadyfilled oninitialconnectorpass.',
limits='Fullstatic startup/wrapper proof, not live runtimeexternalfactoryresolution. Startup hasotherinterface/subsystem initialization beyondtracker; incomingfactoryidentity/result andpreexistingbindingstate stillunresolved. Do notexecute entirestartup without remappingallservices.',
next_action='Executewrapper withcontrolledexternalfactory andactualprivate localtracker factory/lookup, then composewithconnector andtrace incomingapplicationfactory.')
Path('outputs/sfm64-vertex-tracker-startup-factory.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_tracker_startup_factory_bytes='+str(sum(x['bytes'] for x in [startup,wrapper,thunk,getter,lookup]))+' wrapper_factory_count=1 external_first_local_fallback=True')
