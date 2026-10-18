import json, math, random, struct, subprocess, tempfile
from pathlib import Path
from verify_native_convex_brush_ray import ROOT, ENGINE, DIGEST, sha, require
OUT=ROOT/'outputs/model-lighting-point-20261015'
def main():
 require(sha(ENGINE)==DIGEST,'Original engine changed');OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));rng=random.Random(2026101595);rows=[];expected=[];metadata=[];f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
 def matrix():return [f32(rng.uniform(-8,8)) for _ in range(12)]
 def transform(point,m):return [f32(f32(f32(f32(m[4*r+1]*point[1])+f32(m[4*r]*point[0]))+f32(m[4*r+2]*point[2]))+m[4*r+3]) for r in range(3)]
 def append(flags,index,point,model,attachment,offset,explicit,category):
  point=list(map(f32,point));model=list(map(f32,model));attachment=list(map(f32,attachment));offset=list(map(f32,offset));explicit=list(map(f32,explicit));row=struct.pack('<Ii42f',flags,index,*point,*model,*attachment,*offset,*explicit);require(len(row)==176,'Fixture length');rows.append(row);called=bool(flags&1 and index>0);selected=attachment if called and flags&2 else model;base=transform(point,selected);final=explicit if flags&8 else transform(base,offset) if flags&4 else base;expected.append(struct.pack('<6fIi',*base,*final,int(called),index if called else 0));metadata.append((flags,index,category))
 for i in range(8192):append(i%16,[-1,0,1,2,257,2147483647][(i//16)%6],[rng.uniform(-1024,1024) for _ in range(3)],matrix(),matrix(),matrix(),[rng.uniform(-4096,4096) for _ in range(3)],'controlled')
 models=[];folder=ENGINE.parent.parent/'tf/models/player'
 for name in ['soldier','pyro','scout','heavy','medic','engineer','sniper','spy','demo']:
  path=folder/(name+'.mdl')
  if not path.is_file():continue
  raw=path.read_bytes();require(len(raw)>=404 and raw[:4]==b'IDST','MDL fixture header');point=struct.unpack_from('<3f',raw,0x5c);extra=struct.unpack_from('<i',raw,0x190)[0];require(extra>=0 and (not extra or extra+12<=len(raw)),'MDL header2 span');index=struct.unpack_from('<i',raw,extra+8)[0] if extra else 0;models.append(dict(path=str(path),sha256=sha(path),version=struct.unpack_from('<I',raw,4)[0],illumination_point=point,attachment_index=index))
  for i in range(64):append(i%16,index,point,matrix(),matrix(),matrix(),[rng.uniform(-4096,4096) for _ in range(3)],name)
 require(len(models)>0,'Actual MDL metadata coverage absent');inp=run/'queries.bin';inp.write_bytes(b''.join(rows));want=b''.join(expected);outputs=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('point_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture');target=run/(name+'.bin');p=subprocess.run([str(exe),str(ENGINE),str(inp),str(target)],capture_output=True,text=True,timeout=60);(run/(name+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));actual=target.read_bytes();require(len(actual)==len(rows)*32,'Output length');outputs.append(actual);executables.append(dict(executable=str(exe),sha256=sha(exe)))
 different=[i for i in range(len(rows)) if outputs[0][32*i:32*i+32]!=outputs[1][32*i:32*i+32] or outputs[0][32*i:32*i+32]!=want[32*i:32*i+32]];details=[]
 for i in different[:10]:details.append(dict(case=i,metadata=metadata[i],native=struct.unpack_from('<8I',outputs[0],32*i),owned=struct.unpack_from('<8I',outputs[1],32*i),expected=struct.unpack_from('<8I',want,32*i)))
 coverage=dict(attachment_success=0,attachment_failure_fallback=0,no_attachment_query=0,explicit_origin=0,offset_origin=0,direct_origin=0)
 for flags,index,category in metadata:
  called=bool(flags&1 and index>0);coverage['attachment_success' if called and flags&2 else 'attachment_failure_fallback' if called else 'no_attachment_query']+=1;coverage['explicit_origin' if flags&8 else 'offset_origin' if flags&4 else 'direct_origin']+=1
 safety=OUT/'point_safety_x64.exe';p=subprocess.run([str(safety)],capture_output=True,text=True,timeout=30);(run/'safety.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr);executables.append(dict(executable=str(safety),sha256=sha(safety)))
 require(all(v>0 for v in coverage.values()),'Branch coverage missing');require(sha(ENGINE)==DIGEST and all(sha(Path(m['path']))==m['sha256'] for m in models),'Original inputs modified');receipt=dict(status='PASS' if not different else 'FAIL',cases=len(rows),different_count=len(different),details=details,coverage=coverage,safety=p.stdout.strip(),models=models,executables=executables,engine_sha256=DIGEST,query_sha256=sha(inp),limits=['Complete original100D2780 plus original10244180 execute privately with controlled renderable attachment callback','Original100D5760..100D57D2 executes in supplied test frame and terminates before material-system virtual call at57D3','Actual model metadata fixtures; matrices and attachment responses controlled, not live evaluated benchmark poses','Full render-path gating, alternate cached-instance sources, DmeGameModel illumPositionDag binding, model loading and renderer integration not claimed'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(rows),coverage=coverage,actual_models=len(models),different_count=len(different),details=details[:2],receipt=str(run/'results.json'))));require(not different,'Model lighting point differs')
if __name__=='__main__':main()
