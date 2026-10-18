"""Execute original finite ray/triangle fraction kernel and compare AMD64."""
import json,math,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require
from verify_displacement_collision_triangle import TREE_INPUT,TREE_SHA

OUT=ROOT/'outputs/displacement-ray-triangle-20261015'
RANGES=[(0x1014dd40,0x1014dfef),(0x1014dca0,0x1014dd3a)]
CONSTANTS=[(0x1031bdec,4),(0x1031be7c,4),(0x1031cbb0,16),(0x1031d03c,4),(0x1031e234,4),(0x1033e750,8),(0x1031f3a8,8)]
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');image=Image(ENGINE);blobs=[];header='// Original displacement ray triangle primitive and tolerance helper.\n'
 for i,(a,z) in enumerate(RANGES):
  raw=image.b[image.offset(a):image.offset(a)+z-a];blobs.append(raw);header+='static const unsigned char rayTrianglePart%d[]={%s};\n'%(i,','.join('0x%02x'%v for v in raw))
 header+='struct RayTriangleFragment{unsigned offset;const unsigned char*bytes;unsigned size;};\nstatic const RayTriangleFragment rayTriangleFragments[]={'+','.join('{0x%x,rayTrianglePart%d,sizeof(rayTrianglePart%d)}'%(a-0x10000000,i,i) for i,(a,z) in enumerate(RANGES))+'};\n'
 for i,(a,n) in enumerate(CONSTANTS):
  raw=image.b[image.offset(a):image.offset(a)+n];header+='static const unsigned char rayTriangleConstant%d[]={%s};\n'%(i,','.join('0x%02x'%v for v in raw))
 header+='static const RayTriangleFragment rayTriangleConstants[]={'+','.join('{0x%x,rayTriangleConstant%d,sizeof(rayTriangleConstant%d)}'%(a-0x10000000,i,i) for i,(a,n) in enumerate(CONSTANTS))+'};\n'
 require(image.word(0x1031d03c)==0xbf800000,'Miss constant');require(struct.unpack_from('<d',image.b,image.offset(0x1033e750))[0]==1.e-6,'Determinant threshold');require(struct.unpack_from('<f',image.b,image.offset(0x1031e234))[0]==struct.unpack('<f',struct.pack('<f',.001))[0],'Ray tolerance')
 hp=ROOT/'work/native_displacement_ray_triangle_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared original ray triangle kernel');return
 require(hp.read_text()==header,'Stale original code');require(sha(TREE_INPUT)==TREE_SHA,'Verified geometry fixture changed');map_before=sha(MAP);require(map_before=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Map changed');rng=random.Random(202610155);qs=[]
 def append(points,start,delta,one):qs.append(struct.pack('<15fI',*(x for p in points for x in p),*start,*delta,int(one)))
 def targeted(points,i):
  center=[sum(p[a] for p in points)/3 for a in range(3)];a=[points[1][k]-points[0][k] for k in range(3)];b=[points[2][k]-points[0][k] for k in range(3)]
  n=[a[(k+1)%3]*b[(k+2)%3]-a[(k+2)%3]*b[(k+1)%3] for k in range(3)];length=math.sqrt(sum(x*x for x in n));n=[x/length for x in n] if length else [0,0,1]
  distance=32 if i%3 else -32;start=[center[k]+distance*n[k] for k in range(3)];delta=[-2*distance*n[k] for k in range(3)];append(points,start,delta,i%2)
 fixture=TREE_INPUT.read_bytes();pos=0;benchmark=0
 for grid in range(43):
  power=struct.unpack_from('<I',fixture,pos)[0];pos+=4;vc=((1<<power)+1)**2;tc=2*(1<<power)**2;vertices=[struct.unpack_from('<3f',fixture,pos+12*i) for i in range(vc)];pos+=vc*12
  for i in range(tc):
   ids=struct.unpack_from('<3H',fixture,pos+6*i);targeted([vertices[ids[0]],vertices[ids[2]],vertices[ids[1]]],benchmark);benchmark+=1
  pos+=tc*6
 for i in range(8192):
  points=[[rng.uniform(-1024,1024) for _ in range(3)] for _ in range(3)]
  if i<128:points=[[rng.choice([-0.,0.,-1.,1.]) for _ in range(3)] for _ in range(3)]
  if i%2:targeted(points,i)
  else:append(points,[rng.uniform(-2048,2048) for _ in range(3)],[rng.uniform(-2048,2048) for _ in range(3)],i%3==0)
 f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
 tinybits=struct.unpack('<I',struct.pack('<f',1.e-6))[0]
 for i in range(1024):
  points=[[0,0,0],[1,0,0],[0,1,0]];dz=rng.choice([-2.,-1.,0.,1.,2.,struct.unpack('<f',struct.pack('<I',tinybits+(i%3)-1))[0]])
  fraction=rng.choice([-.002,-.001,f32(-.001)-1.e-9,-0.,0.,.5,1.,f32(1.001),1.002])
  xy=rng.choice([(0.,0.),(.25,.25),(1.,0.),(0.,1.),(.5,.5),(-1.e-7,.25),(1.0000001,0.)])
  append(points,[*xy,-dz*fraction],[0.,0.,dz],i%2)
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));inp=run/'queries.bin';inp.write_bytes(b''.join(qs));results=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('ray_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture')
  if name=='native_x86':require(all(blob in binary for blob in blobs),'Original code absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=40);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));raw=target.read_bytes();require(len(raw)==len(qs)*4,'Output length');results.append(raw);executables.append(dict(executable=str(exe),sha256=sha(exe)))
 different=[i for i in range(len(qs)) if results[0][i*4:i*4+4]!=results[1][i*4:i*4+4]];values=struct.unpack('<%df'%len(qs),results[0]);hits=sum(v>=0 for v in values);misses=sum(v==-1 for v in values);require(hits>100 and misses>100,'Insufficient hit/miss coverage')
 require(sha(ENGINE)==DIGEST and sha(MAP)==map_before and sha(TREE_INPUT)==TREE_SHA,'Evidence changed')
 receipt=dict(status='PASS' if not different else 'FAIL',cases=len(qs),benchmark_triangle_queries=benchmark,hits=hits,misses=misses,different_cases=different[:20],executables=executables,engine_sha256=sha(ENGINE),map_sha256=map_before,geometry_fixture_sha256=TREE_SHA,limits=['Single start-plus-delta ray vs triangle; ray flag true, zero extents','Current process floating-point mode, finite moderate inputs','No displacement tree traversal, hit ordering, surfaces or world-trace composition','No editor rendering integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(qs),hits=hits,misses=misses,different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Ray fractions differ')
if __name__=='__main__':main()
