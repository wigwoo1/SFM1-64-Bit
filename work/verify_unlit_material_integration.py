"""Bounded UnlitGeneric integration into the owned whole material caller.

Original installations/sessions are read-only; only private copied-library
processes and disposable workspace files are used. No live SFM or GPU.
"""
from pathlib import Path
from datetime import datetime, timezone
from collections import Counter
import difflib, hashlib, json, random, re, shutil, struct, subprocess, tempfile, time, zlib
from verify_material_numeric_value import ROOT, LIB, PRIVATE, PIN, require, sha
from verify_material_keyvalues_parser import DUMP
from verify_material_parameter_orchestration import node, string, integer
from verify_material_partial_load import request
from verify_material_fallback_reference import decode
from verify_builtin_error_material_seed import source_closure

OUT = ROOT/'outputs/unlit-material-integration-20261026'
CHANGED = ['material_parameter_orchestration_candidate.h','probe_builtin_error_material_seed.cpp',
           'probe_material_parameter_orchestration.cpp','probe_material_file_parameters.cpp','probe_material_fallback_owned.cpp']
NEW = ['build_unlit_material_integration.cmd','verify_unlit_material_integration.py']
TAGS = ['directive','partial','parser_safety','files_safety','numeric_safety','patch_safety','parser','files','fallback','parameters','pipeline','builtin']


