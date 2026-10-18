"""Actual original partition insertion and ordered point-ray enumeration.
Independent membership check is not a reconstructed ordering implementation.
"""
import collections,hashlib,json,math,random,shutil,struct,subprocess,tempfile
from pathlib import Path
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/native-spatial-partition-20261016'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
PRIOR=ROOT/'outputs/static-prop-partition-registration-20261016/verification-or1d0we6/results.json'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
 if not ok:raise RuntimeError(message)
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def expanded(box):return [max(-16384.,min(16384.,f32(x+(-.03125 if j<3 else .03125)))) for j,x in enumerate(box)]
def intersects(box,pose):
 t0,t1=0.,1.
 for j in range(3):
  a,d=pose[j],pose[j+3]
  if d==0:
   if a<box[j] or a>box[j+3]:return False
  else:
   lo,hi=(box[j]-a)/d,(box[j+3]-a)/d
   if lo>hi:lo,hi=hi,lo
   t0=max(t0,lo);t1=min(t1,hi)
   if t0>t1:return False
 return True
def eligible(entityMask,queryMask):
 return bool(entityMask&queryMask) and bool(entityMask&(0x1ac if queryMask&0x1ac else 0xfe53))
def nativeWorldClip(pose):
 # Independent translation of original1C2FD0->1BDDB0/1BDF90. Preserve the
 # observed asymmetry; do not replace these helpers with ideal slab clipping.
 start=pose[:3];delta=pose[3:];end=[f32(start[j]+delta[j]) for j in range(3)]
 if not any(delta):return pose
 inside=lambda point:all(-16384<=x<=16384 for x in point)
 startIn,endIn=inside(start),inside(end)
 if not startIn and not endIn:return None
 if not startIn:
  for j in range(3):
   if abs(delta[j])<1e-10:continue
   t=None
   if delta[j]>0 and start[j]<-16384:t=f32(f32(-16379-start[j])/delta[j])
   elif delta[j]<=0 and start[j]>16384:t=f32(f32(start[j]-16379)*f32(-1/delta[j]))
   if t is not None:start=[f32(f32(delta[k]*t)+start[k]) for k in range(3)]
  delta=[f32(end[j]-start[j]) for j in range(3)]
 elif not endIn:
  for j in range(3):
   if abs(delta[j])<1e-10:continue
   t=None
   if delta[j]<0 and end[j]<-16384:t=f32(f32(start[j]-(-16379))*f32(-1/delta[j]))
   elif delta[j]>=0 and end[j]>16384:t=f32(f32(start[j]-(-16379))/delta[j])
   if t is not None:end=[f32(f32(delta[k]*t)+start[k]) for k in range(3)]
  delta=[f32(end[j]-start[j]) for j in range(3)]
 return start+delta
