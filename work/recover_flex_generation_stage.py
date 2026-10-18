from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll')
entries=[image.word(0x100A066C+index*4) for index in range(7)]
assert entries.count(0x100A0572)==1
checked=[]
for line in Path('work/native-client-scale-disassembly.txt').read_text(encoding='utf-8').splitlines():
 match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if not match:continue
 address=int(match[1],16)
 if not(0x100A0530<=address<0x100A0598 or 0x100A01C0<=address<0x100A0223):continue
 data=bytes.fromhex(match[2]);offset=image.offset(address);assert image.b[offset:offset+len(data)]==data
 checked.append(line.strip())
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),jump_table='0x100a066c',entries=[hex(entry) for entry in entries],generation_update_stage_index=entries.index(0x100A0572),stage_arm='0x100a0572',update_routine='0x100a01c0',increment_call='0x100a021e',verified_instruction_prefixes=checked,limitations='Static stage index recovery; stage argument and enum names, runtime scheduling and indirect paths still require proof. Wrapped continuation bytes not checked by this script.')
Path('outputs/sfm64-flex-generation-stage.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('Generation update stage index:',report['generation_update_stage_index'],'table:',report['entries'])
