from pathlib import Path
import re,json,sys,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tier0.dll');ranges=[(0x100104c0,0x100105e1)];rows=[];pending=None
for line in Path('work/tier0-disassembly.txt').open():
 m=re.match(r'^\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)(.*)',line)
 if m:
  pending=None;va=int(m[1],16)
  if any(a<=va<z for a,z in ranges):pending={'address':hex(va),'bytes':m[2].strip(),'instruction':m[3].strip()};rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+\n?',line):pending['bytes']+=' '+line.strip()
covered=set()
for r in rows:
 va=int(r['address'],16);b=bytes.fromhex(r['bytes']);assert im.b[im.offset(va):im.offset(va)+len(b)]==b;covered.update(range(va,va+len(b)))
assert covered=={va for a,z in ranges for va in range(a,z)}
opt=im.u32(60)+24
imports={}
descriptor=im.base+im.u32(opt+104)
while im.word(descriptor+12):
 dll=im.string(im.base+im.word(descriptor+12))
 names=im.word(descriptor) or im.word(descriptor+16)
 cells=im.word(descriptor+16)
 i=0
 while im.word(im.base+names+4*i):
  entry=im.word(im.base+names+4*i)
  name=('ordinal:'+str(entry&65535)) if entry&0x80000000 else im.string(im.base+entry+2)
  imports[hex(im.base+cells+4*i)]={'dll':dll,'name':name}
  i+=1
 descriptor+=20
report={'tier0_sha256':hashlib.sha256(im.b).hexdigest(),'checked_instructions':len(rows),'bytes':289,'instructions':rows,'imports':{hex(cell):imports[hex(cell)] for cell in (0x1002f07c,0x1002f074,0x1002f11c)},'algorithm':'ExportLock100104c0: recursionorCASowner0->threadthenincrementdepth. sleepParameter=-1 purepause loop. Otherwise8192pause retries,then8192pause retries Sleep(0) everyremainingCount%1024==0. IfsleepParameter0 querycurrentthreadpriority; positivepriority setsparameter1. Nonzero parameter adds8192pause+Sleep(0) retries; finalinfinitepause+Sleep(parameter) retries. No fairness/timeout/cancellation guard.'}
Path('outputs/sfm64-tier0-fast-mutex-recovery.json').write_text(json.dumps(report,indent=2));print('verified',len(rows),'instructions/289bytes',report['imports'])
