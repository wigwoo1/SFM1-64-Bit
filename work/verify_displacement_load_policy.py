import json,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,DIGEST,sha,require
OUT=ROOT/'outputs/displacement-load-policy-20261015'
RANGES=[(0x1014c0f5,0x1014c12c),(0x1014c2d6,0x1014c34d)]
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');im=Image(ENGINE);header='// Original displacement face-lump choice and read-cursor blocks; oracle only.\n';blobs=[]
 for i,(a,z) in enumerate(RANGES):
  raw=im.b[im.offset(a):im.offset(a)+z-a];blobs.append(raw);header+='static const unsigned char loadPolicyPart%d[]={%s};\n'%(i,','.join('0x%02x'%v for v in raw))
 header+='struct LoadPolicyFragment{unsigned offset;const unsigned char*bytes;unsigned size;};\nstatic const LoadPolicyFragment loadPolicyFragments[]={'+','.join('{0x%x,loadPolicyPart%d,sizeof(loadPolicyPart%d)}'%(a-0x10000000,i,i) for i,(a,z) in enumerate(RANGES))+'};\n';hp=ROOT/'work/native_displacement_load_policy_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared original loader policy blocks');return
 require(hp.read_text()==header,'Stale original blocks');rng=random.Random(2026101585);qs=[]
 for i in range(4096):qs.append(struct.pack('<iiIII',rng.choice([0,1,2,-1]),rng.choice([-1,0,1,56,453376]),1+i%4,rng.randrange(1000000),rng.randrange(1000000)))
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));inp=run/'queries.bin';inp.write_bytes(b''.join(qs));outputs=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('load_policy_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture')
  if name=='native_x86':require(all(blob in binary for blob in blobs),'Original blocks missing')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=30);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));output=target.read_bytes();require(len(output)==len(qs)*32,'Output length');outputs.append(output);executables.append(dict(executable=str(exe),sha256=sha(exe)))
 different=[i for i in range(len(qs)) if outputs[0][i*32:(i+1)*32]!=outputs[1][i*32:(i+1)*32]];hdr=sum(struct.unpack_from('<I',outputs[0],i*32)[0]==58 for i in range(len(qs)));safety=OUT/'load_policy_safety_x64.exe';p=subprocess.run([str(safety)],capture_output=True,text=True,timeout=20);(run/'safety.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr);executables.append(dict(executable=str(safety),sha256=sha(safety)));require(sha(ENGINE)==DIGEST,'Original changed');receipt=dict(status='PASS' if not different else 'FAIL',cases=len(qs),hdr_selected=hdr,ldr_selected=len(qs)-hdr,different_count=len(different),executables=executables,engine_sha256=DIGEST,limits=['Original blocks with supplied loader frame and RET ends; HDR capability/lump size/reads are controlled callbacks','Checks original request offsets/sizes/order/cursor arithmetic, not original filesystem or compressed lump I/O','Missing association/non-quad orchestration remains outside these blocks'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(qs),different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Loader policy differs')
if __name__=='__main__':main()
