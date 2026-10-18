from pathlib import Path
import sys,re,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
start,end=0x10004390,0x100048bf
covered={};address=None;instructions=0
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)',line)
 if m:
  address=int(m[1],16)
  if start<=address<end:instructions+=1
  raw=bytes.fromhex(m[2])
 elif address is not None and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):raw=bytes.fromhex(line)
 else:continue
 if start<=address<end:
  for b in raw:
   if address<end:covered[address]=b
   address+=1
 else:address+=len(raw)
assert len(covered)==end-start,(len(covered),end-start)
raw=bytes(covered[a] for a in range(start,end))
assert raw==im.b[im.offset(start):im.offset(start)+len(raw)]
groups=[('position',0x100048c0,0x100048cc,0x100048dc,{0x10004430:4,0x10004458:0,0x10004437:-1},{0x10004443:12,0x1000444a:16,0x10004451:8,0x10004458:0}),
('normal',0x100049a4,0x100049b0,0x100049c0,{0x10004583:4,0x100045ab:0,0x1000458a:-1},{0x10004596:12,0x1000459d:16,0x100045a4:8,0x100045ab:0}),
('texture',0x10004a88,0x10004a94,0x10004aa4,{0x10004696:4,0x10004702:0,0x100046ae:-1},{0x100046ba:12,0x100046d2:16,0x100046ea:8,0x10004702:0}),
('userdata',0x10004b6c,0x10004b78,0x10004b88,{0x100047f2:4,0x1000485a:0,0x10004809:-1},{0x10004815:12,0x1000482c:16,0x10004843:8,0x1000485a:0})]
records=[]
for name,dispatch,index,regular,compressedTargets,regularTargets in groups:
 regularTargets.update({a:n for a,n in compressedTargets.items() if n>=0})
 normal=[regularTargets[im.word(regular+i*4)] for i in range(50)]
 compressed=normal.copy()
 for i in range(16):
  slot=im.b[im.offset(index)+i];size=compressedTargets[im.word(dispatch+slot*4)]
  if size>=0:compressed[i+2]=size
 records.append({'name':name,'normal':normal,'compressed':compressed})
assert all(r['normal']==records[0]['normal'] and r['compressed']==records[0]['compressed'] for r in records)
report={'sha256':hashlib.sha256(im.b).hexdigest(),'routine':hex(start),'instructions':instructions,'checked_code_bytes':len(raw),'element_sizes':records,'notes':['Format compression bit0x400; alternate position/normal bit0x800.','Eight texture channels use3-bit dimensions starting bit18. Valid dimensions1..4; higher dimensions read beyond local element-ID lookup.','Final stride rounds to16 only when larger than16 and format bit0x200 is clear. Active stream stride fields receive final stride.'],'limitations':'Static recovery only. Native execution oracle and full x64 layout translation pending.'}
Path('outputs/sfm64-shader-vertex-layout-recovery.json').write_text(json.dumps(report,indent=2))
normal=records[0]['normal'];compressed=records[0]['compressed']
Path('work/studio_material_vertex_element_size_candidate.h').write_text('#pragma once\n#include <cstdint>\n// Recovered element IDs0..49; out-of-range yields zero.\ninline uint32_t studioMaterialVertexElementSizeCandidate(uint32_t element,bool compressed){\n static constexpr uint8_t regular[50]={'+','.join(map(str,normal))+'};\n static constexpr uint8_t packed[50]={'+','.join(map(str,compressed))+'};\n return element<50?(compressed?packed[element]:regular[element]):0;\n}\n')
print('checked_code_bytes=',len(raw),' instructions=',instructions,' table_families=',len(records))
print('regular=',normal,'compressed=',compressed)
