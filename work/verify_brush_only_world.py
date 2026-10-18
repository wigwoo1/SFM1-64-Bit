import json,random,struct,subprocess,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require
OUTPUT=ROOT/'outputs/brush-only-world-20261015'
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');before=sha(MAP);b=MAP.read_bytes();mo=struct.unpack_from('<I',b,8+14*16)[0];bounds=struct.unpack_from('<6f',b,mo);image=Image(ENGINE);out=OUTPUT;run=Path(tempfile.mkdtemp(prefix='verification-',dir=out));rng=random.Random(20261015);qs=[]
 anchors=[(-2026.15625,-4541.78125,-351.90625),(-2026.15625,-4541.78125,-310.90625),(-299.5,-2278.5,-239.9375)]
 for i in range(4096):
  if i<1024:start=rng.choice(anchors);end=[start[a]+rng.uniform(-128,128) for a in range(3)]
  else:start=[rng.uniform(bounds[a],bounds[a+3]) for a in range(3)];end=[rng.uniform(bounds[a],bounds[a+3]) for a in range(3)]
  if i%16==0:end=start
  qs.append(struct.pack('<6fI',*start,*end,[0x4081,1,0x80,0x4000,0][i%5]))
 inp=run/'queries.bin';inp.write_bytes(b''.join(qs));results=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=out/('world_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Wrong architecture')
  if name=='native_x86':
   for a,z in [(0x10148950,0x10148c2b),(0x10148270,0x10148537),(0x10147690,0x1014791f),(0x10146850,0x10146c24)]:require(image.b[image.offset(a):image.offset(a)+(z-a)] in binary,'Original routine absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(MAP),str(inp),str(target)],capture_output=True,text=True,timeout=50);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));results.append(target.read_bytes());executables.append(dict(executable=str(exe),sha256=sha(exe)))
 require(len(results[0])==len(results[1])==len(qs)*36,'Output length');different=[i for i in range(len(qs)) if results[0][36*i:36*(i+1)]!=results[1][36*i:36*(i+1)]]
 counts=dict(hits=0,start_solid=0,all_solid=0,clear=0)
 for i in range(len(qs)):
  row=struct.unpack_from('<9I',results[0],36*i);fraction=struct.unpack('<f',struct.pack('<I',row[0]))[0];counts['hits']+=fraction<1;counts['clear']+=fraction==1 and not row[2];counts['start_solid']+=bool(row[2]);counts['all_solid']+=bool(row[3])
 require(counts['hits'] and counts['clear'] and counts['start_solid'],'Corpus lacks hit/clear/solid coverage')
 receipt=dict(status='PASS' if not different else 'FAIL',cases=len(qs),coverage=counts,different_count=len(different),examples=[dict(case=i,query=struct.unpack('<6fI',qs[i]),native=struct.unpack_from('<9I',results[0],36*i),owned=struct.unpack_from('<9I',results[1],36*i)) for i in different[:10]],executables=executables,map_sha256=sha(MAP),engine_sha256=sha(ENGINE),limits=['Brush-only trace; native displacement spans deliberately zero','Native runtime records supplied by previously checked owned reader/constructor','Synthetic zero surface metadata; masks with0x2000 rejected','Zero-length inputs use swept-line algorithm, not native stationary dispatch','No editor integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(qs),different_count=len(different),receipt=str(run/'results.json'))));require(not different,'World brush results differ');require(sha(ENGINE)==DIGEST and sha(MAP)==before,'Original changed')
if __name__=='__main__':main()
