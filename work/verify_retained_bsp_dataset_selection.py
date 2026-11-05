"""Owned retained loader integration; originals are only read and hashed."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib,json,re,struct,subprocess,tempfile
from verify_texture_sampling_cross_api import fixture as owned_vtf

ROOT=Path(__file__).resolve().parent.parent
VS=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat')
CHANGED='work/retained_source_bsp_world_candidate.h'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(x,s):
    if not x:raise RuntimeError(s)
def pe(p):
    b=Path(p).read_bytes();i=struct.unpack_from('<I',b,60)[0]
    return struct.unpack_from('<H',b,i+4)[0],struct.unpack_from('<H',b,i+24)[0]
def main():
    parent=Path((ROOT/'outputs/retained-selection-current.txt').read_text().strip())
    before=json.loads((parent/'before.json').read_text());run=Path(tempfile.mkdtemp(prefix='run-',dir=parent))
    receipt={'status':'IN PROGRESS','run':str(run),'time_utc':datetime.now(timezone.utc).isoformat(),'commands':[]}
    def execute(args,label):
        args=list(map(str,args));p=subprocess.run(args,cwd=ROOT,capture_output=True,text=True,timeout=900,creationflags=subprocess.CREATE_NO_WINDOW)
        (run/(label+'.log')).write_text(p.stdout+p.stderr);receipt['commands'].append({'args':args,'exit_code':p.returncode})
        need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1800:]);return p.stdout+p.stderr
    try:
        for p,h in before.items():
            if p!=CHANGED:need(sha(p)==h,'protected initial identity '+p)
        need(sha(parent/'before-retained_source_bsp_world_candidate.h')==before[CHANGED],'before header identity')
        vtf=run/'owned.vtf';vtf.write_bytes(owned_vtf(0,0))
        execute(['cmd.exe','/d','/c',ROOT/'work/build_retained_bsp_dataset_selection_test.cmd',run/'cpu'],'build-new-test')
        exe=run/'cpu/test_retained_bsp_dataset_selection_x64.exe';need(pe(exe)==(0x8664,0x20b),'test AMD64 PE32+')
        receipt['test_exe']={'path':str(exe),'sha256':sha(exe),'architecture':'AMD64 PE32+'}
        outputs=[execute([exe,run/('owned-'+str(n)),vtf],'test-'+str(n)) for n in range(2)]
        need(outputs[0]==outputs[1] and 'PASS retained_dataset_selection=1' in outputs[0],'repeatable test protocol')
        receipt['test_output']=outputs[0];print(outputs[0],flush=True)
        # Existing actual retained-world regression has its own owned fixtures,
        # injection checks, regrouping, displacement and read-only map checks.
        scene=run/'test_source_bsp_lightmap_scene.exe';batch=run/'build-scene.cmd'
        batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\test_source_bsp_lightmap_scene.cpp /Fo:"{run/"scene.obj"}" /Fe:"{scene}" >"{run/"scene-build.log"}" 2>&1','exit /b %errorlevel%','']))
        execute(['cmd.exe','/d','/c',batch],'build-scene');need(pe(scene)==(0x8664,0x20b),'scene AMD64 PE32+')
        content=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod');bsp=content.parent/'tf_movies/maps/mtt_soldier.bsp';bsp_hash=sha(bsp)
        receipt['legacy_regression']=execute([scene,run/'legacy-scene',bsp,content,'mtt_soldier',vtf],'legacy-scene')
        need(sha(bsp)==bsp_hash,'original benchmark map changed');print(receipt['legacy_regression'],flush=True)
        editor=run/'editor-x64';execute(['cmd.exe','/d','/c',ROOT/'work/build_sfm64_editor_x64.cmd',editor],'complete-editor')
        binary=editor/'sfm64_session_ui.exe';need(pe(binary)==(0x8664,0x20b),'editor AMD64 PE32+')
        warnings={}
        for label,p in [('focused',run/'cpu/build.log'),('legacy',run/'scene-build.log'),('editor',editor/'build.log')]:warnings[label]=re.findall(r'warning (C\d+): ([^\r\n]+)',p.read_text(errors='replace'))
        receipt['warnings']=warnings;receipt['editor']={'path':str(binary),'sha256':sha(binary),'architecture':'AMD64 PE32+','runtime_tested':False,'shader_diagnostics':(editor/'shader-build.log').read_text()}
        receipt['shaders']={p.name:sha(p) for p in (editor/'shaders').glob('*.spv')};need(len(receipt['shaders'])==15,'15 shader build outputs')
        for p,h in before.items():
            if p!=CHANGED:need(sha(p)==h,'protected final identity '+p)
        receipt['header_transition']={'path':CHANGED,'before':before[CHANGED],'after':sha(ROOT/CHANGED)}
        receipt['protected_sha256']={p:h for p,h in before.items() if p!=CHANGED}
        receipt['new_source_sha256']={p:sha(ROOT/'work'/p) for p in ['test_retained_bsp_dataset_selection.cpp','build_retained_bsp_dataset_selection_test.cmd','verify_retained_bsp_dataset_selection.py']}
        receipt['legacy_exe']={'path':str(scene),'sha256':sha(scene)};receipt['benchmark_read_only']={'path':str(bsp),'sha256':bsp_hash}
        receipt['status']='VERIFIED explicit retained dataset selection and CPU rollback; face planner/GPU integration incomplete'
    except Exception as e:
        receipt['status']='PARTIALLY VERIFIED';receipt['failure']=repr(e);raise
    finally:
        p=run/'results.json';p.write_text(json.dumps(receipt,indent=2)+'\n');(parent/'latest.json').write_text(json.dumps({'receipt':str(p),'sha256':sha(p)},indent=2)+'\n')
    print(json.dumps({'status':receipt['status'],'receipt':str(run/'results.json'),'editor':receipt['editor'],'warnings':{k:len(v) for k,v in receipt['warnings'].items()}},indent=2))
if __name__=='__main__':main()
