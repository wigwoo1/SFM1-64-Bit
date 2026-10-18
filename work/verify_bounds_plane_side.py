import itertools,json,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');image=Image(ENGINE);offset=image.offset(0x102446e0);raw=image.b[offset:offset+0x238];header='// Original BoxOnPlaneSide instructions and absolute jump table; test only.\nstatic const unsigned char nativeBoundsSideBytes[]={'+','.join('0x%02x'%v for v in raw)+'};\n';hp=ROOT/'work/native_bounds_plane_side_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared native bounds-plane side bytes');return
 require(hp.read_text()==header,'Stale bounds-plane body');before=sha(MAP);rng=random.Random(20261015);qs=[]
 for kind,signs,distance in itertools.product(range(6),range(8),(-1,0,1)):
  normal=[-1.0 if signs&(1<<i) else 1.0 for i in range(3)];qs.append(struct.pack('<10f2I',-1,-1,-1,1,1,1,*normal,distance,kind,signs))
 for i in range(8192):
  low=[rng.uniform(-8192,8192) for _ in range(3)];high=[v+rng.uniform(0,128) for v in low];normal=[rng.uniform(-1,1) for _ in range(3)];qs.append(struct.pack('<10f2I',*low,*high,*normal,rng.uniform(-8192,8192),i%6,i%8))
 b=MAP.read_bytes();po,pn=struct.unpack_from('<II',b,24)
 for i in range(pn//20):
  p=struct.unpack_from('<4fi',b,po+20*i);signs=sum(1<<a for a in range(3) if p[a]<0)
  for low,high in [((-2027,-4543,-353),(-2025,-4541,-351)),((-32768,-32768,-32768),(32768,32768,32768))]:qs.append(struct.pack('<10f2I',*low,*high,*p[:4],p[4]&255,signs))
 out=ROOT/'outputs/bounds-plane-side-20261015';run=Path(tempfile.mkdtemp(prefix='verification-',dir=out));inp=run/'queries.bin';inp.write_bytes(b''.join(qs));results=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=out/('bounds_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Wrong architecture')
  if name=='native_x86':require(raw in binary,'Original body absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=30);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));results.append(target.read_bytes());executables.append(dict(executable=str(exe),sha256=sha(exe)))
 require(len(results[0])==len(results[1])==len(qs)*4,'Result size');different=[i for i in range(len(qs)) if results[0][4*i:4*i+4]!=results[1][4*i:4*i+4]];receipt=dict(status='PASS' if not different else 'FAIL',cases=len(qs),different_cases=different[:20],executables=executables,map_sha256=sha(MAP),engine_sha256=sha(ENGINE),limits=['Bounds/plane side primitive only','Supplied sign bits including inconsistent synthetic cases','No displacement mesh or leaf assignment integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(qs),different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Bounds-plane side differs');require(sha(MAP)==before and sha(ENGINE)==DIGEST,'Original changed')
if __name__=='__main__':main()
