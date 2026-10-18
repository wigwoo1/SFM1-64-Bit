from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
lines=Path('work/engine-disassembly.txt').read_text(encoding='utf-8').splitlines()
references=[]; candidates=[]
for index,line in enumerate(lines):
    if '106941D4h' not in line: continue
    references.append(line.strip())
    window=lines[max(0,index-25):index+65]
    if not any(re.search(r'\[[a-z]+\+70h\]',item) for item in window): continue
    checked=[]
    for item in window:
        match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',item)
        if not match: continue
        data=bytes.fromhex(match[2]);offset=image.offset(int(match[1],16))
        assert image.b[offset:offset+len(data)]==data
        checked.append(item.strip())
    candidates.append(dict(global_reference=line.strip(),instructions=checked))
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),candidate_global='0x106941d4',references=references,draw_slot_windows=candidates,limitations='Proximity-based candidate windows, not complete register/data-flow proof. Checked instruction prefixes only. This global is distinct from the tier helper result at 106A7510; binding remains to trace.')
Path('outputs/sfm64-engine-studio-draw-candidates.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('Global references:',len(references),'draw-slot windows:',len(candidates))
print('\n'.join(item['global_reference'] for item in candidates))
