from pathlib import Path
import json,hashlib
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
checks={0x1002726f:'c7063cd30f10',0x10027275:'c786d0020000dcce0f10'}
for a,h in checks.items():b=bytes.fromhex(h);assert im.b[im.offset(a):im.offset(a)+len(b)]==b
col=im.word(0x100fced8);assert col==0x1016aa20 and im.word(col+4)==0x2d0 and im.word(col+12)==0x1016de34
assert im.string(0x1016de3c)=='.?AVCShaderAPIDx8@@'
assert im.word(0x100fd33c+41*4)==0x1001fb90
sites={}
for path in ('outputs/sfm64-full-restore-command-receivers.json','outputs/sfm64-full-restore-esi-receiver-flow.json','outputs/sfm64-full-restore-remaining-receivers.json'):
 r=json.loads(Path(path).read_text());assert r['sha256']==sha
 for s in r['sites']:
  binding=s.get('receiver',s.get('binding'));binding={'full_owner':'full_owner','device_at_owner648':'live_device_owner648'}.get(binding,binding)
  if s['call'] in sites:assert sites[s['call']]['receiver']==binding and sites[s['call']]['slot']==s['slot']
  sites[s['call']]={'call':s['call'],'slot':s['slot'],'receiver':binding}
assert len(sites)==65
bound=[];external=[]
for s in sites.values():
 table={'full_owner':0x100fd33c,'owner_subobject_2d0':0x100fcedc}.get(s['receiver'])
 if table is None:external.append(s);continue
 target=im.word(table+s['slot']*4);offset=im.offset(target);prefix=im.b[offset:offset+16]
 bound.append(dict(s,vtable=hex(table),cell=hex(table+s['slot']*4),target=hex(target),prefix=prefix.hex()))
Path('outputs/sfm64-full-restore-installed-vtable-bindings.json').write_text(json.dumps({'sha256':sha,'constructor_checks':{hex(a):h for a,h in checks.items()},'embedded_rtti':{'class':'CShaderAPIDx8','offset':720,'locator':hex(col)},'bound_calls':bound,'external_calls':external,'limits':'Installed constructor stores/RTTI/vtable DWORDs checked. Prefix bytes are identification evidence, not whole method recovery or execution. External device/global service tables require runtime/interface binding. Full restore remains unexecuted.'},indent=2)+'\n')
print(f'full_restore_installed_method_bindings={len(bound)} external_interface_calls={len(external)}')
