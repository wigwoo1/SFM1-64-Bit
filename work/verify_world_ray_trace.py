import itertools, json, math, random, re, struct, subprocess, tempfile
from pathlib import Path
from verify_native_convex_brush_ray import ROOT, ENGINE, MAP, DIGEST, sha, require
from verify_swept_world_trace import PREPARED
OUT=ROOT/'outputs/world-ray-trace-20261015'
PRIOR=ROOT/'outputs/swept-world-trace-20261015/verification-qsjl5qhn'
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');map_sha=sha(MAP);require(map_sha=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Map changed');require(sha(PREPARED)=='76d24b8b60040b809791f36da772294c8ec107f084e9b30d245244f1cac7e02c','Prepared input changed')
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));source=PREPARED.read_bytes();pos=0;meshes=[]
 for i in range(43):
  power=struct.unpack_from('<I',source,pos)[0];cells=1<<power;vc=(cells+1)**2;tc=2*cells*cells;corners=[struct.unpack_from('<3f',source,pos+12+12*j) for j in range(4)]
  # Prepared layout stores all positions followed by all alphas.
  vertices=[struct.unpack_from('<3f',source,pos+60+12*j) for j in range(vc)];triangles=[struct.unpack_from('<3H',source,pos+60+16*vc+8*j) for j in range(tc)];meshes.append((corners,vertices,triangles));pos+=60+16*vc+8*tc
 prepared=run/'prepared.bin';prepared.write_bytes(source[:pos]);prior=(PRIOR/'queries.bin').read_bytes();require(len(prior)==9504*36,'Prior swept corpus length');qs=[prior[i:i+36] for i in range(0,len(prior),36)];f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0];categories=['prior']*len(qs)
 def append(point,mask,category,compute=True,available=True):qs.append(struct.pack('<6f3I',*map(f32,point),0.,0.,0.,mask,int(compute),int(available)));categories.append(category)
 masks=[1,128,0x4000,0x4081,0,0xffffffff&~0x2000]
 for mi,(corners,vertices,triangles) in enumerate(meshes):
  e3=[corners[3][a]-corners[0][a] for a in range(3)];e1=[corners[1][a]-corners[0][a] for a in range(3)];normal=[e3[(a+1)%3]*e1[(a+2)%3]-e3[(a+2)%3]*e1[(a+1)%3] for a in range(3)];length=math.sqrt(sum(x*x for x in normal));normal=[x/length for x in normal]
  for ti in sorted(set([0,1,len(triangles)//4,len(triangles)//2,len(triangles)-2,len(triangles)-1])):
   tri=triangles[ti];center=[sum(vertices[index&511][a] for index in tri)/3 for a in range(3)]
   for offset in [-16.,-2.,-.03125,0.,.03125,2.,16.]:
    for mask in [1,0x4081,0]:append([center[a]+offset*normal[a] for a in range(3)],mask,'mesh-centroid',((mi+ti)%3)!=0)
  for vi in [0,len(vertices)//2,len(vertices)-1]:
   for offset in [-1.,0.,1.]:append([vertices[vi][a]+offset*normal[a] for a in range(3)],0x4081,'mesh-vertex')
 rng=random.Random(2026101592)
 for i in range(4096):append([rng.uniform(-5000,2500),rng.uniform(-6500,1000),rng.uniform(-600,600)],rng.choice(masks),'random',i%2==0)
 for i in range(32):append([i,-i,0],1,'empty',True,False)
 inp=run/'queries.bin';inp.write_bytes(b''.join(qs));outputs=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('world_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture');target=run/(name+'.bin');args=[str(exe),str(MAP),str(prepared),str(inp),str(target)];
  if name=='native_x86':args.append(str(ENGINE))
  p=subprocess.run(args,capture_output=True,text=True,timeout=90);(run/(name+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));actual=target.read_bytes();require(len(actual)==len(qs)*68,'Output length');outputs.append(actual);executables.append(dict(executable=str(exe),sha256=sha(exe)))
  if name=='native_x86':
   match=re.search(r'stationary_probe_count=(\d+) stationary_probed_queries=(\d+)',p.stdout);require(match is not None,'Native probe instrumentation missing');probe_count,probed_queries=map(int,match.groups())
 different=[i for i in range(len(qs)) if outputs[0][68*i:68*i+68]!=outputs[1][68*i:68*i+68]];details=[]
 for i in different[:12]:details.append(dict(case=i,category=categories[i],query=struct.unpack('<6f3I',qs[i]),native=struct.unpack_from('<17I',outputs[0],68*i),owned=struct.unpack_from('<17I',outputs[1],68*i)))
 coverage=dict(stationary=0,stationary_marker=0,stationary_solid_with_marker=0,stationary_solid=0,stationary_clear=0,native_normal_probes=probe_count,native_stationary_queries_with_probe=probed_queries)
 for i,category in enumerate(categories):
  if category=='prior':continue
  row=struct.unpack_from('<17I',outputs[0],68*i);coverage['stationary']+=1;coverage['stationary_marker']+=bool(row[10]);coverage['stationary_solid_with_marker']+=bool(row[10] and row[2]);coverage['stationary_solid']+=bool(row[2]);coverage['stationary_clear']+=not row[2]
 # Original D320 finishing branch with controlled CE00 probe results. This is
 # separate policy evidence, not a claim of geometric displacement-solid hits.
 finish_queries=[];finish_expected=[]
 for fraction,left,start_solid,all_solid,marker in itertools.product([0.,.125,.5,1.],[0.,.25,1.],[0,1],[0,1],[0,1,7]):
  row=struct.pack('<2f3I4f2I',fraction,left,start_solid,all_solid,0x4081,11.,-12.,13.,-14.,0xbeef,marker);finish_queries.append(row);answer=bytearray(row)
  if marker and start_solid:struct.pack_into('<2f3I',answer,0,0.,0.,1,1,0x4081)
  else:struct.pack_into('<2f3I',answer,0,1.,0.,0,0,0)
  finish_expected.append(bytes(answer))
 finish_input=run/'finish-queries.bin';finish_input.write_bytes(b''.join(finish_queries))
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('finish_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Finish architecture');target=run/('finish-'+name+'.bin');p=subprocess.run([str(exe),str(ENGINE),str(finish_input),str(target)],capture_output=True,text=True,timeout=30);(run/('finish-'+name+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr);require(target.read_bytes()==b''.join(finish_expected),'Stationary finishing policy mismatch');executables.append(dict(executable=str(exe),sha256=sha(exe)))
 safety=OUT/'world_safety_x64.exe';p=subprocess.run([str(safety),str(MAP)],capture_output=True,text=True,timeout=30);(run/'safety.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr);executables.append(dict(executable=str(safety),sha256=sha(safety)))
 prior_match=outputs[0][:9504*68]==(PRIOR/'native_x86.bin').read_bytes();coverage_ok=probed_queries>100 and coverage['stationary_clear']>0;require(sha(ENGINE)==DIGEST and sha(MAP)==map_sha,'Original changed');receipt=dict(status='PASS' if not different and prior_match and coverage_ok else 'FAIL',cases=len(qs),coverage=coverage,controlled_finish_cases=len(finish_queries),safety=p.stdout.strip(),different_count=len(different),details=details,prior_native_swept_bytes_unchanged=prior_match,executables=executables,engine_path=str(ENGINE),engine_sha256=DIGEST,map_path=str(MAP),map_sha256=map_sha,query_sha256=sha(inp),prepared_sha256=sha(prepared),limits=['Complete original world wrapper and stationary point path execute from private pinned engine image; host work acquire/release and memset are controlled','Native preprepared benchmark meshes versus owned direct BSP loader','Zero extents only; hulls, material surface metadata, plane type bookkeeping, engine work pool and editor integration excluded','68 semantic output bytes compared; no live SFM process dependency in owned x64 implementation','Benchmark stationary queries did not produce displacement hit markers; hit/solid finishing branch checked separately against original D320 with a controlled probe callback'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(qs),coverage=coverage,controlled_finish_cases=len(finish_queries),different_count=len(different),details=details[:3],receipt=str(run/'results.json'))));require(not different and prior_match,'World wrapper differs');require(coverage_ok,'Stationary normal-probe coverage missing')
if __name__=='__main__':main()
