from pathlib import Path
import json,struct
exec(Path('work/recover_device_manager_allocation_owner.py').read_text().split('connect=region')[0])
base=region(0x10028960,0x10028a5a)
resource=region(0x100ddf80,0x100ddfca)
timer=region(0x100d5520,0x100d5592)
bindings=[]
for i in range(56):
 address=0x1017b158+8*i;name=im.word(address);cell=im.word(address+4)
 if cell in [0x101908e4,0x1019090c,0x10190904,0x1019094c]:bindings.append(dict(index=i,name=im.string(name),cell=hex(cell),table_address=hex(address)))
report=dict(sha256=sha,base_connect=base,resource_connect=resource,math_library_initialization=timer,interface_bindings=bindings,direct_interface_name=im.string(0x100fdb74),failure_text=im.string(0x100fdb30),cvar_access_object_initial_vtable='0x100fdb2c',cvar_access_vtable_first_slot=hex(im.word(0x100fdb2c)),initial_float_constant=struct.unpack_from('<f',im.b,im.offset(0x100fdb70))[0],math_cpu_import=lookup[0x100fa1fc],finding='BaseConnect stores incoming factory10183b48; passes local external-first wrapper to56interfaceconnector. Unconditionally initializes the one-time accessobject if its flag bit is unset; only Cvar registration withflags800000h is conditional on provider availability. Resourceconnect registers provider token and optionalresourceaccessgroups. Directnamedinterface lookup uses original incomingfactory first then localfactory. Stores interface58 andself54, clearsincomingfactory BEFOREchecking required directinterface/nonzero1019090c/self. Failure warns/returnsfalse withoutautomaticdisconnect. Success invokes math-library initializer withfloatconstant,floatconstant,0,2,1,1,1,1. Resourceconnect guards registration and callsprovider slot8 then marksregistered and tailcalls100de000. No rollback claim.',limits='Fresh static recoveredbytes/table/import evidence only. No fullBaseConnect execution, virtualservice semantics, failure rollback ownership or math-library initialization behavior verified.')
Path('outputs/sfm64-device-manager-startup-dependencies.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_startup_dependency_bytes='+str(base['bytes']+resource['bytes']+timer['bytes']))
print('direct_interface='+report['direct_interface_name']);print('required_interfaces='+str(bindings));print('failure='+repr(report['failure_text']));print('math_cpu_import='+str(report['math_cpu_import']))



