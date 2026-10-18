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
  match=re.match(r'^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)',line)
  if match:
   va=int(match.group(1),16)
   if va<start:continue
   found=True
   if va>=end:break
   if current:rows.append(current)
   current=[va,bytes.fromhex(match.group(2))]
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

start,end=0x1008B035,0x1008B5CA
verified=checked_span(start,end)
entries=[
 (0x1B9,0x10AA71B8,'Create Animation Set for New Camera',0x1008CDA0),
 (0x1BA,0x10AA7174,'Create Animation Set for New Light',0x1008CDC0),
 (0x1BB,0x10AA7130,'Create Animation Set for New Model',0x1008CDE0),
 (0x1BC,0x10AA70D8,'Create Animation Set for New Particle System',0x1008CE10),
 (0x1BD,0x10AA7080,'Create Animation Set(s) for Existing Element(s)',0x100938C0),
 (0x1BE,0x10AA7060,'Create Preset',None),
 (0x1BF,0x10AA6FE8,'Paste Animation Set(s)',None),
 (0x1C0,0x10AA6FB8,'Show Hidden Controls',None),
 (None,0x10AA6F88,'Show Scene Hierarchy',None),
 (None,0x10AA6F5C,"Show 'Visible' Column",None),
 (None,0x10AA6F28,"Show 'Selectable' Column",None),
]
actions=[]
for command,string_va,label,callback in entries:
 actual=image.string(string_va)
 actions.append({'command_id':hex(command) if command is not None else None,'string_va':hex(string_va),'label':actual,'expected_label':label,'handler_va':hex(callback) if callback else None,'label_matches':actual==label,'command_operand_in_span':bool(verified and (command is None or command.to_bytes(4,'little') in verified)),'label_operand_in_span':bool(verified and string_va.to_bytes(4,'little') in verified)})
sha=hashlib.sha256(image.b).hexdigest()
receipt={'binary':str(binary),'sha256':sha,'range':{'start':hex(start),'end_exclusive':hex(end),'disassembly_matches_installed_bytes':verified is not None,'bytes':len(verified) if verified else 0},'method':'Animation Set Editor gear/menu QAction registration','actions':actions,'limits':['Static registration evidence only; state-dependent visibility and order are unverified.','Action handlers and their runtime effects are not dynamically observed.','This inventory does not claim the prototype implements these commands.']}
receipt['all_match']=receipt['range']['disassembly_matches_installed_bytes'] and all(a['label_matches'] and a['command_operand_in_span'] and a['label_operand_in_span'] for a in actions) and sha=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4'
Path('outputs/animation-set-editor-menu-registration-verification.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({'sha256':sha,'verified_span':receipt['range'],'action_count':len(actions),'all_match':receipt['all_match'],'actions':[{'id':a['command_id'],'label':a['label'],'handler':a['handler_va']} for a in actions]},indent=2))
if not receipt['all_match']:sys.exit(1)
