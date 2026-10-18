"""Compare original complete runtime mesh initializer on prepared CoreDisp data."""
import json,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require
from verify_displacement_collision_triangle import TREE_INPUT,TREE_SHA
from verify_native_displacement_positions import benchmark_queries

OUT=ROOT/'outputs/displacement-runtime-mesh-20261015'
RANGES=[(0x1015bf90,0x1015c3ad),(0x1015c6b0,0x1015c754),(0x101599b0,0x10159a35),(0x1015b1d0,0x1015b378),(0x10141be0,0x10141c2d)]
CONSTANTS=[(0x1031bdec,4),(0x10322ae8,4),(0x1031debc,4),(0x1033ef78,4),(0x10351c44,4)]
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');image=Image(ENGINE);blobs=[];header='// Checked complete runtime collision mesh initializer and prepared-data helpers.\n'
 for i,(a,z) in enumerate(RANGES):
  raw=image.b[image.offset(a):image.offset(a)+z-a];blobs.append(raw);header+='static const unsigned char runtimeMeshPart%d[]={%s};\n'%(i,','.join('0x%02x'%v for v in raw))
 header+='struct RuntimeMeshFragment{unsigned offset;const unsigned char*bytes;unsigned size;};\nstatic const RuntimeMeshFragment runtimeMeshFragments[]={'+','.join('{0x%x,runtimeMeshPart%d,sizeof(runtimeMeshPart%d)}'%(a-0x10000000,i,i) for i,(a,z) in enumerate(RANGES))+'};\n'
 for i,(a,n) in enumerate(CONSTANTS):
  raw=image.b[image.offset(a):image.offset(a)+n];header+='static const unsigned char runtimeMeshConstant%d[]={%s};\n'%(i,','.join('0x%02x'%v for v in raw))
 header+='static const RuntimeMeshFragment runtimeMeshConstants[]={'+','.join('{0x%x,runtimeMeshConstant%d,sizeof(runtimeMeshConstant%d)}'%(a-0x10000000,i,i) for i,(a,n) in enumerate(CONSTANTS))+'};\n'
 require(struct.unpack_from('<f',image.b,image.offset(0x10351c44))[0]==382.5,'Alpha threshold')
 hp=ROOT/'work/native_displacement_runtime_mesh_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared original runtime mesh initializer');return
 require(hp.read_text()==header,'Stale original code');require(sha(TREE_INPUT)==TREE_SHA,'Verified geometry fixture changed');map_digest=sha(MAP);require(map_digest=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Map changed')
 fixture=TREE_INPUT.read_bytes();mapdata=MAP.read_bytes()
 def lump(index):
  offset,size,_,_=struct.unpack_from('<4I',mapdata,8+16*index);require(offset+size<=len(mapdata),'Lump range');return mapdata[offset:offset+size]
 infos=lump(26);dispverts=lump(33);disptris=lump(48);base_queries=benchmark_queries();rng=random.Random(202610157);pos=0;queries=[];powers=[];benchmark_triangle_count=0
 threshold_bits=struct.unpack('<I',struct.pack('<f',127.5))[0]
 while pos<len(fixture):
  grid=len(queries);power=struct.unpack_from('<I',fixture,pos)[0];pos+=4;vc=((1<<power)+1)**2;tc=2*(1<<power)**2
  vertices=fixture[pos:pos+12*vc];pos+=12*vc;indices=[struct.unpack_from('<3H',fixture,pos+6*i) for i in range(tc)];pos+=6*tc
  if grid<43:
   corners=base_queries[grid][4:52];contents=struct.unpack_from('<I',infos,grid*176+32)[0];flags=0
   vert_start=struct.unpack_from('<I',infos,grid*176+12)[0];tri_start=struct.unpack_from('<I',infos,grid*176+16)[0]
   alphas=[struct.unpack_from('<f',dispverts,(vert_start+i)*20+16)[0] for i in range(vc)];tags=[struct.unpack_from('<H',disptris,(tri_start+i)*2)[0] for i in range(tc)];benchmark_triangle_count+=tc
  else:
   corners=struct.pack('<12f',*[rng.uniform(-8192,8192) for _ in range(12)]);contents=rng.randrange(1<<32);flags=rng.randrange(1<<32)
   value=[0.,255.,127.5,struct.unpack('<f',struct.pack('<I',threshold_bits-1))[0],struct.unpack('<f',struct.pack('<I',threshold_bits+1))[0]][grid%5]
   alphas=[rng.uniform(-256,512) if grid%7==0 else value for _ in range(vc)];tags=[rng.randrange(65536) for _ in range(tc)]
  triangles=b''.join(struct.pack('<4H',*index,tag) for index,tag in zip(indices,tags));queries.append(struct.pack('<3I',power,contents,flags)+corners+vertices+struct.pack('<%df'%vc,*alphas)+triangles);powers.append(power)
 require(pos==len(fixture) and len(queries)==427,'Geometry fixture grid count')
 from verify_displacement_collision_tree import RANGES as tree_ranges
 from verify_displacement_collision_triangle import RANGES as metadata_ranges
 all_blobs=blobs+[image.b[image.offset(a):image.offset(a)+z-a] for a,z in tree_ranges+metadata_ranges]
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));inp=run/'queries.bin';inp.write_bytes(b''.join(queries));results=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('mesh_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture')
  if name=='native_x86':require(all(blob in binary for blob in all_blobs),'Original code absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=60);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));results.append(target.read_bytes());executables.append(dict(executable=str(exe),sha256=sha(exe)))
 cursor=0;different=[];alpha_counts={'first':0,'second':0,'both':0};total_triangles=0
 for i,power in enumerate(powers):
  vc=((1<<power)+1)**2;leaves=(1<<power)**2;tc=2*leaves;nodes=(leaves-1)//3;size=120+vc*12+tc*24+leaves*4+nodes*96
  if results[0][cursor:cursor+size]!=results[1][cursor:cursor+size]:different.append(i)
  for t in range(tc):
   flags=struct.unpack_from('<H',results[0],cursor+72+12*vc+24*t+6)[0];mode=flags&0x600;require(mode in (0x200,0x400,0x600),'Missing alpha classification');alpha_counts[{0x200:'first',0x400:'second',0x600:'both'}[mode]]+=1
  total_triangles+=tc;cursor+=size
 require(cursor==len(results[0])==len(results[1]),'Output length');require(all(v>0 for v in alpha_counts.values()),'Alpha mode coverage');require(sha(ENGINE)==DIGEST and sha(MAP)==map_digest and sha(TREE_INPUT)==TREE_SHA,'Evidence changed')
 receipt=dict(status='PASS' if not different else 'FAIL',meshes=len(powers),benchmark_meshes=43,benchmark_triangles=benchmark_triangle_count,total_triangles=total_triangles,alpha_classifications=alpha_counts,different_meshes=different[:20],executables=executables,engine_sha256=sha(ENGINE),map_sha256=map_digest,geometry_fixture_sha256=TREE_SHA,limits=['Prepared CoreDisp inputs; original full BSP corner/position/alpha loading not executed','Native arrays preallocated; allocation/growth paths not tested','Triangle storage initially zero in both probes, including initializer-unwritten upper flag bits','Compared semantic fields and arrays; opaque pointers, debug storage size and material surface binding excluded','No world-trace or editor integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],meshes=len(powers),triangles=total_triangles,different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Runtime mesh constructor differs')
if __name__=='__main__':main()
