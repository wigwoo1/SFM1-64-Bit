import json,random,struct,subprocess,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,DIGEST,sha,require
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');image=Image(ENGINE);start=image.offset(0x10148950);raw=image.b[start:start+0x2db];out=ROOT/'outputs/extended-line-split-20261015';run=Path(tempfile.mkdtemp(prefix='verification-',dir=out));rng=random.Random(20261015);qs=[]
 for i in range(12288):
  axis=i%6;normal=[0.,0.,0.]
  if axis<3:normal[axis]=1
  else:normal=rng.choice([(0.6,0.8,0),(-.5,.5,.70710677),(.26726124,.53452248,.80178373)])
  start=[rng.uniform(-8192,8192) for _ in range(3)];end=[rng.uniform(-8192,8192) for _ in range(3)];distance=rng.uniform(-256,256);a=rng.choice([0,.125,.25,.5,.75]);b=rng.choice([a,(a+1)/2,1]);hit=rng.choice([0,.125,.5,.75,1]);after=rng.choice([0,.1,.5,1]);qs.append(struct.pack('<14f2I',*start,*end,*normal,distance,a,b,hit,after,axis,i%2))
 inp=run/'queries.bin';inp.write_bytes(b''.join(qs));results=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=out/('split_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Wrong architecture')
  if name=='native_x86':require(raw in binary,'Original body absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=30);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));results.append(target.read_bytes());executables.append(dict(executable=str(exe),sha256=sha(exe)))
 require(len(results[0])==len(results[1])==len(qs)*28,'Result size');different=[i for i in range(len(qs)) if results[0][28*i:28*(i+1)]!=results[1][28*i:28*(i+1)]]
 receipt=dict(status='PASS' if not different else 'FAIL',cases=len(qs),different_count=len(different),examples=[dict(case=i,query=struct.unpack('<14f2I',qs[i]),native=struct.unpack_from('<7I',results[0],28*i),owned=struct.unpack_from('<7I',results[1],28*i)) for i in different[:10]],executables=executables,engine_sha256=sha(ENGINE),limits=['Single axial/oblique node only; zero extent','Leaf callback controls hit fraction, collision arithmetic not exercised','Current process x87 precision','No multilevel/world/displacement/editor integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(qs),different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Extended intervals differ');require(sha(ENGINE)==DIGEST,'Original changed')
if __name__=='__main__':main()
