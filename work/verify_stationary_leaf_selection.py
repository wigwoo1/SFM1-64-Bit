import json,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');image=Image(ENGINE);fragments=[('nativeSelectionBytes',0x10145810,0x101459a4),('nativeStackProbeBytes',0x102df090,0x102df0bb),('nativeCookieCheckBytes',0x102de362,0x102de36c)];header='// Checked native stationary leaf selection and stack/cookie helpers.\n';blobs=[]
 for name,a,z in fragments:
  raw=image.b[image.offset(a):image.offset(a)+(z-a)];blobs.append(raw);header+='static const unsigned char '+name+'[]={'+','.join('0x%02x'%v for v in raw)+'};\n'
 hp=ROOT/'work/native_stationary_leaf_selection_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared stationary leaf selector');return
 require(hp.read_text()==header,'Stale native bytes');before=sha(MAP);b=MAP.read_bytes();lo,ln=struct.unpack_from('<II',b,8+10*16);mo=struct.unpack_from('<I',b,8+14*16)[0];bounds=struct.unpack_from('<6f',b,mo);rng=random.Random(20261015);qs=[]
 for i in range(2048):
  if i<1024:
   leaf=rng.randrange(ln//32);box=struct.unpack_from('<6h',b,lo+32*leaf+8);center=[rng.choice([box[a],box[a+3],(box[a]+box[a+3])/2]) for a in range(3)]
  else:center=[rng.uniform(bounds[a],bounds[a+3]) for a in range(3)]
  extent=rng.choice([0,1,2,8,32]);qs.append(struct.pack('<6f',*center,extent,extent,extent))
 out=ROOT/'outputs/stationary-leaf-selection-20261015';run=Path(tempfile.mkdtemp(prefix='verification-',dir=out));inp=run/'queries.bin';inp.write_bytes(b''.join(qs));results=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=out/('selection_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Wrong architecture')
  if name=='native_x86':require(all(v in binary for v in blobs),'Original instructions absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(MAP),str(inp),str(target)],capture_output=True,text=True,timeout=40);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));results.append(target.read_bytes());executables.append(dict(executable=str(exe),sha256=sha(exe)))
 stride=1026*4;require(len(results[0])==len(results[1])==len(qs)*stride,'Output length');different=[i for i in range(len(qs)) if results[0][stride*i:stride*(i+1)]!=results[1][stride*i:stride*(i+1)]]
 receipt=dict(status='PASS' if not different else 'FAIL',cases=len(qs),different_count=len(different),examples=[dict(case=i,query=struct.unpack('<6f',qs[i]),native=struct.unpack_from('<20I',results[0],stride*i),owned=struct.unpack_from('<20I',results[1],stride*i)) for i in different[:10]],executables=executables,map_sha256=sha(MAP),engine_sha256=sha(ENGINE),limits=['Leaf order/topnode only; not stationary world collision','Queue overflow rejected by owned helper; not exercised here','Output capacity1024; current x87precision only','No editor integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(qs),different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Stationary leaf selection differs');require(sha(MAP)==before and sha(ENGINE)==DIGEST,'Original changed')
if __name__=='__main__':main()
