"""Prepare original332-byte helper metadata; no native execution here."""
from pathlib import Path
import hashlib,json,re,struct
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
start,end=0x1002a670,0x1002a7bc
recovery=json.loads(Path('outputs/sfm64-command-device-assignment-recovery.json').read_text())
assert recovery['dll_sha256']==hashlib.sha256(im.b).hexdigest()
assert {'start':hex(start),'end':hex(end),'bytes':end-start} in recovery['verified_regions']
raw=im.b[im.offset(start):im.offset(start)+end-start]
optional=im.u32(60)+24;rva,size=struct.unpack_from('<II',im.b,optional+96+40)
cursor=im.offset(im.base+rva);limit=cursor+size;fixups=[]
while cursor<limit:
    page,length=struct.unpack_from('<II',im.b,cursor)
    assert length>=8 and length%2==0 and cursor+length<=limit
    for offset in range(cursor+8,cursor+length,2):
        entry=im.u16(offset);address=im.base+page+(entry&4095)
        if entry>>12==3 and start<=address<end:
            assert address+4<=end
            fixups.append({'offset':address-start,'target':im.word(address)})
    cursor+=length
calls=[];indirect=[]
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
    m=re.match(r'\s*([0-9A-F]{8}):.*?\bcall\s+(.+)',line)
    if not m:continue
    address=int(m[1],16)
    if not start<=address<end:continue
    operand=m[2].strip()
    if re.fullmatch('[0-9A-F]{8}',operand):
        offset=address-start;target=int(operand,16)
        assert raw[offset]==0xe8 and address+5+struct.unpack_from('<i',raw,offset+1)[0]==target
        calls.append({'offset':offset,'target':target})
    else:indirect.append({'address':hex(address),'operand':operand})
assert {c['target'] for c in calls}=={0x100d6ba0}
header='#pragma once\nstatic constexpr unsigned char nativeDeviceCreationBytes[]={'+','.join(map(str,raw))+'};\n'
header+='static constexpr unsigned nativeDeviceCreationFileOffset='+str(im.offset(start))+';\n'
for name,entries in [('Fixups',fixups),('Calls',calls)]:
    header+='static constexpr unsigned nativeDeviceCreation'+name+'[][2]={'+','.join('{'+str(e['offset'])+','+str(e['target'])+'}' for e in entries)+'};\n'
Path('work/device_creation_native_init.h').write_text(header)
report={'installed_sha256':recovery['dll_sha256'],'body_bytes':len(raw),'absolute_fixups':fixups,'direct_calls':calls,'indirect_calls':indirect,
 'required_bindings':['convar reference constructor and live mode object','CPUInformation import and byte6 storage','service pointer and fake IDirect3D9 slot16','Sleep import','failure window/UI imports and string storage'],
 'native_executed':False,'limitations':'Preparation only. Every absolute/indirect external reference must be remapped; exact x86 calling conventions and complete owner/presentation/output storage required.'}
Path('outputs/sfm64-device-creation-native-preparation.json').write_text(json.dumps(report,indent=2)+'\n')
print('device_creation_bytes='+str(len(raw))+' absolute_fixups='+str(len(fixups))+' direct_calls='+str(len(calls))+' indirect_calls='+str(len(indirect)))
