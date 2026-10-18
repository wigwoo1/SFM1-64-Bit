"""Original full CoreDisp raw input initializer with controlled allocation."""
import json,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require
OUT=ROOT/'outputs/displacement-core-input-20261015'
RANGES=[(0x101428e0,0x10142a70),(0x10142210,0x101428d4),(0x10001010,0x10001033),(0x100b4e10,0x100b4e24),(0x1013cbc0,0x1013cbcf),(0x1013cc10,0x1013cc2f),(0x1013d7d0,0x1013d887),(0x10142110,0x10142174),(0x100b4ce0,0x100b4ce3),(0x10001120,0x10001144),(0x1000bb80,0x1000bb9a),(0x100c32b0,0x100c32de),(0x1013d570,0x1013d721),(0x10141bd0,0x10141be0),(0x10141c30,0x10141c67),(0x100b4cf0,0x100b4d12),(0x10001150,0x1000116c),(0x102df090,0x102df0bb),(0x102de362,0x102de36c)]

def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');im=Image(ENGINE);blobs=[];header='// Checked original CoreDisp raw initializer and helpers; test allocator is separate.\n'
 for i,(a,z) in enumerate(RANGES):
  raw=im.b[im.offset(a):im.offset(a)+z-a];blobs.append(raw);header+='static const unsigned char coreInputPart%d[]={%s};\n'%(i,','.join('0x%02x'%v for v in raw))
 header+='struct CoreInputFragment{unsigned offset;const unsigned char*bytes;unsigned size;};\nstatic const CoreInputFragment coreInputFragments[]={'+','.join('{0x%x,coreInputPart%d,sizeof(coreInputPart%d)}'%(a-0x10000000,i,i) for i,(a,z) in enumerate(RANGES))+'};\n';hp=ROOT/'work/native_displacement_core_input_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared complete original CoreDisp input routines');return
 require(hp.read_text()==header,'Stale original routines');map_sha=sha(MAP);require(map_sha=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Map changed');bsp=MAP.read_bytes()
 def lump(i,stride):
  o,n,_,_=struct.unpack_from('<4I',bsp,8+16*i);require(o+n<=len(bsp) and n%stride==0,'Lump bounds');return bsp[o:o+n]
 infos=lump(26,176);rawverts=lump(33,20);rawtags=lump(48,2);queries=[];metadata=[]
 def append(power,encoded,flags,prior,present,elevation,raw,tags,extra=b''):
  vc=((1<<power)+1)**2;tc=2*(1<<power)**2;queries.append(struct.pack('<5If',power,encoded,flags,prior,present,elevation)+raw+tags+extra);metadata.append((vc,tc,encoded,flags,prior,present,elevation))
 for i in range(43):
  d=i*176;v,t,power=struct.unpack_from('<IIi',infos,d+12);encoded=struct.unpack_from('<I',infos,d+24)[0];elevation=struct.unpack_from('<f',infos,d+28)[0];vc=((1<<power)+1)**2;tc=2*(1<<power)**2;append(power,encoded,0,0,0,elevation,rawverts[v*20:(v+vc)*20],rawtags[t*2:(t+tc)*2])
 rng=random.Random(2026101583)
 for i in range(512):
  power=1+i%4;vc=((1<<power)+1)**2;tc=2*(1<<power)**2;encoded=rng.choice([0,8,0x7fffffff,0x80000000,0x80000008,0xffffffff]);flags=rng.choice([0,1,0x40000000,0x40000001,0xffffffff]);prior=rng.getrandbits(32);present=i%2;elevation=rng.choice([-64.,-0.,0.,1.,128.]);raw=b''.join(struct.pack('<5f',*[rng.choice([-0.,0.,rng.uniform(-256,256)]) for _ in range(5)]) for _ in range(vc));tags=struct.pack('<%dH'%tc,*[rng.randrange(65536) for _ in range(tc)]);extra=b''.join(struct.pack('<20f',*[rng.uniform(-16,16) for _ in range(20)]) for _ in range(vc)) if present else b'';append(power,encoded,flags,prior,present,elevation,raw,tags,extra)
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));inp=run/'queries.bin';inp.write_bytes(b''.join(queries));outputs=[];executables=[];expected=sum(0x314+vc*236+tc*2 for vc,tc,*_ in metadata)
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('core_input_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture')
  if name=='native_x86':require(all(blob in binary for blob in blobs),'Original routine missing')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=60);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));data=target.read_bytes();require(len(data)==expected,'Output size');outputs.append(data);executables.append(dict(executable=str(exe),sha256=sha(exe)))
 pos=0;different=[];counts=dict(encoded_flags_applied=0,prior_flags_retained=0,extra_copied=0,extra_disabled=0,nonzero_elevation_arguments=0)
 for i,(vc,tc,encoded,flags,prior,present,elevation) in enumerate(metadata):
  n=0x314+vc*236+tc*2
  if outputs[0][pos:pos+n]!=outputs[1][pos:pos+n]:different.append(i)
  counts['encoded_flags_applied' if encoded&0x80000000 else 'prior_flags_retained']+=1;counts['extra_copied' if flags&0x40000000 and present else 'extra_disabled']+=1;counts['nonzero_elevation_arguments']+=elevation!=0
  require(struct.unpack_from('<f',outputs[0],pos+0x2e8)[0]==42.25,'Prior scalar unexpectedly overwritten');pos+=n
 safety=OUT/'core_input_safety_x64.exe';p=subprocess.run([str(safety)],capture_output=True,text=True,timeout=20);(run/'safety.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr);executables.append(dict(executable=str(safety),sha256=sha(safety)));require(all(v>100 for v in counts.values()),'Coverage');require(sha(ENGINE)==DIGEST and sha(MAP)==map_sha,'Original changed')
 receipt=dict(status='PASS' if not different else 'FAIL',cases=len(metadata),benchmark_records=43,vertices=sum(m[0] for m in metadata),coverage=counts,compared_bytes=expected,different_count=len(different),different_cases=different[:20],executables=executables,engine_sha256=DIGEST,map_sha256=map_sha,limits=['Complete original 101428E0 and10142210 execute with a test-only guarded allocator replacing1019D6C0; no native allocator/OOM claim','Full0x310object compared with four pointers normalized; full224byte vertex values, zero index buffer and triangle tags compared','Allocated512byte node internals and unwritten triangle indices are not compared; full geometry creation remains separate','Elevation argument unused by these two routines, not proof that all later geometry paths ignore elevation','Full BSP loader/read cursors, geometry creation, material binding and editor integration incomplete'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(metadata),vertices=receipt['vertices'],different_count=len(different),receipt=str(run/'results.json'))));require(not different,'CoreDisp initializer differs')
if __name__=='__main__':main()
