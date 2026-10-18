from pathlib import Path
import json,re
from pe_image import Image
import hashlib
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
r=json.loads(Path('outputs/sfm64-restore-blend-helper-recovery.json').read_text())
assert hashlib.sha256(im.b).hexdigest()==r['sha256']
rows=r['instructions']
for row in rows:
 raw=bytes.fromhex(row['bytes']);assert im.b[im.offset(int(row['address'],16)):im.offset(int(row['address'],16))+len(raw)]==raw
commands=[]
for i,row in enumerate(rows):
 if row['address']=='0x1002f461' or re.fullmatch(r'mov\s+dword ptr \[\w+\],(?:1|11h)',row['instruction']):
  fields=[x for x in rows[i+1:i+12] if re.match(r'mov\s+dword ptr \[\w+\+(?:4|8|0Ch)\],',x['instruction'])]
  commands.append({'opcode_instruction':row,'fields':fields,'preceding_context':rows[max(0,i-14):i]})
assert len(commands)==15
indirect=[{'call':row,'context':rows[max(0,i-6):i]} for i,row in enumerate(rows) if row['instruction'].startswith('call') and bytes.fromhex(row['bytes'])[0]==0xff]
out={'sha256':r['sha256'],'queued_command_sites':commands,'indirect_call_contexts':indirect,
 'sampler_loop':'Capability interface global1016dd08 slot265 returns signed count captured once. Positivecount loops stage0..count-1; stride3bytes cache startingreceiver145/146. First opcode17 sampler type11 uses captured byte145. Then global1019092c slot49 lowbyte controls type8 submission; byte146 zero/nonzero maps to31544547/34544547. Each submission reacquires commandstate throughgetter. Nonpositivecount skipsloop.',
 'tail':'Capture signed16 receiver+4 before suppression global1016dd0c slot36; if permitted, descriptor offset receiver+8 low24/count receiver+0b reread aftergate, invoke1002f2e0 then1002ebb0 andpublishkind+54. Alwaysreload global1016e20c before eachslot9/10 call(-1), thenpublishhandle+56=ffff.',
 'limits':'Checked instruction mapping; previous provisional type7 corrected to11 from verified prefix. No original execution; nesteddispatch/commit still unresolved.'}
Path('outputs/sfm64-restore-blend-command-map.json').write_text(json.dumps(out,indent=2)+'\n')
print(f'mapped_queued_command_sites={len(commands)} indirect_call_sites={len(indirect)}')

