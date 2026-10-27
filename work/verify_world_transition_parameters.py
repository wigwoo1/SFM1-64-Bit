"""WorldVertexTransition alias/concrete CPU parameter initialization and lifetime.

No live SFM or GPU. Private unchanged original code plus disposable file mounts.
"""
from pathlib import Path
from datetime import datetime,timezone
import difflib,hashlib,json,re,shutil,struct,subprocess,tempfile,time,zlib
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_material_keyvalues_parser import DUMP
from verify_builtin_error_material_seed import source_closure
from verify_material_object_parameters import node,string,packet,text,render,decode
from verify_ifm_draw_packet import check_ranges

OUT=ROOT/'outputs/world-transition-parameters-20261026'
LIBS=ROOT/'outputs/material-fallback-owned-20261026/verification-0mhr77s9/copied-libraries'
CHANGED=['material_parameter_orchestration_candidate.h','lightmapped_generic_init_candidate.h','probe_material_object_parameters.cpp']
NEW=['world_transition_parameter_schema_candidate.h','world_transition_init_candidate.h',
     'lightmapped_shared_parameter_init_candidate.h','world_transition_native_fixture.h',
     'probe_world_transition_parameters.cpp','probe_world_transition_discovery.cpp',
     'build_world_transition_discovery.cmd','build_world_transition_parameters.cmd','verify_world_transition_parameters.py']

