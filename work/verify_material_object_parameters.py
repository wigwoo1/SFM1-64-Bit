"""Normally constructed original material wrapper62 versus owned x64 objects.

All files are disposable workspace fixtures. No live SFM, GPU or original writes.
"""
from pathlib import Path
from datetime import datetime,timezone
import difflib,hashlib,json,re,shutil,struct,subprocess,tempfile,time,zlib
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_material_keyvalues_parser import DUMP
from verify_material_parameter_orchestration import node,string,integer,packet
from verify_material_patch_files import text
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
from pe_image import Image

OUT=ROOT/'outputs/material-object-parameters-20261026'
CHANGED=['material_object_store_candidate.h','probe_material_store.cpp']
NEW=['material_object_parameter_initialization_candidate.h','material_object_parameters_native_fixture.h',
     'probe_material_object_parameters.cpp','probe_material_object_parameters_discovery.cpp',
     'build_material_object_parameters.cmd','verify_material_object_parameters.py']


def corpus():
    cases=[];files={}
    def add(label,route,root,env=None,name=None,repeat=1,uncache=1,queued=1,alpha=0x3ec00000,shader=None):
        name=name or 'object/case'+str(len(cases));env=env or [0,0,0,0,0,0,0,0,1,0,92,0]
        cases.append(dict(label=label,route=route,root=root,env=env,name=name,repeat=repeat,uncache=uncache,queued=queued,alpha=alpha,shader=shader))
        if route==2:files['materials/'+name+'.vmt']=render(root)
    for route in range(3):
        for shader in ['UnlitGeneric','VertexLitGeneric','Wireframe_DX9','Wireframe','Missing_Owned_Fixture_Shader']:
            for variant in range(8):
                root=node(shader,[string('$alpha','.375'),string('$color','[.25 .5 1]'),string('$color2','[.5 1 .25]'),string('$srgbtint','[.5 .25 1]')])
                env=[variant%3,variant&1,(variant>>1)&1,0,0,0,0,0,1,variant&1,[90,92,95][variant%3],variant&1]
                add('direct-'+shader,route,root,env,repeat=variant%2,uncache=(variant>>1)&1,queued=(variant>>2)&1,shader='Wireframe_DX9' if shader in ['Wireframe','Missing_Owned_Fixture_Shader'] else shader)
    files['materials/target.vmt']='VertexLitGeneric { $alpha .5 $color "[.5 .25 1]" }'
    files['materials/fallback_base.vmt']='UnlitGeneric { $alpha .125 }'
    files['materials/fallback_patch.vmt']='patch { include materials/fallback_base.vmt insert { $alpha .625 } }'
    files['materials/chain.vmt']='Wireframe { $fallbackmaterial target }'
    for route in range(3):
        for request,alpha,shader in [('target',0x3f000000,'VertexLitGeneric'),('fallback_patch',0x3f200000,'UnlitGeneric'),('chain',0x3f000000,'VertexLitGeneric'),('missing_fallback',0x3f800000,'UnlitGeneric')]:
            add('fallback-'+request,route,node('Wireframe',[string('$fallbackmaterial',request)]),alpha=alpha,shader=shader)
        owner='object/self'+str(route);add('fallback-self',route,node('Wireframe',[string('$fallbackmaterial',owner)]),name=owner,alpha=0x3f800000,shader='UnlitGeneric')
    # A supplied procedural source uses the original no-materials-prefix flag.
    files['target.vmt']='UnlitGeneric { $alpha .25 }';add('procedural-fallback',0,node('Wireframe',[string('$fallbackmaterial','target')]),name='{procedural}',alpha=0x3e800000,shader='UnlitGeneric')
    files['materials/initial_base.vmt']='UnlitGeneric { $alpha .125 }'
    add('initial-patch',2,node('patch',[string('include','materials/initial_base.vmt'),node('insert',[string('$alpha','.75')])]),name='object/initial_patch',alpha=0x3f400000,shader='UnlitGeneric')
    add('missing-root',2,node('UnlitGeneric'),name='object/not_present',repeat=0,uncache=0,alpha=None);del files['materials/object/not_present.vmt']
    return cases,files


def render(n):
    quote=lambda s:'"'+s.replace('\\','\\\\').replace('"','\\"')+'"'
    return quote(n['key'])+' { '+' '.join(render(c) if c['kind']==0 else quote(c['key'])+' '+quote(c['text'] if c['kind']==1 else str(c['bits'])) for c in n['children'])+' }'


