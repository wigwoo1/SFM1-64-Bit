"""Fresh original/x64 ordered enumeration plus frozen native corpus regression."""
import collections,hashlib,json,random,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/spatial-point-partition-20261016'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
PRIOR=ROOT/'outputs/native-spatial-partition-20261016/verification-r0u7wtua/results.json'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
 if not ok:raise RuntimeError(message)
def main():
 old=json.loads(PRIOR.read_text());require(old['status']=='PASS','Native reference status');OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libs=run/'libraries';libs.mkdir()
 for p,s in old['original_hashes'].items():require(sha(Path(p))==s,'Original changed '+p)
 shutil.copyfile(GAME/'bin/tier0.dll',libs/'tier0.dll');executables=[]
 for name,machine in [('spatial_partition_native_x86',0x14c),('spatial_point_partition_owned_x64',0x8664),('spatial_point_partition_safety_x64',0x8664)]:
  exe=OUT/(name+'.exe');raw=exe.read_bytes();p=struct.unpack_from('<I',raw,60)[0];require(raw[p:p+4]==b'PE\0\0' and struct.unpack_from('<H',raw,p+4)[0]==machine and b'asInvoker' in raw,'Executable identity');executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine)))
 def execute(name,args,log):
  p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=60);log.write_text(p.stdout+p.stderr);require(p.returncode==0,'Probe failed '+str(log)+' '+p.stdout+p.stderr);return p.stdout
 safety=execute('spatial_point_partition_safety_x64',[],run/'safety.log');print(safety,flush=True)
 fixtures=[]
 for previous in old['scenarios']:
  p=Path(previous['path']);require(sha(p/'fixture.bin')==previous['fixture_sha256'] and sha(p/'native.bin')==previous['output_sha256'],'Frozen input/reference');fixtures.append(('prior-'+previous['scenario'],(p/'fixture.bin').read_bytes(),p/'native.bin'))
 rng=random.Random(2026101661)
 def fixture(objects,queries):
  b=bytearray(struct.pack('<I',len(objects)))
  for i,mask,box in objects:b+=struct.pack('<QI6f',i,mask,*box)
  b+=struct.pack('<I',len(queries))
  for pose,mask,stop in queries:b+=struct.pack('<I6fII',mask,*pose,0,stop)
  return bytes(b)
 def identity(i):return ((i+1)<<32)|0xdecafbad
 def query(start,delta,mask=0x65,stop=0):return ([*start,*delta],mask,stop)
 directed=[]
 for x in [-16384,-1024,-256,-0.,0.,256,1024,16384]:
  for sign in [-1,1]:
   for delta in [(sign*2048,0,0),(0,sign*2048,0),(0,0,sign*2048),(sign*2048,sign*2048,sign*2048),(sign*2048,-sign*2048,sign*2048)]:directed.append(query([x,x,x],delta))
 for start,delta in [([-20000,0,0],[40000,0,0]),([20000,0,0],[-40000,0,0]),([-16512,0,0],[257.03125,0,0]),([16512,0,0],[-257.03125,0,0]),([0,-16512,0],[0,257.03125,0]),([0,0,16512],[0,0,-257.03125]),([0,0,0],[1e-20,0,0]),([0,0,0],[1e-30,0,0])]:directed.append(query(start,delta))
 for scenario in range(3):
  objects=[]
  for i in range(256):
   center=[rng.uniform(-20000,20000) for _ in range(3)];extent=[rng.choice([0,.01,1,127,128,255,256,1024,4096,10000]) for _ in range(3)];box=[center[j]-extent[j] for j in range(3)]+[center[j]+extent[j] for j in range(3)];objects.append((identity(i),rng.choice([0,1,2,4,8,16,32,64,128,256,512,0x65,0xffff]),box))
  queries=directed[:]
  for _ in range(2048):
   start=[rng.uniform(-17000,17000) for _ in range(3)];end=[rng.uniform(-17000,17000) for _ in range(3)];queries.append(query(start,[end[j]-start[j] for j in range(3)],rng.choice([0,1,2,4,8,16,32,64,128,256,512,0x65,0xffff]),rng.choice([0,0,0,1,5])))
  for i,mask,b in objects:
   center=[(b[j]+b[j+3])*.5 for j in range(3)];queries.append(query(center,[0,0,0],mask))
  fixtures.append(('random-'+str(scenario),fixture(objects,queries),None));fixtures.append(('reversed-'+str(scenario),fixture(list(reversed(objects)),queries),None))
 overlap=[(identity(i),0x65,[-4,-4,-4,4,4,4]) for i in range(1025)];queries=[query([0,0,0],[0,0,0]),query([-100,0,0],[200,0,0]),query([-100,-100,-100],[200,200,200])]+[query([0,0,0],[0,0,0],stop=n) for n in [1,1024,1025,2000]];fixtures.append(('overlap-1025',fixture(overlap,queries),None))
 totals=collections.Counter();records=[]
 for name,wire,frozen in fixtures:
  case=run/name;case.mkdir();q=case/'fixture.bin';q.write_bytes(wire);native=case/'native.bin';owned=case/'owned.bin';execute('spatial_partition_native_x86',[GAME/'bin/engine.dll',libs,q,native],case/'native.log');execute('spatial_point_partition_owned_x64',[q,owned],case/'owned.log')
  if native.read_bytes()!=owned.read_bytes():
   a,b=native.read_bytes(),owned.read_bytes();n=struct.unpack_from('<I',a)[0];pa=pb=8+2*n;total=struct.unpack_from('<I',a,4+2*n)[0]
   for j in range(total):
    ca=struct.unpack_from('<I',a,pa)[0];cb=struct.unpack_from('<I',b,pb)[0];pa+=4;pb+=4;aa=struct.unpack_from('<'+'Q'*ca,a,pa);bb=struct.unpack_from('<'+'Q'*cb,b,pb);pa+=ca*8;pb+=cb*8
    if aa!=bb:(case/'difference.json').write_text(json.dumps(dict(query=j,native=aa,owned=bb),indent=2));break
   raise RuntimeError('Ordered partition mismatch '+str(case))
  require(Path(str(native)+'.stored-boxes').read_bytes()==Path(str(owned)+'.stored-boxes').read_bytes(),'Stored boxes mismatch '+name)
  if frozen:require(native.read_bytes()==frozen.read_bytes(),'Fresh native reference changed '+name)
  n=struct.unpack_from('<I',wire)[0];queries=struct.unpack_from('<I',wire,4+n*36)[0];totals['queries']+=queries;totals['fixtures']+=1
  records.append(dict(name=name,entities=n,queries=queries,path=str(case),fixture_sha256=sha(q),ordered_sha256=sha(owned),stored_boxes_sha256=sha(Path(str(owned)+'.stored-boxes'))));print('PASS native/x64 ordered partition '+name+' queries='+str(queries),flush=True)
 for p,s in old['original_hashes'].items():require(sha(Path(p))==s,'Original changed after tests '+p)
 sources={str(ROOT/'work'/p):sha(ROOT/'work'/p) for p in ['source_spatial_point_partition_candidate.h','probe_spatial_point_partition.cpp','test_spatial_point_partition.cpp','build_spatial_point_partition.cmd','verify_spatial_point_partition.py','probe_native_spatial_partition.cpp','source_displacement_bounds_gate_candidate.h']}
 result=dict(status='PASS',path=str(run),totals=dict(totals),ordered_differences=0,stored_box_differences=0,safety=safety.strip(),executables=executables,sources=sources,records=records,original_hashes=old['original_hashes'],reference_receipt=str(PRIOR),reference_receipt_sha256=sha(PRIOR),limitations=['Owned x64 immutable initial insert/point-ray enumeration; full64bit scene IDs and owned local visit state.','Fresh actual original partition per fixture; privatePE with realtier0allocator/locks and controlled MDLscope/profiling services. No originalx86dependency in owned code.','No dynamic move/removal/hulls/callback mutation/general scheduling/editor/fullscene integration.','Masks limited16bits, scene65535entries; rejects nonfinite/overflow and quantization beyond supported finite domain rather than unchecked conversion.','No performance speedup claim; hashcontainer differences are irrelevant to per-cell lookup/list traversal, but memory/performance not benchmarked.'])
 (run/'results.json').write_text(json.dumps(result,indent=2));print('PASS '+str(run/'results.json'),flush=True)
if __name__=='__main__':main()
