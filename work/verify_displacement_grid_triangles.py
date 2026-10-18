"""Original CoreDisp index generation and record copying vs independent x64."""
import json,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require

OUT=ROOT/'outputs/displacement-grid-triangles-20261015'
MAP_DIGEST='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea'
RANGES=[(0x1013d960,0x1013da5a),(0x1013da60,0x1013db5c),(0x1013db60,0x1013dc29),(0x1013dc30,0x1013dc43),(0x10141c70,0x10141cec)]
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');image=Image(ENGINE);blobs=[];header='// Checked original grid index generation and triangle copy.\n'
 for i,(a,z) in enumerate(RANGES):
  raw=image.b[image.offset(a):image.offset(a)+z-a];blobs.append(raw);header+='static const unsigned char trianglePart%d[]={%s};\n'%(i,','.join('0x%02x'%v for v in raw))
 header+='struct TriangleFragment{unsigned offset;const unsigned char*bytes;unsigned size;};\nstatic const TriangleFragment triangleFragments[]={'+','.join('{0x%x,trianglePart%d,sizeof(trianglePart%d)}'%(a-0x10000000,i,i) for i,(a,z) in enumerate(RANGES))+'};\n';hp=ROOT/'work/native_displacement_triangle_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared checked original triangle code');return
 require(hp.read_text()==header,'Stale original code');require(sha(MAP)==MAP_DIGEST,'Map changed');data=MAP.read_bytes()
 def lump(index):
  offset,size,_,_=struct.unpack_from('<4I',data,8+16*index);require(offset+size<=len(data),'Lump range');return data[offset:offset+size]
 infos=lump(26);rawtags=lump(48);require(len(infos)%176==0 and len(rawtags)%2==0,'Record strides');qs=[];powers=[];benchmark_count=len(infos)//176
 for i in range(benchmark_count):
  d=176*i;power=struct.unpack_from('<I',infos,d+20)[0];start=struct.unpack_from('<I',infos,d+16)[0]*2;count=2*(1<<power)**2;tags=rawtags[start:start+count*2];require(len(tags)==count*2,'Tag span');qs.append(struct.pack('<I',power)+tags);powers.append(power)
 rng=random.Random(202610152)
 for power in range(1,5):
  count=2*(1<<power)**2
  for mode in range(68):
   values=[(0 if mode==0 else 0xffff if mode==1 else (1<<(i%16)) if mode==2 else rng.randrange(65536)) for i in range(count)]
   qs.append(struct.pack('<I',power)+struct.pack('<%dH'%count,*values));powers.append(power)
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));inp=run/'queries.bin';inp.write_bytes(b''.join(qs));results=[];executables=[];expected_size=sum(4+14*(2*(1<<p)**2) for p in powers)
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('triangles_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Wrong architecture')
  if name=='native_x86':require(all(b in binary for b in blobs),'Original code absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=30);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));raw=target.read_bytes();require(len(raw)==expected_size,'Output length');results.append(raw);executables.append(dict(executable=str(exe),sha256=sha(exe)))
 pos=0;different=[];benchmark_triangles=0
 for i,power in enumerate(powers):
  triangles=2*(1<<power)**2;size=4+14*triangles
  require(struct.unpack_from('<I',results[0],pos)[0]==triangles*3,'Index count')
  if results[0][pos:pos+size]!=results[1][pos:pos+size]:different.append(i)
  if i<benchmark_count:benchmark_triangles+=triangles
  pos+=size
 require(sha(ENGINE)==DIGEST and sha(MAP)==MAP_DIGEST,'Original changed')
 receipt=dict(status='PASS' if not different else 'FAIL',cases=len(qs),distinct_grid_powers=[1,2,3,4],benchmark_records=benchmark_count,benchmark_triangles=benchmark_triangles,total_triangle_records=sum(2*(1<<p)**2 for p in powers),different_cases=different[:20],executables=executables,engine_sha256=sha(ENGINE),map_sha256=sha(MAP),original_code_bytes=sum(map(len,blobs)),limits=['Original grid index order and copying to CoreDisp records only','Tag words preinitialized by test, original BSP tag loader not executed','Complete collision triangle metadata, tree and world traces not assembled','No editor/rendering integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(qs),different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Triangle generation differs')
if __name__=='__main__':main()
