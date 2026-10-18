from pathlib import Path
import hashlib,json,struct,subprocess,sys,tempfile
from pe_image import Image
ROOT=Path(__file__).resolve().parent.parent
ENGINE=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
DIGEST='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
def require(ok,message):
 if not ok:raise RuntimeError(message)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 image=Image(ENGINE);require(hashlib.sha256(image.b).hexdigest()==DIGEST,'Installed engine changed')
 raw=image.b[image.offset(0x101947b0):image.offset(0x101947b0)+0x391]
 header='// Generated checked original policy body; isolated test oracle only.\nstatic const unsigned char nativePositionBytes[]={'+','.join('0x%02x'%x for x in raw)+'};\n'
 path=ROOT/'work/native_light_cache_position_bytes.h'
 if '--prepare' in sys.argv:path.write_text(header);print('Prepared original policy body');return
 require(path.read_text()==header,'Stale original policy header')
 out=ROOT/'outputs/light-cache-position-oracle-20261015';run=Path(tempfile.mkdtemp(prefix='verification-',dir=out));records=[]
 for name,machine in [('native_position_x86',0x14c),('position_candidate_x64',0x8664)]:
  exe=out/(name+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];require(struct.unpack_from('<H',b,pe+4)[0]==machine,'Wrong probe architecture')
  if machine==0x14c:
   require(raw in b,'Original policy body missing from executable')
   for start,end in [(0x10192950,0x10192a60),(0x102ded50,0x102dedfb)]:
    fragment=image.b[image.offset(start):image.offset(start)+(end-start)]
    require(fragment in b,'Original dependency body missing from executable')
  p=subprocess.run([str(exe),str(run/(name+'.bin'))],capture_output=True,text=True,timeout=30);(run/(name+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));records.append(dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),stdout=p.stdout))
 native=(run/'native_position_x86.bin').read_bytes();candidate=(run/'position_candidate_x64.bin').read_bytes()
 require(native==candidate,'Native and x64 event/result streams differ')
 cursor=0;cases=0
 while cursor<len(native):
  count=struct.unpack_from('<I',native,cursor)[0];cursor+=4+count*4;cases+=1
 require(cursor==len(native) and cases==4608,'Unexpected case coverage')
 require(sha(ENGINE)==DIGEST,'Original engine changed')
 result=dict(status='PASS_NATIVE_POLICY_WITH_SYNTHETIC_WORLD',cases=cases,exact_event_and_result_bytes=len(native),executables=records,engine_sha256=DIGEST,output_sha256=hashlib.sha256(native).hexdigest(),unrelated_record_bytes_preserved=True,input_points_preserved=True,original_routine_bytes_preserved=True,limits=['Original position and cell bodies unmodified in private mapping','Leaf/contents/clip/trace dependencies synthetic','Security checker synthetic cookie validator; CRT capability fixed to SSE','No DLL initialization or real map collision parity','Not wired into editor'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(status=result['status'],cases=cases,receipt=str(run/'results.json'))))
if __name__=='__main__':main()
