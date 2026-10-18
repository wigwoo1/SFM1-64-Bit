from pathlib import Path
import json,hashlib,re,struct
root=Path.cwd();game=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game');dll=game/'bin/tools/ifm.dll'
b=dll.read_bytes();pe=struct.unpack_from('<I',b,60)[0];opt=pe+24;base=struct.unpack_from('<I',b,opt+28)[0];sections=[]
for n in range(struct.unpack_from('<H',b,pe+6)[0]):
 o=opt+struct.unpack_from('<H',b,pe+20)[0]+40*n
 name=b[o:o+8].split(b'\0')[0].decode('ascii');rva,size,raw=struct.unpack_from('<III',b,o+12);sections.append((name,rva,size,raw))
strings=[]
for m in re.finditer(rb'[ -~]{5,}\x00',b):
 text=m.group()[:-1].decode('ascii')
 if not re.search(r'clip.?editor|motion.?editor|graph.?editor|context.?menu|timeline|timestrip|keyframe|animation.?set',text,re.I):continue
 off=m.start();sec=next((s for s in sections if s[3]<=off<s[3]+s[2]),None)
 if sec:strings.append(dict(text=text,va=hex(base+sec[1]+off-sec[3]),section=sec[0],file_offset=off,status='string anchor; function and behavior unverified'))
resources=[]
for folder in [game/'platform/resource',game/'usermod/resource']:
 if not folder.exists():continue
 for p in folder.rglob('*'):
  if not p.is_file() or p.suffix.lower() not in ['.res','.txt','.cur']:continue
  data=p.read_bytes();text=data.decode('utf-8',errors='replace') if p.suffix.lower()!='.cur' else ''
  if not re.search(r'ifm|clipeditor|timeline|keyframe|motioneditor|grapheditor',p.name+' '+text,re.I):continue
  resources.append(dict(path=str(p),sha256=hashlib.sha256(data).hexdigest(),bytes=len(data),commands=re.findall(r'"(?:command|actionSignal)"\s*"([^"]+)"',text,re.I)))
report=dict(binary=str(dll),sha256=hashlib.sha256(b).hexdigest(),ui_string_anchors=strings,official_resources=resources,coverage='Discovery inventory only; not decompiled functions or implemented UI')
(root/'outputs/ui-function-recovery-inventory.json').write_text(json.dumps(report,indent=2))
print(json.dumps(dict(ui_string_anchors=len(strings),official_resources=len(resources),sample=strings[:8]),indent=2))
