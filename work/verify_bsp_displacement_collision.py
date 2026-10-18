"""BSP-backed x64 meshes and world queries against existing original oracles."""
import json,struct,subprocess,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require
OUT=ROOT/'outputs/bsp-displacement-collision-20261015'
PREPARED=ROOT/'outputs/displacement-runtime-mesh-20261015/verification-z8netl3p/queries.bin'
QUERIES=ROOT/'outputs/prepared-world-ray-20261015/verification-_1xsfa6g/queries.bin'
MESH_ORACLE=ROOT/'outputs/displacement-runtime-mesh-20261015/mesh_native_x86.exe'
WORLD_ORACLE=ROOT/'outputs/prepared-world-ray-20261015/combined_native_x86.exe'
def main():
 pins=[(ENGINE,DIGEST),(MAP,'60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea'),(PREPARED,'76d24b8b60040b809791f36da772294c8ec107f084e9b30d245244f1cac7e02c'),(QUERIES,'87824ae97aed79aa845769ab8934d051b3648e226ae5a2ae3e2665e1b8c1b087'),(MESH_ORACLE,'be923bafb3018d7f11f142c3d3bff2a0afe571164d9dced93a633846886fc45a'),(WORLD_ORACLE,'dfddb172e4fc40cdbd30bc0c4a23e7dbae72cd22944c6f6366e3a6e1d0a96605')]
 for p,h in pins:require(sha(p)==h,'Pinned evidence changed: '+str(p))
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));source=PREPARED.read_bytes();pos=0;expected_mesh_bytes=0;vertex_count=triangle_count=0
 for i in range(43):
  power=struct.unpack_from('<I',source,pos)[0];cells=1<<power;vc=(cells+1)**2;tc=2*cells*cells;pos+=60+vc*16+tc*8;expected_mesh_bytes+=120+vc*12+tc*24+cells*cells*4+((cells*cells-1)//3)*96;vertex_count+=vc;triangle_count+=tc
 prepared=run/'native-prepared-input.bin';prepared.write_bytes(source[:pos]);executables=[]
 def execute(exe,args,log,machine):
  binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Wrong executable architecture');p=subprocess.run([str(exe)]+[str(a) for a in args],capture_output=True,text=True,timeout=60);(run/log).write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));entry=dict(executable=str(exe),sha256=sha(exe));
  if entry not in executables:executables.append(entry)
 native_mesh=run/'native-meshes.bin';execute(MESH_ORACLE,[prepared,native_mesh],'native-meshes.log',0x14c);require(native_mesh.stat().st_size==expected_mesh_bytes,'Native mesh output size');native_traces=run/'native-traces.bin';execute(WORLD_ORACLE,[MAP,prepared,QUERIES,native_traces],'native-world.log',0x14c);query_count=QUERIES.stat().st_size//28;require(query_count==7552 and native_traces.stat().st_size==query_count*44,'Native trace coverage')
 exe=OUT/'bsp_collision_owned_x64.exe';modes=[]
 for hdr in [0,1]:
  dump=run/('owned-meshes-hdr%d.bin'%hdr);traces=run/('owned-traces-hdr%d.bin'%hdr);execute(exe,[MAP,hdr,dump,QUERIES,traces],'owned-hdr%d.log'%hdr,0x8664);raw=dump.read_bytes();lump,count=struct.unpack_from('<2I',raw);require(lump==(58 if hdr else 7) and count==43,'BSP selection/count');require(raw[8:]==native_mesh.read_bytes(),'BSP-backed runtime meshes differ from original initializer');require(traces.read_bytes()==native_traces.read_bytes(),'BSP-backed world queries differ from original routines');modes.append(dict(hdr_type=hdr,selected_face_lump=lump,meshes=count,mesh_bytes_compared=len(raw)-8,world_queries=query_count))
 safety=OUT/'bsp_collision_safety_x64.exe';execute(safety,[MAP],'safety.log',0x8664)
 coverage=dict(displacement_final_hits=0,hits=0,start_solid=0);native=native_traces.read_bytes()
 for i in range(query_count):
  fraction=struct.unpack_from('<f',native,44*i)[0];row=struct.unpack_from('<11I',native,44*i);coverage['displacement_final_hits']+=bool(row[10]);coverage['hits']+=fraction<1;coverage['start_solid']+=bool(row[2])
 require(coverage['displacement_final_hits']>100,'Displacement hits missing')
 for p,h in pins:require(sha(p)==h,'Evidence changed during tests')
 receipt=dict(status='PASS',benchmark_meshes=43,vertices=vertex_count,triangles=triangle_count,modes=modes,coverage=coverage,executables=executables,engine_sha256=DIGEST,map_sha256=sha(MAP),limits=['New owned loader reads BSP directly; original runtime/world executables are pinned existing builds re-executed, not freshly compiled in this task','Native runtime oracle uses previously proven prepared geometry; full original engine BSP loader orchestration is not executed','Both benchmark LDR/HDR face modes match; broader maps unverified','Inactive/non-quad displacement records and compressed/unknown-version lumps explicitly rejected','Material binding/surface metadata, stationary/hull displacement, ambient-anchor/editor integration and performance remain incomplete'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status='PASS',meshes=43,modes=2,queries_per_mode=query_count,coverage=coverage,receipt=str(run/'results.json'))))
if __name__=='__main__':main()