def decode(raw):
    at=0
    def u(fmt='<I'):
        nonlocal at
        require(at+struct.calcsize(fmt)<=len(raw),'Output packet extent');v=struct.unpack_from(fmt,raw,at)[0];at+=struct.calcsize(fmt);return v
    def s():
        nonlocal at
        n=u();require(n<100000 and at+n<=len(raw),'Output text extent');v=raw[at:at+n].decode('ascii');at+=n;return v
    def tree(depth=0):
        require(depth<16,'Output tree depth');n=dict(key=s(),kind=u(),bits=u(),text=s(),pointer=u('<Q'));n['children']=[tree(depth+1) for _ in range(u())];return n
    count=u();records=[]
    while at<len(raw):
        q=[u() for _ in range(17)];r=dict(case=q[0],stage=q[1],result=q[2],alive=q[3],source=q[4],source_freed=q[5],refs=q[6],flags=q[7],count=q[8],forward_valid=q[9],forward=q[10:17]);r['warnings']=u();r['developer']=u();r['lookups']=[s() for _ in range(u())];r['queued']=u();r['stored_root']=tree() if u() else None;r['external']=tree();r['patch']=tree();r['shader']=s();r['values']=[]
        for _ in range(u()):
            words=[u() for _ in range(28)]
            def chars(n):
                nonlocal at
                require(at+n<=len(raw),'Value extent');v=raw[at:at+n].decode('ascii');at+=n;return v
            value=chars(words[10]);name=chars(words[11]);r['values'].append(dict(words=words,text=value,name=name))
        records.append(r)
    require(at==len(raw),'Complete output');return count,records


def independent(cases,records):
    checks=0
    def check(b,msg):
        nonlocal checks
        require(b,msg);checks+=1
    for index,c in enumerate(cases):
        rows=[r for r in records if r['case']==index];stages=[0,1]
        if c['alpha'] is not None and c['repeat']:stages.append(2)
        if c['alpha'] is not None and c['uncache']:stages.extend([3,4,5])
        stages.append(6)
        if c['queued']:stages.append(7)
        check([r['stage'] for r in rows]==stages,'Exact lifecycle phases '+str(index));by={r['stage']:r for r in rows};initial=by[0];first=by[1]
        source_flags=(16 if c['route']==0 else 0)|(32 if c['name'].startswith('{') else 0)
        check(initial['flags']==source_flags and initial['refs']==1 and initial['count']==0 and not initial['forward_valid'] and not initial['source_freed'],'Fresh actual constructor state')
        check(all(r['source']==int(c['route']==0) for r in rows),'Only constructor-supplied root belongs to material')
        if c['alpha'] is None:
            check(not first['result'] and first['flags']==0 and first['count']==0 and first['warnings']==1 and not first['forward_valid'],'Missing filename safely fails before initialization flag')
        else:
            expected_count={'UnlitGeneric':68,'VertexLitGeneric':125,'Wireframe_DX9':15}[c['shader']]
            # Builtin error declaration has one undeclared retained variable,
            # $decalscale, beyond the registered68 UnlitGeneric descriptors.
            builtin=c['label'] in ['fallback-missing_fallback','fallback-self']
            if builtin:expected_count+=1
            check(first['result'] and first['flags']==(source_flags|4) and first['count']==expected_count and first['shader']==c['shader'],'Actual wrapper publishes shader/schema/flag')
            if builtin:check(first['values'][-1]['name']=='$decalscale','Unknown builtin variable survives schema completion')
            check(first['values'][5]['words'][3]==c['alpha'] and first['forward'][0]==c['alpha'],'Independent alpha and forwarding snapshot')
            for r in rows:
                if r['values']:
                    check(r['count']==len(r['values']) and r['forward_valid'],'Complete typed variable publication')
                    check([v['name'] for v in r['values'][:6]]==['$flags','$flags_defined','$flags2','$flags_defined2','$color','$alpha'],'Common parameter identity/order')
            if 'direct-' in c['label']:
                check(first['forward'][1:4]==[0x3e800000,0x3f000000,0x3f800000],'Color getter projection')
                check(first['forward'][4:]==([0x3d800000,0x3e000000,0x3e800000] if c['env'][2] else [0x3e000000,0x3f000000,0x3e800000]),'Tint multiplication and conditional sRGB tint')
            if c['repeat']:
                repeated=by[2];check(repeated['result'] and repeated['values']==first['values'] and repeated['forward']==first['forward'] and repeated['lookups']==first['lookups'],'Initialized guard ignores subsequent source mutation')
            if c['uncache']:
                keep=by[3];base=by[2] if c['repeat'] else first;check(keep['values']==base['values'] and keep['flags']==base['flags'] and keep['forward']==base['forward'],'Preserve-variables uncache is inert for parameters')
                clear=by[4];check(clear['count']==0 and not clear['values'] and clear['flags']==source_flags and clear['forward']==base['forward'] and clear['forward_valid'],'Uncache releases variables but keeps source and cached forward values')
                again=by[5];check(again['result'] and again['count']==expected_count and again['values'][5]['words'][3]==(0x3e000000 if c['repeat'] and c['route']!=2 else c['alpha']),'Reinitialization reads current source')
        last=rows[-1];check(not last['alive'] and not last['values'] and last['count']==0 and not last['queued'] and last['source_freed']==int(c['route']==0),'Complete actual object/parameter/source lifetime')
        if c['queued']:check(by[6]['alive'] and by[6]['refs']==1 and by[6]['queued']==2 and not by[6]['source_freed'],'Deferred references/source remain until FIFO drain')
        else:check(not by[6]['alive'],'Null queue teardown applies immediately')
        if c['route']==1:check(last['external'] is not None,'Borrowed external root remains valid after object destruction')
        sentinel=lambda r:any(n['key']=='sentinel' for n in r['patch']['children'])
        changed_patch=c['route']==1 and c['label'] in ['fallback-target','fallback-fallback_patch','fallback-chain']
        check(sentinel(initial) and sentinel(first)==(not changed_patch),'Borrowed accumulator clears only on successful fallback file load')
    return checks


