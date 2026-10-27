"""Original CMaterialVar GetString text/storage lifetime vs owned x64 snapshot."""
from pathlib import Path
from datetime import datetime,timezone
from decimal import Decimal,localcontext,ROUND_HALF_UP
import hashlib,json,random,re,struct,subprocess,tempfile,time
from verify_ifm_draw_packet import check_ranges
from verify_builtin_error_material_seed import source_closure

ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-get-string-20261027'
PARENT=ROOT/'outputs/material-test-fixtures-20261027/verification-92a6ght1/results.json'
LIB=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['material_get_string_candidate.h','probe_material_get_string.cpp','build_material_get_string.cmd','verify_material_get_string.py']

def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def need(b,s):
 if not b:raise RuntimeError(s)

def fixed(bits,places):
 sign='-' if bits&0x80000000 else ''
 if bits&0x7f800000==0x7f800000:
  # Original legacy printf special spellings, including its short-precision
  # textual rounding artifact. Signaling payloads quiet on float->double.
  return sign+('1.#QNAN0' if bits&0x7fffff else '1.#INF00') if places==6 else sign+('1.#QO' if bits&0x7fffff else '1.#IO')
 value=struct.unpack('<f',struct.pack('<I',bits))[0]
 with localcontext() as ctx:
  ctx.prec=256;ctx.rounding=ROUND_HALF_UP;number=Decimal.from_float(value)
  if number.copy_abs()>=Decimal('1e17'):number=number.quantize(Decimal(1).scaleb(number.copy_abs().adjusted()-16))
  number=number.quantize(Decimal(1).scaleb(-places));return format(number,'.'+str(places)+'f')

def packet(case):
 t,n,integer,env,meta,values,raw,resource=case
 if t==0:s=fixed(values[0],6).encode()
 elif t==1:s=raw.split(b'\0')[0]
 elif t==2:s=('[ '+''.join(fixed(v,6)+' ' for v in values[:n])+']').encode()
 elif t in [3,8]:s=b'env_cubemap' if t==3 and env else resource.split(b'\0')[0]
 elif t==4:s=str(integer if integer<0x80000000 else integer-0x100000000).encode()
 elif t==6:s=b'<UNDEFINED>'
 elif t==7:s=('[ '+''.join(fixed(values[col*4+row],3)+' ' for row in range(4) for col in range(4))+']').encode()[:511]
 else:s=b''
 storage=1 if t==1 else 2 if t in [0,2,4,7,8] or (t==3 and not env) else 0
 names=int(t==8 or (t==3 and not env));warnings=int(t==5 or t>=9)
 return struct.pack('<I',storage)+text(s)+text(b'42' if storage==2 else s)+struct.pack('<4I',2,warnings,names,names)

def text(s):return struct.pack('<I',len(s))+s

def corpus():
 rows=[];special=[0,0x80000000,1,0x80000001,0x00800000,0x3f800000,0xbf800000,0x3f000000,0x3dcccccd,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc12345,0x7fa12345,0xffa12345]
 def add(t,values=None,n=4,integer=0,env=0,raw=b'',resource=b'resource/name'):
  for meta in [0,1]:rows.append((t,n,integer,env,meta,values or [0]*16,raw,resource))
 for t in range(16):add(t,values=[0x3fa00000+i*0x80000 for i in range(16)],integer=0xfffffff9,raw=b'hello')
 for bits in special:
  add(0,values=[bits]*16);add(7,values=[bits]*16)
  for n in range(5):add(2,values=[bits]*16,n=n)
 for value in [0,1,0x7fffffff,0x80000000,0xffffffff,1000000000]:add(4,integer=value)
 for size in [0,1,510,511,4095]:add(1,raw=b'x'*size)
 for raw in [b'first\0ignored',bytes(range(1,256)),b' leading whitespace ',b'$basetexture']:
  add(1,raw=raw)
 for t in [3,8]:
  for name in [b'',b'%',b'a'*511,b'A/B.VTF',b'first\0ignored']:
   add(t,resource=name)
 add(3,env=1,resource=b'ignored sentinel name')
 rng=random.Random(2026102744)
 for i in range(1024):
  values=[rng.getrandbits(32) for _ in range(16)]
  add(0,values);add(2,values,n=i%5)
  if i<256:add(7,values)
 # Explicit halfway/precision/large-number boundaries independent of RNG.
 for value in [0.0000005,-0.0000005,0.0005,-0.0005,1.2345675,1.0625,1.125,1e16,1e17,1e18,1e20]:
  bits=struct.unpack('<I',struct.pack('<f',value))[0]
  for delta in [-1,0,1]:add(0,values=[(bits+delta)&0xffffffff]*16);add(7,values=[(bits+delta)&0xffffffff]*16)
 return rows

