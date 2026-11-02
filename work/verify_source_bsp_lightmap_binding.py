"""Explicit parser transition, before/after geometry oracle and full editor build."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,math,os,re,shutil,struct,subprocess,tempfile
from verify_session_preview_aspect import VS,INCLUDE,CONTENT
from verify_texture_sampling_cross_api import fixture as owned_vtf

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/bsp-lightmap-binding-20261101'
EDITOR=ROOT/'outputs/sfm64-editor-lightmap-binding-x64-20261101/sfm64_session_ui.exe'

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,message):
    if not b:raise RuntimeError(message)
def put(p,value):Path(p).write_text(json.dumps(value,indent=2)+'\n')
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def pe(p):
    b=Path(p).read_bytes();at=struct.unpack_from('<I',b,60)[0]
    return struct.unpack_from('<H',b,at+4)[0],struct.unpack_from('<H',b,at+24)[0]

def check_bindings(b,path,model):
    """Independent source-edge/base-grid enumeration; no coordinate matching."""
    def u32(p):return struct.unpack_from('<I',b,p)[0]
    def i32(p):return struct.unpack_from('<i',b,p)[0]
    def i16(p):return struct.unpack_from('<h',b,p)[0]
    def vec(p):return list(struct.unpack_from('<3f',b,p))
    def lump(i):return u32(8+16*i),u32(12+16*i)
    mo,_=lump(14);first,count=struct.unpack_from('<2i',b,mo+model*48+40)
    hdr=not lump(8)[1] and lump(53)[1];fo,_=lump(58 if hdr and lump(58)[1] else 7)
    vo,_=lump(3);eo,_=lump(12);so,_=lump(13);ti,_=lump(6);dp,_=lump(26);tags,_=lump(48)
    vertices=[];triangles=[];base_differences=0
    for face in range(first,first+count):
        p=fo+56*face;edge_start=i32(p+4);edges=i16(p+8);info=i16(p+10);disp=i16(p+12)
        axes=[struct.unpack_from('<4f',b,ti+72*info+32+a*16) for a in range(2)]
        minima=struct.unpack_from('<2i',b,p+28)
        points=[]
        for n in range(edges):
            se=i32(so+4*(edge_start+n));v=struct.unpack_from('<H',b,eo+abs(se)*4+(2 if se<0 else 0))[0];points.append(vec(vo+v*12))
        if disp>=0:
            d=dp+176*disp;power=i32(d+20);side=(1<<power)+1;start=vec(d)
            corner=min(range(4),key=lambda n:sum((points[n][k]-start[k])**2 for k in range(3)))
            c0,c1,c2,c3=[points[(corner+n)%4] for n in range(4)]
            bases=[]
            for y in range(side):
                fy=f32(y/(side-1))
                left=[f32(c0[k]+f32(f32(c1[k]-c0[k])*fy)) for k in range(3)]
                right=[f32(c3[k]+f32(f32(c2[k]-c3[k])*fy)) for k in range(3)]
                for x in range(side):
                    fx=f32(x/(side-1));bases.append([f32(left[k]+f32(f32(right[k]-left[k])*fx)) for k in range(3)])
            tri_start=u32(d+16)
            triangles.extend([face for n in range(2*(side-1)**2) if not struct.unpack_from('<H',b,tags+2*(tri_start+n))[0]&32])
            base_differences+=len(bases)
        else:
            bases=points
            for n in range(1,len(points)-1):
                a,q,c=points[0],points[n],points[n+1]
                cross=[f32(f32(f32(q[1]-a[1])*f32(c[2]-a[2]))-f32(f32(q[2]-a[2])*f32(c[1]-a[1]))),
                       f32(f32(f32(q[2]-a[2])*f32(c[0]-a[0]))-f32(f32(q[0]-a[0])*f32(c[2]-a[2]))),
                       f32(f32(f32(q[0]-a[0])*f32(c[1]-a[1]))-f32(f32(q[1]-a[1])*f32(c[0]-a[0])))]
                length=f32(math.sqrt(f32(f32(f32(cross[0]*cross[0])+f32(cross[1]*cross[1]))+f32(cross[2]*cross[2]))))
                if length>=1e-8:triangles.append(face)
        for base in bases:
            luxels=[]
            for axis in range(2):
                value=float(axes[axis][3])-minima[axis]
                for k in range(3):value+=float(base[k])*axes[axis][k]
                luxels.append(value)
            vertices.append((face,*luxels))
    capture=Path(path).read_bytes();at=8;need(struct.unpack_from('<Q',capture)[0]==len(vertices),'independent emitted vertex count')
    for index,want in enumerate(vertices):
        actual=struct.unpack_from('<Idd',capture,at);at+=20
        need(actual==want,'independent raw luxel/source-face mismatch '+str(index)+': '+str((actual,want)))
    need(struct.unpack_from('<Q',capture,at)[0]==len(triangles),'independent emitted triangle count');at+=8
    need(struct.unpack_from('<'+'I'*len(triangles),capture,at)==tuple(triangles),'independent triangle face order');at+=4*len(triangles)
    need(at==len(capture),'complete binding capture')
    return {'vertices':len(vertices),'triangles':len(triangles),'faces':count,'displacement_base_vertices':base_differences,'raw_luxel_values_exact':True}

def main():
    before=json.loads((OUT/'before.json').read_text());parent=before['parent'];need(sha(parent['receipt'])==parent['receipt_sha256'],'parent checkpoint');prior=json.loads(Path(parent['receipt']).read_text())
    sources=dict(before['source_sha256']);protected=dict(before['protected_sha256']);protected[parent['receipt']]=parent['receipt_sha256'];transitions=[]
    for t in before['transitions']:
        need(sha(t['backup'])==t['before_sha256']==sources[t['path']],'explicit before bytes');t=dict(t,after_sha256=sha(t['path']));sources[t['path']]=t['after_sha256'];protected[t['backup']]=t['before_sha256'];transitions.append(t)
    for name in ('source_bsp_lightmap_binding_candidate.h','test_source_bsp_lightmap_binding.cpp',Path(__file__).name):
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
    for name in ('test_source_bsp_lightmap_binding','test_source_bsp_lightmap_data','test_source_bsp_inline_brush','test_source_bsp_world_materials','test_scene_shading_attributes','test_mtt_soldier_scene_render_preparation','test_editor_transaction'):
        register(ROOT/'work'/(name+'.cpp'))
    for group in (sources,protected):
        for p,h in group.items():need(sha(p)==h,'initial frozen identity '+p)
    run=Path(tempfile.mkdtemp(prefix='r-',dir=OUT));print(str(run),flush=True)
    clean={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')}
    def execute(args,label,cwd=ROOT,env=None):
        p=subprocess.run(list(map(str,args)),cwd=cwd,env=env or clean,capture_output=True,text=True,timeout=600,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-2200:]);return p.stdout+p.stderr
    identity=json.loads((ROOT/'outputs/bsp-lightmap-contract-20261101/benchmark-input-identity.json').read_text());map_path=Path(identity['path']);need(sha(map_path)==identity['sha256'],'read-only benchmark map')
    names=['test_source_bsp_lightmap_data','test_source_bsp_inline_brush','test_source_bsp_world_materials','test_scene_shading_attributes','test_mtt_soldier_scene_render_preparation','test_editor_transaction']
    builds=[('geometry-before','test_source_bsp_lightmap_binding',' /DSFM64_BSP_BINDING_BASELINE'),('geometry-current','test_source_bsp_lightmap_binding','')]+[(n,n,'') for n in names]
    lines=['@echo off',f'cd /d "{ROOT}"',f'call "{VS}" >nul','if errorlevel 1 exit /b 1']
    for label,source,extra in builds:
        lines+=[f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /I"{INCLUDE}" /I"{OUT}" /I"{ROOT/"work"}"{extra} work\\{source}.cpp /Fo:"{run/(label+".obj")}" /Fe:"{run/(label+".exe")}" >"{run/(label+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1']
    batch=run/'build-cpu.cmd';batch.write_text('\n'.join(lines+['exit /b 0','']));execute(['cmd.exe','/d','/c',batch],'build-cpu-driver')
    warnings={};executables={}
    for label,_,_ in builds:
        e=run/(label+'.exe');need(pe(e)==(0x8664,0x20b),'native CPU test');executables[label]={'path':str(e),'sha256':sha(e)};warnings[label]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(label+'-build.log')).read_text())
    print('Freshly compiled8CPU executables',flush=True)
    logs={}
    logs['geometry-before']=execute([run/'geometry-before.exe',run/'before-geometry',map_path],'geometry-before')
    for repeat in range(2):logs['geometry-current-'+str(repeat)]=execute([run/'geometry-current.exe',run/('current-geometry-'+str(repeat)),map_path],'geometry-current-'+str(repeat))
    comparisons={}
    for base,other in [('multi.geometry','multi-bound.geometry'),('displaced.geometry','displaced-bound.geometry'),('benchmark-world.geometry','benchmark-world-bound.geometry'),('benchmark-inline.geometry','benchmark-inline-bound.geometry')]:
        h=sha(run/'before-geometry'/base);need(all(sha(run/('current-geometry-'+str(n))/base)==h==sha(run/('current-geometry-'+str(n))/other) for n in range(2)),'byte-exact before/default/bound geometry '+base);comparisons[base]=h
    need(sha(run/'current-geometry-0/multi-after-failure.geometry')==comparisons['multi.geometry'],'failed combined geometry preserved')
    binding_metrics={}
    for repeat in range(2):
        target=run/('current-geometry-'+str(repeat))
        for label,path,model in [('multi',target/'multi.bsp',1),('displaced',target/'displaced.bsp',0),('benchmark-world',map_path,0),('benchmark-inline',map_path,16)]:
            metrics=check_bindings(path.read_bytes(),target/(label+'.bindings'),model)
            if repeat:need(metrics==binding_metrics[label],'repeatable binding metrics')
            else:binding_metrics[label]=metrics
    print('Before/default/bound geometry byte-exact; independent vertex/triangle bindings exact',flush=True)
    session_source=ROOT/'outputs/d3d9-prepared-scene-20261101/packet-gpu-zhsn3_pu/benchmark-input-copy.dmx';need(sha(session_source)==protected[str(session_source)],'preserved benchmark session copy');session=run/'input-copy.dmx';shutil.copyfile(session_source,session)
    data=run/'cpu-data';(data/'materials/test').mkdir(parents=True);(data/'materials/test/pixel.vtf').write_bytes(owned_vtf(0,0))
    args_by_name={'test_source_bsp_lightmap_data':[run/'raw-data',map_path],'test_source_bsp_inline_brush':[map_path],'test_source_bsp_world_materials':[map_path,CONTENT/'gameinfo.txt'],'test_scene_shading_attributes':[data,session,CONTENT],'test_mtt_soldier_scene_render_preparation':[session,CONTENT/'gameinfo.txt'],'test_editor_transaction':[]}
    for name in names:
        logs[name]=execute([run/(name+'.exe'),*args_by_name[name]],name);print(name+' passed',flush=True)
    cpu={'status':'VERIFIED CPU emitted face/luxel bindings and before/default/bound geometry','executables':executables,'compiler_warnings':warnings,'logs':logs,'binding_metrics':binding_metrics,'geometry_sha256':comparisons}
    put(run/'cpu-results.json',cpu)
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
    for p in before['complete_editor_dependencies_sha256']:
        if p not in dependencies:dependencies[p]=sha(p)  # Explicit unchanged shader/build inputs.
    need(set(dependencies)==set(before['complete_editor_dependencies_sha256']),'complete editor dependency set unchanged')
    affected=[];closures={}
    for name,old in prior['unaffected_cpu_test_closures'].items():
        current={p:sha(p) for p in old};closures[name]=current
        if current!=old:affected.append(name)
    need(set(affected)=={'test_scene_shading_attributes','test_mtt_soldier_scene_render_preparation','test_editor_transaction'},'only three affected prior CPU closures')
    for group in (sources,protected,dependencies):
        for p,h in group.items():need(sha(p)==h,'final frozen identity '+p)
    need(sha(map_path)==identity['sha256'],'original benchmark map unchanged')
    result={'status':'VERIFIED opt-in per-brush CPU lightmap bindings and unchanged complete-editor pixels; retained scene/GPU lightmapping incomplete','time_utc':datetime.now(timezone.utc).isoformat(),'previous_goal_turn':'PROGRESS','parent':parent,'transitions':transitions,'source_sha256':sources,'protected_sha256':protected,'cpu_receipt':{'receipt':str(run/'cpu-results.json'),'receipt_sha256':sha(run/'cpu-results.json')},'binding_metrics':binding_metrics,'complete_editor':{'path':str(EDITOR),'sha256':sha(EDITOR)},'complete_editor_dependencies_sha256':dependencies,'current_cpu_test_closures':closures,'affected_cpu_tests_freshly_run':affected,'unchanged_cpu_tests_not_rerun':[n for n in closures if n not in affected],'editor_compiler_warnings':editor_warnings,'owned_shader_sha256':shaders,'editor_gpu_frames':sum(c['frames'] for c in commands),'captures':captures,'commands':commands,'canonical_preserved_sha256':prior['canonical_preserved_sha256'],'benchmark_read_only':identity,'limits':['Opt-in per-brush result only; retained scene/GPU material paths not wired.','Wrapper follows existing geometry LegacyPreview dataset selection, preserving the unpaired-fallback flag. Native HDR geometry selection when both data sets exist is future work.','Original Source lighting/color/shader equivalence, device loss/reset and arbitrary driver certification not claimed.'],'full_goal':'active/incomplete; independent reconstruction'}
    receipt=run/'results.json';put(receipt,result);put(OUT/'latest-verification.json',{'receipt':str(receipt),'receipt_sha256':sha(receipt)});print(json.dumps({'status':result['status'],'complete_editor':result['complete_editor'],'editor_gpu_frames':result['editor_gpu_frames'],'binding_metrics':binding_metrics},indent=2),flush=True)

if __name__=='__main__':main()