def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    ptr=json.loads((OUT/'preflight.json').read_text())['prior'];receipt=Path(ptr['receipt']);require(sha(receipt)==ptr['receipt_sha256'],'Prior store receipt');prior=json.loads(receipt.read_text());changes={}
    allowed={str(ROOT/'work'/n) for n in CHANGED}
    for p,h in prior['sources'].items():
        if p in allowed:
            before=OUT/'before'/Path(p).relative_to(ROOT);require(sha(before)==h,'Frozen preceding source');changes[p]=dict(before=h,after=sha(Path(p)),diff=''.join(difflib.unified_diff(before.read_text().splitlines(True),Path(p).read_text().splitlines(True),fromfile='before',tofile='after')))
        else:require(sha(Path(p))==h,'Unrelated source '+p)
    require(set(changes)==allowed,'Exact accepted edit set');(run/'source-changes.json').write_text(json.dumps(changes,indent=2))
    for key in ['original_hashes','private_library_hashes']:
        for p,h in prior[key].items():require(sha(Path(p))==h,'Protected original '+p)
    previous=Path(ptr['replay']);require(sha(previous/'manifest.json')==ptr['replay_manifest_sha256'] and all(sha(previous/k)==h for k,h in json.loads((previous/'manifest.json').read_text()).items()),'Prior frozen replay')
    require(sha(LIB)==sha(PRIVATE)==PIN,'Reference PE identity');im=Image(PRIVATE);require(im.word(0x10095d74+62*4)==0x10007360 and im.word(0x10095d74+60*4)==0x10005d10,'Actual wrapper/uncache vtable slots')
    _,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('normal_object_parameter_wrapper',0x10007360,0x10007511),('whole_parameter_loop',0x10006f10,0x10007352),('forward_value_sync',0x10007720,0x10007767),('uncache_parameters',0x10005d10,0x10005db2),('delete_parameter_array',0x10002c00,0x10002c4a),('proxy_factory_boundary',0x10002c50,0x10002dcd),('raw_alpha',0x10003dd0,0x10003df1),('raw_color',0x10003e00,0x10003e75),('raw_final_tint',0x10003e80,0x10003f90),('float_getter_cached_projection',0x10017c20,0x10017c6a),('vector_getter_cached_projection',0x10017cc0,0x10017d80)])
    sources=source_closure([Path(p) for p in prior['sources']]+[ROOT/'work'/n for n in NEW]+[ROOT/'work/material_trig_x87_candidate.inc'])
    started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_object_parameters.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Fresh reference/owned/affected store build')
    exes=[];logs={}
    for name,machine,tag in [('parameters_reference_x86',0x14c,'native'),('parameters_owned_x64',0x8664,'owned'),('store_regression_x64',0x8664,'store')]:
        exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if machine==0x14c else 0x20b) and b'asInvoker' in raw,'Fresh architecture/manifest '+name);text_imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',text_imports,re.M|re.I);require({s.lower() for s in modules}=={'kernel32.dll'},'Runtime imports '+name);exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
        log=OUT/(tag+'-build.log');require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Clean compiler '+name);logs[str(log)]=sha(log)
    log=OUT/'asm-build.log';require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Clean existing assembly build');logs[str(log)]=sha(log)
    cases,files=corpus();mount=run/'mount';mount.mkdir()
    for path,source in files.items():p=mount/path;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(source,encoding='ascii')
    fixture_hashes={str(p):sha(p) for p in mount.rglob('*') if p.is_file()};query=run/'query.bin';query.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<4I',c['route'],c['repeat'],c['uncache'],c['queued'])+struct.pack('<12I',*c['env'])+text(c['name'])+packet(c['root']) for c in cases));(run/'cases.json').write_text(json.dumps(cases,indent=2));outputs=[];execution=[]
    libs=ROOT/'outputs/material-fallback-owned-20261026/verification-0mhr77s9/copied-libraries'
    for tag,material,directory in [('reference_x86',PRIVATE,libs),('owned_x64',run/'absent-original.dll',run/'absent-original-directory')]:
        dest=run/(tag+'.bin');p=subprocess.run([str(OUT/('parameters_'+tag+'.exe')),str(material),str(directory),str(mount),str(query),str(dest)],cwd=run,capture_output=True,text=True,timeout=120);(run/(tag+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,tag+' exit='+str(p.returncode)+'; inspect log');outputs.append(dest.read_bytes());execution.append(p.stdout+p.stderr)
    require(sha(query)=='714689962ff53c40cd0e593664963dcc227030c239f250322f6ec2336e719184' and hashlib.sha256(outputs[0]).hexdigest()=='8d37ad3101797d51d456c028dbeb8402c1e8fb46707c2a4316a7735c5d665edc','Original full corpus and native output pinned before owned corrections')
    decoded=[decode(raw) for raw in outputs];require(decoded[0][0]==decoded[1][0]==len(cases),'Exact case count');a,b=decoded[0][1],decoded[1][1];require(len(a)==len(b),'Exact lifecycle record count');diff=[i for i,(x,y) in enumerate(zip(a,b)) if x!=y];(run/'comparison.json').write_text(json.dumps(dict(cases=len(cases),records=len(a),bytes=len(outputs[0]),differences=len(diff),first=[dict(record=i,case=cases[a[i]['case']],native=a[i],owned=b[i]) for i in diff[:1]]),indent=2));require(not diff and outputs[0]==outputs[1],'Exact normal-object initialized state/values/source/teardown')
    checks=independent(cases,a);(run/'independent.json').write_text(json.dumps(dict(checks=checks,cases=len(cases),records=len(a)),indent=2));counts=re.search(r'native_allocations=(\d+) releases=(\d+)',execution[0]);require(counts and counts[1]==counts[2],'Original allocations released')
    safety=subprocess.run([str(OUT/'parameters_owned_x64.exe'),'--test'],cwd=run,capture_output=True,text=True,timeout=90);(run/'safety.log').write_text(safety.stdout+safety.stderr);require(safety.returncode==0 and 'PASS object_parameter_safety_checks=' in safety.stdout,'Owned initialization allocation/failure/guard safety')
    oldrun=Path(receipt).parent
    oldquery=oldrun/'query.txt';require(sha(oldquery)==prior['query_sha256'],'Unchanged6064-operation prior query');reg=run/'store-regression.txt';p=subprocess.run([str(OUT/'store_regression_x64.exe'),str(run/'absent-original.dll'),str(run/'absent-original-directory'),str(mount),str(oldquery),str(reg)],cwd=run,capture_output=True,text=True,timeout=120);(run/'store-regression.log').write_text(p.stdout+p.stderr);require(p.returncode==0 and sha(reg)==prior['trace_sha256'],'Entire6064-operation prior logical trace unchanged');p=subprocess.run([str(OUT/'store_regression_x64.exe'),'--test'],cwd=run,capture_output=True,text=True,timeout=90);(run/'store-safety.log').write_text(p.stdout+p.stderr);require(p.returncode==0 and 'owned_store_safety_checks=4657' in p.stdout and 'actual_allocation_failures=6' in p.stdout,'Prior4657 store safety assertions unchanged')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());graph=set()
    def visit(p):
        k=p.relative_to(ROOT).as_posix()
        if k in graph:return
        graph.add(k)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            child=p.parent/inc
            if child.is_file():visit(child.resolve())
    visit((ROOT/'work/probe_vulkan_win32_surface_x64.cpp').resolve());require(graph==set(build['editor_source_dependencies']) and all(sha(ROOT/k)==h for k,h in build['editor_source_dependencies'].items()),'Whole editor graph unchanged');require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor unchanged')
    for p,h in sources.items():require(sha(Path(p))==h,'Source drift '+p)
    for p,h in fixture_hashes.items():require(sha(Path(p))==h,'Read-only fixture modified '+p)
    for key in ['original_hashes','private_library_hashes']:
        for p,h in prior[key].items():require(sha(Path(p))==h,'Original drift '+p)
    require(not (run/'absent-original.dll').exists() and not (run/'absent-original-directory').exists(),'Owned runtime absent original paths')
    capture=run/'complete-states.bin.zlib';capture.write_bytes(zlib.compress(outputs[0],9));require(zlib.decompress(capture.read_bytes())==outputs[0],'Complete lossless state capture')
    limits=['Standalone CPU pre-texture initialization/lifetime composition only; no editor, renderer, texture/proxy/GPU resource Init or IMaterial ABI integration',
      'Actual normally constructed original source+error materials, real dictionary, wrapper62, parameter/fallback/file pipeline, forward getters/snapshot, uncache/destructor and queued deletion execute unchanged RX. Proxy factory is explicitly null; nonempty proxy declarations are refused by owned policy',
      'External root and patch accumulator are borrowed and may be changed in place by fallback. Persistent root identity is kept; temporary filename roots are not adopted. Optional native dependency-vector wrapper argument is not yet supported',
      'Supported existing parameter schemas: VertexLitGeneric, UnlitGeneric, Wireframe and Wireframe_DX9 plus unknown-name fallback. Other concrete schemas, unsupported file grammar/encoding and unsupported getter resource types refuse transactionally',
      'Missing-root failure is compared; arbitrary native mid-initialization OOM/recovery is not claimed. Owned allocation/commit refusal remains safe policy; original may publish its initialized bit before a later failure',
      'Imported heap/KV symbol/lock/context/hardware/mode/logging services remain controlled fixtures; neither actual renderer context nor general worker-thread safety is established',
      'No live SFM/GPU/Blender, original writes or full-editor build/run. Full project goal remains active/incomplete']
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',scope='Owned normally-constructed material CPU parameter initialization and lifetime',cases=len(cases),records=len(a),exact_bytes=len(outputs[0]),query_sha256=sha(query),output_sha256=hashlib.sha256(outputs[0]).hexdigest(),independent_checks=checks,native_allocations=int(counts[1]),native_releases=int(counts[2]),safety=safety.stdout,store_regression=dict(operations=6064,output_sha256=sha(reg),safety=(run/'store-safety.log').read_text()),executables=exes,build_logs=logs,sources=sources,source_changes=changes,original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],fixture_hashes=fixture_hashes,static_ranges=ranges,prior_receipt=str(receipt),prior_receipt_sha256=sha(receipt),editor_dependencies_unchanged=len(graph),editor_sha256=build['executable_sha256'],limits=limits)
    dest=run/'results.json';dest.write_text(json.dumps(result,indent=2)+'\n');replay=OUT/('replay-'+run.name.removeprefix('verification-'));replay.mkdir()
    files_to_copy=[Path(p) for p in sources]+[Path(e['path']) for e in exes]+[Path(p) for p in logs]+[dest,query,capture,OUT/'trig-wrapper.asm']+list(run.glob('*.log'))+list(run.glob('*.json'))+[Path(p) for p in fixture_hashes]+[OUT/'before/work'/n for n in CHANGED]
    for p in files_to_copy:t=replay/p.relative_to(ROOT);t.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,t)
    manifest={p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()};(replay/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');pointer=dict(receipt=str(dest),receipt_sha256=sha(dest),replay=str(replay),replay_manifest_sha256=sha(replay/'manifest.json'));(OUT/'latest-verification.json').write_text(json.dumps(pointer,indent=2)+'\n');print(json.dumps(dict(status=result['status'],receipt=str(dest),cases=len(cases),records=len(a),exact_bytes=len(outputs[0]),independent_checks=checks,safety=safety.stdout)))


if __name__=='__main__':main()
