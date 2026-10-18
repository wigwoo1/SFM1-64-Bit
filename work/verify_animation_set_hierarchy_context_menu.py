import hashlib,json,re,sys
from pathlib import Path
sys.path.insert(0,'work')
from pe_image import Image
binary=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
image=Image(binary)
disassembly=Path('work/ifm-disassembly.txt')
def checked_span(start,end):
 rows=[];current=None;found=False
 for line in disassembly.open(errors='replace'):
  m=re.match(r'^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)',line)
  if m:
   va=int(m.group(1),16)
   if va<start:continue
   found=True
   if va>=end:break
   if current:rows.append(current)
   current=[va,bytes.fromhex(m.group(2))]
  elif found and current and re.match(r'^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$',line):current[1]+=bytes.fromhex(line.strip())
 if current:rows.append(current)
 cursor=start;result=bytearray()
 for va,chunk in rows:
  if va!=cursor:return None
  try:raw=image.b[image.offset(va):image.offset(va)+len(chunk)]
  except (ValueError,IndexError):return None
  if raw!=chunk:return None
  result.extend(raw);cursor+=len(chunk)
 return bytes(result) if cursor==end else None
start,end=0x10087E19,0x10088252
code=checked_span(start,end)
items=[
 ('Rename Dag',0x10AA6B14,0x101D6930,'DmeDag selection branch'),
 ('Delete Empty Dag',0x10AA6B00,0x1008CBE0,'DmeDag selection branch; enable-state input from 0x104C2B00'),
 ('Group Selected Dags',0x10AA6AEC,0x1008ED10,'DmeDag selection branch'),
 ('Rename AnimationSet',0x10AA6AD8,0x101D74C0,'Animation-set selection branch'),
 ('Copy AnimationSet(s)',0x10AA6AC0,0x10092010,'Animation-set selection branch'),
 ('Delete AnimationSet(s)',0x10AA6AA8,0x1008E570,'Animation-set selection branch'),
]
actions=[]
for label,va,handler,condition in items:
 actions.append({'label':image.string(va),'expected_label':label,'label_va':hex(va),'handler_va':hex(handler),'menu_label_operand_verified':bool(code and va.to_bytes(4,'little') in code),'handler_pointer_verified':bool(code and handler.to_bytes(4,'little') in code),'static_branch_note':condition})
sha=hashlib.sha256(image.b).hexdigest()
receipt={'binary':str(binary),'sha256':sha,'method':'Animation Set Editor hierarchy context-action registration path','range':{'start':hex(start),'end_exclusive':hex(end),'bytes':len(code) if code else 0,'disassembly_matches_installed_bytes':code is not None},'actions':actions,'limits':['The branch is state-conditional; runtime menu order, visibility, and enablement were not observed.','Handler addresses identify callback routes only; complete effects, undo behavior, and notification behavior remain unverified.','Delete Empty Dag uses an unresolved predicate at 0x104C2B00; its exact semantics are not inferred here.','This is reverse-engineering evidence, not implementation parity.']}
receipt['all_match']=receipt['range']['disassembly_matches_installed_bytes'] and all(a['label']==a['expected_label'] and a['menu_label_operand_verified'] and a['handler_pointer_verified'] for a in actions) and sha=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4'
Path('outputs/animation-set-hierarchy-context-menu-verification.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({'sha256':sha,'verified_span':receipt['range'],'actions':len(actions),'all_match':receipt['all_match'],'labels':[a['label'] for a in actions]},indent=2))
if not receipt['all_match']:sys.exit(1)
