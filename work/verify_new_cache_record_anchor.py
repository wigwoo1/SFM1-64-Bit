import json, struct, subprocess, tempfile
from pathlib import Path
from verify_native_convex_brush_ray import ROOT, ENGINE, MAP, DIGEST, sha, require
from verify_swept_world_trace import PREPARED
OUT=ROOT/'outputs/new-cache-record-anchor-20261015'
PRIOR=ROOT/'outputs/bsp-light-cache-position-20261015/verification-p9l7g6a0'
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');map_sha=sha(MAP);require(map_sha=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Map changed');require(sha(PREPARED)=='76d24b8b60040b809791f36da772294c8ec107f084e9b30d245244f1cac7e02c','Prepared input changed');prior_queries=(PRIOR/'queries.bin').read_bytes();require(sha(PRIOR/'queries.bin')=='1151b799bf687e7a83be1df4d439b594939d484d5add57caf23e8742b5e73556','Prior cache query corpus changed');prior_results=(PRIOR/'native_x86.bin').read_bytes();require(len(prior_queries)==17407*36 and len(prior_results)==17407*92,'Prior corpus length')
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));source=PREPARED.read_bytes();pos=0
 for i in range(43):power=struct.unpack_from('<I',source,pos)[0];cells=1<<power;pos+=60+16*(cells+1)**2+8*2*cells*cells
 prepared=run/'prepared.bin';prepared.write_bytes(source[:pos]);queries=[];settings=[0,1,2,-1,-2147483648];want=[]
 for i in range(17407):
  for setting in settings:
   row=bytearray(prior_queries[36*i:36*i+36]);struct.pack_into('<i',row,24,setting);queries.append(bytes(row));answer=bytearray(prior_results[92*i:92*i+92])
   if setting==0:struct.pack_into('<I',answer,4,0);answer[8:80]=bytes(72);answer[80:92]=row[:12]
   want.append(bytes(answer))
 inp=run/'queries.bin';inp.write_bytes(b''.join(queries));expected=b''.join(want);outputs=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('selection_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture');target=run/(name+'.bin');args=[str(exe),str(MAP),str(prepared),str(inp),str(target)]
  if name=='native_x86':args.append(str(ENGINE))
  p=subprocess.run(args,capture_output=True,text=True,timeout=120);(run/(name+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));actual=target.read_bytes();require(len(actual)==len(queries)*92,'Output length');outputs.append(actual);executables.append(dict(executable=str(exe),sha256=sha(exe)))
 different=[i for i in range(len(queries)) if outputs[0][92*i:92*i+92]!=outputs[1][92*i:92*i+92] or outputs[0][92*i:92*i+92]!=expected[92*i:92*i+92]];details=[]
 for i in different[:10]:details.append(dict(case=i,setting=settings[i%len(settings)],query=struct.unpack('<6f3I',queries[i]),native=struct.unpack_from('<23I',outputs[0],92*i),owned=struct.unpack_from('<23I',outputs[1],92*i),expected=struct.unpack_from('<23I',expected,92*i)))
 safety=OUT/'selection_safety_x64.exe';p=subprocess.run([str(safety)],capture_output=True,text=True,timeout=30);(run/'safety.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr);executables.append(dict(executable=str(safety),sha256=sha(safety)))
 require(sha(ENGINE)==DIGEST and sha(MAP)==map_sha,'Original changed');receipt=dict(status='PASS' if not different else 'FAIL',cases=len(queries),points=17407,cache_center_settings=settings,different_count=len(different),details=details,safety=p.stdout.strip(),executables=executables,engine_sha256=DIGEST,map_sha256=map_sha,query_sha256=sha(inp),limits=['Original new-record selection fragment1019791B..101979C5 executes in a supplied original-frame fixture; test epilogue is at next block101979C6','Actual cache-position policy and real BSP/world collision dependencies execute for nonzero settings; zero mode must make no trace and copy point exactly','Cache key fields and fresh record storage are controlled; full allocation/lookup/budget/reuse logic is not executed','ConVar integer is controlled; registered default1 is static evidence only, live SFM setting not measured','Existing/borrowed-record selection and upstream model lighting-point choice remain incomplete','No editor rendering/ambient interpolation/shader integration claim'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(queries),settings=settings,different_count=len(different),details=details[:2],receipt=str(run/'results.json'))));require(not different,'New cache-record position selection differs')
if __name__=='__main__':main()
