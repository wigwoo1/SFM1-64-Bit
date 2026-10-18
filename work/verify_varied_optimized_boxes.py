"""Original-instruction oracle for translated, thin and asymmetric line boxes."""
import json,random,struct,subprocess,tempfile
from pathlib import Path
from verify_native_convex_brush_ray import cube
from verify_native_optimized_box import ROOT,ENGINE,DIGEST,require,sha
from pe_image import Image

def main():
 require(sha(ENGINE)==DIGEST,'Original engine changed')
 out=ROOT/'outputs/optimized-box-x64-20261015'
 run=Path(tempfile.mkdtemp(prefix='varied-',dir=out));rng=random.Random(20261015)
 engine=Image(ENGINE);raw=engine.b[engine.offset(0x10146850):engine.offset(0x10146850)+0x3d4]
 executables=[]
 for name,path,machine in [('native',ROOT/'outputs/optimized-box-native-20261015/optimized_box_native_x86.exe',0x14c),('owned',out/'optimized_box_candidate_x64.exe',0x8664)]:
  b=path.read_bytes();pe=struct.unpack_from('<I',b,60)[0];require(struct.unpack_from('<H',b,pe+4)[0]==machine,'Wrong architecture')
  if name=='native':require(raw in b,'Original box instructions absent')
  executables.append(dict(name=name,executable=str(path),sha256=sha(path)))
 boxes=[((-1,-1,-1),(1,1,1)),((100,-200,50),(101,-196,66)),((-4096,-8192,-256),(-4000,-8184,0)),((0,0,0),(.015625,.03125,.0625)),((-100,-2,-.001),(300,2,.001)),((7,8,9),(7,8,9))]
 datasets=[];differences=[]
 for index,(mins,maxs) in enumerate(boxes):
  b=cube();po=struct.unpack_from('<I',b,24)[0]
  for axis in range(3):
   struct.pack_into('<f',b,po+40*axis+12,maxs[axis]);struct.pack_into('<f',b,po+40*axis+32,-mins[axis])
  map_path=run/f'box-{index}.bsp';map_path.write_bytes(b)
  center=[(a+c)/2 for a,c in zip(mins,maxs)];span=[max(c-a,1) for a,c in zip(mins,maxs)]
  points=[center,list(mins),list(maxs)]
  for axis in range(3):
   for coordinate in [mins[axis]-span[axis],mins[axis],maxs[axis],maxs[axis]+span[axis]]:
    p=center.copy();p[axis]=coordinate;points.append(p)
  states=[(1,0,0,0xffffffff),(.125,0,0,0),(.5,.25,1,1),(1,.75,3,0)]
  qs=[]
  for start in points:
   for end in points:
    for fraction,left,flags,side in states:qs.append((0,*start,*end,fraction,left,flags,0x123456,side))
  for _ in range(2048):
   fraction,left,flags,side=rng.choice(states)
   coords=[rng.uniform(mins[a]-span[a],maxs[a]+span[a]) for a in range(3) for repeat in range(2)]
   qs.append((0,*coords[::2],*coords[1::2],fraction,left,flags,0x123456,side))
  inp=run/f'queries-{index}.bin';inp.write_bytes(b''.join(struct.pack('<I8f3I',*q) for q in qs));results=[]
  for record in executables:
   target=run/f'{index}-{record["name"]}.bin';p=subprocess.run([record['executable'],str(map_path),str(inp),str(target)],capture_output=True,text=True,timeout=30)
   target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr);results.append(target.read_bytes())
  require(len(results[0])==len(results[1])==len(qs)*36,'Wrong result length')
  count=0
  for i in range(len(qs)):
   if results[0][i*36:(i+1)*36]!=results[1][i*36:(i+1)*36]:
    count+=1
    if len(differences)<20:differences.append(dict(box=index,case=i,query=qs[i],native=struct.unpack_from('<9I',results[0],36*i),owned=struct.unpack_from('<9I',results[1],36*i)))
  datasets.append(dict(mins=mins,maxs=maxs,cases=len(qs),different_cases=count,output_sha256=sha(run/f'{index}-native.bin')))
 receipt=dict(status='PASS' if not differences else 'FAIL',datasets=datasets,cases=sum(d['cases'] for d in datasets),examples=differences,executables=executables,engine_sha256=sha(ENGINE),limits=['Six controlled axial boxes; zero-extent rays','Synthetic surface metadata','No loader classification, hull or displacement/world/editor integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=receipt['cases'],receipt=str(run/'results.json'))))
 require(not differences,'Box comparison failed; inspect saved examples');require(sha(ENGINE)==DIGEST,'Original engine changed')
if __name__=='__main__':main()
