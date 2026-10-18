"""Original loader face association and quad extraction blocks vs owned x64."""
import json,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require
OUT=ROOT/'outputs/displacement-face-input-20261015'
RANGES=[(0x1014c1f9,0x1014c244),(0x1014c42f,0x1014c5ae)]

def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');im=Image(ENGINE);blobs=[];header='// Original loader face scan and four-edge extraction blocks; oracle only.\n'
 for i,(a,z) in enumerate(RANGES):
  raw=im.b[im.offset(a):im.offset(a)+z-a];blobs.append(raw);header+='static const unsigned char faceInputPart%d[]={%s};\n'%(i,','.join('0x%02x'%v for v in raw))
 header+='struct FaceInputFragment{unsigned offset;const unsigned char*bytes;unsigned size;};\nstatic const FaceInputFragment faceInputFragments[]={'+','.join('{0x%x,faceInputPart%d,sizeof(faceInputPart%d)}'%(a-0x10000000,i,i) for i,(a,z) in enumerate(RANGES))+'};\n'
 hp=ROOT/'work/native_displacement_face_input_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared original loader blocks');return
 require(hp.read_text()==header,'Stale original blocks');map_sha=sha(MAP);require(map_sha=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Map changed');bsp=MAP.read_bytes()
 def lump(i,stride):
  o,n,_,_=struct.unpack_from('<4I',bsp,8+16*i);require(o+n<=len(bsp) and n%stride==0,'Lump bounds');return bsp[o:o+n]
 vertices=lump(3,12);edges=lump(12,4);surfedges=lump(13,4);faces=lump(58,56) or lump(7,56);info=lump(26,176);fixtures=[];specs=[]
 def append(dc,faces,vertices,edges,surfedges):
  counts=(dc,len(faces)//56,len(vertices)//12,len(edges)//4,len(surfedges)//4);fixtures.append(struct.pack('<5I',*counts)+faces+vertices+edges+surfedges)
  mapping=[0xffff]*dc
  for i in range(counts[1]):
   d=struct.unpack_from('<h',faces,56*i+12)[0]
   if 0<=d<dc:mapping[d]=i&65535
  quads=sum(v!=65535 and struct.unpack_from('<h',faces,56*v+8)[0]==4 for v in mapping);specs.append((dc,quads))
 append(len(info)//176,faces,vertices,edges,surfedges)
 rng=random.Random(2026101582)
 for i in range(2048):
  dc=i%33;fc=rng.randrange(0,160);vc=64;ec=64;sc=64
  vv=b''.join(struct.pack('<3f',*[rng.choice([-0.,0.,rng.uniform(-8192,8192)]) for _ in range(3)]) for _ in range(vc));ee=b''.join(struct.pack('<2H',rng.randrange(vc),rng.randrange(vc)) for _ in range(ec));ss=struct.pack('<64i',*[rng.randrange(ec)*rng.choice([-1,1]) for _ in range(sc)]);ff=bytearray(fc*56)
  for j in range(fc):struct.pack_into('<ihhh',ff,56*j+4,rng.randrange(sc-3),rng.choice([0,3,4,4,5]),0,rng.choice([-1,rng.randrange(max(dc,1)),dc,32767]))
  append(dc,ff,vv,ee,ss)
 # Native stores AX: face65535 becomes the missing sentinel; later IDs wrap.
 for fc in [65535,65536,65537,65538]:
  ff=bytearray(fc*56)
  for j in range(fc):struct.pack_into('<h',ff,56*j+12,-1)
  struct.pack_into('<h',ff,(fc-1)*56+12,0);append(1,ff,b'',b'',b'')
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));inp=run/'queries.bin';inp.write_bytes(b''.join(fixtures));outputs=[];executables=[];expected=sum(dc*2+q*48 for dc,q in specs)
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('face_input_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture')
  if name=='native_x86':require(all(blob in binary for blob in blobs),'Original block missing')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=45);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));data=target.read_bytes();require(len(data)==expected,'Output size');outputs.append(data);executables.append(dict(executable=str(exe),sha256=sha(exe)))
 pos=0;different=[]
 for i,(dc,q) in enumerate(specs):
  n=dc*2+q*48
  if outputs[0][pos:pos+n]!=outputs[1][pos:pos+n]:different.append(i)
  pos+=n
 benchmark_mapping=struct.unpack_from('<%dH'%specs[0][0],outputs[0],0)
 for i,face in enumerate(benchmark_mapping):require(face==struct.unpack_from('<H',info,176*i+36)[0],'Benchmark mapFace association mismatch')
 require(specs[0][0]==43 and specs[0][1]==43,'Benchmark coverage')
 safety=OUT/'face_input_safety_x64.exe';p=subprocess.run([str(safety)],capture_output=True,text=True,timeout=20);(run/'safety.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr);executables.append(dict(executable=str(safety),sha256=sha(safety)))
 require(sha(ENGINE)==DIGEST and sha(MAP)==map_sha,'Original changed')
 receipt=dict(status='PASS' if not different else 'FAIL',cases=len(specs),benchmark_displacements=specs[0][0],benchmark_quads=specs[0][1],benchmark_mapface_matches_native_scan=True,total_quads=sum(q for dc,q in specs),face_index_wrap_cases=4,compared_bytes=expected,different_count=len(different),different_cases=different[:20],executables=executables,engine_sha256=DIGEST,map_sha256=map_sha,limits=['Original basic blocks executed with supplied loader frame and test-only return stubs at exclusive ends; not the entire loader','Invalid negative labels below -1 are rejected only in owned guard tests, never executed in native code','Only four-edge quad extraction; non-quad mappings compared but extraction skipped','Sequential input offsets, CoreDisp allocation/initialization, elevation/subdivision/material bindings and editor integration remain incomplete'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(specs),total_quads=receipt['total_quads'],different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Face input differs')
if __name__=='__main__':main()
