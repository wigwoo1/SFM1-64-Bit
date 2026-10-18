import json,random,struct,subprocess,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');before=sha(MAP);b=MAP.read_bytes();mo=struct.unpack_from('<I',b,8+14*16)[0];bounds=struct.unpack_from('<6f',b,mo);image=Image(ENGINE);offset=image.offset(0x10148950);body=image.b[offset:offset+0x2db];out=ROOT/'outputs/benchmark-line-walk-20261015';run=Path(tempfile.mkdtemp(prefix='verification-',dir=out));rng=random.Random(20261015);qs=[]
 anchors=[(-2026.15625,-4541.78125,-351.90625),(-2026.15625,-4541.78125,-310.90625),(-299.5,-2278.5,-239.9375)]
 for i in range(2048):
  if i<512:start=rng.choice(anchors);end=[start[a]+rng.uniform(-128,128) for a in range(3)]
  else:start=[rng.uniform(bounds[a],bounds[a+3]) for a in range(3)];end=[rng.uniform(bounds[a],bounds[a+3]) for a in range(3)]
  a=rng.choice([0,.125,.5]);c=rng.choice([(a+1)/2,1]);hit=rng.choice([.25,.5,1]);after=rng.choice([0,.1,.5,1]);qs.append(struct.pack('<14f2I',*start,*end,1,0,0,0,a,c,hit,after,0,i%2))
 inp=run/'queries.bin';inp.write_bytes(b''.join(qs));results=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=out/('split_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Wrong architecture')
  if name=='native_x86':require(body in binary,'Original walker absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target),str(MAP)],capture_output=True,text=True,timeout=50);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));results.append(target.read_bytes());executables.append(dict(executable=str(exe),sha256=sha(exe)))
 def decode(data):
  position=0;records=[]
  for _ in qs:
   require(position+4<=len(data),'Truncated event count');count=struct.unpack_from('<I',data,position)[0];length=4+12*count;require(position+length<=len(data),'Truncated leaf events');records.append(data[position:position+length]);position+=length
  require(position==len(data),'Trailing events');return records
 a,c=map(decode,results);different=[i for i in range(len(qs)) if a[i]!=c[i]]
 receipt=dict(status='PASS' if not different else 'FAIL',cases=len(qs),leaf_events=sum((len(v)-4)//12 for v in a),different_cases=different[:20],examples=[dict(case=i,query=struct.unpack('<14f2I',qs[i]),native_words=list(struct.unpack('<'+'I'*(len(a[i])//4),a[i]))[:40],owned_words=list(struct.unpack('<'+'I'*(len(c[i])//4),c[i]))[:40]) for i in different[:5]],executables=executables,map_sha256=sha(MAP),engine_sha256=sha(ENGINE),limits=['Actual benchmark tree; controlled leaf callbacks only','No brush/displacement/surface collision or editor integration','Current x87precision/zeroextent only'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(qs),leaf_events=receipt['leaf_events'],different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Benchmark leaf walks differ');require(sha(MAP)==before and sha(ENGINE)==DIGEST,'Original changed')
if __name__=='__main__':main()
