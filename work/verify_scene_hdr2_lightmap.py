"""Owned retained geometry -> HDR2 CPU face plan; no original code execution."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,struct,subprocess,tempfile
from verify_texture_sampling_cross_api import fixture as owned_vtf
ROOT=Path(__file__).resolve().parent.parent
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(x,s):
    if not x:raise RuntimeError(s)
def main():
    parent=Path((ROOT/'outputs/scene-hdr2-plan-current.txt').read_text().strip());before=json.loads((parent/'before.json').read_text());run=Path(tempfile.mkdtemp(prefix='run-',dir=parent))
    result={'status':'IN PROGRESS','time_utc':datetime.now(timezone.utc).isoformat(),'commands':[]}
    def execute(args,label):
        args=list(map(str,args));p=subprocess.run(args,cwd=ROOT,capture_output=True,text=True,timeout=600,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr);result['commands'].append({'args':args,'exit_code':p.returncode});need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1800:]);return p.stdout+p.stderr
    try:
        for p,h in before.items():need(sha(p)==h,'initial protected identity '+p)
        vtf=run/'owned.vtf';vtf.write_bytes(owned_vtf(0,0))
        execute(['cmd.exe','/d','/c',ROOT/'work/build_scene_hdr2_lightmap_test.cmd',run/'cpu'],'build')
        exe=run/'cpu/test_scene_hdr2_lightmap_x64.exe';b=exe.read_bytes();i=struct.unpack_from('<I',b,60)[0];need((struct.unpack_from('<H',b,i+4)[0],struct.unpack_from('<H',b,i+24)[0])==(0x8664,0x20b),'AMD64 PE32+')
        logs=[execute([exe,run/('owned-'+str(n)),vtf],'test-'+str(n)) for n in range(2)]
        need(logs[0]==logs[1] and 'PASS hdr2_cpu_face_plan=1' in logs[0],'identical complete success output')
        for p,h in before.items():need(sha(p)==h,'final protected identity '+p)
        result.update(status='VERIFIED retained HDR2 CPU face plan; GPU byte roundtrip and draw integration remain incomplete',test_output=logs[0],executable={'path':str(exe),'sha256':sha(exe),'architecture':'AMD64 PE32+'},warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/'cpu/build.log').read_text(errors='replace')),protected_sha256=before,source_sha256={n:sha(ROOT/'work'/n) for n in ['scene_hdr2_lightmap_candidate.h','test_scene_hdr2_lightmap.cpp','build_scene_hdr2_lightmap_test.cmd','verify_scene_hdr2_lightmap.py']},editor_rebuilt=False,editor_runtime_tested=False,new_gpu_frames=0,limits=['New planner is not yet included in the editor dependency closure. Current editor sources/helpers/executables unchanged.','Explicit placement is a controlled per-face page policy; native atlas/filter/output parity is unverified.','CPU owner/encoded bytes/UV/source identities only; no Vulkan resource or draw claim.'])
    except Exception as e:result.update(status='PARTIALLY VERIFIED',failure=repr(e));raise
    finally:
        p=run/'results.json';p.write_text(json.dumps(result,indent=2)+'\n');(parent/'latest.json').write_text(json.dumps({'receipt':str(p),'sha256':sha(p)},indent=2)+'\n')
    print(json.dumps({'status':result['status'],'receipt':str(p),'exe':result['executable'],'warnings':len(result['warnings']),'test':logs[0]},indent=2))
if __name__=='__main__':main()
