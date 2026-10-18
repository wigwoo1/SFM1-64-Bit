import json, random, struct, subprocess, tempfile
from pathlib import Path
from verify_native_convex_brush_ray import ROOT, ENGINE, MAP, DIGEST, sha, require
from verify_swept_world_trace import PREPARED
OUT=ROOT/'outputs/bsp-light-cache-position-20261015'
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');map_sha=sha(MAP);require(map_sha=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Map changed');require(sha(PREPARED)=='76d24b8b60040b809791f36da772294c8ec107f084e9b30d245244f1cac7e02c','Prepared input changed');OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));source=PREPARED.read_bytes();pos=0
 for i in range(43):power=struct.unpack_from('<I',source,pos)[0];cells=1<<power;pos+=60+16*(cells+1)**2+8*2*cells*cells
 prepared=run/'prepared.bin';prepared.write_bytes(source[:pos]);points=[];seen=set();outside_domain=0
 def append(point):
  nonlocal outside_domain
  if any(abs(x)>32768 for x in point):outside_domain+=1;return
  raw=struct.pack('<3f',*point)
  if raw not in seen:seen.add(raw);points.append(struct.unpack('<3f',raw))
 for p in [(1,2,3),(0,0,0),(16,16,64),(-16,-16,-64),(-32,-32,-128),(32,32,128),(-2026.15625,-4541.78125,-351.90625),(-299.5,-2278.5,-239.9375)]:append(p)
 previous=ROOT/'outputs/world-ray-trace-20261015/verification-2i321pi7/queries.bin';old=previous.read_bytes()
 for i in range(0,len(old),36):append(struct.unpack_from('<3f',old,i))
 rng=random.Random(2026101594)
 for i in range(2048):
  center=[rng.randrange(-156,78)*32+16,rng.randrange(-203,32)*32+16,rng.randrange(-5,5)*128+64];append(center);append([center[0]+.25,center[1]-.5,center[2]+1.])
 inp=run/'queries.bin';inp.write_bytes(b''.join(struct.pack('<6f3I',*p,0.,0.,0.,0x4081,1,1) for p in points));outputs=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('anchor_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture');target=run/(name+'.bin');args=[str(exe),str(MAP),str(prepared),str(inp),str(target)]
  if name=='native_x86':args.append(str(ENGINE))
  p=subprocess.run(args,capture_output=True,text=True,timeout=120);(run/(name+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));actual=target.read_bytes();require(len(actual)==len(points)*92,'Output length');outputs.append(actual);executables.append(dict(executable=str(exe),sha256=sha(exe)))
 different=[i for i in range(len(points)) if outputs[0][92*i:92*i+92]!=outputs[1][92*i:92*i+92]];details=[]
 for i in different[:12]:details.append(dict(case=i,point=points[i],native=struct.unpack_from('<23I',outputs[0],92*i),owned=struct.unpack_from('<23I',outputs[1],92*i)))
 coverage=dict(one_trace=0,two_traces=0,stationary_traces=0,blocked_traces=0,start_solid_traces=0,point_retained=0,point_moved=0)
 for i,point in enumerate(points):
  r=struct.unpack_from('<23I',outputs[0],92*i);require(r[1] in [1,2],'Trace count');coverage['one_trace' if r[1]==1 else 'two_traces']+=1
  for event in range(r[1]):
   at=92*i+8+36*event;start_delta_mask=struct.unpack_from('<6fI',outputs[0],at);fraction,solid=struct.unpack_from('<fI',outputs[0],at+28);require(start_delta_mask[6]==0x4081,'Cache world mask');coverage['stationary_traces']+=all(x==0 for x in start_delta_mask[3:6]);coverage['blocked_traces']+=fraction<1.;coverage['start_solid_traces']+=bool(solid)
  anchor=struct.unpack_from('<3f',outputs[0],92*i+80);coverage['point_retained' if anchor==point else 'point_moved']+=1
 safety=OUT/'anchor_safety_x64.exe';p=subprocess.run([str(safety),str(MAP)],capture_output=True,text=True,timeout=30);(run/'safety.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr);executables.append(dict(executable=str(safety),sha256=sha(safety)))
 require(sha(ENGINE)==DIGEST and sha(MAP)==map_sha,'Original changed');coverage_ok=coverage['stationary_traces']>0 and coverage['blocked_traces']>0 and coverage['point_moved']>0 and coverage['two_traces']>0;receipt=dict(status='PASS' if not different and coverage_ok else 'FAIL',cases=len(points),outside_checked_coordinate_domain_inputs_excluded=outside_domain,different_count=len(different),details=details,coverage=coverage,safety=p.stdout.strip(),executables=executables,engine_sha256=DIGEST,map_sha256=map_sha,query_sha256=sha(inp),limits=['Original full cache-position policy/cell/point-leaf/contents/clip routines execute on real benchmark spatial records','Test bridge checks original filter requests world-only type1, then executes original full world collision wrapper; EngineTraceClient virtual dispatch and entity provider are not executed','Native prepared benchmark meshes versus owned direct BSP displacement loader; earlier separate loader/initializer evidence applies','92 bytes compare input leaf, trace count, ordered start/delta/mask/fraction/start-solid events and final cache position','Existing checked cache-cell domain is finite coordinates within +/-32768; outside-domain inputs explicitly excluded and rejected by owned policy','No model-caller anchor selection, ambient interpolation combination, shader upload or editor integration claim'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(points),coverage=coverage,different_count=len(different),details=details[:2],receipt=str(run/'results.json'))));require(not different,'Cache positions/events differ');require(coverage_ok,'Cache branch coverage missing')
if __name__=='__main__':main()
