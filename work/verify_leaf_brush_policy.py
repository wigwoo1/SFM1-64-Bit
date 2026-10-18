import itertools,json,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,DIGEST,sha,require
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');image=Image(ENGINE);start=image.offset(0x10148270);raw=image.b[start:start+0x2c7];header='// Original leaf routine; brush callbacks controlled, displacement spans empty.\nstatic const unsigned char nativeLeafPolicyBytes[]={'+','.join('0x%02x'%v for v in raw)+'};\n';hp=ROOT/'work/native_leaf_policy_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared checked leaf policy bytes');return
 require(hp.read_text()==header,'Stale native leaf body');out=ROOT/'outputs/leaf-brush-policy-20261015';run=Path(tempfile.mkdtemp(prefix='verification-',dir=out));rng=random.Random(20261015);qs=[]
 masks=[0,1,0x80,0x2080,0x2081,0x4081,0xffffffff]
 orders=[[],[0],[0,0,1],[3,2,1,0],[0,1,2,3,0,1,2,3]]
 for mask,order,action,box in itertools.product(masks,orders,range(4),(0,15)):
  packed=bytes(order+[255]*(8-len(order)));qs.append([mask,0,5,box,0,0x3f800000,1,0x80,0x2080,0x4000,action,action,action,action,*struct.unpack('<2I',packed)])
 for _ in range(4096):
  order=[rng.randrange(4) for _ in range(rng.randrange(9))];packed=bytes(order+[255]*(8-len(order)));qs.append([rng.choice(masks),rng.randrange(16),rng.randrange(16),rng.randrange(16),rng.randrange(2),rng.choice([0,0x3f000000,0x3f800000]),*[rng.choice([0,1,0x80,0x2080,0x4000]) for i in range(4)],*[rng.randrange(4) for i in range(4)],*struct.unpack('<2I',packed)])
 inp=run/'queries.bin';inp.write_bytes(b''.join(struct.pack('<16I',*q) for q in qs));results=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=out/('leaf_policy_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Wrong architecture')
  if name=='native_x86':require(raw in binary,'Original leaf instructions absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=30);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));results.append(target.read_bytes());executables.append(dict(executable=str(exe),sha256=sha(exe)))
 require(len(results[0])==len(results[1])==len(qs)*60,'Result size');different=[i for i in range(len(qs)) if results[0][60*i:60*(i+1)]!=results[1][60*i:60*(i+1)]]
 receipt=dict(status='PASS' if not different else 'FAIL',cases=len(qs),different_cases=different[:20],executables=executables,engine_sha256=sha(ENGINE),limits=['Original leaf body, controlled brush callback replaces collision arithmetic','Empty displacement spans','Synthetic nondefault surface flags; default65535 surface not covered','Only one leaf per invocation; no world traversal/editor integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(qs),receipt=str(run/'results.json'))));require(not different,'Leaf policy differs');require(sha(ENGINE)==DIGEST,'Original changed')
if __name__=='__main__':main()
