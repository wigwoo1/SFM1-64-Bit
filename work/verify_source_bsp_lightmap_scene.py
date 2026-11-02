"""Immutable retained world metadata, transactional publication and full editor checks."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,math,os,re,shutil,struct,subprocess,tempfile
from verify_session_preview_aspect import VS,INCLUDE,CONTENT
from verify_texture_sampling_cross_api import fixture as owned_vtf

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/bsp-lightmap-scene-20261101'
EDITOR=ROOT/'outputs/sfm64-editor-lightmap-scene-generated-guard-x64-20261101/sfm64_session_ui.exe'

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,message):
    if not b:raise RuntimeError(message)
def put(p,value):Path(p).write_text(json.dumps(value,indent=2)+'\n')
from verify_source_bsp_lightmap_binding import check_bindings

def pe(p):
    b=Path(p).read_bytes();at=struct.unpack_from('<I',b,60)[0]
    return struct.unpack_from('<H',b,at+4)[0],struct.unpack_from('<H',b,at+24)[0]


def main():
    before=json.loads((OUT/'before.json').read_text());parent=before['parent'];need(sha(parent['receipt'])==parent['receipt_sha256'],'parent checkpoint');prior=json.loads(Path(parent['receipt']).read_text())
    sources=dict(before['source_sha256']);protected=dict(before['protected_sha256']);protected[parent['receipt']]=parent['receipt_sha256'];transitions=[]
    for t in before['transitions']:
        need(sha(t['backup'])==t['before_sha256']==sources[t['path']],'explicit before bytes');t=dict(t,after_sha256=sha(t['path']));sources[t['path']]=t['after_sha256'];protected[t['backup']]=t['before_sha256'];transitions.append(t)
    for name in ('source_bsp_lightmap_snapshot_candidate.h','scene_bsp_lightmap_candidate.h','test_source_bsp_lightmap_scene.cpp',Path(__file__).name):
        p=ROOT/'work'/name;need(str(p) not in sources,'new work files');sources[str(p)]=sha(p)
    # Register every actual CPU entry point and quote-include dependency before
    # compilation, including existing tests not tracked by the parent snapshot.
    seen=set()
    def register(p):
        p=p.resolve()
        if p in seen:return
        seen.add(p);key=str(p)
        if key in protected:need(sha(p)==protected[key],'protected original-parser snapshot')
        elif key in sources:need(sha(p)==sources[key],'CPU source before compilation')
        else:sources[key]=sha(p)
        for name in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            for target in (p.parent/name,ROOT/'work'/name,OUT/name):
                if target.is_file():register(target);break
    for name in ('test_source_bsp_lightmap_scene','test_source_bsp_lightmap_binding','test_source_bsp_lightmap_data','test_source_bsp_inline_brush','test_source_bsp_world_materials','test_scene_shading_attributes','test_fur_scene_upload','test_camera_frustum_scene_scope','test_mtt_soldier_scene_render_preparation','test_editor_transaction'):
        register(ROOT/'work'/(name+'.cpp'))
    for group in (sources,protected):
        for p,h in group.items():need(sha(p)==h,'initial frozen identity '+p)
    run=Path(tempfile.mkdtemp(prefix='r-',dir=OUT));print(str(run),flush=True)
    clean={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')}
    def execute(args,label,cwd=ROOT,env=None):
        p=subprocess.run(list(map(str,args)),cwd=cwd,env=env or clean,capture_output=True,text=True,timeout=600,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-2200:]);return p.stdout+p.stderr
    identity=json.loads((ROOT/'outputs/bsp-lightmap-contract-20261101/benchmark-input-identity.json').read_text());map_path=Path(identity['path']);need(sha(map_path)==identity['sha256'],'read-only benchmark map')
    names=['test_source_bsp_lightmap_binding','test_source_bsp_lightmap_data','test_source_bsp_inline_brush','test_source_bsp_world_materials','test_scene_shading_attributes','test_fur_scene_upload','test_camera_frustum_scene_scope','test_mtt_soldier_scene_render_preparation','test_editor_transaction']
    builds=[('world-before','test_source_bsp_lightmap_scene',' /DSFM64_SCENE_LIGHTMAP_BASELINE'),('world-current','test_source_bsp_lightmap_scene','')]+[(n,n,'') for n in names]
    lines=['@echo off',f'cd /d "{ROOT}"',f'call "{VS}" >nul','if errorlevel 1 exit /b 1']
    for label,source,extra in builds:
        lines+=[f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /I"{INCLUDE}" /I"{OUT}" /I"{ROOT/"work"}"{extra} work\\{source}.cpp /Fo:"{run/(label+".obj")}" /Fe:"{run/(label+".exe")}" >"{run/(label+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1']
    batch=run/'build-cpu.cmd';batch.write_text('\n'.join(lines+['exit /b 0','']));execute(['cmd.exe','/d','/c',batch],'build-cpu-driver')
    warnings={};executables={}
    for label,_,_ in builds:
        e=run/(label+'.exe');need(pe(e)==(0x8664,0x20b),'native CPU test');executables[label]={'path':str(e),'sha256':sha(e)};warnings[label]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(label+'-build.log')).read_text())
    print('Freshly compiled11CPU executables',flush=True)
    logs={}
    (run/'owned.vtf').write_bytes(owned_vtf(0,0))
    world_args=[map_path,CONTENT,'mtt_soldier',run/'owned.vtf']
    logs['world-before']=execute([run/'world-before.exe',run/'before-world',*world_args],'world-before')
    for repeat in range(2):logs['world-current-'+str(repeat)]=execute([run/'world-current.exe',run/('current-world-'+str(repeat)),*world_args],'world-current-'+str(repeat))
    comparisons={}
    for name in ('owned.scene','partial.scene','benchmark.scene'):
        h=sha(run/'before-world'/name);need(all(sha(run/('current-world-'+str(n))/name)==h for n in range(2)),'byte-exact before/current scene '+name);comparisons[name]=h
    binding_metrics={}
    for repeat in range(2):
        target=run/('current-world-'+str(repeat))
        metrics=check_bindings(map_path.read_bytes(),target/'benchmark-source.bindings',0)
        capture=(target/'benchmark.snapshot').read_bytes();vertices,indices=struct.unpack_from('<IQ',capture);at=12
        local=struct.unpack_from('<'+'I'*indices,capture,at);at+=4*indices
        need(struct.unpack_from('<Q',capture,at)[0]==vertices,'snapshot vertex metadata coverage');at+=8
        records=[]
        for _ in range(vertices):records.append(struct.unpack_from('<Idd',capture,at));at+=20
        triangles,=struct.unpack_from('<Q',capture,at);at+=8;faces=struct.unpack_from('<'+'I'*triangles,capture,at);at+=4*triangles
        need(at==len(capture) and triangles*3==indices,'complete immutable snapshot')
        source=(target/'benchmark-source.bindings').read_bytes();need(struct.unpack_from('<Q',source)[0]==vertices,'source vertex count')
        for n,record in enumerate(records):need(record==struct.unpack_from('<Idd',source,8+n*20),'retained source/luxel values unchanged')
        for n,face in enumerate(faces):need(all(records[local[n*3+k]][0]==face for k in range(3)),'regrouped triangle face identity')
        legacy=(run/'before-world/benchmark.scene').read_bytes();count,=struct.unpack_from('<Q',legacy,64);start=72+count*36;total,=struct.unpack_from('<Q',legacy,start);all_indices=struct.unpack_from('<'+'I'*total,legacy,start+8)
        need(tuple(index-3 for index in all_indices[3:])==local,'exact baseline material-regrouped local indices')
        metrics.update({'ranges':59,'lighting_bytes':6026768,'snapshot_indices':indices,'retained_vertex_values_exact':True,'material_regrouped_faces_exact':True})
        if repeat:need(metrics==binding_metrics['benchmark-world'],'repeated retained metadata')
        else:binding_metrics['benchmark-world']=metrics
    print('Before/current scene bytes exact; retained metadata and regrouped indices exact',flush=True)
    session_source=ROOT/'outputs/d3d9-prepared-scene-20261101/packet-gpu-zhsn3_pu/benchmark-input-copy.dmx';need(sha(session_source)==protected[str(session_source)],'preserved benchmark session copy');session=run/'input-copy.dmx';shutil.copyfile(session_source,session)
    data=run/'cpu-data';(data/'materials/test').mkdir(parents=True);(data/'materials/test/pixel.vtf').write_bytes(owned_vtf(0,0))
    args_by_name={'test_source_bsp_lightmap_binding':[run/'binding-data',map_path],'test_fur_scene_upload':[],'test_camera_frustum_scene_scope':[],'test_source_bsp_lightmap_data':[run/'raw-data',map_path],'test_source_bsp_inline_brush':[map_path],'test_source_bsp_world_materials':[map_path,CONTENT/'gameinfo.txt'],'test_scene_shading_attributes':[data,session,CONTENT],'test_mtt_soldier_scene_render_preparation':[session,CONTENT/'gameinfo.txt'],'test_editor_transaction':[]}
    for name in names:
        logs[name]=execute([run/(name+'.exe'),*args_by_name[name]],name);print(name+' passed',flush=True)
    cpu={'status':'VERIFIED shared retained world lightmap snapshots, transactional cache/packet publication and legacy scenes','executables':executables,'compiler_warnings':warnings,'logs':logs,'binding_metrics':binding_metrics,'geometry_sha256':comparisons}
    put(run/'cpu-results.json',cpu)
    # SceneGpuUpload changed, so refresh the existing native indexed admission/
    # pixel regression under an explicit current profile; its reference lookup
    # still uses the original owned bitness checkpoint, not a rewritten receipt.
    import verify_d3d9_prepared_scene as gpu
    old_indexed=json.loads(Path(json.loads(Path(ROOT/'outputs/d3d9-shading-consumption-20261101/r-cs8t2nhu/results.json').read_text())['default_gpu_receipt']['receipt']).read_text())
    gpu_dir=run/'indexed-gpu';gpu_dir.mkdir();put(gpu_dir/'before.json',{'parent':old_indexed['parent'],'actual_parent':parent,'source_sha256':sources,'protected_sha256':protected,'explicit_source_transitions':transitions})
    gpu.OUT=gpu_dir;gpu.main();gpu_ref=json.loads((gpu_dir/'latest-verification.json').read_text());need(sha(gpu_ref['receipt'])==gpu_ref['receipt_sha256'],'fresh indexed receipt');gd=json.loads(Path(gpu_ref['receipt']).read_text());need(gd['cases']==730 and all(m['over_2e_6']==0 for m in gd['metrics'].values()),'indexed730 pixel gates')
    need(not EDITOR.parent.exists(),'fresh complete editor output');execute(['cmd.exe','/d','/c',ROOT/'work/build_sfm64_editor_x64.cmd',EDITOR.parent],'complete-editor-build');need(pe(EDITOR)==(0x8664,0x20b),'complete editor AMD64')
    baseline=Path(prior['complete_editor_preserved']['path']);need(sha(baseline)==prior['complete_editor_preserved']['sha256'],'preserved isolated editor')
    editor_warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(EDITOR.parent/'build.log').read_text());old_warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(baseline.parent/'build.log').read_text());need(editor_warnings==old_warnings and len(editor_warnings)==15,'same15 inherited editor warnings');need(not (EDITOR.parent/'shader-build.log').read_text().strip(),'no shader diagnostics')
    shaders={str(p.relative_to(EDITOR.parent)):sha(p) for p in (EDITOR.parent/'shaders').glob('*.spv')};need(len(shaders)==15 and all(sha(baseline.parent/n)==h for n,h in shaders.items()),'15 unchanged owned shaders');print('Complete editor built with15unchanged shaders and inherited warnings',flush=True)
    # Same real-GPU controls as the previous complete build. Never launch or
    # automate original SFM; these are owned editor --control-test processes.
    modes=[('default',{}),('two-texture',{'SFM64_DIAGNOSTIC_TWO_TEXTURE_TEST':'1'}),('selfillum-red',{'SFM64_DIAGNOSTIC_SELFILLUM_TEST':'1','SFM64_DIAGNOSTIC_MASK_UPLOAD_TEST':'1','SFM64_DIAGNOSTIC_SELFILLUM_FIXTURE':'red'}),('selfillum-alpha-half',{'SFM64_DIAGNOSTIC_SELFILLUM_TEST':'1','SFM64_DIAGNOSTIC_MASK_UPLOAD_TEST':'1','SFM64_DIAGNOSTIC_SELFILLUM_FIXTURE':'alpha-half'}),('selfillum-color',{'SFM64_DIAGNOSTIC_SELFILLUM_TEST':'1','SFM64_DIAGNOSTIC_MASK_UPLOAD_TEST':'1','SFM64_DIAGNOSTIC_SELFILLUM_FIXTURE':'red-half','SFM64_DIAGNOSTIC_MATERIAL_COLOR_TEST':'1','SFM64_DIAGNOSTIC_FIXTURE_GAMMA_READ':'mixed'})]
    foreign=run/'empty-cwd';foreign.mkdir();commands=[];captures={}
    for mode,options in modes:
        variants=[('default',{})]
        if mode in ('default','two-texture'):variants += [('bc-disabled',{'SFM64_DIAGNOSTIC_DISABLE_BC_TEXTURES':'1'}),('anisotropy',{'SFM64_DIAGNOSTIC_ANISOTROPIC_TEXTURE_TEST':'1'})]
        for variant,extra in variants:
            pair=[]
            for label,exe in [('baseline',baseline),('candidate',EDITOR)]:
                target=run/(mode+'-'+variant+'-'+label);target.mkdir();env=dict(clean,**options,**extra,SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(target));args=[exe,'--control-test','--session',session,'--content-directory',CONTENT,'--start-frame','119']
                log=execute(args,mode+'-'+variant+'-'+label,cwd=foreign,env=env);need('model_gpu_presentation_passed=1 frames=5' in log and 'gpu_readback_copies=5 capture_frames=1' in log,'actual editor frames');frames={str(n):sha(target/f'vulkan-retained-frame-{n}.rgba') for n in (0,119,120)};pair.append(frames);captures[mode+'-'+variant+'-'+label]=frames;commands.append({'args':list(map(str,args)),'options':{**options,**extra},'frames':5});print(mode+' '+variant+' '+label+' passed',flush=True)
            need(pair[0]==pair[1],'unchanged editor pixels '+mode+' '+variant)
    args=[EDITOR,'--control-test','--session',session,'--content-directory',CONTENT,'--start-frame','119'];log=execute(args,'clean-startup',cwd=foreign);need('gpu_readback_copies=0 capture_frames=0' in log and 'model_gpu_presentation_passed=1 frames=5' in log and not list(foreign.iterdir()),'clean startup');commands.append({'args':list(map(str,args)),'frames':5,'clean':True})
    dependencies={}
    def visit(p):
        p=p.resolve()
        if str(p) in dependencies:return
        dependencies[str(p)]=sha(p)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            if (p.parent/inc).is_file():visit(p.parent/inc)
    visit(ROOT/'work/probe_vulkan_win32_surface_x64.cpp')
    # Reconcile actual C++ closure plus the separately tracked shader/build inputs.
    old_editor=before['complete_editor_dependencies_sha256']
    for p in old_editor:
        if p not in dependencies:dependencies[p]=sha(p)
    expected_new={str(ROOT/'work'/n) for n in ('scene_bsp_lightmap_candidate.h','source_bsp_lightmap_snapshot_candidate.h','source_bsp_lightmap_binding_candidate.h','source_bsp_lightmap_data_candidate.h')}
    need(set(dependencies)-set(old_editor)==expected_new and len(dependencies)==370,'four actual new complete-editor headers')
    affected=[];closures={}
    for name,old in prior['cpu_test_closures'].items():
        current={}
        def scan_test(p):
            p=p.resolve()
            if str(p) in current:return
            current[str(p)]=sha(p)
            for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
                if (p.parent/inc).is_file():scan_test(p.parent/inc)
        scan_test(ROOT/'work'/(name+'.cpp'));closures[name]=current
        if current!=old:affected.append(name)
    need(set(affected)=={'test_scene_shading_attributes','test_fur_scene_upload','test_camera_frustum_scene_scope','test_mtt_soldier_scene_render_preparation','test_editor_transaction'},'five affected prior CPU closures')
    for group in (sources,protected,dependencies):
        for p,h in group.items():need(sha(p)==h,'final frozen identity '+p)
    need(sha(map_path)==identity['sha256'],'original benchmark map unchanged')
    result={'status':'VERIFIED retained shared CPU world lightmap snapshots and unchanged complete-editor pixels; GPU lightmapping incomplete','time_utc':datetime.now(timezone.utc).isoformat(),'previous_goal_turn':'PROGRESS','parent':parent,'transitions':transitions,'source_sha256':sources,'protected_sha256':protected,'cpu_receipt':{'receipt':str(run/'cpu-results.json'),'receipt_sha256':sha(run/'cpu-results.json')},'binding_metrics':binding_metrics,'indexed_gpu_receipt':gpu_ref,'complete_editor':{'path':str(EDITOR),'sha256':sha(EDITOR)},'complete_editor_dependencies_sha256':dependencies,'current_cpu_test_closures':closures,'affected_cpu_tests_freshly_run':affected,'unchanged_cpu_tests_not_rerun':[n for n in closures if n not in affected],'editor_compiler_warnings':editor_warnings,'owned_shader_sha256':shaders,'editor_gpu_frames':sum(c['frames'] for c in commands),'captures':captures,'commands':commands,'canonical_preserved_sha256':prior['canonical_preserved_sha256'],'benchmark_read_only':identity,'limits':['World CPU metadata retained; inline brush scene transport and GPU material/lightmap paths not wired.','Wrapper follows existing geometry LegacyPreview dataset selection, preserving the unpaired-fallback flag. Native HDR geometry selection when both data sets exist is future work.','Original Source lighting/color/shader equivalence, device loss/reset and arbitrary driver certification not claimed.'],'full_goal':'active/incomplete; independent reconstruction'}
    receipt=run/'results.json';put(receipt,result);put(OUT/'latest-verification.json',{'receipt':str(receipt),'receipt_sha256':sha(receipt)});print(json.dumps({'status':result['status'],'complete_editor':result['complete_editor'],'editor_gpu_frames':result['editor_gpu_frames'],'binding_metrics':binding_metrics},indent=2),flush=True)

if __name__=='__main__':main()
