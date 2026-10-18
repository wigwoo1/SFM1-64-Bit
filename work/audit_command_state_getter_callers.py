from pathlib import Path
import re,json,struct,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
getter=0x1000df50
assert im.b[im.offset(getter):im.offset(getter)+6]==bytes.fromhex('B8 A8 0A 18 10 C3')
lines=Path('work/shaderapidx9-disassembly.txt').read_text().splitlines()
callers=[]
for i,line in enumerate(lines):
    m=re.match(r'\s*([0-9A-F]{8}):.*call\s+1000DF50\s*$',line)
    if not m:continue
    address=int(m[1],16);offset=im.offset(address)
    assert im.b[offset]==0xe8
    assert address+5+struct.unpack_from('<i',im.b,offset+1)[0]==getter
    context=[]
    for following in lines[i+1:i+36]:
        match=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',following)
        if not match:continue
        va=int(match[1],16);raw=bytes.fromhex(match[2]);file_offset=im.offset(va)
        assert im.b[file_offset:file_offset+len(raw)]==raw
        context.append({'address':hex(va),'instruction':match[3].strip()})
    callers.append({'call':hex(address),'following_instruction_context':context})
report={'dll_sha256':hashlib.sha256(im.b).hexdigest(),'getter':hex(getter),
        'returned_command_state':'0x10180aa8','direct_callers':callers,
        'queue_activation_identified':False,'device_assignment_identified':False,
        'limits':'Verified direct-call inventory and short instruction context only; alias/dataflow, indirect calls and runtime writes unresolved. Context may cross function boundaries.'}
Path('outputs/sfm64-command-state-getter-callers.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_command_state_getter_direct_callers='+str(len(callers)))
