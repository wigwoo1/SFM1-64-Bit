from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
lines=Path('work/engine-disassembly.txt').read_text(encoding='utf-8').splitlines(); decoded=[]
for line in lines:
    match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
    if match: decoded.append([int(match[1],16),bytes.fromhex(match[2]),line.strip()])
    elif decoded:
        continuation=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
        if continuation: decoded[-1][1]+=bytes.fromhex(continuation[1])
selected=[]
for begin,end in [(0x100DABE0,0x100DAD88),(0x100DB14B,0x100DB189)]:
    for address,data,line in decoded:
        if not begin<=address<end: continue
        offset=image.offset(address);assert image.b[offset:offset+len(data)]==data
        selected.append(dict(address=hex(address),bytes=data.hex(),instruction=line))
arguments=[dict(number=1,source='&[ebp-54h]',role='results candidate'),dict(number=2,source='&[ebp-234h]',role='model information candidate'),dict(number=3,source='[ebp-258h] copied from incoming [ebp+10h]',role='bone matrices candidate'),dict(number=4,source='[ebp-260h]',role='first flex weights candidate'),dict(number=5,source='[ebp-254h]',role='second/delayed flex weights candidate'),dict(number=6,source='edi',role='model origin candidate'),dict(number=7,source='esi',role='flags candidate')]
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),routine='0x100dabe0',dispatch='0x100db187',candidate_interface_global='0x106941d4',arguments=arguments,verified_complete_instructions=selected,limitations='Argument roles inferred from seven pushes and recovered callee signature; global binding and upstream array-writing semantics remain unverified.')
Path('outputs/sfm64-engine-draw-arguments.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('Verified',len(selected),'complete instructions; seven draw arguments recovered')
