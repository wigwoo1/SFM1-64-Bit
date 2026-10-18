import re,json,hashlib
from pathlib import Path
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
assert hashlib.sha256(im.b).hexdigest()=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
assert im.word(0x103666c4+29*4)==0x101e5670
assert im.word(0x1040c0b0)==0x103666c4
assert im.word(0x103666c4+92*4)==0x101a8fe0
rows=[];pending=None
for line in Path('work/engine-disassembly.txt').open():
 m=re.match(r'^\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)(.*)',line)
 if m:
  va=int(m[1],16);pending=None
  if 0x101e5670<=va<0x101e568a or 0x100832d0<=va<0x100832d9 or 0x101e6470<=va<0x101e6476 or 0x10318ef0<=va<0x10318f05 or 0x101a8fe0<=va<0x101a8fed:
   pending={'address':hex(va),'bytes':m[2].strip(),'instruction':m[3].strip()};rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+\n?',line):pending['bytes']+=' '+line.strip()
for row in rows:
 b=bytes.fromhex(row['bytes']);o=im.offset(int(row['address'],16));assert im.b[o:o+len(b)]==b
report={'sha256':hashlib.sha256(im.b).hexdigest(),'slot':29,'method':'0x101e5670','checked_instructions':len(rows),'predicate':'engine tool byte+4 nonzero AND signed client-state+60 >=2','client_accessor':'[103ffaa4]+8','limitations':'Static recovery. Engine tool byte lifecycle and live client-state binding remain unfinished.','instructions':rows}
report['singleton']={'address':'0x1040c0b0','file_vtable':'0x103666c4','factory':'0x101e6470','file_initial_flag4':im.b[im.offset(0x1040c0b4)],'destruction_path':'0x10318ef0 calls10002120 with singleton+8 then replaces singleton vtable with1033e76c','limitations':'File initial byte is not proof of final initialized flag; aliased/indirect runtime writes remain untraced.'}
report['flag_setter']={'slot':92,'address':'0x101a8fe0','operation':'load low byte of argument+8; store receiver byte+4; RET4','limitations':'Setter identified; call-site ordering and startup enablement still need recovery.'}
Path('outputs/sfm64-document-engine-gate-recovery.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
