"""Verify the Animation Set Editor paste MIME key's static registration evidence."""
import hashlib,json,re,sys
from pathlib import Path
sys.path.insert(0,'work')
from pe_image import Image
binary=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
expected_sha='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4'
im=Image(binary);digest=hashlib.sha256(im.b).hexdigest()
if digest!=expected_sha: raise SystemExit(f'ifm.dll hash changed: {digest}')
dis=Path('work/ifm-disassembly.txt')
def span(start,end):
 rows=[];cur=None;entered=False
 for line in dis.open(errors='replace'):
  m=re.match(r'^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)',line)
  if m:
   va=int(m.group(1),16)
   if va<start:continue
   entered=True
   if va>=end:break
   if cur:rows.append(cur)
   cur=[va,bytes.fromhex(m.group(2))]
  elif entered and cur and re.match(r'^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$',line):cur[1]+=bytes.fromhex(line.strip())
 if cur:rows.append(cur)
 out=bytearray();cursor=start
 for va,b in rows:
  if va!=cursor: return None
  try: original=im.b[im.offset(va):im.offset(va)+len(b)]
  except ValueError:return None
  if original!=b:return None
  out.extend(b);cursor+=len(b)
 return bytes(out) if cursor==end else None
ranges={'animation_set_editor_menu_registration':(0x1008B035,0x1008B5CA),'paste_mime_key_descriptor_builder':(0x101F6BA0,0x101F6C2B)}
code={n:span(*r) for n,r in ranges.items()}
menu,mime=code['animation_set_editor_menu_registration'],code['paste_mime_key_descriptor_builder']
key=im.string(0x10AA7000)
facts={
 'paste_menu_label_is_registered':'Paste Animation Set(s)' in [im.string(0x10AA6FE8)],
 'paste_command_id_0x1bf_in_registration':bool(menu and bytes.fromhex('68BF010000') in menu),
 'mime_key_exactly_PasteAnimationSets':key=='PasteAnimationSets',
 'descriptor_builder_embeds_mime_key_address':bool(mime and bytes.fromhex('C745B00070AA10') in mime),
 'descriptor_builder_appends_through_0x10026560':bool(mime and bytes.fromhex('E83CF9E2FF') in mime),
}
receipt={'binary':str(binary),'sha256':digest,'ranges':{n:{'start':hex(r[0]),'end_exclusive':hex(r[1]),'bytes':len(code[n]) if code[n] else 0,'matches_installed_bytes':code[n] is not None} for n,r in ranges.items()},'menu_label_va':'0x10AA6FE8','clipboard_mime_key_va':'0x10AA7000','clipboard_mime_key':key,'verified_findings':facts,'limits':['The descriptor-builder routine statically includes the MIME key in a data record and appends it through helper 0x10026560; record semantics beyond this role are not fully resolved.','This does not reveal serialized payload structure, dependency/graph closure, Paste Animation Set(s) dispatch, validation, document mutation, or undo semantics.','The command ID appears in generic command-registration/state paths; no paste execution callback was identified by this audit.','No live SFM clipboard contents or paste action was observed.']}
receipt['all_match']=all(x is not None for x in code.values()) and all(facts.values())
Path('outputs/paste-animation-set-mime-verification.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'sha256':digest,'mime_key':key,'ranges':receipt['ranges'],'findings':facts,'all_match':receipt['all_match']},indent=2))
if not receipt['all_match']:sys.exit(1)
