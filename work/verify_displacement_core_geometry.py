"""Complete original CoreDisp create sequence vs owned collision composition."""
import json,math,random,struct,subprocess,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require
OUT=ROOT/'outputs/displacement-core-geometry-20261015'

def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');map_sha=sha(MAP);require(map_sha=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Map changed');bsp=MAP.read_bytes();im=Image(ENGINE)
 def lump(i,stride):
  o,n,_,_=struct.unpack_from('<4I',bsp,8+16*i);require(o+n<=len(bsp) and n%stride==0,'Lump bounds');return bsp[o:o+n]
 vv=lump(3,12);ee=lump(12,4);ss=lump(13,4);ff=lump(58,56) or lump(7,56);info=lump(26,176);dv=lump(33,20);dt=lump(48,2);queries=[];metadata=[]
 def append(power,contents,encoded,elevation,start,corners,raw,tags):
  vc=((1<<power)+1)**2;tc=2*(1<<power)**2;queries.append(struct.pack('<3I16f',power,contents,encoded,elevation,*start,*(x for c in corners for x in c))+raw+tags);metadata.append((vc,tc))
 for i in range(43):
  d=176*i;v,t,power=struct.unpack_from('<IIi',info,d+12);encoded=struct.unpack_from('<I',info,d+24)[0];elevation=struct.unpack_from('<f',info,d+28)[0];contents=struct.unpack_from('<I',info,d+32)[0];face=struct.unpack_from('<H',info,d+36)[0]*56;first,n,_,disp=struct.unpack_from('<ihhh',ff,face+4);require(n==4 and disp==i,'Benchmark quad');corners=[]
  for j in range(4):
   se=struct.unpack_from('<i',ss,4*(first+j))[0];vi=struct.unpack_from('<H',ee,4*abs(se)+(2 if se<0 else 0))[0];corners.append(struct.unpack_from('<3f',vv,12*vi))
  vc=((1<<power)+1)**2;tc=2*(1<<power)**2;append(power,contents,encoded,elevation,struct.unpack_from('<3f',info,d),corners,dv[v*20:(v+vc)*20],dt[t*2:(t+tc)*2])
 rng=random.Random(2026101584)
 for i in range(128):
  power=1+i%4;vc=((1<<power)+1)**2;tc=2*(1<<power)**2;x,y,z=[rng.uniform(-1024,1024) for _ in range(3)];w,h=rng.uniform(16,256),rng.uniform(16,256);corners=[(x,y,z),(x,y+h,z),(x+w,y+h,z),(x+w,y,z)];start=corners[i%4];raw=b''.join(struct.pack('<5f',rng.uniform(-1,1),rng.uniform(-1,1),rng.uniform(-1,1),rng.uniform(-32,32),rng.uniform(0,255)) for _ in range(vc));tags=struct.pack('<%dH'%tc,*[rng.randrange(32) for _ in range(tc)]);append(power,rng.choice([1,0x80,0x4000]),rng.choice([0x80000000,0x80000001,0x80000008,7]),rng.choice([0.,1.,64.]),start,corners,raw,tags)
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));inp=run/'queries.bin';inp.write_bytes(b''.join(queries));outputs=[];executables=[];expected=sum(60+vc*16+tc*8 for vc,tc in metadata)
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('core_geometry_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(ENGINE),str(inp),str(target)],capture_output=True,text=True,timeout=60);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));data=target.read_bytes();require(len(data)==expected,'Output length');outputs.append(data);executables.append(dict(executable=str(exe),sha256=sha(exe)))
 different=[];pos=0
 for i,(vc,tc) in enumerate(metadata):
  n=60+vc*16+tc*8
  if outputs[0][pos:pos+n]!=outputs[1][pos:pos+n]:different.append(i)
  for j in range(vc):require(all(math.isfinite(x) for x in struct.unpack_from('<4f',outputs[0],pos+60+16*j)),'Nonfinite native collision output')
  pos+=n
 prepared=ROOT/'outputs/displacement-runtime-mesh-20261015/verification-z8netl3p/queries.bin';require(sha(prepared)=='76d24b8b60040b809791f36da772294c8ec107f084e9b30d245244f1cac7e02c','Prepared source changed');source=prepared.read_bytes();sp=op=0
 for i in range(43):
  vc,tc=metadata[i];prefix=source[sp:sp+60];sp+=60;positions=source[sp:sp+vc*12];sp+=vc*12;alphas=source[sp:sp+vc*4];sp+=vc*4;triangles=source[sp:sp+tc*8];sp+=tc*8;expected_case=prefix+b''.join(positions[j*12:j*12+12]+alphas[j*4:j*4+4] for j in range(vc))+triangles;require(outputs[0][op:op+len(expected_case)]==expected_case,'Complete native creation differs from prior prepared benchmark mesh');op+=len(expected_case)
 safety=OUT/'core_geometry_safety_x64.exe';p=subprocess.run([str(safety)],capture_output=True,text=True,timeout=20);(run/'safety.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr);executables.append(dict(executable=str(safety),sha256=sha(safety)));require(sha(ENGINE)==DIGEST and sha(MAP)==map_sha,'Original changed')
 receipt=dict(status='PASS' if not different else 'FAIL',cases=len(metadata),benchmark_records=43,benchmark_matches_prior_prepared_geometry=True,vertices=sum(x[0] for x in metadata),triangles=sum(x[1] for x in metadata),compared_bytes=expected,different_count=len(different),different_cases=different[:20],executables=executables,engine_sha256=DIGEST,map_sha256=map_sha,limits=['Original private PE sections copied at preferred base; no DLL entrypoint/initialization or import resolution; guarded allocator replaces only1019D6C0','Original file-initialized global state used; live engine feature/configuration state not established','Complete original constructor/raw initializer/surface orientation/full42180creation execute, including normals/texture stages','Only collision-relevant corners/flags/contents/positions/alphas/triangle records compared; render normal/UV/LOD outputs not reconstructed or claimed','Controlled auxiliary surface fields from original constructor, no material binding; complete BSP I/O/read cursors/editor integration incomplete'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(metadata),vertices=receipt['vertices'],different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Complete creation collision outputs differ')
if __name__=='__main__':main()
