import json,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,require,sha

def main():
 image=Image(ENGINE);require(sha(ENGINE)==DIGEST,'Engine changed');start=image.offset(0x1014a5b0);raw=image.b[start:start+0x386]
 header='// Checked original box constructor; isolated tests only.\nstatic const unsigned char nativeConstructorBytes[]={'+','.join('0x%02x'%v for v in raw)+'};\n';hp=ROOT/'work/native_box_constructor_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared constructor bytes');return
 require(hp.read_text()==header,'Stale constructor header');before=sha(MAP);b=MAP.read_bytes()
 out=ROOT/'outputs/box-constructor-20261015';run=Path(tempfile.mkdtemp(prefix='verification-',dir=out));rng=random.Random(20261015);queries=[]
 def record(normal,distance,axis,texture,flags,surface):return struct.pack('<4fi hBB',*normal,distance,axis,texture,flags,surface)
 for _ in range(2048):
  sides=[]
  for axis in range(3):
   for sign in (-1,1):
    n=[0,0,0];n[axis]=sign;sides.append(record(n,rng.uniform(-8192,8192),axis,rng.choice([-1,0]),rng.randrange(256),rng.randrange(256)))
  rng.shuffle(sides);queries.append(b''.join(sides))
 def lump(i):return struct.unpack_from('<II',b,8+16*i)
 po,pn=lump(1);so,sn=lump(19);bo,bn=lump(18);selected=0
 for i in range(bn//12):
  first,count,_=struct.unpack_from('<iii',b,bo+12*i)
  if count!=6:continue
  sides=[]
  for s in range(first,first+6):
   plane,texture=struct.unpack_from('<Hh',b,so+8*s);p=struct.unpack_from('<4fi',b,po+20*plane)
   if p[4]&255>2:break
   # Constructor receives raw side byte+7. Texture-to-surface mapping is
   # deliberately synthetic; it is not a claim about installed material IDs.
   sides.append(record(p[:3],p[3],p[4]&255,texture,b[so+8*s+7],s&255))
  if len(sides)==6:queries.append(b''.join(sides));selected+=1
 inp=run/'queries.bin';inp.write_bytes(b''.join(queries));results=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=out/('constructor_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Wrong architecture')
  if name=='native_x86':require(raw in binary,'Original body absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=30);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr);results.append(target.read_bytes());executables.append(dict(executable=str(exe),sha256=sha(exe)))
 require(len(results[0])==len(results[1])==len(queries)*48,'Output length');different=[i for i in range(len(queries)) if results[0][48*i:48*(i+1)]!=results[1][48*i:48*(i+1)]]
 receipt=dict(status='PASS' if not different else 'FAIL',controlled_cases=2048,benchmark_cases=selected,cases=len(queries),record_bytes=48,different_cases=different[:20],executables=executables,engine_sha256=sha(ENGINE),map_sha256=sha(MAP),limits=['Controlled texture mapping, not actual map surface IDs','Native full loader not executed','Padding initialized to A5 on both paths','Reusable owned constructor; no editor integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(queries),receipt=str(run/'results.json'))));require(not different,'Constructor records differ');require(sha(ENGINE)==DIGEST and sha(MAP)==before,'Original changed')
if __name__=='__main__':main()
