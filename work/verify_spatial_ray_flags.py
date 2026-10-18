"""Preserved SourceRay swept flags: actual TraceRay observation and partition.
References are test-only; owned runtime needs no original process/library.
"""
import collections,hashlib,json,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/spatial-ray-flags-20261016'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
PARTITION=ROOT/'outputs/spatial-point-partition-20261016/verification-dpjk77a5/results.json'
CALLER=ROOT/'outputs/engine-point-trace-20261016/verification-ttxdnmws/results.json'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
 if not ok:raise RuntimeError(message)
def parseOutput(raw):
 n=struct.unpack_from('<I',raw)[0];p=4+n*2;count=struct.unpack_from('<I',raw,p)[0];p+=4;rows=[]
 for _ in range(count):
  size=struct.unpack_from('<I',raw,p)[0];p+=4;rows.append(struct.unpack_from('<'+'Q'*size,raw,p));p+=size*8
 require(p==len(raw),'Partition output span');return rows
def main():
 prior=json.loads(PARTITION.read_text());caller=json.loads(CALLER.read_text());require(prior['status']==caller['status']=='PASS','Prior proof status');OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libs=run/'libraries';libs.mkdir()
 for p,s in prior['original_hashes'].items():require(sha(Path(p))==s,'Original identity '+p)
 shutil.copyfile(GAME/'bin/tier0.dll',libs/'tier0.dll');executables=[]
 for name in ['partition_native_x86','partition_owned_x64','partition_safety_x64','adapter_safety_x64','caller_native_x86','caller_owned_x64']:
  exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];machine=0x14c if name.endswith('x86') else 0x8664;require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Executable identity');executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine)))
 def execute(name,args,log):
  r=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=60);log.write_text(r.stdout+r.stderr);require(r.returncode==0,'Probe failed '+str(log)+' '+r.stdout+r.stderr);return r.stdout
 safety=execute('partition_safety_x64',[],run/'partition-safety.log');adapter=execute('adapter_safety_x64',[],run/'adapter-safety.log');print(safety+adapter,flush=True)
 # Observe unchanged TraceRay body with its existing controlled world/partition
 # services. Assert actual forwarded flag, retain original output wire records.
 old=CALLER.parent;inputs=old/'inputs.bin';require(sha(inputs)==caller['input_sha256'],'Caller input identity');native=run/'caller-native.bin';owned=run/'caller-owned.bin';execute('caller_native_x86',[GAME/'bin/engine.dll',inputs,native],run/'caller-native.log');execute('caller_owned_x64',[GAME/'bin/engine.dll',inputs,owned],run/'caller-owned.log');require(native.read_bytes()==owned.read_bytes()==(old/'expected.bin').read_bytes(),'Caller trace/event regression');flags=Path(str(native)+'.swept').read_bytes();require(flags==Path(str(owned)+'.swept').read_bytes() and len(flags)==caller['cases']*4,'Caller swept observations')
 wire=inputs.read_bytes();trace=native.read_bytes();pi=po=0;shortened=[];coverage=collections.Counter()
 for i in range(caller['cases']):
  start=struct.unpack_from('<3f',wire,pi);inputFlag=struct.unpack_from('<I',wire,pi+24)[0];worldFraction=struct.unpack_from('<f',wire,pi+84)[0];n=struct.unpack_from('<I',wire,pi+132)[0];pi+=136+n*96
  length=struct.unpack_from('<I',trace,po)[0];row=trace[po+4:po+4+length];po+=4+length;calls=struct.unpack_from('<I',row,96)[0];delta=struct.unpack_from('<3f',row,104);observed=struct.unpack_from('<I',flags,i*4)[0];require(observed==(inputFlag if calls else 0xffffffff),'Original caller flag mismatch '+str(i));coverage['cases']+=1
  if calls:
   coverage['forwarded']+=1;coverage['swept_true' if inputFlag else 'swept_false']+=1;shortened.append((*start,*delta,inputFlag))
   if inputFlag and all(x==0 for x in delta):coverage['zero_delta_swept_true']+=1
   if inputFlag and worldFraction==0:coverage['zero_fraction_swept_true']+=1
 require(pi==len(wire) and po==len(trace) and coverage['zero_fraction_swept_true']>0,'Caller shortening coverage')
 fixtures=[]
 for record in prior['records']:
  p=Path(record['path']);require(sha(p/'fixture.bin')==record['fixture_sha256'] and sha(p/'owned.bin')==record['ordered_sha256'],'Frozen partition data');b=(p/'fixture.bin').read_bytes();n=struct.unpack_from('<I',b)[0];at=4+n*36;count=struct.unpack_from('<I',b,at)[0];header=b[:at];rows=b[at+4:];expanded=bytearray(header+struct.pack('<I',count*2))
  for i in range(count):
   row=rows[i*36:(i+1)*36]
   for flag in [0,1]:expanded+=row+struct.pack('<I',flag)
  fixtures.append((record['name'],b,bytes(expanded),p/'owned.bin'))
 # Directed boundary and exact shortened caller inputs, on controlled center/
 # edge boxes: these make deriving the flag from delta observably incorrect.
 objects=[(0x11223344decafbad,[-4,-4,-4,4,4,4]),(0x55667788decafbad,[16383,-4,-4,16384,4,4]),(0x99aabbccdecafbad,[-16384,-4,-4,-16383,4,4])];b=bytearray(struct.pack('<I',len(objects)))
 for id,box in objects:b+=struct.pack('<QI6f',id,0x65,*box)
 directed=[]
 for pos in [-20000,-16384,-16383,0,16383,16384,20000]:
  for axis in range(3):
   for d in [0,-0.,1e-30,1e-20,.00001,-256,256,-40000,40000]:
    start=[0.,0.,0.];start[axis]=pos;delta=[0.,0.,0.];delta[axis]=d
    for mask in [0,1,4,64,0x65]:
     for flag in [0,1]:directed.append(struct.pack('<I6fIII',mask,*start,*delta,0,0,flag))
 for q in shortened:directed.append(struct.pack('<I6fIII',0x65,*q[:6],0,0,q[6]))
 fixtures.append(('directed-and-caller-shortened',None,bytes(b+struct.pack('<I',len(directed))+b''.join(directed)),None));records=[];totals=collections.Counter();flagDifferences=0
 for name,default,explicit,frozen in fixtures:
  case=run/name;case.mkdir()
  for label,data,mode in [('default',default,[]),('explicit',explicit,['explicit-swept'])]:
   if data is None:continue
   fixture=case/(label+'-fixture.bin');fixture.write_bytes(data);a=case/(label+'-native.bin');b=case/(label+'-owned.bin');execute('partition_native_x86',[GAME/'bin/engine.dll',libs,fixture,a,*mode],case/(label+'-native.log'));execute('partition_owned_x64',[fixture,b,*mode],case/(label+'-owned.log'))
   if a.read_bytes()!=b.read_bytes():
    aa,bb=parseOutput(a.read_bytes()),parseOutput(b.read_bytes());first=next(i for i,(x,y) in enumerate(zip(aa,bb)) if x!=y);(case/'difference.json').write_text(json.dumps(dict(mode=label,query=first,native=aa[first],owned=bb[first]),indent=2));raise RuntimeError('Ray flag order mismatch '+str(case))
   require(Path(str(a)+'.stored-boxes').read_bytes()==Path(str(b)+'.stored-boxes').read_bytes(),'Stored box regression');rows=parseOutput(a.read_bytes());totals[label+'_queries']+=len(rows)
   if label=='default' and frozen:require(a.read_bytes()==frozen.read_bytes(),'Default/frozen partition regression')
   if label=='explicit' and name!='directed-and-caller-shortened':flagDifferences+=sum(rows[i]!=rows[i+1] for i in range(0,len(rows),2))
   records.append(dict(name=name,mode=label,queries=len(rows),path=str(case),fixture=str(fixture),fixture_sha256=sha(fixture),output=str(b),ordered_sha256=sha(b),stored_boxes_sha256=sha(Path(str(b)+'.stored-boxes'))));print('PASS explicit/default ray flags '+name+' '+label+' queries='+str(len(rows)),flush=True)
 require(totals['default_queries']==18371 and flagDifferences>0,'Ray flag regression/difference coverage')
 for p,s in prior['original_hashes'].items():require(sha(Path(p))==s,'Original changed after tests '+p)
 names=['source_spatial_point_partition_candidate.h','source_spatial_point_ray_adapter_candidate.h','probe_native_spatial_partition.cpp','probe_spatial_point_partition.cpp','probe_engine_point_trace.cpp','test_spatial_point_ray_adapter.cpp','build_spatial_ray_flags.cmd','verify_spatial_ray_flags.py'];sources={str(ROOT/'work'/p):sha(ROOT/'work'/p) for p in names}
 result=dict(status='PASS',path=str(run),totals=dict(totals),caller_coverage=dict(coverage),flag_changes_candidate_order_or_membership=flagDifferences,ordered_differences=0,caller_differences=0,safety=safety.strip(),adapter_safety=adapter.strip(),executables=executables,sources=sources,records=records,original_hashes=prior['original_hashes'],reference_receipts=[str(PARTITION),str(CALLER)],limitations=['Explicit swept flag supported by immutable x64 point partition and typed SourceWorldLightRay adapter; default fresh-delta API retained.','Original full TraceRay flag observation uses existing controlled world service jump and controlled enumeration/clip services; TraceRay body executes, not a full scene.','Original spatial partition separately executes with real tier0 allocator/locks and original code; explicit flag is input, no recorded native states drive owned results.','Owned zero-world-fraction adapter integration uses controlled world/clip services. Complete actual world+prop+material integration remains pending.','Point rays/coarse0/static prepared partition only; hulls/move/remove/callback mutation and editor integration not claimed.'])
 (run/'results.json').write_text(json.dumps(result,indent=2));print('PASS '+str(run/'results.json'),flush=True)
if __name__=='__main__':main()
