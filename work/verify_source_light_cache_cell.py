"""Generate checked original instruction fragments and compare x86/x64 bounds."""
from pathlib import Path
import hashlib,json,math,random,struct,subprocess,sys,tempfile
from pe_image import Image
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/light-cache-cell-20261015'
ENGINE=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
EXPECTED='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
def require(ok,message):
 if not ok:raise RuntimeError(message)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def f(value):return struct.unpack('<f',struct.pack('<f',value))[0]
def main():
 image=Image(ENGINE);require(hashlib.sha256(image.b).hexdigest()==EXPECTED,'Installed engine changed')
 fragments=[('nativeCellBytes',0x10192950,0x10192a60),('nativeTruncateBytes',0x102ded50,0x102dedfb),('nativeWidthXY',0x103593ec,0x103593f0),('nativeWidthZ',0x10322e30,0x10322e34)]
 text='// Generated from hash-checked installed engine; test oracle only.\n'
 for name,start,end in fragments:
  raw=image.b[image.offset(start):image.offset(start)+(end-start)]
  text+='static const unsigned char '+name+'[]={'+','.join('0x%02x'%x for x in raw)+'};\n'
 if '--prepare' in sys.argv:
  (ROOT/'work/native_light_cache_cell_bytes.h').write_text(text);print('Prepared checked oracle fragments');return
 require((ROOT/'work/native_light_cache_cell_bytes.h').read_text()==text,'Oracle fragments stale; prepare and rebuild')
 OUT.mkdir(parents=True,exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
 points=[(0.0,-0.0,0.0),(-32.0,-32.0,-128.0),(32.0,32.0,128.0),(-32768.0,32768.0,-32768.0)]
 for boundary in (-256.0,-128.0,-64.0,-32.0,0.0,32.0,64.0,128.0,256.0):
  for offset in (-0.001,0.0,0.001):points.append(tuple(f(boundary+offset) for _ in range(3)))
 random_source=random.Random(1752026)
 points.extend(tuple(f(random_source.uniform(-32768,32768)) for _ in range(3)) for _ in range(4096))
 points.extend([(-2026.15625,-4541.78125,-351.90625),(-2026.15625,-4541.78125,-310.90625),(-299.5,-2278.5,-239.9375)])
 inp=run/'queries.bin';inp.write_bytes(b''.join(struct.pack('<3f',*p) for p in points));records=[]
 for name,machine in [('native_cell_oracle_x86',0x14c),('cache_cell_candidate_x64',0x8664)]:
  exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(struct.unpack_from('<H',raw,pe+4)[0]==machine,'Unexpected executable architecture')
  if machine==0x14c:
   for _,start,end in fragments[:2]:
    original=image.b[image.offset(start):image.offset(start)+(end-start)]
    require(original in raw,'Compiled native oracle lacks checked original instructions')
  result=subprocess.run([str(exe),str(inp),str(run/(name+'.bin'))],capture_output=True,text=True,timeout=30)
  (run/(name+'.log')).write_text(result.stdout+result.stderr);require(result.returncode==0,result.stdout+result.stderr)
  records.append(dict(executable=str(exe),sha256=sha(exe),architecture=hex(machine)))
 native=(run/'native_cell_oracle_x86.bin').read_bytes();candidate=(run/'cache_cell_candidate_x64.bin').read_bytes()
 require(len(native)==len(points)*24 and candidate==native,'Native/x64 bounds mismatch')
 require(struct.unpack_from('<6f',candidate,24)==(-64.0,-64.0,-256.0,-32.0,-32.0,-128.0),'Negative-boundary contract failed')
 rejected=0
 for point in [(float('nan'),0,0),(0,float('inf'),0),(0,0,32769)]:
  bad=run/('bad-'+str(rejected)+'.bin');bad.write_bytes(struct.pack('<3f',*point))
  p=subprocess.run([str(OUT/'cache_cell_candidate_x64.exe'),str(bad),str(run/('bad-'+str(rejected)+'.out'))],capture_output=True,text=True,timeout=15)
  require(p.returncode!=0,'Unsafe candidate query accepted');rejected+=1
 require(sha(ENGINE)==EXPECTED,'Engine changed during test')
 receipt=dict(status='PASS_EXTRACTED_X86_VS_X64',cases=len(points),exact_float_comparisons=len(points)*6,rejected_queries=rejected,executables=records,engine_sha256=EXPECTED,output_sha256=hashlib.sha256(native).hexdigest(),limits=['Extracted original bounds routine and original CRT conversion; private mapping only','CRT capability flag fixed to SSE conversion branch','No DLL initialization or live SFM execution','Collision-aware final cache position not implemented'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(points),receipt=str(run/'results.json'))))
if __name__=='__main__':main()
