"""Execute native start-corner choice and complete associated surface rotation."""
import json,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require
OUT=ROOT/'outputs/displacement-surface-orientation-20261015'
RANGES=[(0x1013cfa0,0x1013d0b1),(0x1013d0c0,0x1013d567),(0x10001010,0x10001033),(0x100b4ce0,0x100b4ce3),(0x100b4cc0,0x100b4cd5),(0x100b4cf0,0x100b4d12),(0x10001150,0x1000116c),(0x102de362,0x102de36c)]

def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');im=Image(ENGINE);require(im.word(0x1031cd30)==0x4e6e6b28,'Nearest sentinel changed');blobs=[];header='// Checked original surface start-corner/rotation routines and helpers; oracle only.\n'
 for i,(a,z) in enumerate(RANGES):
  raw=im.b[im.offset(a):im.offset(a)+z-a];blobs.append(raw);header+='static const unsigned char surfaceOrientationPart%d[]={%s};\n'%(i,','.join('0x%02x'%v for v in raw))
 header+='struct SurfaceOrientationFragment{unsigned offset;const unsigned char*bytes;unsigned size;};\nstatic const SurfaceOrientationFragment surfaceOrientationFragments[]={'+','.join('{0x%x,surfaceOrientationPart%d,sizeof(surfaceOrientationPart%d)}'%(a-0x10000000,i,i) for i,(a,z) in enumerate(RANGES))+'};\n'
 hp=ROOT/'work/native_displacement_surface_orientation_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared checked original surface orientation routines');return
 require(hp.read_text()==header,'Stale native header');map_sha=sha(MAP);require(map_sha=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Map changed')
 bsp=MAP.read_bytes()
 def lump(i,stride):
  o,n,_,_=struct.unpack_from('<4I',bsp,8+i*16);require(o+n<=len(bsp) and n%stride==0,'BSP lump');return bsp[o:o+n]
 verts=lump(3,12);edges=lump(12,4);surfedges=lump(13,4);faces=lump(58,56) or lump(7,56);infos=lump(26,176);quads=[]
 for i in range(len(infos)//176):
  face=struct.unpack_from('<H',infos,i*176+36)[0]*56;first,n,_,disp=struct.unpack_from('<ihhh',faces,face+4);require(n==4 and disp==i,'Quad identity');quad=[]
  for j in range(4):
   edge=struct.unpack_from('<i',surfedges,4*(first+j))[0];vi=struct.unpack_from('<H',edges,4*abs(edge)+(2 if edge<0 else 0))[0];quad.append(struct.unpack_from('<3f',verts,vi*12))
  quads.append((quad,struct.unpack_from('<3f',infos,i*176)))
 require(len(quads)==43,'Benchmark count');rng=random.Random(2026101581);queries=[];metadata=[]
 groups=[(8,12),(0x38,12),(0x68,8),(0x118,16),(0x158,16),(0x198,48)]
 def query(corners,start,cached):
  raw=bytearray([0xa5]*0x2e8)
  for o,stride in groups:
   for j in range(stride):struct.pack_into('<f',raw,o+4*j,rng.choice([-0.,0.,rng.uniform(-256,256)]))
  for i in range(4):struct.pack_into('<f',raw,0x108+4*i,1000.+i);struct.pack_into('<3f',raw,8+12*i,*corners[i])
  struct.pack_into('<i3f',raw,0x2d8,cached,*start);queries.append(bytes(raw));metadata.append(cached)
 for corners,start in quads:
  for rotation in range(4):query(corners[rotation:]+corners[:rotation],start,-1)
 for i in range(8192):
  corners=[[rng.uniform(-8192,8192) for _ in range(3)] for _ in range(4)];start=rng.choice(corners) if i%3==0 else [rng.uniform(-8192,8192) for _ in range(3)];cached=[-1,-1,-1,0,1,2,3][i%7]
  if i<128:corners=[[rng.choice([-1.,-0.,0.,1.]) for _ in range(3)] for _ in range(4)];start=[0.,0.,0.];cached=-1
  if 128<=i<192:corners=[[0.,0.,0.]]*4;start=[10000.,30000. if i%2==0 else 29999.,0.];cached=-1
  query(corners,start,cached)
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));inp=run/'queries.bin';inp.write_bytes(b''.join(queries));outputs=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('orientation_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture')
  if name=='native_x86':require(all(blob in binary for blob in blobs),'Original routine absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=45);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));data=target.read_bytes();require(len(data)==len(queries)*748,'Output length');outputs.append(data);executables.append(dict(executable=str(exe),sha256=sha(exe)))
 different=[i for i in range(len(queries)) if outputs[0][748*i:748*(i+1)]!=outputs[1][748*i:748*(i+1)]];counts={str(i):0 for i in range(-1,4)}
 for i,cached in enumerate(metadata):
  selected=struct.unpack_from('<i',outputs[0],748*i)[0];counts[str(selected)]+=1
  if cached!=-1:require(selected==cached,'Cache ignored')
  row=outputs[0][748*i+4:748*(i+1)];require(row[0x108:0x118]==queries[i][0x108:0x118],'Unrotated scalars moved')
 prepared=ROOT/'outputs/displacement-runtime-mesh-20261015/verification-z8netl3p/queries.bin';require(sha(prepared)=='76d24b8b60040b809791f36da772294c8ec107f084e9b30d245244f1cac7e02c','Prepared mesh source changed');source=prepared.read_bytes();offset=0
 for face in range(43):
  power=struct.unpack_from('<I',source,offset)[0];expected=source[offset+12:offset+60];cells=1<<power;offset+=60+16*(cells+1)**2+8*2*cells*cells
  for rotation in range(4):require(outputs[0][748*(4*face+rotation)+12:748*(4*face+rotation)+60]==expected,'Benchmark orientation differs from prepared collision geometry')
 safety=OUT/'orientation_safety_x64.exe';p=subprocess.run([str(safety)],capture_output=True,text=True,timeout=20);(run/'safety.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr);executables.append(dict(executable=str(safety),sha256=sha(safety)))
 require(all(counts[str(i)]>100 for i in range(4)) and counts['-1']>=32,'Selection coverage');require(sha(ENGINE)==DIGEST and sha(MAP)==map_sha,'Original changed')
 receipt=dict(status='PASS' if not different else 'FAIL',cases=len(queries),benchmark_faces=43,benchmark_quad_rotations=172,benchmark_corners_match_prepared_meshes=True,compared_bytes_per_case=748,selection_counts=counts,different_count=len(different),different_cases=different[:20],executables=executables,engine_sha256=DIGEST,map_sha256=map_sha,limits=['Only original surface selection and rotation, with auxiliary finite fixture values; complete BSP loader not executed','Native rotation skipped if selector returns -1, because original caller would otherwise have an invalid index','Auxiliary data meaning, face matching, sequential vertex/triangle reads, elevation/subdivision, complete CoreDisp initialization and editor integration remain unverified'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(queries),selection_counts=counts,different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Surface orientation differs')
if __name__=='__main__':main()
