"""Execute full original outer displacement adapter, walker and triangle test."""
import json,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require

OUT=ROOT/'outputs/displacement-world-ray-20261015'
FIXTURE=ROOT/'outputs/displacement-ray-tree-20261015/verification-y31ruqku/queries.bin'
FIXTURE_SHA='f2dbb07a66aca78bf203287743ee06385f8eac5f3d67a9f77440beff4801a459'

def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');image=Image(ENGINE)
 a,z=0x1014d010,0x1014d0f8;raw=image.b[image.offset(a):image.offset(a)+z-a]
 header='// Checked original outer world displacement adapter; oracle only.\nstatic const unsigned char nativeDispWorldRayBytes[]={'+','.join('0x%02x'%v for v in raw)+'};\n'
 hp=ROOT/'work/native_displacement_world_ray_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared checked original outer displacement adapter');return
 require(hp.read_text()==header,'Stale original adapter');require(sha(FIXTURE)==FIXTURE_SHA,'Fixture changed');map_sha=sha(MAP)
 require(map_sha=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Map changed')
 from verify_displacement_collision_tree import RANGES as tree_ranges
 from verify_displacement_collision_triangle import RANGES as metadata_ranges
 from verify_displacement_ray_triangle import RANGES as ray_ranges
 from verify_displacement_ray_tree import RANGES as walker_ranges
 blobs=[raw]+[image.b[image.offset(a):image.offset(a)+z-a] for a,z in tree_ranges+metadata_ranges+ray_ranges+walker_ranges]
 fixture=FIXTURE.read_bytes();pos=0;metadata=[];grids=0
 while pos<len(fixture):
  power=struct.unpack_from('<I',fixture,pos)[0];pos+=4;require(1<=power<=4,'Power');cells=1<<power;pos+=12*(cells+1)**2+6*2*cells*cells
  count=struct.unpack_from('<I',fixture,pos)[0];pos+=4
  for q in range(count):metadata.append((grids,q,struct.unpack_from('<10f4I',fixture,pos)));pos+=56
  grids+=1
 require(pos==len(fixture) and grids==51 and len(metadata)==13056,'Fixture layout')
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));inp=run/'queries.bin';inp.write_bytes(fixture);outputs=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('world_ray_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture')
  if name=='native_x86':require(all(blob in binary for blob in blobs),'Original routines absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=60);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));data=target.read_bytes();require(len(data)==len(metadata)*0x220,'Output length');outputs.append(data);executables.append(dict(executable=str(exe),sha256=sha(exe)))
 different=[i for i in range(len(metadata)) if outputs[0][i*0x220:(i+1)*0x220]!=outputs[1][i*0x220:(i+1)*0x220]]
 counts=dict(hits=0,first_surface=0,second_surface=0,retained=0,filtered=0,prior_displacement_marker_retained=0)
 for i,(grid,q,query) in enumerate(metadata):
  row=outputs[0][i*0x220:(i+1)*0x220];fraction=struct.unpack_from('<f',row,0x80)[0];prior=query[9];flags,contents=query[12:14];hit=fraction<prior
  counts['hits']+=hit;counts['retained']+=not hit;counts['filtered']+=bool(flags&8 or not contents&0x4081)
  marker=struct.unpack_from('<I',row,0x114)[0]
  if hit:
   disp_flags=struct.unpack_from('<H',row,0x88)[0];prop=struct.unpack_from('<H',row,0x94)[0];second=bool(disp_flags&16);counts['second_surface' if second else 'first_surface']+=1
   require(prop==(500 if second else 100)+grid and marker==1,'Surface/marker update');require(struct.unpack_from('<I',row,0x84)[0]==contents and struct.unpack_from('<I',row,0x90)[0]==0x10350ee0 and struct.unpack_from('<H',row,0x96)[0]==0,'Hit bookkeeping')
  else:
   require(fraction==prior and marker==q%2,'Prior state changed');counts['prior_displacement_marker_retained']+=marker==1
 require(counts['first_surface']>100 and counts['second_surface']>100 and counts['filtered']>100 and counts['prior_displacement_marker_retained']>100,'Coverage missing')
 require(sha(ENGINE)==DIGEST and sha(MAP)==map_sha and sha(FIXTURE)==FIXTURE_SHA,'Original evidence changed')
 receipt=dict(status='PASS' if not different else 'FAIL',grids=grids,benchmark_grids=43,queries=len(metadata),compared_bytes_per_query=0x220,coverage=counts,different_count=len(different),different_cases=different[:20],executables=executables,fixture_sha256=FIXTURE_SHA,engine_sha256=sha(ENGINE),map_sha256=map_sha,limits=['Prepared collision grids and controlled triangle tags/surface property IDs; full original BSP loader and material binding not proven','Original private x86 executes complete outer adapter, wrapper, traversal and triangle routines; engine DLL never initialized','Owned semantic displacement surface name normalized to original symbol for full work-buffer comparison','BSP traversal, brush/displacement interleaving, stationary/hull paths and editor integration remain outside this test'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],queries=len(metadata),coverage=counts,different_count=len(different),receipt=str(run/'results.json'))));require(not different,'World displacement adapter differs')
if __name__=='__main__':main()
