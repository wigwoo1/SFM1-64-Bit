"""Explicit shading-channel transitions, CPU/owned GPU and complete-editor checks."""
from pathlib import Path
from datetime import datetime,timezone
import json,os,re,struct,subprocess,tempfile,shutil
from verify_session_preview_aspect import ROOT,VS,INCLUDE,CONTENT
from verify_anisotropic_bitness import sha,need,put,pe
from verify_texture_sampling_cross_api import fixture
OUT=ROOT/'outputs/scene-shading-attributes-20261101'
EDITOR=ROOT/'outputs/sfm64-editor-shading-attributes-x64-20261101/sfm64_session_ui.exe'

def profile():
    before=json.loads((OUT/'before.json').read_text());need(sha(before['parent']['receipt'])==before['parent']['receipt_sha256'],'prior checkpoint')
    sources=dict(before['source_sha256']);protected=dict(before['protected_sha256']);protected[before['parent']['receipt']]=before['parent']['receipt_sha256'];transitions=[]
    for t in before['transitions']:
        need(sha(t['backup'])==t['before_sha256']==sources[t['path']],'explicit before source')
        transitions.append(dict(t,after_sha256=sha(t['path'])));sources[t['path']]=sha(t['path']);protected[t['backup']]=t['before_sha256']
    visited=set()
    def visit(p):
        p=p.resolve()
        if p in visited:return
        visited.add(p)
        if str(p) in sources:need(sha(p)==sources[str(p)],'unchanged or explicit source transition')
        sources[str(p)]=sha(p)
        for n in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            if (p.parent/n).exists():visit(p.parent/n)
    for n in ['scene_shading_attributes_candidate.h','test_scene_shading_attributes.cpp','test_fur_scene_upload.cpp','test_camera_frustum_scene_scope.cpp','test_mtt_soldier_scene_render_preparation.cpp','test_atomic_session_save.cpp','test_session_document_save_copy.cpp','test_editor_transaction.cpp',Path(__file__).name]:visit(ROOT/'work'/n)
    protected[str(ROOT/'work/roundtrip-11.dmx')]=sha(ROOT/'work/roundtrip-11.dmx')
    for group in (sources,protected):
        for p,h in group.items():need(sha(p)==h,'frozen current identity '+p)
    return before,sources,protected,transitions

