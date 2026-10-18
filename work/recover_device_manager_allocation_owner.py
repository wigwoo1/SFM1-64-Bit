from pathlib import Path
import json
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
pe=im.u32(60);optional=pe+24
rva=im.u32(optional+104);size=im.u32(optional+108)
imports=[]
for n in range(size//20):
 o=im.offset(im.base+rva+20*n)
 original,timestamp,chain,name,first=struct.unpack_from('<5I',im.b,o)
 if not any((original,timestamp,chain,name,first)):break
 dll=im.string(im.base+name)
 for i in range(65536):
  entry=im.word(im.base+(original or first)+4*i)
  if not entry:break
  symbol=('ordinal:'+str(entry&65535)) if entry&0x80000000 else im.string(im.base+entry+2)
  imports.append(dict(cell=hex(im.base+first+4*i),dll=dll,symbol=symbol))
lookup={int(x['cell'],16):x for x in imports}
connect=region(0x100290a0,0x100290dd)
thunk=region(0x100568ac,0x100568b2)
disconnect=region(0x100290e0,0x10029101)
print(lookup[0x100fa1cc]);print(lookup[0x100fa244]);print(im.string(0x100fdc7c))
assert lookup[0x100fa1cc]['dll'].lower()=='d3d9.dll' and lookup[0x100fa1cc]['symbol']=='Direct3DCreate9'
assert lookup[0x100fa244]['symbol']=='Warning'
assert bytes.fromhex(thunk['instructions'][0]['bytes'])==bytes.fromhex('ff25cca10f10')
report=dict(finding='Derived ShaderDeviceMgr001 Connect calls imported Direct3DCreate9 with argument32, stores returned interface at receiver+30h, warns and returns false on null. Disconnect checks that field, invokes vtable offset8 with interface as explicit stack argument, clears the field, then tail-calls base disconnect10027c50. Under the identified D3D9 COM interface this is Release. Argument32 is the API SDK version, not an allocation byte count. Original prior allocation interpretation is superseded.',sha256=sha,connect=connect,allocation_thunk=thunk,disconnect=disconnect,import_bindings=[lookup[0x100fa1cc],lookup[0x100fa244]],failure_text=im.string(0x100fdc7c),limits='Static byte and import-table proof only; imported provider implementation, allocation return object layout, lifetime, runtime invocation and synchronization remain unverified.')
Path('outputs/sfm64-device-manager-allocation-owner.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_device_manager_allocation_owner_bytes='+str(connect['bytes']+thunk['bytes']+disconnect['bytes']))