def main():
 old=json.loads(PRIOR.read_text());require(old['status']=='PASS','Registration proof');OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libs=run/'libraries';libs.mkdir()
 for p,s in old['original_hashes'].items():require(sha(Path(p))==s,'Original changed '+p)
 shutil.copyfile(GAME/'bin/tier0.dll',libs/'tier0.dll');image,evidence=check_ranges(GAME/'bin/engine.dll','engine-disassembly.txt',[('constructor',0x101c37d0,0x101c3845),('initialize',0x101c3850,0x101c38c6),('CreateHandle',0x101bead0,0x101beb13),('EnumerateAlongRay',0x101becf0,0x101bedb0),('insert',0x101c1600,0x101c166c),('destructor',0x101c3e60,0x101c4080)])
 _,clipEvidence=check_ranges(GAME/'bin/engine.dll','engine-disassembly.txt',[('start_world_clip',0x101bddb0,0x101bdf8c),('end_world_clip',0x101bdf90,0x101be176)])
 require([image.word(a) for a in [0x1040b558,0x1040b564,0x1033fa60,0x1035f490,0x1035f4a0]]==[0xc6800000,0x46800000,0x3d000000,0x467fec00,0xc67fec00],'Native partition bounds/margin/clip changed');(run/'original-code-evidence.json').write_text(json.dumps(dict(partition=evidence,world_clip=clipEvidence),indent=2))
 raw=Path(old['path'])/'native-ordered.bin';cases=json.loads((Path(old['path'])/'cases.json').read_text());answers=raw.read_bytes();baseline=[]
 for i,c in enumerate(cases):
  if c['label']=='benchmark' and struct.unpack_from('<I',answers,i*56)[0]:baseline.append(dict(id=((c['ordinal']+1)<<32)|0xdecafbad,mask=0x65,box=list(struct.unpack_from('<6f',answers,i*56+32))))
 require(len(baseline)==145,'Benchmark collision insertion count');rng=random.Random(2026101653);scenarios=[]
 def queriesFor(objects,randomCount):
  queries=[]
  def add(start,delta,mask=0x65,label='directed'):queries.append(dict(pose=[f32(x) for x in [*start,*delta]],mask=mask,coarse=0,stop=0,label=label))
  for e in objects:
   box=expanded(e['box']);center=[f32((box[j]+box[j+3])*.5) for j in range(3)]
   add(center,[0,0,0],e['mask'],'center')
   for j in range(3):
    p=center[:];p[j]=box[j];add(p,[0,0,0],e['mask'],'boundary-min');p=center[:];p[j]=box[j+3];add(p,[0,0,0],e['mask'],'boundary-max')
    p=center[:];p[j]=f32(box[j]-128);d=[0.,0.,0.];d[j]=f32(box[j+3]+128-p[j]);add(p,d,e['mask'],'axis-ray')
  for _ in range(randomCount):
   start=[rng.uniform(-16000,16000) for _ in range(3)];end=[rng.uniform(-16000,16000) for _ in range(3)];add(start,[end[j]-start[j] for j in range(3)],rng.choice([0,1,4,32,64,0x65,0x100,0x200]),'random')
  return queries
 scenarios.append(('benchmark',baseline,queriesFor(baseline,2048)))
 same=[dict(id=((i+1)<<32)|0xdecafbad,mask=0x65,box=[-4,-4,-4,4,4,4]) for i in range(12)];sameq=queriesFor(same,0);sameq+=[dict(pose=[0,0,0,0,0,0],mask=0x65,coarse=0,stop=n,label='early-stop') for n in [1,2,5,12,20]];scenarios.append(('same-cell',same,sameq))
 mixed=[dict(id=((i+1)<<32)|0x12345678,mask=mask,box=[-4,-4,-4,4,4,4]) for i,mask in enumerate([1,4,32,64,0x65,0x100,0x200])];mixedq=[dict(pose=[0,0,0,0,0,0],mask=mask,coarse=0,stop=0,label='mixed-mask') for mask in [0,1,4,32,64,0x65,0x100,0x200,0xffff]];scenarios.append(('mixed-masks',mixed,mixedq))
 cells=[]
 for i,(center,size) in enumerate([([0,0,0],1),([256,0,0],1),([-256,0,0],1),([1024,1024,1024],128),([0,0,0],256),([0,0,0],1024),([0,0,0],4096),([16384,0,0],1),([-16384,0,0],1)]):cells.append(dict(id=((i+1)<<32)|0x87654321,mask=0x65,box=[x-size for x in center]+[x+size for x in center]))
 gridQueries=queriesFor(cells,256);gridQueries+=[dict(pose=[-20000,0,0,40000,0,0],mask=0x65,coarse=0,stop=0,label='both-outside'),dict(pose=[20000,0,0,-40000,0,0],mask=0x65,coarse=0,stop=0,label='both-outside')];scenarios.append(('grid-levels-boundaries',cells,gridQueries));scenarios.append(('empty',[],queriesFor([],32)))
 exe=OUT/'spatial_partition_native_x86.exe';pe=exe.read_bytes();p=struct.unpack_from('<I',pe,60)[0];require(pe[p:p+4]==b'PE\0\0' and struct.unpack_from('<H',pe,p+4)[0]==0x14c and b'asInvoker' in pe,'Native executable identity');records=[];totals=collections.Counter()
 for label,objects,queries in scenarios:
  case=run/label;case.mkdir();wire=bytearray(struct.pack('<I',len(objects)))
  for e in objects:wire+=struct.pack('<QI6f',e['id'],e['mask'],*e['box'])
  wire+=struct.pack('<I',len(queries))
  for q in queries:wire+=struct.pack('<I6fII',q['mask'],*q['pose'],q['coarse'],q['stop'])
  fixture=case/'fixture.bin';fixture.write_bytes(wire);output=case/'native.bin';args=[str(exe),str(GAME/'bin/engine.dll'),str(libs),str(fixture),str(output)];r=subprocess.run(args,cwd=run,capture_output=True,text=True,timeout=60);(case/'native.log').write_text(r.stdout+r.stderr);require(r.returncode==0,'Native partition failed '+label+' '+r.stdout+r.stderr)
  data=output.read_bytes();n=struct.unpack_from('<I',data)[0];require(n==len(objects),'Entity count');handles=struct.unpack_from('<'+'H'*n,data,4);require(handles==tuple(range(n)),'Unexpected fresh handle allocation');p=4+n*2;count=struct.unpack_from('<I',data,p)[0];p+=4;require(count==len(queries),'Query count');stored=Path(str(output)+'.stored-boxes').read_bytes();require(len(stored)==len(objects)*32,'Stored box extent')
  for i,e in enumerate(objects):
   expected=struct.pack('<6fII',*expanded(e['box']),e['mask'],(2 if e['mask']&0x1ac else 0)|(4 if e['mask']&0xfe53 else 0));require(stored[i*32:(i+1)*32]==expected,'Native expanded box/mask/index flags '+label+' '+str(i))
  orderDifferent=0;nonempty=0;ordered=[]
  for i,q in enumerate(queries):
   n=struct.unpack_from('<I',data,p)[0];p+=4;actual=list(struct.unpack_from('<'+'Q'*n,data,p));p+=n*8;require(len(actual)==len(set(actual)),'Native returned duplicate entity');clipped=nativeWorldClip(q['pose']);expected=[e['id'] for e in objects if eligible(e['mask'],q['mask']) and clipped is not None and intersects(expanded(e['box']),clipped)]
   if clipped!=q['pose']:totals['world_clipped_queries']+=1
   if q['stop']:require(len(actual)==min(q['stop'],len(expected)) and set(actual)<=set(expected),'Early-stop membership/count')
   else:
    if set(actual)!=set(expected):(case/'difference.json').write_text(json.dumps(dict(query=i,input=q,expected=expected,actual=actual),indent=2));raise RuntimeError('Native membership mismatch '+str(case/'difference.json'))
   nonempty+=bool(actual);orderDifferent+=actual!=[x for x in expected if x in set(actual)];ordered.append(actual);totals['queries']+=1;totals['callbacks']+=len(actual);totals[q['label']]+=1
  if label=='same-cell':
   for q,answer in zip(queries,ordered):
    if q['stop']:require(answer==ordered[0][:q['stop']],'Early-stop changed enumeration prefix')
  require(p==len(data),'Trailing output');(case/'cases.json').write_text(json.dumps(dict(objects=objects,queries=queries,ordered=ordered),indent=2));repeat=case/'repeat.bin';r=subprocess.run([*args[:-1],str(repeat)],cwd=run,capture_output=True,text=True,timeout=60);(case/'repeat.log').write_text(r.stdout+r.stderr);require(r.returncode==0 and repeat.read_bytes()==data and Path(str(repeat)+'.stored-boxes').read_bytes()==stored,'Native repeat changed '+label);records.append(dict(scenario=label,entities=len(objects),queries=len(queries),nonempty=nonempty,order_differs_from_insertion=orderDifferent,path=str(case),fixture_sha256=sha(fixture),output_sha256=sha(output),stored_boxes_sha256=sha(Path(str(output)+'.stored-boxes'))));print('PASS actual native partition '+label+' queries='+str(len(queries)),flush=True)
 require(any(x['order_differs_from_insertion'] for x in records),'No order-sensitive observations')
 for p,s in old['original_hashes'].items():require(sha(Path(p))==s,'Original changed after test '+p)
 sources={str(ROOT/'work'/p):sha(ROOT/'work'/p) for p in ['probe_native_spatial_partition.cpp','build_native_spatial_partition.cmd','verify_native_spatial_partition.py','native_private_world_image_fixture.h']}
 result=dict(status='PASS',path=str(run),totals=dict(totals),scenarios=records,membership_differences=0,stored_bounds_differences=0,repeat_differences=0,executable=dict(path=str(exe),sha256=sha(exe),machine='0x14c'),sources=sources,original_hashes=old['original_hashes'],registration_receipt=str(PRIOR),registration_receipt_sha256=sha(PRIOR),limitations=['Actual original constructors/initialization/handle allocation/insertion/ray enumeration/destructor execute; tier0 allocator/locks and Windows imports normally bound, no executable patches.','Private engine image does not perform full SFM startup. MDL-cache begin/end and disabled profiling cvar are explicit controlled services.','Inputs are prepared physics boxes from prior original registration proof or controlled valid boxes; not a single live engine loading/registration pipeline.','Independent source-specific world clipping + brute slab check verifies membership, NOT original order. Native ordered output is characterization data for future x64 partition reconstruction.','Point rays only; no hull sweeps, moves/removals, query callback mutations, parallel native operations or full-scene trace integration.','No editor build/runtime integration or owned x64 spatial implementation yet.'])
 (run/'results.json').write_text(json.dumps(result,indent=2));print('PASS '+str(run/'results.json'),flush=True)
if __name__=='__main__':main()
