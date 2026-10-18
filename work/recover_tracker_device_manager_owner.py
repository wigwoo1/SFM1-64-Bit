from pathlib import Path
import json
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
constructor=region(0x10028bf0,0x10028c1c)
initializer=region(0x100f9690,0x100f96c1)
registration=region(0x100f96d0,0x100f96e5)
factory=region(0x10029090,0x10029096)
connect=region(0x100290a0,0x100290dd)
name=im.string(0x100fd864)
assert name=='ShaderDeviceMgr001'
base=[im.word(0x100fdb8c+4*i) for i in range(8)]
derived=[im.word(0x100fdc24+4*i) for i in range(8)]
assert base[0]==0x10028960 and derived[0]==0x100290a0
assert any(x['target']=='0x10028960' for x in connect['direct_calls'])
report=dict(sha256=sha,interface=name,object='0x10183bb0',base_vtable='0x100fdb8c',derived_vtable='0x100fdc24',base_first_eight=list(map(hex,base)),derived_first_eight=list(map(hex,derived)),constructor=constructor,static_initializer=initializer,interface_registration=registration,object_factory=factory,derived_connect=connect,
finding='ShaderDeviceMgr001 node10183eb0 registersfactory10029090 returningstaticobject10183bb0. Staticinitializer callsbaseconstructor10028bf0 thenoverwritesvptrwithderived100fdc24 andregistersexitcallback. Slot0derived100290a0 forwardsincomingfactory tobase10028960; onbasesuccess calls100568ac with32, storesresultatobject+30 andfailswhenzero. This identifies trackerconnector startupowner asdevice-managerConnect.',
limits='Staticinstalledproof only. Meaning/ownership of32byte allocationtarget unproved, no full objectlifetime or derivedConnect runtimeclaim. Incomingfactoryorigin inmaterialsystem/application stillunresolved.',
next_action='Trace materialsystem ShaderDeviceMgr001 request andslot0Connect call toidentify factoryargument; preservederivedstartupdependency/lifetimebeforeporting.')
Path('outputs/sfm64-tracker-device-manager-owner.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_tracker_device_manager_owner_bytes='+str(sum(x['bytes'] for x in [constructor,initializer,registration,factory,connect]))+' interface='+name)