def main():
    before,sources,protected,transitions=profile();run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print(str(run),flush=True)
    clean={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')}
    def execute(args,label,cwd=ROOT,env=None,timeout=600):
        p=subprocess.run(list(map(str,args)),cwd=cwd,env=env or clean,capture_output=True,text=True,timeout=timeout,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1800:]);return p.stdout+p.stderr
    source=ROOT/'outputs/d3d9-prepared-scene-20261101/packet-gpu-zhsn3_pu/benchmark-input-copy.dmx';need(str(source) in protected and sha(source)==protected[str(source)],'owned benchmark source copy')
    session=run/'input-copy.dmx';shutil.copyfile(source,session);protected[str(session)]=sha(session);protected[str(CONTENT/'gameinfo.txt')]=sha(CONTENT/'gameinfo.txt')
    data=run/'cpu-data';data.mkdir();(data/'materials/test').mkdir(parents=True);(data/'materials/test/pixel.vtf').write_bytes(fixture(0,0))
    names=['test_scene_shading_attributes','test_fur_scene_upload','test_camera_frustum_scene_scope','test_mtt_soldier_scene_render_preparation','test_atomic_session_save','test_session_document_save_copy','test_editor_transaction']
    lines=['@echo off',f'cd /d "{ROOT}"',f'call "{VS}" >nul','if errorlevel 1 exit /b 1']
    for n in names:lines += [f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /I"{INCLUDE}" work\\{n}.cpp /Fo:"{run/(n+".obj")}" /Fe:"{run/(n+".exe")}" >"{run/(n+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1']
    build=run/'build-cpu.cmd';build.write_text('\n'.join(lines+['exit /b 0','']));execute(['cmd.exe','/d','/c',build],'build-cpu-driver');logs={};warnings={}
    for n in names:
        need(pe(run/(n+'.exe'))==(0x8664,0x20b),'native CPU test executable')
        warnings[n]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(n+'-build.log')).read_text())
        args=[run/(n+'.exe')]
        if n=='test_scene_shading_attributes':args += [data,session,CONTENT]
        elif n=='test_mtt_soldier_scene_render_preparation':args += [session,CONTENT/'gameinfo.txt']
        elif n in ['test_atomic_session_save','test_session_document_save_copy']:
            target=run/(n+'-data');target.mkdir();args += [target]
        logs[n]=execute(args,n);print(n+' passed',flush=True)
    cpu={'status':'VERIFIED CPU shading transport/guards plus existing CPU regressions','time_utc':datetime.now(timezone.utc).isoformat(),'logs':logs,'compiler_warnings':warnings,'source_sha256':sources,'protected_sha256':protected,'transitions':transitions,'executables':{n:{'path':str(run/(n+'.exe')),'sha256':sha(run/(n+'.exe'))} for n in names}}
    cpu_receipt=run/'cpu-results.json';put(cpu_receipt,cpu);put(OUT/'latest-cpu-verification.json',{'receipt':str(cpu_receipt),'receipt_sha256':sha(cpu_receipt)})
    # Reuse the previous owned indexed verifier unchanged. Its raw-reference
    # lookup expects the bitness checkpoint, while this explicit new profile
    # carries the actual current parent and declared before/after transitions.
    import verify_d3d9_prepared_scene as gpu
    gpu_dir=run/'indexed-gpu';gpu_dir.mkdir();previous=json.loads(Path(before['parent']['receipt']).read_text());original_gpu=json.loads(Path(previous['verification']['receipt']).read_text())
    put(gpu_dir/'before.json',{'parent':original_gpu['parent'],'source_sha256':sources,'protected_sha256':protected,'current_parent':before['parent'],'explicit_source_transitions':transitions})
    gpu.OUT=gpu_dir;gpu.main();gpu_ref=json.loads((gpu_dir/'latest-verification.json').read_text());need(sha(gpu_ref['receipt'])==gpu_ref['receipt_sha256'],'new indexed proof');g=json.loads(Path(gpu_ref['receipt']).read_text());need(g['cases']==730 and all(m['over_2e_6']==0 for m in g['metrics'].values()),'unchanged native pixel gate')
    need(not EDITOR.parent.exists(),'fresh isolated complete editor output required')
    execute(['cmd.exe','/d','/c',ROOT/'work/build_sfm64_editor_x64.cmd',EDITOR.parent],'complete-editor-build');need(pe(EDITOR)==(0x8664,0x20b),'complete editor native x64')
    baseline=Path(previous['candidate_preserved']['path']);need(sha(baseline)==previous['candidate_preserved']['sha256'],'isolated baseline editor')
    baseline_warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(baseline.parent/'build.log').read_text());editor_warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(EDITOR.parent/'build.log').read_text());need(editor_warnings==baseline_warnings and len(editor_warnings)==15,'same15 inherited editor warnings')
    need(not (EDITOR.parent/'shader-build.log').read_text().strip(),'owned shader diagnostics');shaders={str(p.relative_to(EDITOR.parent)):sha(p) for p in (EDITOR.parent/'shaders').glob('*.spv')};need(len(shaders)==15 and all(sha(baseline.parent/n)==h for n,h in shaders.items()),'15 unchanged owned shaders')
    dependencies={}
    def closure(p):
        p=p.resolve()
        if str(p) in dependencies:return
        dependencies[str(p)]=sha(p)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            if (p.parent/inc).exists():closure(p.parent/inc)
    closure(ROOT/'work/probe_vulkan_win32_surface_x64.cpp')
    modes=[('default',{}),('two-texture',{'SFM64_DIAGNOSTIC_TWO_TEXTURE_TEST':'1'}),('selfillum-red',{'SFM64_DIAGNOSTIC_SELFILLUM_TEST':'1','SFM64_DIAGNOSTIC_MASK_UPLOAD_TEST':'1','SFM64_DIAGNOSTIC_SELFILLUM_FIXTURE':'red'}),('selfillum-alpha-half',{'SFM64_DIAGNOSTIC_SELFILLUM_TEST':'1','SFM64_DIAGNOSTIC_MASK_UPLOAD_TEST':'1','SFM64_DIAGNOSTIC_SELFILLUM_FIXTURE':'alpha-half'}),('selfillum-color',{'SFM64_DIAGNOSTIC_SELFILLUM_TEST':'1','SFM64_DIAGNOSTIC_MASK_UPLOAD_TEST':'1','SFM64_DIAGNOSTIC_SELFILLUM_FIXTURE':'red-half','SFM64_DIAGNOSTIC_MATERIAL_COLOR_TEST':'1','SFM64_DIAGNOSTIC_FIXTURE_GAMMA_READ':'mixed'})]
    foreign=run/'empty-cwd';foreign.mkdir();commands=[];captures={}
    for mode,options in modes:
        variants=[('default',{})]
        if mode in ['default','two-texture']:variants += [('bc-disabled',{'SFM64_DIAGNOSTIC_DISABLE_BC_TEXTURES':'1'}),('anisotropy',{'SFM64_DIAGNOSTIC_ANISOTROPIC_TEXTURE_TEST':'1'})]
        for variant,extra in variants:
            pair=[]
            for label,exe in [('baseline',baseline),('candidate',EDITOR)]:
                target=run/(mode+'-'+variant+'-'+label);target.mkdir();env=dict(clean,**options,**extra,SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(target));args=[exe,'--control-test','--session',session,'--content-directory',CONTENT,'--start-frame','119']
                log=execute(args,mode+'-'+variant+'-'+label,cwd=foreign,env=env);need('model_gpu_presentation_passed=1 frames=5' in log and 'gpu_readback_copies=5 capture_frames=1' in log,'actual editor GPU frames');frames={str(n):sha(target/f'vulkan-retained-frame-{n}.rgba') for n in [0,119,120]};pair.append(frames);captures[mode+'-'+variant+'-'+label]=frames;commands.append({'args':list(map(str,args)),'options':{**options,**extra},'frames':5});print(mode+' '+variant+' '+label+' passed',flush=True)
            need(pair[0]==pair[1],'byte-exact preceding editor capture '+mode+' '+variant)
    args=[EDITOR,'--control-test','--session',session,'--content-directory',CONTENT,'--start-frame','119'];log=execute(args,'clean-startup',cwd=foreign);need('gpu_readback_copies=0 capture_frames=0' in log and 'model_gpu_presentation_passed=1 frames=5' in log and not list(foreign.iterdir()),'clean cwd startup');commands.append({'args':list(map(str,args)),'frames':5,'clean':True})
    for group in (sources,protected,dependencies):
        for p,h in group.items():need(sha(p)==h,'final tested identity '+p)
    result={'status':'VERIFIED bounded CPU lighting-attribute transport and unchanged editor pixels; full Source lighting incomplete','time_utc':datetime.now(timezone.utc).isoformat(),'previous_goal_turn':'PROGRESS','parent':before['parent'],'source_sha256':sources,'protected_sha256':protected,'transitions':transitions,'cpu_receipt':{'receipt':str(cpu_receipt),'receipt_sha256':sha(cpu_receipt)},'indexed_gpu_receipt':gpu_ref,'complete_editor':{'path':str(EDITOR),'sha256':sha(EDITOR)},'editor_source_dependencies':dependencies,'editor_compiler_warnings':editor_warnings,'owned_shader_sha256':shaders,'editor_gpu_frames':sum(c['frames'] for c in commands),'captures':captures,'commands':commands,'canonical_preserved_sha256':previous['canonical_preserved_sha256'],'limitations':['No new lit shader, lightmap transport, renderer selector or Source parity claim.','Missing channels explicit; model normals/tangents and world normals are copies, no geometry arithmetic change.','Device loss/reset and arbitrary driver certification remain unverified.']}
    receipt=run/'results.json';put(receipt,result);put(OUT/'latest-verification.json',{'receipt':str(receipt),'receipt_sha256':sha(receipt)});print(json.dumps({'status':result['status'],'complete_editor':result['complete_editor'],'editor_gpu_frames':result['editor_gpu_frames']},indent=2),flush=True)

if __name__=='__main__':main()
