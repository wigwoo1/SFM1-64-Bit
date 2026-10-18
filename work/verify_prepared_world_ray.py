"""Original world walker + brushes + real displacement ray routines."""
import json,math,random,struct,subprocess,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require
OUT=ROOT/'outputs/prepared-world-ray-20261015'
MESH=ROOT/'outputs/displacement-runtime-mesh-20261015/verification-z8netl3p/queries.bin'
MESH_SHA='76d24b8b60040b809791f36da772294c8ec107f084e9b30d245244f1cac7e02c'

def main():
 require(sha(ENGINE)==DIGEST and sha(MESH)==MESH_SHA,'Evidence changed');map_sha=sha(MAP);require(map_sha=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Map changed')
 data=MESH.read_bytes();pos=0;meshes=[]
 for _ in range(43):
  power,contents,flags=struct.unpack_from('<3I',data,pos);pos+=12+48;cells=1<<power;vc=(cells+1)**2;tc=2*cells*cells
  vertices=[struct.unpack_from('<3f',data,pos+12*i) for i in range(vc)];pos+=vc*16
  triangles=[struct.unpack_from('<4H',data,pos+8*i) for i in range(tc)];pos+=tc*8;meshes.append((vertices,triangles))
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));mesh_file=run/'prepared-meshes.bin';mesh_file.write_bytes(data[:pos]);rng=random.Random(202610157);queries=[]
 for vertices,triangles in meshes:
  for i in range(128):
   ids=triangles[rng.randrange(len(triangles))][:3];p=[vertices[j&511] for j in ids];center=[sum(v[a] for v in p)/3 for a in range(3)];a=[p[1][k]-p[0][k] for k in range(3)];b=[p[2][k]-p[0][k] for k in range(3)];normal=[a[(k+1)%3]*b[(k+2)%3]-a[(k+2)%3]*b[(k+1)%3] for k in range(3)];length=math.sqrt(sum(v*v for v in normal));normal=[v/length for v in normal] if length else [0,0,1];distance=[.125,1,16,64,-.125,-1,-16,-64][i%8]
   start=[center[a]+normal[a]*distance for a in range(3)];end=[center[a]-normal[a]*distance for a in range(3)];mask=0x4081 if i<96 else [1,0x80,0x4000,0][i%4];queries.append(struct.pack('<6fI',*start,*end,mask))
 bsp=MAP.read_bytes();mo=struct.unpack_from('<I',bsp,8+14*16)[0];bounds=struct.unpack_from('<6f',bsp,mo)
 for i in range(2048):
  start=[rng.uniform(bounds[a],bounds[a+3]) for a in range(3)];end=[rng.uniform(bounds[a],bounds[a+3]) for a in range(3)]
  if i%16==0:end=start
  queries.append(struct.pack('<6fI',*start,*end,[0x4081,1,0x80,0x4000,0][i%5]))
 inp=run/'queries.bin';inp.write_bytes(b''.join(queries));outputs=[];executables=[];image=Image(ENGINE)
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('combined_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture')
  if name=='native_x86':
   from verify_displacement_ray_triangle import RANGES as triangle_ranges
   from verify_displacement_ray_tree import RANGES as tree_ranges
   ranges=triangle_ranges+tree_ranges+[(0x1014d010,0x1014d0f8),(0x1014cfa0,0x1014cfe9),(0x1014e790,0x1014e899),(0x10148950,0x10148c2b),(0x10148270,0x10148537),(0x10147690,0x1014791f),(0x10146850,0x10146c24)]
   for a,z in ranges:require(image.b[image.offset(a):image.offset(a)+z-a] in binary,'Original routine missing '+hex(a))
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(MAP),str(mesh_file),str(inp),str(target)],capture_output=True,text=True,timeout=60);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));output=target.read_bytes();require(len(output)==44*len(queries),'Output length');outputs.append(output);executables.append(dict(executable=str(exe),sha256=sha(exe)))
 safety=OUT/'combined_safety_x64.exe';p=subprocess.run([str(safety),str(MAP)],capture_output=True,text=True,timeout=30);(run/'safety.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr);executables.append(dict(executable=str(safety),sha256=sha(safety)));
 different=[i for i in range(len(queries)) if outputs[0][44*i:44*(i+1)]!=outputs[1][44*i:44*(i+1)]]
 coverage=dict(hits=0,displacement_final_hits=0,start_solid=0,clear=0)
 for i in range(len(queries)):
  row=struct.unpack_from('<11I',outputs[0],44*i);fraction=struct.unpack('<f',struct.pack('<I',row[0]))[0];coverage['hits']+=fraction<1;coverage['displacement_final_hits']+=bool(row[10]);coverage['start_solid']+=bool(row[2]);coverage['clear']+=fraction==1 and not row[2]
 require(sha(ENGINE)==DIGEST and sha(MAP)==map_sha and sha(MESH)==MESH_SHA,'Evidence changed after tests')
 covered=coverage['displacement_final_hits']>100 and coverage['hits']>100 and coverage['start_solid']>100 and coverage['clear']>100
 receipt=dict(status=('PASS' if covered else 'INSUFFICIENT_COVERAGE') if not different else 'FAIL',queries=len(queries),prepared_meshes=43,coverage=coverage,different_count=len(different),examples=[dict(case=i,query=struct.unpack('<6fI',queries[i]),native=struct.unpack_from('<11I',outputs[0],44*i),owned=struct.unpack_from('<11I',outputs[1],44*i)) for i in different[:12]],executables=executables,engine_sha256=DIGEST,map_sha256=map_sha,prepared_source_sha256=MESH_SHA,limits=['Swept zero-extent algorithm only; zero-length inputs do not prove stationary dispatcher','Prepared meshes, ordered leaf mapping and runtime records shared from independently checked owned constructors; full original loader not executed here','Complete original world walker/leaf/brush/bounds/displacement routines execute without mocked hit callbacks','Only fraction/leftSolid/solid flags/contents/plane/displacement flags/hit marker compared; surface metadata outside comparison','Mask0x2000 rejected; full material binding, stationary/hull displacement and editor integration incomplete'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],queries=len(queries),coverage=coverage,different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Combined world result differs');require(covered,'World coverage missing')
if __name__=='__main__':main()

