import itertools,json,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,DIGEST,sha,require
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');image=Image(ENGINE);start=image.offset(0x10148950);raw=image.b[start:start+0x2db];header='// Original swept-line walker; controlled one-node tree/leaf callbacks.\nstatic const unsigned char nativeLineSplitBytes[]={'+','.join('0x%02x'%v for v in raw)+'};\n';hp=ROOT/'work/native_line_split_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared native walker');return
 require(hp.read_text()==header,'Stale native body');out=ROOT/'outputs/line-plane-split-20261015';run=Path(tempfile.mkdtemp(prefix='verification-',dir=out));rng=random.Random(20261015)
 points=[-32768,-3,-1,-.03125,-.000001,-0.0,0,.000001,.03125,1,3,32768];qs=list(itertools.product(points,repeat=2));qs.extend((rng.uniform(-32768,32768),rng.uniform(-32768,32768)) for _ in range(8192));inp=run/'queries.bin';inp.write_bytes(b''.join(struct.pack('<2f',*q) for q in qs));results=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=out/('split_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Wrong architecture')
  if name=='native_x86':require(raw in binary,'Original body absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=30);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));results.append(target.read_bytes());executables.append(dict(executable=str(exe),sha256=sha(exe)))
 require(len(results[0])==len(results[1])==len(qs)*28,'Result length');different=[i for i in range(len(qs)) if results[0][28*i:28*(i+1)]!=results[1][28*i:28*(i+1)]]
 receipt=dict(status='PASS' if not different else 'FAIL',cases=len(qs),different_cases=different[:20],examples=[dict(case=i,query=qs[i],native=struct.unpack_from('<7I',results[0],28*i),owned=struct.unpack_from('<7I',results[1],28*i)) for i in different[:10]],executables=executables,engine_sha256=sha(ENGINE),limits=['One axial node, zero extent, initial global interval0..1, hitfraction1','Leaf callback records intervals without collision','Current process x87 precision; other precision modes not claimed','No oblique/multilevel/world/displacement/editor integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(qs),receipt=str(run/'results.json'))));require(not different,'Split intervals differ');require(sha(ENGINE)==DIGEST,'Original changed')
if __name__=='__main__':main()
