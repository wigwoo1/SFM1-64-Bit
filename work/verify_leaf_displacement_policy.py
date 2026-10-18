"""Original leaf displacement control flow with real gate and controlled hits."""
import json,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,DIGEST,sha,require
from verify_displacement_bounds_gate import prepare as prepare_bounds

OUT=ROOT/'outputs/leaf-displacement-policy-20261015'
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');image=Image(ENGINE);post=image.b[image.offset(0x1014cfa0):image.offset(0x1014cfa0)+0x49];header='// Checked original leaf post-trace solid policy.\nstatic const unsigned char nativeDispPostBytes[]={'+','.join('0x%02x'%v for v in post)+'};\n';hp=ROOT/'work/native_leaf_displacement_policy_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared original leaf post-trace helper');return
 require(hp.read_text()==header,'Stale post-trace code');bounds_header,bounds_blob=prepare_bounds();require((ROOT/'work/native_displacement_bounds_gate_bytes.h').read_text()==bounds_header,'Stale gate code');leaf_blob=image.b[image.offset(0x10148270):image.offset(0x10148270)+0x2c7];rng=random.Random(202610159);qs=[];initial_states=[]
 def append(mask,pre,startsolid,allsolid,marker,swept,generation,fraction,actions,modes,initial_mode,ordered,start,delta,records):
  require(len(ordered)<=8,'Reference count');refs=(ordered+[255]*8)[:8];words=[mask,pre,startsolid,allsolid,marker,swept,generation,struct.unpack('<I',struct.pack('<f',fraction))[0],actions,modes,initial_mode]+[sum(refs[4*j+k]<<(8*k) for k in range(4)) for j in range(2)]
  raw=struct.pack('<13I6f',*words,*start,*delta)+b''.join(struct.pack('<6f2I',*lo,*hi,contents,index) for lo,hi,contents,index in records);require(len(raw)==204,'Query stride');qs.append(raw);initial_states.append((startsolid,allsolid,marker,fraction))
 for i in range(6144):
  mask=rng.choice([0,1,0x80,0x4081,0x2000,0xffffffff]);pre=rng.randrange(16);startsolid=rng.randrange(2);allsolid=rng.randrange(2);marker=rng.randrange(2);swept=rng.randrange(2);generation=rng.choice([0,7,0xffffffff]);fraction=rng.choice([0.,-0.,.5,1.]);actions=rng.randrange(256);modes=rng.randrange(256);initial_mode=rng.randrange(4)
  ordered=[rng.randrange(4) for _ in range(rng.randrange(9))];start=[rng.uniform(-4,4) for _ in range(3)];delta=[rng.choice([-0.,0.,-1.,1.,-8.,8.]) for _ in range(3)];records=[]
  for index in range(4):
   center=[rng.uniform(-4,4) for _ in range(3)];extent=rng.choice([0.,.03125,1.,8.]);records.append(([x-extent for x in center],[x+extent for x in center],rng.choice([0,1,0x80,0x4000,0x2000,0xffffffff]),rng.randrange(4)))
  if i<1024:
   startsolid=(i>>0)&1;allsolid=(i>>1)&1;marker=(i>>2)&1;swept=(i>>3)&1;start=[0,0,0];delta=[0,0,1 if i&16 else -1];ordered=[3,1,3,0,2,1];records=[([-1,-1,-1],[1,1,1],1,j) for j in range(4)];mask=1;pre=(i>>5)&15
  if 1024<=i<1280:
   # Masked ID0 shares its visit slot with eligible ID1: mask-before-mark.
   startsolid=allsolid=marker=0;swept=1;pre=0;mask=1;start=[0,0,0];delta=[0,0,1];ordered=[0,1,0,1];records=[([-1,-1,-1],[1,1,1],0 if j==0 else 1,0) for j in range(4)];actions=0
  if 1280<=i<1536:
   # No callback, but existing displacement marker triggers post adjustment.
   startsolid=allsolid=0;marker=1;mask=0;ordered=[0];initial_mode=0;delta=[0,0,1]
  swept|=(i%3==0)<<1
  append(mask,pre,startsolid,allsolid,marker,swept,generation,fraction,actions,modes,initial_mode,ordered,start,delta,records)
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));inp=run/'queries.bin';inp.write_bytes(b''.join(qs));results=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('policy_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture')
  if name=='native_x86':require(all(blob in binary for blob in [leaf_blob,bounds_blob,post]),'Original code absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=40);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));raw=target.read_bytes();require(len(raw)==80*len(qs),'Output length');results.append(raw);executables.append(dict(executable=str(exe),sha256=sha(exe)))
 different=[i for i in range(len(qs)) if results[0][80*i:80*i+80]!=results[1][80*i:80*i+80]];calls=0;solid_adjustments=0;empty_calls=0
 for i,initial in enumerate(initial_states):
  record=struct.unpack_from('<20I',results[0],80*i);calls+=record[0];empty_calls+=record[0]==0;solid_adjustments+=not initial[0] and record[14]!=0
 require(calls>100 and empty_calls>100 and solid_adjustments>100,'Policy coverage');require(sha(ENGINE)==DIGEST,'Original changed')
 receipt=dict(status='PASS' if not different else 'FAIL',cases=len(qs),trace_callbacks=calls,cases_without_trace_callbacks=empty_calls,new_start_solid_cases=solid_adjustments,different_cases=different[:20],executables=executables,engine_sha256=sha(ENGINE),limits=['Original full leaf body with zero brushes or one zero-contents brush; controlled displacement-hit dependency','Original bounds gate and post-trace helper execute unmodified','Proves ordered visits/filtering/solid-state policy, not actual displacement hits or world composition','No editor/rendering integration']);(run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(qs),trace_callbacks=calls,new_start_solid_cases=solid_adjustments,different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Leaf displacement policy differs')
if __name__=='__main__':main()
