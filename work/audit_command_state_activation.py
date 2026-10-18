"""Bounded command-state literal and getter-call-site inventory."""
from pathlib import Path
import hashlib,json,re,struct
from pe_image import Image
image=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
digest=hashlib.sha256(image.b).hexdigest()
assert digest=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
instructions=[]
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
    m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
    if m: instructions.append((int(m[1],16),bytes.fromhex(m[2]),m[3].strip()))
def checked(item):
    address,raw,text=item
    offset=image.offset(address)
    assert image.b[offset:offset+len(raw)]==raw
    return {'address':hex(address),'bytes':raw.hex(),'instruction':text}
getter=next(item for item in instructions if item[0]==0x1000df50)
assert getter[1]==bytes.fromhex('b8a80a1810')
assert image.b[image.offset(0x1000df50)+5]==0xc3
references=[checked(item) for item in instructions if any(f'{address:X}h' in item[2] for address in range(0x10180aa8,0x10180ac8,4))]
calls=[]
for index,item in enumerate(instructions):
    if item[2]!='call        1000DF50': continue
    assert item[1][0]==0xe8 and len(item[1])==5
    assert item[0]+5+struct.unpack('<i',item[1][1:])[0]==0x1000df50
    calls.append({'call':checked(item),'following_window':[checked(entry) for entry in instructions[index+1:index+17]]})
assert calls
literals={}
for address in range(0x10180aa8,0x10180ac8,4):
    needle=struct.pack('<I',address);offset=0;hits=[]
    while True:
        offset=image.b.find(needle,offset)
        if offset<0: break
        hits.append(offset);offset+=1
    literals[hex(address)]=hits
report={'source':str(Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')),
        'sha256':digest,'getter':checked(getter),'state_base':'0x10180aa8','queue_mode_field':'0x10180ab8',
        'verified_absolute_references':references,'getter_call_windows':calls,'raw_literal_file_offsets':literals,
        'activation_identified':False,
        'limits':'Verified displayed instruction prefixes and bounded sixteen-instruction windows only. Does not follow register aliases, virtual dispatch, computed addresses or external writes. Literal absence is not proof of missing activation.',
        'next_action':'Use the verified getter caller inventory for register-alias tracing and inspect external device interfaces for command-mode setters.'}
Path('outputs/sfm64-command-state-activation-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'verified_absolute_references={len(references)} getter_call_windows={len(calls)} activation_identified=0')
