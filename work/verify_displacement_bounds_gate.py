"""Isolated original SIMD displacement outer bounds gate vs x64."""
import json,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,DIGEST,sha,require

OUT=ROOT/'outputs/displacement-bounds-gate-20261015'
CONSTANTS=[0x103b6020,0x103b6030,0x103b6110,0x103b6120,0x103b6170]
def prepare():
 require(sha(ENGINE)==DIGEST,'Engine changed');im=Image(ENGINE);raw=im.b[im.offset(0x1014e790):im.offset(0x1014e790)+0x109];header='// Checked original displacement outer bounds gate.\nstatic const unsigned char nativeDispBoundsBytes[]={'+','.join('0x%02x'%v for v in raw)+'};\n'
 for i,a in enumerate(CONSTANTS):header+='static const unsigned char dispBoundsConstant%d[]={%s};\n'%(i,','.join('0x%02x'%v for v in im.b[im.offset(a):im.offset(a)+16]))
 header+='struct DispBoundsConstant{unsigned offset;const unsigned char*bytes;unsigned size;};\nstatic const DispBoundsConstant dispBoundsConstants[]={'+','.join('{0x%x,dispBoundsConstant%d,sizeof(dispBoundsConstant%d)}'%(a-0x10000000,i,i) for i,a in enumerate(CONSTANTS))+'};\n';return header,raw
def main():
 header,blob=prepare();hp=ROOT/'work/native_displacement_bounds_gate_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared original displacement bounds gate');return
 require(hp.read_text()==header,'Stale native bytes');rng=random.Random(202610158);max_float=struct.unpack('<f',struct.pack('<I',0x7f7fffff))[0];f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0];qs=[]
 for i in range(16384):
  center=[rng.uniform(-4096,4096) for _ in range(3)];extent=[rng.choice([0.,.03125,1.,64.,256.]) for _ in range(3)];lo=[f32(center[a]-extent[a]) for a in range(3)];hi=[f32(center[a]+extent[a]) for a in range(3)];s=[f32(rng.uniform(lo[a]-512,hi[a]+512)) for a in range(3)];d=[f32(rng.uniform(-1024,1024)) for _ in range(3)]
  if i<8192:
   s=[rng.choice([lo[a],hi[a],f32(lo[a]-.001),f32(hi[a]+.001),f32((lo[a]+hi[a])/2)]) for a in range(3)];d=[rng.choice([-0.,0.,-.03125,.03125,-1.,1.,-512.,512.]) for _ in range(3)]
  inv=[f32(1/v) if v!=0 else max_float for v in d];values=[]
  for v in [lo,hi,s,d,inv]:values.extend([*v,rng.uniform(-8192,8192)])
  values.append(rng.choice([0.,.03125,.125,1.]));qs.append(struct.pack('<21f',*values))
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));inp=run/'queries.bin';inp.write_bytes(b''.join(qs));results=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('gate_'+name+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];require(struct.unpack_from('<H',b,pe+4)[0]==machine,'Architecture')
  if name=='native_x86':require(blob in b,'Original code absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=30);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));result=target.read_bytes();require(len(result)==4*len(qs),'Output length');results.append(result);executables.append(dict(executable=str(exe),sha256=sha(exe)))
 different=[i for i in range(len(qs)) if results[0][4*i:4*i+4]!=results[1][4*i:4*i+4]];passes=sum(struct.unpack('<%dI'%len(qs),results[0]));require(0<passes<len(qs),'Gate coverage');require(sha(ENGINE)==DIGEST,'Original changed')
 receipt=dict(status='PASS' if not different else 'FAIL',cases=len(qs),passed_gates=passes,rejected_gates=len(qs)-passes,different_cases=different[:20],executables=executables,engine_sha256=sha(ENGINE),limits=['Outer gate only, no leaf policy or actual triangles','Finite xyz inputs, current SSE mode, arbitrary ignored fourth lanes','No editor integration']);(run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(qs),passed_gates=passes,different_count=len(different),receipt=str(run/'results.json'))));require(not different,'Bounds gate differs')
if __name__=='__main__':main()
