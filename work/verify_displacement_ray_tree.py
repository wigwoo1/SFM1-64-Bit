"""Combined original displacement tree traversal, triangle fractions and planes."""
import json,math,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require
from verify_displacement_collision_triangle import TREE_INPUT,TREE_SHA

OUT=ROOT/'outputs/displacement-ray-tree-20261015'
RANGES=[(0x1015a130,0x1015a565),(0x1015b520,0x1015b5a3)]
CONSTANTS=[(0x1033fa60,4),(0x103b6020,16),(0x103b6030,16)]
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');image=Image(ENGINE);blobs=[];header='// Original zero-extent displacement tree traversal.\n'
 for i,(a,z) in enumerate(RANGES):
  raw=image.b[image.offset(a):image.offset(a)+z-a];blobs.append(raw);header+='static const unsigned char rayTreePart%d[]={%s};\n'%(i,','.join('0x%02x'%v for v in raw))
 header+='struct RayTreeFragment{unsigned offset;const unsigned char*bytes;unsigned size;};\nstatic const RayTreeFragment rayTreeFragments[]={'+','.join('{0x%x,rayTreePart%d,sizeof(rayTreePart%d)}'%(a-0x10000000,i,i) for i,(a,z) in enumerate(RANGES))+'};\n'
 for i,(a,n) in enumerate(CONSTANTS):
  raw=image.b[image.offset(a):image.offset(a)+n];header+='static const unsigned char rayTreeConstant%d[]={%s};\n'%(i,','.join('0x%02x'%v for v in raw))
 header+='static const RayTreeFragment rayTreeConstants[]={'+','.join('{0x%x,rayTreeConstant%d,sizeof(rayTreeConstant%d)}'%(a-0x10000000,i,i) for i,(a,n) in enumerate(CONSTANTS))+'};\n'
 require(struct.unpack_from('<f',image.b,image.offset(0x1033fa60))[0]==.03125,'Bounds padding constant')
 hp=ROOT/'work/native_displacement_ray_tree_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared checked original ray tree traversal');return
 require(hp.read_text()==header,'Stale original traversal');require(sha(TREE_INPUT)==TREE_SHA,'Geometry fixture changed');map_digest=sha(MAP);require(map_digest=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Map changed')
 from verify_displacement_collision_tree import RANGES as tree_ranges
 from verify_displacement_collision_triangle import RANGES as metadata_ranges
 from verify_displacement_ray_triangle import RANGES as ray_ranges
 all_blobs=blobs+[image.b[image.offset(a):image.offset(a)+z-a] for a,z in tree_ranges+metadata_ranges+ray_ranges]
 fixture=TREE_INPUT.read_bytes();pos=0;grids=[]
 for _ in range(43):
  start=pos;power=struct.unpack_from('<I',fixture,pos)[0];pos+=4;vc=((1<<power)+1)**2;tc=2*(1<<power)**2;vertices=[struct.unpack_from('<3f',fixture,pos+12*i) for i in range(vc)];pos+=vc*12;triangles=[struct.unpack_from('<3H',fixture,pos+6*i) for i in range(tc)];pos+=tc*6;grids.append((fixture[start:pos],vertices,triangles,False))
 for power in range(1,5):
  cells=1<<power;width=cells+1
  for collapsed in [False,True]:
   vertices=[(float(x%2 if collapsed else x),float(y%2 if collapsed else y),0.) for y in range(width) for x in range(width)];triangles=[]
   for y in range(cells):
    for x in range(cells):
     i=y*width+x
     triangles.extend([(i,i+width,i+1),(i+1,i+width,i+width+1)] if i&1 else [(i,i+width,i+width+1),(i,i+width+1,i+1)])
   raw=struct.pack('<I',power)+b''.join(struct.pack('<3f',*v) for v in vertices)+b''.join(struct.pack('<3H',*t) for t in triangles);grids.append((raw,vertices,triangles,True))
 rng=random.Random(202610156);f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0];max_float=struct.unpack('<f',struct.pack('<I',0x7f7fffff))[0];chunks=[];query_metadata=[]
 for grid_id,(raw,vertices,triangles,controlled) in enumerate(grids):
  qs=[];mins=[min(p[a] for p in vertices) for a in range(3)];maxs=[max(p[a] for p in vertices) for a in range(3)]
  for i in range(256):
   prior=rng.choice([0.,-0.,.25,.5,.75,1.]);prior_tri=rng.choice([0xffffffff,0,len(triangles)-1]);one=i%2
   if i<192:
    ids=triangles[rng.randrange(len(triangles))];points=[vertices[j&511] for j in ids];center=[sum(p[a] for p in points)/3 for a in range(3)]
    a=[points[1][k]-points[0][k] for k in range(3)];b=[points[2][k]-points[0][k] for k in range(3)];n=[a[(k+1)%3]*b[(k+2)%3]-a[(k+2)%3]*b[(k+1)%3] for k in range(3)];length=math.sqrt(sum(x*x for x in n));n=[x/length for x in n] if length else [0,0,1]
    distance=rng.choice([-64.,-1.,1.,64.]);s=[center[k]+distance*n[k] for k in range(3)];d=[-2*distance*n[k] for k in range(3)]
    if i<16:s=list(points[i%3]);d=[rng.choice([-1.,0.,1.]) for _ in range(3)]
   else:s=[rng.uniform(mins[a]-64,maxs[a]+64) for a in range(3)];end=[rng.uniform(mins[a]-64,maxs[a]+64) for a in range(3)];d=[end[a]-s[a] for a in range(3)]
   if controlled and i<64:
    x=rng.choice([0.,.25,.5,1.]);y=rng.choice([0.,.25,.5,1.]);s=[x,y,1.];d=[0.,0.,-2.];prior=[0.,.5,1.,.25][i%4];one=i%2
   if i==255:d=[0.,-0.,0.]
   s=list(map(f32,s));d=list(map(f32,d));inv=[f32(1./v) if v!=0 else max_float for v in d]
   flags=rng.choice([0,0,8,16,0xffffffff]);contents=rng.choice([1,0x80,0x4000,0x4081,0,0x1000])
   qs.append(struct.pack('<10f4I',*s,*d,*inv,prior,one,prior_tri,flags,contents));query_metadata.append((grid_id,prior,prior_tri,flags,contents))
  chunks.append(raw+struct.pack('<I',len(qs))+b''.join(qs))
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));inp=run/'queries.bin';inp.write_bytes(b''.join(chunks));results=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('tree_ray_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture')
  if name=='native_x86':require(all(blob in binary for blob in all_blobs),'Original code missing')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=60);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));output=target.read_bytes();require(len(output)==len(query_metadata)*112,'Output length');results.append(output);executables.append(dict(executable=str(exe),sha256=sha(exe)))
 different=[i for i in range(len(query_metadata)) if results[0][112*i:112*i+112]!=results[1][112*i:112*i+112]];improved=0;unchanged=0;ties=0;wrapper_hits=0;filtered=0
 for i,(_,prior,prior_tri,flags,contents) in enumerate(query_metadata):
  fraction,triangle=struct.unpack_from('<fI',results[0],112*i)
  if fraction<prior:improved+=1
  else:require(fraction==prior and triangle==prior_tri,'Prior hit unexpectedly changed');unchanged+=1
  if prior==.5 and fraction==.5 and triangle==prior_tri:ties+=1
  wrapper_hit=struct.unpack_from('<I',results[0],112*i+24)[0];wrapper_hits+=wrapper_hit
  if flags&8 or not contents&0x4081:require(wrapper_hit==0,'Eligibility gate');filtered+=1
 require(improved>100 and unchanged>100 and ties>100 and wrapper_hits>100 and filtered>100,'Hit/prior/tie/filter coverage')
 require(sha(ENGINE)==DIGEST and sha(MAP)==map_digest and sha(TREE_INPUT)==TREE_SHA,'Evidence changed')
 receipt=dict(status='PASS' if not different else 'FAIL',grids=len(grids),benchmark_grids=43,queries=len(query_metadata),improved_hits=improved,retained_prior_hits=unchanged,half_fraction_prior_retained=ties,wrapper_hits=wrapper_hits,filtered_by_flags_or_contents=filtered,different_cases=different[:20],executables=executables,engine_sha256=sha(ENGINE),map_sha256=map_digest,geometry_fixture_sha256=TREE_SHA,limits=['Zero-extent ray traversal; hull path not reconstructed','Original arrays preallocated and triangle tag bits controlled before plane/extrema helpers','Half-fraction retention count includes misses; explicit true-tie behavior checked separately by safety test','84-byte wrapper trace compared, but outer world contents/surface updates not tested','Full original mesh loader and world-trace integration remain incomplete','No editor/rendering integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],grids=len(grids),queries=len(query_metadata),improved_hits=improved,different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Tree ray results differ')
if __name__=='__main__':main()