def corpus():
    cases=[];files={};names=[];inside=False
    for line in (OUT/'schema.txt').read_text().splitlines():
        if line.startswith('shader '):inside=line.startswith('shader WorldVertexTransition_DX9 ')
        elif inside and '\t' in line:names.append(line.split('\t')[1])
    require(len(names)==50,'Actual registered50-field schema')
    def add(label,shader,route,fields=(),env=None,controls=None,expect=None):
        i=len(cases);root=node(shader,list(fields));name='world/case'+str(i)
        c=dict(label=label,route=route,repeat=i%2,uncache=(i//2)%2,queued=(i//4)%2,name=name,root=root,env=env or [0,0,0,0,0,0,0,0,1,0,92,0],controls=controls or [0,0,0,0,0,0],expect=expect or {},shader='WorldVertexTransition_DX9')
        cases.append(c)
        if route==2:files['materials/'+name+'.vmt']=render(root)
        return c
    for route in range(3):
        for shader in ['WorldVertexTransition','WorldVertexTransition_DX9']:
            add('empty',shader,route,expect={'empty':True})
            for config in [0,0x80,0x100,0x180]:
                for feature in range(16):
                    fields=[string('$alpha','.375'),string('$color','[.25 .5 1]'),string('$color2','[.5 1 .25]'),string('$srgbtint','[.5 .25 1]')]
                    if feature&1:fields+=[string('$basetexture','fixture/base'),string('$albedo','fixture/albedo'),string('$basetexture2','fixture/second')]
                    if feature&2:fields+=[string('$bumpmap','fixture/normal'),string('$envmapmask','fixture/mask')]
                    if feature&4:fields+=[string('$envmap','EnV_CuBeMaP')]
                    if feature&8:fields+=[string('$nodiffusebumplighting','1')]
                    fields+=[string('$lightwarptexture','fixture/warp'),string('$blendmodulatetexture','fixture/blend')]
                    controls=[feature%3-1,(feature//2)%2,(feature//3)%2,[0,1,256,257][feature%4],[0,1,256][feature%3],feature%2]
                    env=[feature%3,feature%2,(feature//2)%2,0,0,0,0,config,1,feature%2,[80,90,92,95][feature%4],feature%2]
                    add('config-feature',shader,route,fields,env,controls,dict(base=bool(feature&1),bump=bool(feature&2),env=bool(feature&4),nodiff=bool(feature&8)))
            for index,name in enumerate(names):
                if index<4:continue
                for value in ['.375','[.25 .5 1]']:add('declared-'+str(index),shader,route,[string(name,value)])
            # These similarly named inputs are dynamic extras, not fields in
            # this family's actual info mapping. Paint cannot initialize them.
            extra=[string(n,'.625') for n in ['$alpha2','$alphatestreference','$paintsplatnormalmap','$paintsplatenvmap','$height_scale','$shadersrgbread360','$envmaplightscale']]
            add('undeclared-fields',shader,route,extra,controls=[0,0,0,0,0,1],expect={'extras':True})
            for fog in [0,1]:add('explicit-fog',shader,route,[string('$vertexfog',str(fog))],controls=[1,0,0,0,0,0],expect={'fog':fog})
            add('model',shader,route,[string('$model','1')],expect={'model':True})
            add('selected-override',shader,route,[string('$alpha','.25'),node('GPU<1',[string('$alpha','.75'),string('$basetexture2','fixture/selected')])],expect={'alpha':0x3f400000})
            if shader=='WorldVertexTransition':add('concrete-override',shader,route,[string('$alpha','.25'),node('WorldVertexTransition_DX9',[string('$alpha','.625')])],env=[2,0,0,0,0,0,0,0,1,0,92,0],expect={'alpha':0x3f200000})
            else:add('concrete-ignores-fallback',shader,route,[string('$fallbackmaterial','must_not_load')],expect={'noFallback':True})
        for request,target in [('world_target','WorldVertexTransition_DX9'),('world_chain','WorldVertexTransition_DX9'),('unlit_target','UnlitGeneric'),('missing_world_fallback','UnlitGeneric')]:
            c=add('alias-fallback-'+request,'WorldVertexTransition',route,[string('$fallbackmaterial',request)]);c['shader']=target
        owner='world/self'+str(route);c=add('alias-self-fallback','WorldVertexTransition',route,[string('$fallbackmaterial',owner)]);c['name']=owner;c['shader']='UnlitGeneric'
        if route==2:del files['materials/world/case'+str(len(cases)-1)+'.vmt'];files['materials/'+owner+'.vmt']=render(c['root'])
    files['materials/world_target.vmt']='WorldVertexTransition_DX9 { $alpha .5 $basetexture fixture/loaded $basetexture2 fixture/second $bumpmap fixture/normal }'
    files['materials/world_chain.vmt']='WorldVertexTransition { $fallbackmaterial world_target }'
    files['materials/unlit_target.vmt']='UnlitGeneric { $alpha .25 }'
    originals=json.loads((OUT/'preflight.json').read_text())['benchmark_material_hashes']
    for path,h in originals.items():
        p=Path(path);require(sha(p)==h,'Original world material identity');c=add('real-benchmark','WorldVertexTransition',2,expect={'benchmark':path});files['materials/'+c['name']+'.vmt']=p.read_bytes().decode('ascii')
    return cases,files

def independent_world(cases,records):
    checks=0
    def check(b,msg):
        nonlocal checks
        require(b,msg);checks+=1
    for index,c in enumerate(cases):
        rows=[r for r in records if r['case']==index];by={r['stage']:r for r in rows};first=by[1];last=rows[-1];v=first['values'];control=c['controls'];config=c['env'][7];expect=c['expect'];world=c['shader']=='WorldVertexTransition_DX9';alias=c['root']['key']=='WorldVertexTransition'
        check(first['result'] and first['shader']==c['shader'] and first['count']==len(v) and first['count']>=(50 if world else 68),'Concrete shader/schema published')
        check(first['flags']==(20 if c['route']==0 else 4) and first['forward_valid'],'Source and initialized state')
        if world:
            check(v[2]['words'][2]&4,'Lightmapping requirement')
            check(v[9]['text']=='effects/flashlight001','Flashlight declaration')
            if alias:check(first['lookups']==(['WorldVertexTransition','WorldVertexTransition','WorldVertexTransition_DX9'] if c['label']=='alias-fallback-world_chain' else ['WorldVertexTransition','WorldVertexTransition_DX9']),'Alias recollects each loaded root before concrete schema')
            else:check(first['lookups']==['WorldVertexTransition_DX9'],'Concrete path has one lookup')
        if expect.get('empty'):
            check(first['count']==50 and v[2]['words'][2]==4 and first['forward']==[0x3f800000]*7,'Empty50-var defaults')
            for i,b in [(17,0x40800000),(19,0x3f800000),(30,0),(31,0x3f800000),(32,0x3f800000)]:check(v[i]['words'][3]==b,'Independent remapped float default')
            check(v[20]['words'][3:7]==[0x3f800000]*4,'Remapped detail tint')
        if 'bump' in expect:
            bump=expect['bump'];base=expect['base'];env=expect['env'];nodiff=expect['nodiff'];warn=env and bool(control[3]&255) and not(control[4]&255)
            check(bool(v[2]['words'][2]&8)==(bump and not nodiff and not(config&0x100)),'Bump light flag predicate')
            if bump:check((v[23]['words'][1]&15)==6,'Bump clears remapped env mask')
            if base:check(v[6]['text']==('fixture/albedo' if bump and not nodiff and not(config&0x100) else 'fixture/base') and v[38]['text']=='fixture/second','Albedo selection preserves second base')
            check(first['warnings']==int(warn),'World envmap warning predicate')
            check((v[21]['words'][1]&15)==(1 if env and not warn and not(config&0x80 and base) else 6),'Remapped envmap clear')
            check((v[43]['words'][1]&15)==(6 if control[1] else 1) and (v[44]['words'][1]&15)==(6 if control[2] else 1),'Remapped disable controls')
            check(bool(v[0]['words'][2]&0x80000000)==bool(control[0]),'Fog default control')
        if expect.get('extras'):
            check(first['count']==57,'Undeclared variables retained after real50 descriptors')
            # The already verified filename parser normalizes numeric tokens
            # before collection; supplied/borrowed string nodes keep strings.
            expectedType,expectedText=(0,'') if c['route']==2 else (1,'.625')
            check(all(x['text']==expectedText and (x['words'][1]&15)==expectedType and x['words'][3]==0x3f200000 for x in v[50:]),'Undeclared extras retain route-appropriate type/value without helper capture')
        if 'fog' in expect:check(bool(v[0]['words'][2]&0x80000000)==bool(expect['fog']),'Explicit fog wins')
        if expect.get('model'):check(v[0]['words'][2]&0x800,'Model flag survives alias collection')
        if 'alpha' in expect:check(v[5]['words'][3]==expect['alpha'],'Selected condition/concrete override')
        if expect.get('noFallback'):check(v[-1]['name']=='$fallbackmaterial' and len(first['lookups'])==1,'Concrete shader does not follow unused fallback field')
        if expect.get('benchmark'):check(v[6]['text'] and v[38]['text'] and first['warnings']==0,'Both actual benchmark bases interpreted')
        if c['repeat']:check(by[2]['values']==v and by[2]['forward']==first['forward'],'Initialized guard preserves state')
        if c['uncache']:check(by[3]['values']==v and by[4]['count']==0 and by[4]['forward']==first['forward'] and by[5]['shader']==c['shader'],'Uncache/reinitialize actual schema')
        check(not last['alive'] and not last['values'] and not last['queued'] and last['source_freed']==int(c['route']==0),'Normal object and source lifetime')
    return checks

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    preflight=json.loads((OUT/'preflight.json').read_text());pointer=preflight['prior'];priorPath=Path(pointer['receipt']);require(sha(priorPath)==pointer['receipt_sha256'],'Prior Lightmapped receipt');prior=json.loads(priorPath.read_text());changes={};allowed={str(ROOT/'work'/n) for n in CHANGED}
    for p,h in prior['sources'].items():
        if p in allowed:
            before=OUT/'before'/Path(p).relative_to(ROOT);require(sha(before)==h,'Prior frozen source');changes[p]=dict(before=h,after=sha(Path(p)),diff=''.join(difflib.unified_diff(before.read_text().splitlines(True),Path(p).read_text().splitlines(True))))
        else:require(sha(Path(p))==h,'Unrelated source '+p)
    require(set(changes)==allowed,'Exact edit set');(run/'source-changes.json').write_text(json.dumps(changes,indent=2));sources=source_closure([Path(p) for p in prior['sources']]+[ROOT/'work'/n for n in NEW])
    benchmark=dict(prior['benchmark_material_hashes'],**preflight['benchmark_material_hashes'])
    for group in [prior['original_hashes'],prior['private_library_hashes'],benchmark]:
        for p,h in group.items():require(sha(Path(p))==h,'Protected original '+p)
    _,ranges=check_ranges(LIBS/'stdshader_dx9.dll','stdshader_dx9-disassembly.txt',[('alias_noop_initializer',0x1000f6e0,0x1000f6e3),('alias_fallback',0x10061ac0,0x10061ac8),('concrete_no_fallback',0x10041370,0x10041375),('world_field_mapping',0x10061b00,0x10061ca4),('world_concrete_initializer',0x10061cb0,0x10061cdb),('shared_initializer',0x10028970,0x100290d5)])
    started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_world_transition_parameters.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=240);(run/'build-driver.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Fresh world/prior builds');exes=[];logs={}
    targets=[('world_reference_x86',0x14c,'native'),('world_owned_x64',0x8664,'owned'),('prior_reference_x86',0x14c,'prior-native'),('prior_owned_x64',0x8664,'prior-owned'),('light_reference_x86',0x14c,'light-native'),('light_owned_x64',0x8664,'light-owned'),('store_regression_x64',0x8664,'store')]
    for stem,machine,tag in targets:
        exe=OUT/(stem+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if machine==0x14c else 0x20b) and b'asInvoker' in raw,'Fresh PE architecture '+stem)
        imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require({s.lower() for s in modules}=={'kernel32.dll'},'Runtime imports '+stem);exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
        log=OUT/(tag+'-build.log');require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Clean compiler '+stem);logs[str(log)]=sha(log)
    log=OUT/'asm-build.log';require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Clean assembly');logs[str(log)]=sha(log)
    cases,files=corpus();mount=run/'mount';mount.mkdir()
    for name,contents in files.items():p=mount/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(contents,encoding='ascii')
    fixtureHashes={str(p):sha(p) for p in mount.rglob('*') if p.is_file()};query=run/'query.bin';query.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<4I',c['route'],c['repeat'],c['uncache'],c['queued'])+struct.pack('<12I',*c['env'])+text(c['name'])+packet(c['root'])+struct.pack('<6I',*[n&0xffffffff for n in c['controls']]) for c in cases));(run/'cases.json').write_text(json.dumps(cases,indent=2))
    def execute(stem,query,dest,mount):
        native=stem.endswith('x86');p=subprocess.run([str(OUT/(stem+'.exe')),str(PRIVATE if native else run/'absent-original.dll'),str(LIBS if native else run/'absent-original-directory'),str(mount),str(query),str(dest)],cwd=run,capture_output=True,text=True,timeout=180);(run/(dest.stem+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,stem+' exit='+str(p.returncode)+'; inspect log');return p.stdout+p.stderr
    reference=run/'reference.bin';execution=execute('world_reference_x86',query,reference,mount);(run/'reference-identity.json').write_text(json.dumps(dict(query=sha(query),reference=sha(reference),cases=len(cases)),indent=2))
    require(sha(query)=='a44ac6aac0510a375d470d41aa1a8abbc43d62d5aa2114b3b8251f023213f8f2' and sha(reference)=='e2bf525d2c77a21da6f1b56488895a323f3e56125d388e48dae8bdb9dadbf173','Original995-case input/output pinned before independent chain expectation correction')
    owned=run/'owned.bin';execute('world_owned_x64',query,owned,mount);a=reference.read_bytes();b=owned.read_bytes();na,ra=decode(a);nb,rb=decode(b);diff=[i for i,(x,y) in enumerate(zip(ra,rb)) if x!=y]
    (run/'comparison.json').write_text(json.dumps(dict(cases=na,records=len(ra),owned_records=len(rb),bytes=len(a),differences=len(diff),first=[dict(record=i,case=cases[ra[i]['case']],reference=ra[i],owned=rb[i]) for i in diff[:1]]),indent=2));require(na==nb==len(cases) and len(ra)==len(rb) and a==b,'Complete world alias/concrete states')
    checks=independent_world(cases,ra);counts=re.search(r'native_allocations=(\d+) releases=(\d+)',execution);require(counts and counts[1]==counts[2],'Native allocations balanced')
    regressions=[]
    priorParametersPath=Path(prior['prior_receipt']);priorParameters=json.loads(priorParametersPath.read_text())
    for previous,previousPath,stems in [(prior,priorPath,['light_reference_x86','light_owned_x64']),(priorParameters,priorParametersPath,['prior_reference_x86','prior_owned_x64'])]:
        oldrun=previousPath.parent;oldquery=oldrun/'query.bin';require(sha(oldquery)==previous['query_sha256'],'Prior input identity')
        for stem in stems:
            dest=run/(stem+'.bin');execute(stem,oldquery,dest,oldrun/'mount');require(sha(dest)==previous['output_sha256'],'Entire prior logical corpus unchanged '+stem);regressions.append(dict(executable=stem,cases=previous['cases'],records=previous['records'],output_sha256=sha(dest)))
    safety={}
    for exe,arg,key in [('world_owned_x64','--test-world-transition','world'),('light_owned_x64','--test-lightmapped','lightmapped'),('prior_owned_x64','--test','parameters'),('store_regression_x64','--test','ownership')]:
        p=subprocess.run([str(OUT/(exe+'.exe')),arg],cwd=run,capture_output=True,text=True,timeout=120);(run/(key+'-safety.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,'Owned safety '+key);safety[key]=p.stdout
    require(safety['world']=='PASS world_transition_safety_checks=831 actual_allocation_failures=820\n' and safety['lightmapped']==prior['lightmapped_safety'] and safety['parameters']==prior['safety'] and safety['ownership']==prior['store_regression']['safety'],'Exact affected safety counts')
    storePath=Path(priorParameters['prior_receipt']);store=json.loads(storePath.read_text());storeQuery=storePath.parent/'query.txt';require(sha(storeQuery)==store['query_sha256'],'Prior ownership input');dest=run/'store-regression.txt';execute('store_regression_x64',storeQuery,dest,mount);require(sha(dest)==store['trace_sha256'],'Prior6064-operation trace unchanged')
    buildInfo=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());graph=set()
    def visit(p):
        name=p.relative_to(ROOT).as_posix()
        if name in graph:return
        graph.add(name)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            q=p.parent/inc
            if q.is_file():visit(q.resolve())
    visit((ROOT/'work/probe_vulkan_win32_surface_x64.cpp').resolve());require(graph==set(buildInfo['editor_source_dependencies']) and all(sha(ROOT/k)==h for k,h in buildInfo['editor_source_dependencies'].items()),'Whole editor graph unchanged');require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==buildInfo['executable_sha256'],'Editor unchanged')
    for group in [sources,prior['original_hashes'],prior['private_library_hashes'],benchmark,fixtureHashes]:
        for p,h in group.items():require(sha(Path(p))==h,'Protected identity drift '+p)
    require(not (run/'absent-original.dll').exists() and not (run/'absent-original-directory').exists(),'Independent owned runtime arguments absent')
    capture=run/'complete-states.bin.zlib';capture.write_bytes(zlib.compress(a,9));require(zlib.decompress(capture.read_bytes())==a,'Full lossless comparison evidence')
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',scope='WorldVertexTransition alias/concrete CPU initialization through real material objects and independent x64',cases=len(cases),records=len(ra),exact_bytes=len(a),query_sha256=sha(query),output_sha256=sha(reference),independent_checks=checks,native_allocations=int(counts[1]),native_releases=int(counts[2]),safety=safety,regressions=regressions,store_regression=dict(operations=6064,trace_sha256=sha(dest)),sources=sources,source_changes=changes,original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],benchmark_material_hashes=benchmark,world_benchmark_material_hashes=preflight['benchmark_material_hashes'],fixture_hashes=fixtureHashes,executables=exes,build_logs=logs,static_ranges=ranges,prior_receipt=str(priorPath),prior_receipt_sha256=sha(priorPath),editor_dependencies_unchanged=len(graph),editor_sha256=buildInfo['executable_sha256'],limits=['CPU InitParams only; no GPU textures/proxies/resource Init, editor/render integration or visual parity','Shared helper uses explicit mappings into real68/50-variable arrays; undeclared fields never capture dynamic variables','Original text remains RX/unchanged with existing controlled host/file/shader-init/cvar services; slot5/6 semantic renderer integration still unproven','String getter conversions outside existing string values refuse; original arbitrary OOM behavior not claimed','Owner-thread serialized API, existing proxy and file-format limits; full goal incomplete; no live SFM/Blender or original writes'])
    dest=run/'results.json';dest.write_text(json.dumps(result,indent=2)+'\n');replay=OUT/('replay-'+run.name.removeprefix('verification-'));replay.mkdir()
    for p in [Path(p) for p in sources]+[Path(e['path']) for e in exes]+[Path(p) for p in logs]+[dest,query,capture,OUT/'preflight.json',OUT/'schema.txt',OUT/'discovery_x86.exe',OUT/'discovery-build.log']+list(run.glob('*.json'))+list(run.glob('*.log'))+[Path(p) for p in fixtureHashes]+[OUT/'before/work'/n for n in CHANGED]:
        t=replay/p.relative_to(ROOT);t.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,t)
    manifest={p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()};(replay/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');pointer=dict(receipt=str(dest),receipt_sha256=sha(dest),replay=str(replay),replay_manifest_sha256=sha(replay/'manifest.json'));(OUT/'latest-verification.json').write_text(json.dumps(pointer,indent=2)+'\n');print(json.dumps(dict(status='PASS',receipt=str(dest),cases=len(cases),records=len(ra),exact_bytes=len(a),checks=checks)))

if __name__=='__main__':main()