def main():
 need(sha(PARENT)=='5d712d698543ebe34de7429070e495cc67c648a5a6f3fb36f547f3d5cd8a6339','accepted parent receipt')
 parent=json.loads(PARENT.read_text());protected=parent['protected']|parent['source']|parent['original_inputs']
 need(all(sha(p)==h for p,h in protected.items()),'preceding source/input identities before')
 need(sha(LIB)=='c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8','historical material image')
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 image,static=check_ranges(LIB,'materialsystem-disassembly.txt',[('whole_GetString',0x10017de0,0x10018045),('CRT_float_format_init',0x1007d4e7,0x1007d547),('format_wrapper',0x1006e590,0x1006e5bc),('original_clear',0x1001a0a0,0x1001a16c)])
 targets=[image.word(0x10018048+i*4) for i in range(9)]
 need(targets==[0x10017e76,0x10017e45,0x10017ea9,0x10017ff9,0x10017e4f,0x1001802b,0x1001801f,0x10017f69,0x1001800b],'dispatch table distinct from disassembly')
 need(image.word(0x10098f34+6*4)==0x10017de0,'actual virtual slot6')
 for a,s in [(0x10099040,'%d'),(0x1009903c,'%f'),(0x10099038,'%f '),(0x10099030,'%.3f '),(0x10099024,'env_cubemap'),(0x10099018,'<UNDEFINED>')]:need(image.string(a)==s,'format/static string')
 (run/'static-reference.json').write_text(json.dumps(dict(ranges=static,jump_targets=list(map(hex,targets))),indent=2)+'\n')
 sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/p for p in NEW])
 started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_get_string.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180)
 (run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh probe builds')
 exes=[]
 for tag,machine in [('reference',0x14c),('owned',0x8664)]:
  exe=OUT/('get_string_'+tag+('_x86.exe' if tag=='reference' else '_x64.exe'));raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
  need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'fresh architecture/manifest')
  log=(OUT/(tag+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log);need(not re.search(r'\b(?:warning|error)\b',log,re.I),'clean build '+tag)
  inspection=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout
  (run/(tag+'-inspection.txt')).write_text(inspection);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspection,re.M|re.I);need(modules==['KERNEL32.dll'],'system-only direct imports')
  exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=modules))
 cases=corpus();query=bytearray(struct.pack('<I',len(cases)));expected=bytearray(query)
 for c in cases:query+=struct.pack('<21I',*c[:5],*c[5])+text(c[6])+text(c[7]);expected+=packet(c)
 (run/'query.bin').write_bytes(query);(run/'expected.bin').write_bytes(expected)
 for tag,exe in zip(['reference','owned'],exes):
  target=run/(tag+'.bin');p=subprocess.run([exe['path'],str(LIB if tag=='reference' else run/'absent.dll'),str(run/'query.bin'),str(target)],cwd=run,capture_output=True,text=True,timeout=90)
  (run/(tag+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,tag+' runtime '+p.stderr);actual=target.read_bytes()
  if actual!=expected:
   at=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
   raise RuntimeError(tag+' independent-model mismatch at byte '+str(at))
 native=(run/'reference.log').read_text();m=re.search(r'allocations=(\d+) releases=(\d+)',native);need(m and m[1]==m[2],'native all transient allocations released')
 p=subprocess.run([exes[1]['path'],'--self-test'],cwd=run,capture_output=True,text=True,timeout=30);(run/'safety.log').write_text(p.stdout+p.stderr)
 need(p.returncode==0 and 'checks=10' in p.stdout,'ten output-preserving refusal/safety checks')
 need(all(sha(p)==h for p,h in protected.items()) and all(sha(p)==h for p,h in sources.items()),'preceding source/inputs and new source unchanged after')
 need(sha(parent['editor'])==parent['editor_sha256'] and all(sha(ROOT/p)==h for p,h in parent['editor_dependencies'].items()),'editor313 dependencies unchanged')
 result=dict(status='VERIFIED bounded GetString snapshot/conversion/lifetime; PARTIALLY VERIFIED complete material ownership',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',
  cases=len(cases),exact_output_bytes=len(expected),allocations=int(m[1]),releases=int(m[2]),safety_checks=10,
  per_type_cases={str(t):sum(c[0]==t for c in cases) for t in range(16)},matrix_cases=sum(c[0]==7 for c in cases),
  source=sources,protected=protected,parent=str(PARENT),parent_sha256=sha(PARENT),executables=exes,
  query_sha256=sha(run/'query.bin'),output_sha256=sha(run/'reference.bin'),static_reference_sha256=sha(run/'static-reference.json'),
  editor=parent['editor'],editor_sha256=parent['editor_sha256'],editor_dependencies=parent['editor_dependencies'],editor_rebuilt=False,
  limits=['actual original GetString/formatters/constructors/Clear execute unpatched; float/vector/int/string/undefined use original constructors and matrix uses original setter',
   'resource types3/8 use constructor-initialized objects with explicit controlled layout/name/release services; not real engine texture/material construction',
   'unknown types5/9..15 and zero-component vector are controlled getter inputs; unsupported storage reset to undefined before cleanup',
   'queue service always null, owner absent; no queued redirection, concurrency, other locale or FP status equivalence claim',
   'owned result is an owned snapshot, unlike original shared512-byte/member-string/static-literal pointer; host lifecycle trace is semantic, not std::string allocation identity',
   'resource names at most511bytes; input string at most4095, vector components0..4; broader unsafe/null resource states refused',
   'existing original IFM typed capture still has host getter service; joined capture integration not performed yet',
   'editor unchanged; no SFM, GPU, original writes, rendering or normal material-variable resource lifetime claim'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n')
 print(json.dumps({k:result[k] for k in ['status','cases','exact_output_bytes','allocations','matrix_cases']},indent=2),flush=True)

if __name__=='__main__':main()
