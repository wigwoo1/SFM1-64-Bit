import json,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require
OUT=ROOT/'outputs/swept-world-trace-20261015'
RANGES=[(0x10149eb0,0x1014a12c),(0x10145e20,0x10145f33),(0x10146de0,0x10146e63)]
PREPARED=ROOT/'outputs/displacement-runtime-mesh-20261015/verification-z8netl3p/queries.bin'
OLD_QUERIES=ROOT/'outputs/prepared-world-ray-20261015/verification-_1xsfa6g/queries.bin'
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');im=Image(ENGINE);blobs=[];header='// Original complete world wrapper, position finisher and base trace copy; oracle only.\n'
 for i,(a,z) in enumerate(RANGES):
  raw=im.b[im.offset(a):im.offset(a)+z-a];blobs.append(raw);header+='static const unsigned char sweptWorldPart%d[]={%s};\n'%(i,','.join('0x%02x'%v for v in raw))
 header+='struct SweptWorldFragment{unsigned offset;const unsigned char*bytes;unsigned size;};\nstatic const SweptWorldFragment sweptWorldFragments[]={'+','.join('{0x%x,sweptWorldPart%d,sizeof(sweptWorldPart%d)}'%(a-0x10000000,i,i) for i,(a,z) in enumerate(RANGES))+'};\n';hp=ROOT/'work/native_swept_world_trace_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared complete original world wrapper');return
 require(hp.read_text()==header and im.word(0x1031debc)==0x7f7fffff,'Stale wrapper/constants');map_sha=sha(MAP);require(map_sha=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Map changed');require(sha(PREPARED)=='76d24b8b60040b809791f36da772294c8ec107f084e9b30d245244f1cac7e02c','Prepared input changed');require(sha(OLD_QUERIES)=='87824ae97aed79aa845769ab8934d051b3648e226ae5a2ae3e2665e1b8c1b087','Prior queries changed')
 source=PREPARED.read_bytes();pos=0
 for i in range(43):power=struct.unpack_from('<I',source,pos)[0];cells=1<<power;pos+=60+16*(cells+1)**2+8*2*cells*cells
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));prepared=run/'prepared.bin';prepared.write_bytes(source[:pos]);qs=[];metadata=[];f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0];old=OLD_QUERIES.read_bytes()
 def append(start,delta,mask,compute,available):
  start=list(map(f32,start));delta=list(map(f32,delta))
  if available and not any(delta):return
  qs.append(struct.pack('<6f3I',*start,*delta,mask,compute,available));metadata.append((start,delta,compute,available))
 for i in range(len(old)//28):
  q=struct.unpack_from('<6fI',old,28*i);append(q[:3],[f32(q[3+a]-q[a]) for a in range(3)],q[6],i%2,1)
 rng=random.Random(2026101586)
 for i in range(2048):
  start=rng.choice([[-2026.15625,-4541.78125,-351.90625],[-299.5,-2278.5,-239.9375],[100000.,100000.,100000.]]);delta=[rng.uniform(-128,128) for _ in range(3)];append(start,delta,0x4081,1,1)
 for i in range(32):append([i,-i,0],[0,0,0],1,1,0)
 inp=run/'queries.bin';inp.write_bytes(b''.join(qs));outputs=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('wrapper_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture')
  if name=='native_x86':require(all(blob in binary for blob in blobs),'Original wrappers absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(MAP),str(prepared),str(inp),str(target)],capture_output=True,text=True,timeout=60);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));data=target.read_bytes();require(len(data)==len(qs)*68,'Output length');outputs.append(data);executables.append(dict(executable=str(exe),sha256=sha(exe)))
 different=[i for i in range(len(qs)) if outputs[0][68*i:68*(i+1)]!=outputs[1][68*i:68*(i+1)]];coverage=dict(displacement_hits=0,start_solid=0,roundtrip_delta_changes=0,positions_disabled=0,empty_world=0)
 for i,(start,delta,compute,available) in enumerate(metadata):
  row=struct.unpack_from('<17I',outputs[0],i*68);coverage['displacement_hits']+=bool(row[10]);coverage['start_solid']+=bool(row[2]);coverage['positions_disabled']+=not compute;coverage['empty_world']+=not available;coverage['roundtrip_delta_changes']+=any(f32(f32(start[a]+delta[a])-start[a])!=delta[a] for a in range(3))
 require(coverage['displacement_hits']>100 and coverage['roundtrip_delta_changes']>100,'Coverage missing');safety=OUT/'wrapper_safety_x64.exe';p=subprocess.run([str(safety),str(MAP)],capture_output=True,text=True,timeout=30);(run/'safety.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr);executables.append(dict(executable=str(safety),sha256=sha(safety)));require(sha(ENGINE)==DIGEST and sha(MAP)==map_sha,'Original changed')
 receipt=dict(status='PASS' if not different else 'FAIL',cases=len(qs),coverage=coverage,different_count=len(different),different_cases=different[:20],executables=executables,engine_sha256=DIGEST,map_sha256=map_sha,limits=['Complete original10149EB0 wrapper/finisher/copy with controlled work acquire/release and memset dependencies; actual BSP brush/displacement swept routines execute','Native runtime setup/prepared meshes controlled as previous oracles; owned path loads BSP directly','Zero-extent swept queries only; stationary rays with real world explicitly unsupported; empty-world zero queries checked','68semantic bytes compared; material surface metadata/plane type bookkeeping not compared','Ambient cache anchor integration, stationary/hull displacement and editor integration incomplete'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(qs),coverage=coverage,different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Full world wrapper differs')
if __name__=='__main__':main()
