"""Characterize original optimized boxes; differences are evidence, not parity."""
from pathlib import Path
import collections,hashlib,json,struct,subprocess,sys,tempfile
from pe_image import Image
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/optimized-box-native-20261015'
ENGINE=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
DIGEST='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
def require(ok,message):
 if not ok:raise RuntimeError(message)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=Image(ENGINE);require(hashlib.sha256(p.b).hexdigest()==DIGEST,'Engine changed');raw=p.b[p.offset(0x10146850):p.offset(0x10146850)+0x3d4]
 header='// Checked original optimized-box instructions/constants; test only.\nstatic const unsigned char nativeBoxBytes[]={'+','.join('0x%02x'%v for v in raw)+'};\nstruct NativeBoxConstant{unsigned offset;unsigned char bytes[16];};\nstatic const NativeBoxConstant nativeBoxConstants[]={\n'
 constants=[0x103b6020,0x103b6170,0x103b6120,0x103b6110,0x103b6030,0x103506a0,0x103506c0,0x103506b0,0x103506d0,0x1031cbb0,0x1031d03c]
 for address in constants:header+='{0x%x,{'%(address-0x10000000)+','.join('0x%02x'%v for v in p.b[p.offset(address):p.offset(address)+16])+'}},\n'
 header+='};\n';hp=ROOT/'work/native_optimized_box_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared original SIMD box kernel/constants');return
 require(hp.read_text()==header,'Stale optimized box header');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
 # Fixed, already verified general-kernel fixture/query set. No benchmark boxes
 # are invented from brushes whose native optimization classification is unknown.
 source=ROOT/'outputs/convex-brush-oracle-20261015/verification-4vmweidj';map_path=source/'cube.bsp';queries=source/'0-queries.bin';outputs=[];records=[]
 for name,exe in [('general',ROOT/'outputs/convex-brush-oracle-20261015/brush_ray_native_x86.exe'),('box',OUT/'optimized_box_native_x86.exe')]:
  b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];require(struct.unpack_from('<H',b,pe+4)[0]==0x14c,'Expected x86 original-code probe')
  if name=='box':require(raw in b,'Original box body missing')
  target=run/(name+'.bin');result=subprocess.run([str(exe),str(map_path),str(queries),str(target)],capture_output=True,text=True,timeout=30);(run/(name+'.log')).write_text(result.stdout+result.stderr);require(result.returncode==0,result.stdout+result.stderr+' exit='+str(result.returncode));outputs.append(target.read_bytes());records.append(dict(executable=str(exe),sha256=sha(exe)))
 require(len(outputs[0])==len(outputs[1])==4420*36,'Unexpected output length');counts=collections.Counter();examples=[]
 names=['fraction','left_solid','start_solid','all_solid','contents','plane_x','plane_y','plane_z','plane_distance']
 for i in range(4420):
  a=struct.unpack_from('<9I',outputs[0],i*36);b=struct.unpack_from('<9I',outputs[1],i*36);different=[names[j] for j in range(9) if a[j]!=b[j]]
  if different:
   counts['different_cases']+=1
   for field in different:counts[field]+=1
   if len(examples)<20:examples.append(dict(case=i,query_words=list(struct.unpack_from('<12I',queries.read_bytes(),i*48)),fields=different,general=list(a),box=list(b)))
 require(sha(ENGINE)==DIGEST,'Original engine changed');receipt=dict(status='PASS_ORIGINAL_BOX_CHARACTERIZATION_NOT_X64_PARITY',cases=4420,executables=records,differences=dict(counts),examples=examples,engine_sha256=DIGEST,constant_addresses=[hex(a) for a in constants],limits=['Controlled six-plane boxes only','No owned optimized-box implementation yet','Synthetic zero surface table/normal globals','Native box output not assumed equivalent to general kernel','No world/displacement trace or editor integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],differences=dict(counts),receipt=str(run/'results.json'))))
if __name__=='__main__':main()
