"""Original triangle plane/extrema helpers vs independent AMD64 implementation."""
import json,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require

OUT=ROOT/'outputs/displacement-triangle-metadata-20261015'
TREE_INPUT=ROOT/'outputs/displacement-collision-tree-20261015/verification-xztaj6wg/tree-input.bin'
TREE_SHA='12e30a11e4c4682f9a68568c2edea8d6097b275881897e3b6316360688d03cde'
RANGES=[(0x10159170,0x101592d6),(0x101592e0,0x10159513),(0x10244060,0x102440cd)]

def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');image=Image(ENGINE);blobs=[];header='// Checked original displacement triangle plane/extrema and normalization.\n'
 for i,(a,z) in enumerate(RANGES):
  raw=image.b[image.offset(a):image.offset(a)+z-a];blobs.append(raw);header+='static const unsigned char triangleMetadataPart%d[]={%s};\n'%(i,','.join('0x%02x'%v for v in raw))
 header+='struct TriangleMetadataFragment{unsigned offset;const unsigned char*bytes;unsigned size;};\nstatic const TriangleMetadataFragment triangleMetadataFragments[]={'+','.join('{0x%x,triangleMetadataPart%d,sizeof(triangleMetadataPart%d)}'%(a-0x10000000,i,i) for i,(a,z) in enumerate(RANGES))+'};\n'
 require(image.word(0x1031bdec)==0x3f800000 and image.word(0x10322ae8)==0x34000000,'Normalization constants')
 hp=ROOT/'work/native_displacement_triangle_metadata_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared original triangle metadata helpers');return
 require(hp.read_text()==header,'Stale original code');map_before=sha(MAP);require(map_before=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Map changed');require(sha(TREE_INPUT)==TREE_SHA,'Verified tree geometry fixture missing or changed')
 rng=random.Random(202610154);queries=[];fixture=TREE_INPUT.read_bytes();pos=0;benchmark_triangles=0
 def initial(indices):return struct.pack('<4H4f',*[int(v)|(rng.randrange(128)<<9) for v in indices],rng.randrange(65536),*[rng.uniform(-16,16) for _ in range(4)])
 for _ in range(43):
  power=struct.unpack_from('<I',fixture,pos)[0];pos+=4;vertex_count=((1<<power)+1)**2;triangle_count=2*(1<<power)**2
  vertices=fixture[pos:pos+vertex_count*12];pos+=vertex_count*12;triangles=fixture[pos:pos+triangle_count*6];pos+=triangle_count*6
  for i in range(triangle_count):
   indices=struct.unpack_from('<3H',triangles,i*6);queries.append(struct.pack('<I',vertex_count)+vertices+initial(indices));benchmark_triangles+=1
 for i in range(8192):
  count=512 if i<64 else 3;vertices=[rng.uniform(-8192,8192) for _ in range(count*3)];indices=[0,256,511] if count==512 else [0,1,2]
  if i<256:
   for index in indices:vertices[index*3:index*3+3]=[rng.choice([-0.,0.,-1.,1.]) for _ in range(3)]
  if 256<=i<512:
   scale=[0.,1.e-15,1.e-6,1.,8.,1024.][i%6];axis=i%3
   points=[[0.,0.,2.],[0.,scale,2.],[scale,0.,2.]]
   if i&1:points[1],points[2]=points[2],points[1]
   for k,index in enumerate(indices):vertices[index*3:index*3+3]=points[k][axis:]+points[k][:axis]
  queries.append(struct.pack('<I',count)+struct.pack('<%df'%len(vertices),*vertices)+initial(indices))
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));inp=run/'queries.bin';inp.write_bytes(b''.join(queries));results=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('metadata_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture')
  if name=='native_x86':require(all(blob in binary for blob in blobs),'Original code absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=40);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));raw=target.read_bytes();require(len(raw)==len(queries)*24,'Output length');results.append(raw);executables.append(dict(executable=str(exe),sha256=sha(exe)))
 different=[i for i in range(len(queries)) if results[0][i*24:i*24+24]!=results[1][i*24:i*24+24]]
 require(sha(ENGINE)==DIGEST and sha(MAP)==map_before and sha(TREE_INPUT)==TREE_SHA,'Evidence files changed')
 receipt=dict(status='PASS' if not different else 'FAIL',cases=len(queries),benchmark_triangles=benchmark_triangles,controlled_cases=8192,different_cases=different[:20],executables=executables,engine_sha256=sha(ENGINE),map_sha256=map_before,input_geometry_fixture=str(TREE_INPUT),input_geometry_sha256=TREE_SHA,original_bytes=sum(map(len,blobs)),limits=['Only original plane/packed-extrema helpers executed, not enclosing tag/alpha setup','Initial flags and upper vertex bits controlled; not yet full runtime initialization','Current process SSE floating-point environment; finite moderate/tiny/zero inputs only','Ray/triangle intersections and editor integration not implemented'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(queries),different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Triangle metadata differs')
if __name__=='__main__':main()