def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    preflight=json.loads((OUT/'preflight.json').read_text());pointer=preflight['prior'];receipt=Path(pointer['receipt'])
    require(sha(receipt)==pointer['receipt_sha256'],'Accepted Unlit initializer receipt');prior=json.loads(receipt.read_text())
    allowed={str(ROOT/'work'/n) for n in CHANGED};changes={}
    for p,h in prior['sources'].items():
        current=sha(Path(p))
        if p in allowed:
            before=OUT/'before'/Path(p).relative_to(ROOT);require(sha(before)==h and current!=h,'Exact before/current source '+p)
            changes[p]=dict(before=h,after=current,diff=''.join(difflib.unified_diff(before.read_text().splitlines(True),Path(p).read_text().splitlines(True),fromfile='before/'+Path(p).name,tofile='after/'+Path(p).name)))
        else:require(current==h,'Unrelated accepted source changed '+p)
    require(set(changes)==allowed,'Exact modified source set');(run/'source-changes.json').write_text(json.dumps(changes,indent=2)+'\n')
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Protected original '+p)
    for p,h in prior['private_library_hashes'].items():require(sha(Path(p))==h,'Private library '+p)
    require(sha(LIB)==sha(PRIVATE)==PIN,'Original/private material identity')
    sources=source_closure([Path(p) for p in prior['sources']]+[ROOT/'work'/n for n in NEW])
    # The isolated Unlit concrete implementation and its actual quoted compile
    # closure are unchanged, although this receipt also tracks unrelated caller
    # sources now intentionally edited above. Preserve its prior2077-case proof.
    concrete_graph=set()
    def visit_concrete(p):
        if p in concrete_graph:return
        concrete_graph.add(p)
        for n in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            child=p.parent/n
            if child.is_file():visit_concrete(child.resolve())
    visit_concrete((ROOT/'work/probe_unlit_generic_concrete.cpp').resolve())
    require(not concrete_graph&{Path(p) for p in changes},'Unlit implementation graph must remain unchanged')
    for p in concrete_graph:require(sha(p)==prior['sources'][str(p)],'Concrete compile dependency '+str(p))
    for e in prior['executables']:require(sha(Path(e['path']))==e['sha256'],'Prior concrete executable identity')
    prior_replay=Path(pointer['replay']);require(sha(prior_replay/'manifest.json')==pointer['replay_manifest_sha256'],'Prior concrete snapshot manifest')
    require(all(sha(prior_replay/k)==h for k,h in json.loads((prior_replay/'manifest.json').read_text()).items()),'Prior concrete frozen evidence')
    started=time.time();build=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_unlit_material_integration.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180)
    (run/'build-driver.log').write_text(build.stdout+build.stderr);require(build.returncode==0,'Fresh isolated affected component build');print('Build passed; comparing prior633 callers',flush=True)
    executables=[];logs={}
    for tag in [*TAGS,'reference']:
        p=OUT/(tag+'_owned_x64.exe' if tag!='reference' else 'reference_x86.exe');machine=0x14c if tag=='reference' else 0x8664;raw=p.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(p.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if machine==0x14c else 0x20b) and b'asInvoker' in raw,'Fresh architecture/manifest '+tag)
        imports=subprocess.run([str(DUMP),'/imports',str(p)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I)
        require({n.lower() for n in modules}=={'kernel32.dll'},'Direct imports '+tag);executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine),direct_imports=modules))
    for tag in [*TAGS,'reference','asm']:
        p=OUT/(tag+'-build.log');require(p.stat().st_mtime>=started-2 and not re.search(r'\b(?:fatal\s+)?(?:warning|error)\s+[A-Z]+\d+\s*:|^\s*(?:warning|error)\s*:',p.read_text(),re.I|re.M),'Clean fresh build '+tag);logs[str(p)]=sha(p)
    libs=ROOT/'outputs/material-fallback-owned-20261026/verification-0mhr77s9/copied-libraries';allocations=0;comparisons=[]
    def execute(exe,args,label):
        nonlocal allocations
        p=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,errors='replace',timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        require(p.returncode==0,label+' exit='+str(p.returncode)+'; see log')
        count=re.search(r'allocations=(\d+) releases=(\d+) null_frees=(\d+)',p.stdout)
        if count:require(count[1]==count[2],'Tracked native cleanup '+label);allocations+=int(count[1])
        return p.stdout+p.stderr
    def compare(mount,inp,label,count,expected=None,dev=0):
        before=sha(inp);a=run/(label+'-native.bin');b=run/(label+'-owned.bin')
        nlog=execute(OUT/'reference_x86.exe',['--caller',PRIVATE,libs,mount,inp,a],label+'-native');olog=execute(OUT/'fallback_owned_x64.exe',[mount,inp,b,run/'no-engine.dll'],label+'-owned')
        nd=re.search(r'dev_messages=(\d+)',nlog);od=re.search(r'dev_messages=(\d+)',olog);require(nd and od and int(nd[1])==int(od[1])==dev,'Separate DevMsg counts '+label)
        require(sha(inp)==before and (expected is None or sha(a)==expected),'Frozen query/native bytes '+label)
        n=decode(a.read_bytes());o=decode(b.read_bytes());require(len(n)==len(o)==count,'Record count '+label)
        diff=[dict(case=i,native=x,owned=y) for i,(x,y) in enumerate(zip(n,o)) if x!=y];(run/(label+'-comparison.json')).write_text(json.dumps(dict(differences=len(diff),first=diff[:2]),indent=2)+'\n')
        require(not diff and a.read_bytes()==b.read_bytes(),'Complete caller equality '+label)
        comparisons.append(dict(label=label,cases=count,bytes=a.stat().st_size,query_path=str(inp),mount=str(mount),query_sha256=before,output_sha256=sha(a),developer_messages=dev));return n
    directive=ROOT/'outputs/material-directive-failures-20261026/verification-g68o6ibf/results.json';dv=json.loads(directive.read_text())
    require(sha(directive)=='07463461c2ec7699d331e064a7acca03baad1fe5f1b6c51d33a4c717d75842d8','Prior633 receipt')
    partial=Path(dv['prior_receipt']);require(sha(partial)==dv['prior_receipt_sha256'],'Prior partial receipt')
    failure=Path(json.loads((ROOT/'outputs/material-fallback-failures-20261026/summary.json').read_text())['receipt']);fv=json.loads(failure.read_text());acc=Path(fv['prior_receipt']);av=json.loads(acc.read_text());old=Path(av['prior_regression_receipt']);malformed=[x for x in fv['records'] if x['mode']=='caller']
    for batch in dv['comparisons']:
        label=batch['label']
        if label=='prior-legacy160':mount=old.parent/'mount';inp=old.parent/'queries.bin'
        elif label=='prior-legacy128':mount=acc.parent/'mount';inp=acc.parent/'queries.bin'
        elif label=='prior-expanded':mount=partial.parent/'mount';inp=partial.parent/'expanded.in'
        elif label.startswith('prior-failure'):row=malformed[int(label.removeprefix('prior-failure'))];mount=failure.parent/'mount';inp=failure.parent/(row['label']+'.in')
        elif label=='repaired-original-fault':folder=ROOT/'outputs/material-fallback-failures-20261026/discovery-ywoz0zl_';mount=folder/'mount';inp=folder/'missing_directive-caller.in'
        else:require(label.startswith('directive'),'Known prior group');mount=directive.parent/'mount';inp=directive.parent/('new'+label.removeprefix('directive')+'.in')
        require(sha(inp)==batch['query_sha256'],'Exact prior request '+label);compare(mount,inp,'legacy-'+label,batch['cases'],batch['output_sha256'],batch['developer_messages'])
    require(sum(x['cases'] for x in comparisons)==633,'All prior633 records retained')
    print('Prior633 complete records unchanged; checking integrated Unlit paths',flush=True)
    seed_path=ROOT/'outputs/builtin-error-material-seed-20261026/verification-8jkzmq7j/results.json';seed=json.loads(seed_path.read_text());error=seed['seeds'][0]['source']
    mount=run/'mount';(mount/'materials').mkdir(parents=True);fixture_hashes={};cases=[];expectations=[]
    def write(name,source):
        p=mount/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(source,encoding='ascii');fixture_hashes[str(p)]=sha(p)
    def add(source,kind,owner='fixture/root',prefix=0,**extra):cases.append((owner,prefix,source,error));expectations.append(dict(kind=kind,**extra))
    add(node('Wireframe',[string('$fallbackmaterial','bounded_missing_builtin_seed')]),'exact_builtin')
    for model in [0,1]:
        for target in ['missing','empty','fixture/root','FiXtUrE/RoOt']:
            write('materials/empty.vmt','');add(node('Wireframe',[integer('$model',model),string('$fallbackmaterial',target)]),'error',self_reference='/' in target)
        add(node('UnlitGeneric',[integer('$model',model),string('$basetexture','fixture/base'),string('$fallbackmaterial','../../ignored')]),'direct',model=model)
        add(node('uNlItGeNeRiC',[integer('$model',model),integer('$vertexfog',0)]),'direct',model=model)
    rng=random.Random(2026102620)
    for i in range(128):
        model=(i//8)%2;mode=i%8;prefix=i%3==0;name='unlit/'+str(i)+'/target';logical=('materials/' if prefix else '')+name
        common=[integer('$model',model),integer('$selfillum',i%3==0),integer('$decal',i%5==0),string('$alpha','.375'),string('$color','[.2 .4 .6]'),string('$basetexture','fixture/base'),integer('extra_'+str(i),rng.randrange(256))]
        if mode==0:source=node('UnlitGeneric',common+[string('$fallbackmaterial','ignored_missing')]);kind='direct'
        elif mode==1:
            source=node('UnlitGeneric',common+[node('dx9',[string('$alpha','.25'),integer('overrideMarker',i)])]);kind='direct_override'
        else:
            source=node('Wireframe',[integer('$model',model),string('$fallbackmaterial',logical)])
            if mode==2:kind='loaded';write('materials/'+name+'.vmt','UnlitGeneric{$alpha .5 $basetexture fixture/base $envmap fixture/env}')
            elif mode==3:kind='error';write('materials/'+name+'.vmt','')
            elif mode==4:kind='error'
            elif mode==5:
                kind='preceding_error';write('materials/'+name+'.vmt','patch{include materials/unlit/'+str(i)+'/base.vmt insert{precedingMarker '+str(i)+'}}');write('materials/unlit/'+str(i)+'/base.vmt','Wireframe{$fallbackmaterial '+('materials/' if prefix else '')+'unlit/absent'+str(i)+'}')
            elif mode==6:
                kind='patched_unlit';write('materials/'+name+'.vmt','patch{include materials/unlit/'+str(i)+'/base.vmt insert{$alpha .25 patchMarker '+str(i)+'}}');write('materials/unlit/'+str(i)+'/base.vmt','UnlitGeneric{$alpha 1}')
            else:
                kind='loaded_override';write('materials/'+name+'.vmt','UnlitGeneric{$alpha .5 dx9{$alpha .125 $fallbackmaterial ignored}}')
        add(source,kind,prefix=int(prefix),model=model,index=i)
    capacity_index=len(cases);add(node('UnlitGeneric',[integer('extra_'+str(i),i) for i in range(187)]),'capacity')
    inp=run/'unlit-queries.bin';inp.write_bytes(request(cases));(run/'unlit-cases.json').write_text(json.dumps(expectations,indent=2)+'\n');rows=compare(mount,inp,'integrated-unlit',len(cases));independent=0
    def check(ok,message):
        nonlocal independent
        require(ok,message);independent+=1
    check(rows[0]==seed['caller'][0],'Exact original complete builtin error record9061 bytes, not only parameter values')
    for row,e,case in zip(rows,expectations,cases):
        check(row['shader']=='UnlitGeneric','Actual selected shader')
        if e['kind'] in ['error','exact_builtin','preceding_error']:
            check(row['root']==error and len(row['values'])==69 and row['values'][68]['name']=='$decalscale','Actual builtin tree/extra parameter')
            check(row['warnings']==1,'Recovery diagnostic count')
        else:check(row['warnings']==0,'No false fallback diagnostic')
        if e['kind']=='direct':check(row['lookups']==[case[2]['key']],'Null-returning Unlit fallback field not consumed')
        if e['kind']=='direct_override':check(row['selected']!=0 and row['values'][5]['words'][3]==0x3e800000,'Actual direct condition override')
        if e['kind']=='loaded':check(row['values'][5]['words'][3]==0x3f000000 and bool(row['values'][0]['words'][2]&0x800)==bool(e['model']),'File reload alpha/model carry')
        if e['kind']=='patched_unlit':check(row['values'][5]['words'][3]==0x3e800000 and any(n['key']=='patchMarker' for n in row['root']['children']),'Actual patch applied before Unlit init')
        if e['kind']=='loaded_override':check(row['values'][5]['words'][3]==0x3e000000 and row['selected']!=0,'Reloaded selected Unlit block')
        if e['kind']=='preceding_error':check(any(n['key']=='precedingMarker' for n in row['aggregate']['children'][0]['children']),'Preceding patch state survives missing-root recovery')
    check(len(rows[capacity_index]['values'])==255,'Whole caller255-variable capacity remains bounded')
    print('Integrated Unlit records match; running fresh strict regressions',flush=True)
    regressions=[]
    loop=Path(json.loads((ROOT/'outputs/material-parameter-orchestration-20261026/summary.json').read_text())['receipt']);lv=json.loads(loop.read_text());dest=run/'strict-loop.bin';require(sha(loop.parent/'queries.bin')==lv['query_sha256'],'Strict loop request');execute(OUT/'parameters_owned_x64.exe',[run/'absent.dll',run/'absent-dir',loop.parent/'queries.bin',dest],'strict-loop');require(sha(dest)==lv['output_sha256'],'Strict loop records');regressions.append(dict(kind='parameter_loop',cases=lv['cases']))
    files=Path(json.loads((ROOT/'outputs/material-file-parameters-20261026/summary.json').read_text())['receipt']);filev=json.loads(files.read_text())
    for group in filev['groups']:
        label=group['name'];folder=files.parent/label;require(sha(folder/'file-queries.bin')==group['query_sha256'],'File request');raw=run/(label+'-raw.bin');params=run/(label+'-params.bin');projection=run/(label+'-projection.bin');meta=run/(label+'-meta.bin');execute(OUT/'pipeline_owned_x64.exe',[folder/'file-queries.bin',raw,params,projection,meta,run/'no-engine.dll'],'strict-file-'+label);require(sha(raw)==group['raw_output_sha256'] and sha(params)==group['parameter_output_sha256'] and sha(projection)==group['projection_sha256'],'Strict file bytes')
    regressions.append(dict(kind='file_parameters',cases=filev['cases']))
    folder=ROOT/'outputs/material-patch-files-20261017/verification-zsdt1ra8';rawv=json.loads((folder/'results.json').read_text())
    for group in rawv['scenarios']:
        inp=folder/(group['name']+'-input.bin');require(sha(inp)==group['input_sha256'],'Raw patch request');dest=run/(group['name']+'-raw.bin');execute(OUT/'files_owned_x64.exe',[inp,dest],'strict-raw-'+group['name']);require(sha(dest)==group['output_sha256'],'Raw patch bytes')
    regressions.append(dict(kind='raw_patch_files',cases=sum(x['cases'] for x in rawv['scenarios'])))
    rp=Path(json.loads((ROOT/'outputs/material-keyvalues-numeric-20261026/summary.json').read_text())['raw_receipt']);rv=json.loads(rp.read_text());inp=rp.parent/'input.bin';require(sha(inp)==rv['raw']['input_sha256'],'Numeric request');dest=run/'numeric.bin';execute(OUT/'parser_owned_x64.exe',[run/'no-engine.dll',inp,dest],'strict-numeric');require(sha(dest)==rv['raw']['output_sha256'],'Numeric parser bytes');regressions.append(dict(kind='numeric_parser',cases=rv['raw']['cases']))
    safety={}
    for tag in ['directive','partial','parser_safety','files_safety','numeric_safety','patch_safety','fallback','parameters','pipeline','builtin']:
        safety[tag]=execute(OUT/(tag+'_owned_x64.exe'),['--test'] if tag in ['fallback','parameters','pipeline','builtin'] else [],'safety-'+tag).strip()
    require(safety['builtin']=='PASS builtin_seed_checks=6 unlit_parameter_path=SUPPORTED','Real builtin recovery/transaction safety')
    for tag,expected in dv['safety'].items():require(safety[tag]==expected,'Preserved legacy safety checks '+tag)
    seed_output=run/'owned-seed.bin';execute(OUT/'builtin_owned_x64.exe',[seed_output,run/'no-original.dll'],'seed-definition')
    require(sha(seed_output)==seed['seed_output_sha256'],'Original261-byte builtin definition unchanged')
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Original changed '+p)
    for p,h in prior['private_library_hashes'].items():require(sha(Path(p))==h,'Private library changed '+p)
    for p,h in sources.items():require(sha(Path(p))==h,'Source changed during verification '+p)
    for p,h in fixture_hashes.items():require(sha(Path(p))==h,'Disposable source changed '+p)
    build_info=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());graph=set()
    def visit_editor(p):
        k=p.relative_to(ROOT).as_posix()
        if k in graph:return
        graph.add(k)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            if (p.parent/inc).is_file():visit_editor((p.parent/inc).resolve())
    visit_editor((ROOT/'work/probe_vulkan_win32_surface_x64.cpp').resolve())
    require(graph==set(build_info['editor_source_dependencies']) and all(sha(ROOT/k)==h for k,h in build_info['editor_source_dependencies'].items()),'Complete production editor dependency graph unchanged')
    require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==prior['editor_sha256']==build_info['executable_sha256'],'Editor executable unchanged')
    require(not set('work/'+n for n in CHANGED+NEW)&graph,'Integration outside editor include graph')
    require(not (run/'no-engine.dll').exists() and not (run/'absent.dll').exists() and not (run/'absent-dir').exists(),'Owned original runtime paths absent')
    captures={}
    for row in comparisons:
        p=run/(row['label']+'-native.bin');dest=run/(row['label']+'-exact.bin.zlib');raw=p.read_bytes();dest.write_bytes(zlib.compress(raw,9));require(zlib.decompress(dest.read_bytes())==raw,'Exact compressed evidence '+row['label']);captures[str(dest)]=sha(dest)
    limits=['Owned shared caller now supports VertexLitGeneric125,UnlitGeneric68,Wireframe13,Wireframe_DX9 15, retaining original per-family initialization and extra variables; strict default file/parser policies unchanged','Complete real builtin error declaration is still supplied explicitly through the existing API; no normal IMaterial constructor/resource factory/default wrapper introduced','Original comparison executes unchanged material/KeyValues/file/filesystem/shader methods with controlled owner/name/allocator/config/hardware/diagnostic/error-source services. No texture/resource Init, proxies, GPU/renderer or visual fidelity claim','Prior2077 isolated Unlit cases were not rerun: functional compile closure/executables/receipt/frozen snapshot unchanged. Fresh633 whole-caller and14429 owned strict regressions pass frozen original hashes; strict native regressions were not rerun','All242 protected originals and full310 editor dependencies/executable unchanged. No editor integration/rebuild/run, live SFM launch/control/enumeration/capture, original writes, GPU or Blender; application incomplete']
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),scope='VERIFIED bounded UnlitGeneric whole-caller integration; application incomplete',previous_goal_turn='progress',cases=sum(x['cases'] for x in comparisons),prior_fallback_cases=633,new_cases=len(cases),exact_bytes=sum(x['bytes'] for x in comparisons),differences=0,new_groups=dict(Counter(e['kind'] for e in expectations)),independent_checks=independent,comparisons=comparisons,regressions=regressions,strict_regression_cases=sum(x['cases'] for x in regressions),safety=safety,allocations=allocations,releases=allocations,executables=executables,build_logs=logs,sources=sources,documented_source_changes=changes,original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],fixture_hashes=fixture_hashes,captures=captures,prior_receipt=str(receipt),prior_receipt_sha256=sha(receipt),prior633_receipt=str(directive),prior633_receipt_sha256=sha(directive),prior_unlit_functional_compile_closure={str(p):sha(p) for p in concrete_graph},builtin_error_receipt=str(seed_path),builtin_error_receipt_sha256=sha(seed_path),builtin_error_record_sha256=hashlib.sha256(struct.pack('<I',1)+(run/'integrated-unlit-native.bin').read_bytes()[4:4+seed['original_caller_bytes']-4]).hexdigest(),seed_definition_sha256=sha(seed_output),editor_dependencies_unchanged=len(graph),editor_sha256=build_info['executable_sha256'],limits=limits)
    require(result['builtin_error_record_sha256']==seed['original_caller_sha256'],'Original complete builtin9061-byte record')
    dest=run/'results.json';dest.write_text(json.dumps(result,indent=2)+'\n');replay=OUT/('replay-'+run.name.removeprefix('verification-'));replay.mkdir()
    # Strict regression helpers reuse the local variable inp. Archive the
    # explicitly named integrated query plus every compared request/mount.
    # These are disposable WORKSPACE fixtures, never original installation files.
    replay_inputs={run/'unlit-queries.bin',run/'unlit-cases.json'}
    for row in comparisons:
        replay_inputs.add(Path(row['query_path']))
        folder=Path(row['mount']);require(folder.is_relative_to(ROOT/'outputs'),'Workspace-only fixture snapshot')
        replay_inputs.update(p for p in folder.rglob('*') if p.is_file())
    files=[Path(p) for p in sources]+[Path(e['path']) for e in executables]+[Path(p) for p in logs]+[dest,seed_output,run/'source-changes.json']+[Path(p) for p in captures]+list(run.glob('*.log'))+list(replay_inputs)
    for p in files:
        to=replay/p.relative_to(ROOT);to.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,to)
    manifest={p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()};(replay/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    pointer=dict(receipt=str(dest),receipt_sha256=sha(dest),replay=str(replay),replay_manifest_sha256=sha(replay/'manifest.json'));(OUT/'latest-verification.json').write_text(json.dumps(pointer,indent=2)+'\n')
    print(json.dumps({**pointer,**{k:result[k] for k in ['status','cases','prior_fallback_cases','new_cases','exact_bytes','strict_regression_cases','independent_checks','safety','allocations','releases']}},indent=2))


if __name__=='__main__':main()
