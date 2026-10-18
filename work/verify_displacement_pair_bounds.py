import json,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,DIGEST,sha,require
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');image=Image(ENGINE);ranges=[(0x10159a40,0x10159c6e),(0x102450e0,0x10245117),(0x10245120,0x10245186),(0x102de362,0x102de36c)];header='// Checked original collision pair bounds and helpers.\n';blobs=[]
 for i,(a,z) in enumerate(ranges):
  raw=image.b[image.offset(a):image.offset(a)+(z-a)];blobs.append(raw);header+='static const unsigned char pairBoundsPart'+str(i)+'[]={'+','.join('0x%02x'%v for v in raw)+'};\n'
 header+='struct PairBoundsFragment{unsigned offset;const unsigned char*bytes;unsigned size;};\nstatic const PairBoundsFragment pairBoundsFragments[]={'+','.join('{0x%x,pairBoundsPart%d,sizeof(pairBoundsPart%d)}'%(a-0x10000000,i,i) for i,(a,z) in enumerate(ranges))+'};\n';hp=ROOT/'work/native_displacement_pair_bounds_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared original pair-bounds code');return
 require(hp.read_text()==header,'Stale original code');rng=random.Random(20261015);qs=[]
 for i in range(8192):
  vertices=[rng.uniform(-32768,32768) for _ in range(18)]
  if i<128:vertices=[rng.choice([-1.,-0.,0.,1.]) for _ in range(18)]
  packed=[rng.randrange(6)|(rng.randrange(128)<<9) for _ in range(6)];qs.append(struct.pack('<18f6I',*vertices,*packed))
 out=ROOT/'outputs/displacement-pair-bounds-20261015';run=Path(tempfile.mkdtemp(prefix='verification-',dir=out));inp=run/'queries.bin';inp.write_bytes(b''.join(qs));results=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=out/('pair_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Wrong architecture')
  if name=='native_x86':require(all(blob in binary for blob in blobs),'Original code absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=30);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));results.append(target.read_bytes());executables.append(dict(executable=str(exe),sha256=sha(exe)))
 require(len(results[0])==len(results[1])==len(qs)*24,'Output length');different=[i for i in range(len(qs)) if results[0][24*i:24*(i+1)]!=results[1][24*i:24*(i+1)]];receipt=dict(status='PASS' if not different else 'FAIL',cases=len(qs),different_cases=different[:20],executables=executables,engine_sha256=sha(ENGINE),limits=['Tight bounds for one collision-tree triangle pair','Synthetic vertices/packedindices; no actual displacement mesh reconstruction','Internal tree nodes/one-unit expansion not executed','No renderer/editor integration']);(run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(qs),different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Pair bounds differ');require(sha(ENGINE)==DIGEST,'Original changed')
if __name__=='__main__':main()
