import json, random, struct, subprocess, sys, tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT, ENGINE, DIGEST, sha, require
OUT=ROOT/'outputs/stationary-displacement-normal-20261015'
RANGES=[(0x1014cd40,0x1014cdf5),(0x10158e80,0x10158ea6),(0x1014e580,0x1014e5d0)]
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');im=Image(ENGINE);header='// Original stationary normal selector and point-only bounds helpers; oracle only.\n';blobs=[]
 for i,(a,z) in enumerate(RANGES):
  raw=im.b[im.offset(a):im.offset(a)+z-a];blobs.append(raw);header+='static const unsigned char normalPart%d[]={%s};\n'%(i,','.join('0x%02x'%v for v in raw))
 header+='struct StationaryNormalFragment{unsigned offset;const unsigned char*bytes;unsigned size;};\nstatic const StationaryNormalFragment stationaryNormalFragments[]={'+','.join('{0x%x,normalPart%d,sizeof(normalPart%d)}'%(a-0x10000000,i,i) for i,(a,z) in enumerate(RANGES))+'};\n'
 hp=ROOT/'work/native_stationary_displacement_normal_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);return
 require(hp.read_text()==header,'Stale original bytes');OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));rng=random.Random(2026101591);queries=[];expected=[];coverage=dict(empty=0,fallback=0,selected=0,boundary=0)
 for i in range(8192):
  meshes=[]
  for j in range(4):
   low=[rng.randint(-8,0) for _ in range(3)];high=[rng.randint(0,8) for _ in range(3)];meshes.append((low,high,[float(j+1),float(-j),float(j%2)],rng.choice([0,1,2,128,0x4000,0x4081])))
  point=[rng.randint(-12,12) for _ in range(3)];refs=[rng.randrange(4) for _ in range(i%9)];mask=rng.choice([0,1,2,128,0x4000,0x4081,0xffffffff]);initial=[7.,-8.,9.];initial_contents=0x12345678
  if i%4==0 and refs:point=list(meshes[refs[-1]][i%2]);coverage['boundary']+=1
  normal=initial;contents=initial_contents
  if not refs:coverage['empty']+=1
  else:
   normal=meshes[refs[0]][2];contents=1;selected=False
   for ref in refs:
    low,high,n,c=meshes[ref]
    if c&mask and all(low[a]<=point[a]<=high[a] for a in range(3)):normal=n;contents=c;selected=True;break
   coverage['selected' if selected else 'fallback']+=1
  row=struct.pack('<3f2I8I3fI',*point,mask,len(refs),*(refs+[0]*(8-len(refs))),*initial,initial_contents)
  for low,high,n,c in meshes:row+=struct.pack('<9fI',*low,*high,*n,c)
  require(len(row)==228,'Fixture size');queries.append(row);expected.append(struct.pack('<3fI',*normal,contents))
 inp=run/'queries.bin';inp.write_bytes(b''.join(queries));want=b''.join(expected);executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('normal_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture');target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=60);(run/(name+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr);actual=target.read_bytes();different=[i for i in range(len(queries)) if actual[16*i:16*i+16]!=want[16*i:16*i+16]];require(len(actual)==len(want) and not different,'Mismatch '+str(different[:20]));executables.append(dict(executable=str(exe),sha256=sha(exe)))
 require(sha(ENGINE)==DIGEST,'Engine modified');receipt=dict(status='PASS',cases=len(queries),semantic_bytes_per_case=16,coverage=coverage,executables=executables,engine_sha256=DIGEST,limits=['Normal-selection helper only; not complete stationary displacement collision','Native executes original selector, point-only runtime bounds branch and inclusive bounds primitive privately without DLL entrypoint','No hull path, world probe, editor integration or complete SFM compatibility claim','Owned malformed-reference and bounds rejection preserve outputs; native invalid inputs deliberately excluded'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status='PASS',cases=len(queries),coverage=coverage,receipt=str(run/'results.json'))))
if __name__=='__main__':main()
