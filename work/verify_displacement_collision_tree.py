"""Compare native pair mapping, recursive node bounds and bounds finishing.

Uses prior original-code position/index probes for combined mesh input. No
original installation writes, DLL initialization, or SFM process is involved.
"""
import json,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require
from verify_native_displacement_positions import benchmark_queries,original_fragments

OUT=ROOT/'outputs/displacement-collision-tree-20261015'
MAP_DIGEST='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea'
RANGES=[(0x1015b8e0,0x1015b9e3),(0x1015b040,0x1015b0c7),(0x1015b0d0,0x1015b14f),
        (0x10159a40,0x10159c6e),(0x102450e0,0x10245117),(0x10245120,0x10245186),
        (0x102de362,0x102de36c),(0x1015c6d1,0x1015c754)]

def execute(exe,inp,target,code_blobs=(),machine=0x14c):
 binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0]
 require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture')
 require(all(b in binary for b in code_blobs),'Original code absent from executable')
 result=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=45)
 target.with_suffix('.log').write_text(result.stdout+result.stderr)
 require(result.returncode==0,result.stdout+result.stderr+' exit='+str(result.returncode))
 return target.read_bytes(),dict(executable=str(exe),sha256=sha(exe))

def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');image=Image(ENGINE);blobs=[];header='// Checked original displacement collision pair/tree bounds routines.\n'
 for i,(a,z) in enumerate(RANGES):
  raw=image.b[image.offset(a):image.offset(a)+z-a];blobs.append(raw);header+='static const unsigned char treePart%d[]={%s};\n'%(i,','.join('0x%02x'%v for v in raw))
 header+='struct TreeFragment{unsigned offset;const unsigned char*bytes;unsigned size;};\nstatic const TreeFragment treeFragments[]={'+','.join('{0x%x,treePart%d,sizeof(treePart%d)}'%(a-0x10000000,i,i) for i,(a,z) in enumerate(RANGES))+'};\n'
 hp=ROOT/'work/native_displacement_tree_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared original tree routines');return
 require(hp.read_text()==header,'Stale native tree code');require(sha(MAP)==MAP_DIGEST,'Map changed')
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));rng=random.Random(202610153)
 grid_queries=benchmark_queries();benchmark_count=len(grid_queries)
 for i in range(256):
  power=1+i%4;count=((1<<power)+1)**2;corners=[rng.uniform(-16384,16384) for _ in range(12)]
  if i<32:corners=[rng.choice([-1.,-0.,0.,1.]) for _ in range(12)]
  offsets=b''.join(struct.pack('<5f',*[rng.uniform(-1,1) for _ in range(3)],rng.uniform(-256,256),0.) for _ in range(count))
  grid_queries.append(struct.pack('<I12f',power,*corners)+offsets)
 position_input=run/'position-input.bin';position_input.write_bytes(b''.join(grid_queries))
 _,position_blobs=original_fragments()
 positions,position_exe=execute(ROOT/'outputs/native-displacement-positions-20261015/positions_native_x86.exe',position_input,run/'native-positions.bin',position_blobs)
 data=MAP.read_bytes()
 def lump(index):
  offset,size,_,_=struct.unpack_from('<4I',data,8+16*index);return data[offset:offset+size]
 infos=lump(26);rawtags=lump(48);tag_queries=[]
 for i,q in enumerate(grid_queries):
  power=struct.unpack_from('<I',q)[0];count=2*(1<<power)**2
  if i<benchmark_count:start=struct.unpack_from('<I',infos,i*176+16)[0]*2;tags=rawtags[start:start+count*2]
  else:tags=struct.pack('<%dH'%count,*[rng.randrange(65536) for _ in range(count)])
  require(len(tags)==count*2,'Tag length');tag_queries.append(struct.pack('<I',power)+tags)
 triangle_input=run/'triangle-input.bin';triangle_input.write_bytes(b''.join(tag_queries))
 from verify_displacement_grid_triangles import RANGES as triangle_ranges
 triangle_blobs=[image.b[image.offset(a):image.offset(a)+z-a] for a,z in triangle_ranges]
 triangle_data,triangle_exe=execute(ROOT/'outputs/displacement-grid-triangles-20261015/triangles_native_x86.exe',triangle_input,run/'native-triangles.bin',triangle_blobs)
 queries=[];powers=[];vp=0;tp=0
 for q in grid_queries:
  power=struct.unpack_from('<I',q)[0];vertex_count=((1<<power)+1)**2;triangle_count=2*(1<<power)**2
  require(struct.unpack_from('<I',positions,vp)[0]==vertex_count,'Position count');vp+=4;vertices=positions[vp:vp+vertex_count*12];vp+=vertex_count*12
  require(struct.unpack_from('<I',triangle_data,tp)[0]==triangle_count*3,'Triangle count');tp+=4;triangles=triangle_data[tp:tp+triangle_count*6];tp+=triangle_count*14
  queries.append(struct.pack('<I',power)+vertices+triangles);powers.append(power)
 require(vp==len(positions) and tp==len(triangle_data),'Reference output length')
 for i in range(128):
  power=1+i%4;vertex_count=((1<<power)+1)**2;triangle_count=2*(1<<power)**2
  values=[rng.uniform(-32768,32768) for _ in range(vertex_count*3)]
  if i<32:values=[rng.choice([-0.,0.,-1.,1.]) for _ in values]
  packed=[rng.randrange(vertex_count)|(rng.randrange(128)<<9) for _ in range(triangle_count*3)]
  queries.append(struct.pack('<I',power)+struct.pack('<%df'%len(values),*values)+struct.pack('<%dH'%len(packed),*packed));powers.append(power)
 inp=run/'tree-input.bin';inp.write_bytes(b''.join(queries));results=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  raw,exe=execute(OUT/('tree_'+name+'.exe'),inp,run/(name+'.bin'),blobs if name=='native_x86' else (),machine);results.append(raw);executables.append(exe)
 pos=0;different=[];nodes=0;leaves=0;benchmark_bounds=[]
 for i,power in enumerate(powers):
  leaf_count=1<<(2*power);node_count=(leaf_count-1)//3;size=8+4*leaf_count+96*node_count+48
  require(struct.unpack_from('<2I',results[0],pos)==(node_count,leaf_count),'Output tree counts')
  if results[0][pos:pos+size]!=results[1][pos:pos+size]:different.append(i)
  if i<benchmark_count:benchmark_bounds.append(dict(displacement=i,tight=list(struct.unpack_from('<6f',results[0],pos+size-48)),expanded=list(struct.unpack_from('<6f',results[0],pos+size-24))))
  pos+=size;nodes+=node_count;leaves+=leaf_count
 require(pos==len(results[0])==len(results[1]),'Tree output length')
 require(not different,'Tree differs before leaf assignment')
 # Feed each side's verified expanded bounds to its corresponding mapper.
 # Run the checked owned mapper first: it rejects queue/output limits before
 # attempting the original mapper on these actual-sized benchmark bounds.
 leaf_outputs={};leaf_executables=[];leaf_stride=1026*4
 assignment_raw=image.b[image.offset(0x1014d610):image.offset(0x1014d610)+0x242]
 for name,machine in [('owned_x64',0x8664),('native_x86',0x14c)]:
  exe=ROOT/'outputs/displacement-leaf-assignment-20261015'/('selection_'+name+'.exe')
  binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Leaf mapper architecture')
  if name=='native_x86':require(assignment_raw in binary,'Original mapper absent')
  leaf_input=run/(name+'-leaf-input.bin');leaf_input.write_bytes(b''.join(struct.pack('<6f',*b['expanded']) for b in benchmark_bounds))
  leaf_target=run/(name+'-leaf-output.bin')
  p=subprocess.run([str(exe),str(MAP),str(leaf_input),str(leaf_target)],capture_output=True,text=True,timeout=40)
  leaf_target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode))
  raw=leaf_target.read_bytes();require(len(raw)==benchmark_count*leaf_stride,'Leaf mapper output length');leaf_outputs[name]=raw;leaf_executables.append(dict(executable=str(exe),sha256=sha(exe)))
 require(leaf_outputs['owned_x64']==leaf_outputs['native_x86'],'Actual displacement leaf mapping differs')
 leaf_reference_count=0
 for i,b in enumerate(benchmark_bounds):
  record=struct.unpack_from('<1026I',leaf_outputs['native_x86'],i*leaf_stride);require(record[0]==record[1] and record[0]<=1024,'Leaf reference counts')
  b['ordered_leaves']=list(record[2:2+record[0]]);leaf_reference_count+=record[0]
 require(sha(ENGINE)==DIGEST and sha(MAP)==MAP_DIGEST,'Original changed')
 receipt=dict(status='PASS',cases=len(queries),benchmark_grids=benchmark_count,controlled_original_generated_grids=256,controlled_pointcloud_grids=128,node_records=nodes,leaf_pairs=leaves,different_cases=[],executables=executables,input_reference_executables=[position_exe,triangle_exe],leaf_assignment_executables=leaf_executables,benchmark_ordered_leaf_references=leaf_reference_count,engine_sha256=sha(ENGINE),map_sha256=sha(MAP),benchmark_bounds=benchmark_bounds,limits=['Native arrays preallocated; allocation/growth/memmove branches not exercised','Only bounds-finishing block of initializer executed; full original initialization not run','Positions have zero elevation/subdivision offsets; original full corner loader not run','Runtime triangle plane/edge metadata and actual displacement ray collision not reconstructed','No editor/rendering integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(queries),node_records=nodes,leaf_pairs=leaves,different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Tree differs')
if __name__=='__main__':main()
