"""Real Vulkan HDR2 image bytes from the retained CPU face plan, owned fixtures."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,struct,subprocess,tempfile
from verify_texture_sampling_cross_api import fixture as owned_vtf
ROOT=Path(__file__).resolve().parent.parent
CHANGED='work/test_scene_hdr2_lightmap.cpp'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(x,s):
    if not x:raise RuntimeError(s)
def main():
    parent=Path((ROOT/'outputs/vulkan-hdr2-pages-current.txt').read_text());before=json.loads((parent/'before.json').read_text());run=Path(tempfile.mkdtemp(prefix='run-',dir=parent));result={'status':'IN PROGRESS','time_utc':datetime.now(timezone.utc).isoformat(),'commands':[]}
    def execute(args,label):
        args=list(map(str,args));p=subprocess.run(args,cwd=ROOT,capture_output=True,text=True,timeout=600,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr);result['commands'].append({'args':args,'exit_code':p.returncode});need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-2000:]);return p.stdout+p.stderr
    def architecture(p):
        b=p.read_bytes();i=struct.unpack_from('<I',b,60)[0];need((struct.unpack_from('<H',b,i+4)[0],struct.unpack_from('<H',b,i+24)[0])==(0x8664,0x20b),'AMD64 PE32+')
    try:
        for p,h in before.items():
            if p!=CHANGED:need(sha(p)==h,'initial protected identity '+p)
        old=(parent/'before-test_scene_hdr2_lightmap.cpp').read_bytes();need(hashlib.sha256(old).hexdigest()==before[CHANGED],'saved CPU test identity')
        current=(ROOT/CHANGED).read_bytes();normalized=current.replace(b'#ifndef SFM64_HDR2_PLAN_NO_MAIN\n',b'');need(normalized==old+b'#endif\n','only CPU test main include guard changed')
        vtf=run/'owned.vtf';vtf.write_bytes(owned_vtf(0,0))
        execute(['cmd.exe','/d','/c',ROOT/'work/build_vulkan_hdr2_lightmap_images_probe.cmd',run/'gpu'],'build-gpu')
        exe=run/'gpu/probe_vulkan_hdr2_lightmap_images_x64.exe';architecture(exe)
        outputs=[execute([exe,run/('owned-gpu-'+str(n)),vtf],'gpu-'+str(n)) for n in range(2)]
        need(outputs[0]==outputs[1] and 'PASS vulkan_hdr2_page_cases=6' in outputs[0],'repeatable GPU complete output');print(outputs[0],flush=True)
        execute(['cmd.exe','/d','/c',ROOT/'work/build_scene_hdr2_lightmap_test.cmd',run/'cpu'],'build-cpu-regression');cpu=run/'cpu/test_scene_hdr2_lightmap_x64.exe';architecture(cpu);result['cpu_regression']=execute([cpu,run/'owned-cpu',vtf],'cpu-regression')
        for p,h in before.items():
            if p!=CHANGED:need(sha(p)==h,'final protected identity '+p)
        result.update(status='VERIFIED Vulkan HDR2 image byte roundtrip and prepare/discard ownership; shader/editor draw integration incomplete',gpu_output=outputs[0],executable={'path':str(exe),'sha256':sha(exe),'architecture':'AMD64 PE32+'},cpu_executable={'path':str(cpu),'sha256':sha(cpu)},warnings={n:re.findall(r'warning (C\d+): ([^\r\n]+)',(run/n/'build.log').read_text(errors='replace')) for n in ('cpu','gpu')},protected_sha256={p:h for p,h in before.items() if p!=CHANGED},test_guard_transition={'path':CHANGED,'before':before[CHANGED],'after':sha(ROOT/CHANGED)},new_source_sha256={n:sha(ROOT/'work'/n) for n in ['vulkan_hdr2_lightmap_images_candidate.h','probe_vulkan_hdr2_lightmap_images_x64.cpp','build_vulkan_hdr2_lightmap_images_probe.cmd','verify_vulkan_hdr2_lightmap_images.py']},editor_rebuilt=False,new_shader_draws=0,limits=['No renderer descriptors, sampled shader draw, live SFM output or atlas/filter equivalence tested.','Vulkan validation layer not enabled; byte comparisons and tracked resources are scoped evidence.','Fault VkResult returns injected by existing dispatch seam around real Vulkan resources, not actual driver failures.','New owner/probe are outside editor include closure; existing editor sources/binaries unchanged.'])
    except Exception as e:result.update(status='PARTIALLY VERIFIED',failure=repr(e));raise
    finally:
        p=run/'results.json';p.write_text(json.dumps(result,indent=2)+'\n');(parent/'latest.json').write_text(json.dumps({'receipt':str(p),'sha256':sha(p)},indent=2)+'\n')
    print(json.dumps({'status':result['status'],'receipt':str(p),'exe':result['executable'],'warnings':{k:len(v) for k,v in result['warnings'].items()}},indent=2))
if __name__=='__main__':main()
