from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll');decoded=[]
for line in Path('work/native-client-scale-disassembly.txt').read_text(encoding='utf-8').splitlines():
 match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if match:decoded.append([int(match[1],16),bytes.fromhex(match[2]),line.strip()])
 elif decoded:
  tail=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
  if tail:decoded[-1][1]+=bytes.fromhex(tail[1])
checked=[]
for address,data,line in decoded:
 if not 0x10046FE0<=address<0x1004712F:continue
 offset=image.offset(address);assert image.b[offset:offset+len(data)]==data
 checked.append(dict(address=hex(address),bytes=data.hex(),instruction=line))
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),reset_candidate='0x10046fe0',base_object_history_field='0xd5c',renderable_object_history_field='0xd58',offset_adjustment=4,base_cache_fields=['0xd70','0xd84'],renderable_cache_fields=['0xd6c','0xd80'],verified_complete_instructions=checked,limitations='Base-object interpretation supported by explicit +4 renderable dispatch; routine invocation/reset scheduling, allocator and zero-fill helper identities, generation initialization and SFM seeking remain unresolved.')
Path('outputs/sfm64-flex-history-reset.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('Verified history reset candidate:',len(checked),'complete instructions')
