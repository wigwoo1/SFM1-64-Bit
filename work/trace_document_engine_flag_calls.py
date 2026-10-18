from pathlib import Path
from collections import deque
import re,json,hashlib
from pe_image import Image
results=[]
for source,binary,receiver in [('work/ifm-disassembly.txt',r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll','10FD4398h'),('work/engine-disassembly.txt',r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll','1040C0B0h')]:
 im=Image(binary);history=deque(maxlen=40);candidates=[];slots=0;direct=[]
 for line in Path(source).open():
  m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if m:
   if '+00000170h]' in line and 'mov ' in line:
    slots+=1
    if any(receiver in previous for previous in history):candidates.append(list(history)+[line])
   if 'call        101A8FE0' in line:
    b=bytes.fromhex(m[2]);o=im.offset(int(m[1],16));assert im.b[o:o+len(b)]==b
    direct.append({'call':line.strip(),'preceding_context':list(history)[-10:]})
   history.append(line)
 checked=0
 for context in candidates:
  for line in context:
   m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
   if m:
    b=bytes.fromhex(m[2]);o=im.offset(int(m[1],16));assert im.b[o:o+len(b)]==b;checked+=1
 results.append({'source':source,'sha256':hashlib.sha256(im.b).hexdigest(),'offset170_mov_matches':slots,'near_receiver_candidates':candidates,'direct_engine_setter_calls':direct,'checked_candidate_prefixes':checked})
report={'results':results,'limitations':'Forty decoded instruction window is a search filter, not receiver data-flow proof. Register aliases, retained receiver fields, indirect vtable indexing and other modules are not excluded. Offset170 matches on unrelated interfaces are not setter calls. Direct101A8FE0 calls use different receivers; shared byte-store implementation does not prove CEngineTool lifecycle.'}
Path('outputs/sfm64-document-engine-flag-call-audit.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
