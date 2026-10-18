import json,struct,subprocess,tempfile
from pathlib import Path
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require,cube
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');before=sha(MAP);b=MAP.read_bytes();out=ROOT/'outputs/box-constructor-20261015';run=Path(tempfile.mkdtemp(prefix='bsp-records-',dir=out))
 def lump(i):return struct.unpack_from('<II',b,8+16*i)
 po,_=lump(1);so,_=lump(19);bo,bn=lump(18);queries=[];indices=[]
 for i in range(bn//12):
  first,count,_=struct.unpack_from('<iii',b,bo+12*i)
  if count!=6:continue
  sides=[]
  for j in range(6):
   s=so+8*(first+j);plane,texture=struct.unpack_from('<Hh',b,s);p=struct.unpack_from('<4fi',b,po+20*plane)
   if p[4]&255>2:break
   sides.append(struct.pack('<4fi hBB',*p[:4],p[4]&255,texture,b[s+7],texture&255))
  if len(sides)==6:indices.append(i);queries.append(b''.join(sides))
 inp=run/'native-input.bin';inp.write_bytes(b''.join(queries));results=[];executables=[]
 for name,exe,arg,machine in [('native',out/'constructor_native_x86.exe',inp,0x14c),('owned',out/'bsp_collision_records_x64.exe',MAP,0x8664)]:
  binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Wrong architecture');target=run/(name+'.bin');p=subprocess.run([str(exe),str(arg),str(target)],capture_output=True,text=True,timeout=30);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr);results.append(target.read_bytes());executables.append(dict(executable=str(exe),sha256=sha(exe)))
 require(len(results[0])==len(results[1])==len(indices)*48,'Record count mismatch')
 # Padding at12..15 and28..31 is never written by native constructor.
 fields=list(range(12))+list(range(16,28))+list(range(32,48));different=[]
 for i in range(len(indices)):
  if any(results[0][48*i+j]!=results[1][48*i+j] for j in fields):different.append(indices[i])
 rejected=[]
 for invalid in ('zero-sides','duplicate-face','inverted-bounds'):
  fixture=cube();fp=struct.unpack_from('<I',fixture,24)[0];fs=struct.unpack_from('<I',fixture,8+19*16)[0];fb=struct.unpack_from('<I',fixture,8+18*16)[0]
  # The generic cube fixture marks all planes type3; classify its true axes.
  for axis in range(3):
   for sign in range(2):struct.pack_into('<i',fixture,fp+20*(2*axis+sign)+16,axis)
  if invalid=='zero-sides':struct.pack_into('<i',fixture,fb+4,0)
  if invalid=='duplicate-face':struct.pack_into('<H',fixture,fs+8,0)
  if invalid=='inverted-bounds':struct.pack_into('<f',fixture,fp+20+12,-10)
  source=run/(invalid+'.bsp');source.write_bytes(fixture);target=run/(invalid+'.bin');p=subprocess.run([str(out/'bsp_collision_records_x64.exe'),str(source),str(target)],capture_output=True,text=True,timeout=10);require(p.returncode!=0 and not target.exists(),'Unsafe fixture accepted or output created: '+invalid);rejected.append(dict(case=invalid,message=p.stderr.strip()))
 receipt=dict(status='PASS' if not different else 'FAIL',brushes=bn//12,optimized_records=len(indices),compared_bytes_per_record=len(fields),different_brushes=different,rejected=rejected,executables=executables,map_sha256=sha(MAP),engine_sha256=sha(ENGINE),limits=['Synthetic texture-to-surface values on both paths','Unwritten padding excluded','Full original loader/allocation not executed','No tracing or editor integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],records=len(indices),receipt=str(run/'results.json'))));require(not different,'BSP records differ');require(sha(MAP)==before and sha(ENGINE)==DIGEST,'Original changed')
if __name__=='__main__':main()
