"""LightmappedGeneric CPU initialization in real material objects versus owned x64.

Private original code remains RX/unmodified. Only disposable workspace mounts
are writable. This gate does not launch SFM or establish GPU/editor fidelity.
"""
from pathlib import Path
from datetime import datetime,timezone
import difflib,hashlib,json,re,shutil,struct,subprocess,tempfile,time,zlib
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_material_keyvalues_parser import DUMP
from verify_builtin_error_material_seed import source_closure
from verify_material_object_parameters import node,string,integer,packet,text,render,decode,independent
from verify_ifm_draw_packet import check_ranges

OUT=ROOT/'outputs/lightmapped-parameters-20261026'
CHANGED=['material_parameter_orchestration_candidate.h','material_fallback_parameters_candidate.h',
         'material_object_parameter_initialization_candidate.h','probe_material_object_parameters.cpp']
NEW=['lightmapped_generic_parameter_schema_candidate.h','lightmapped_generic_init_candidate.h',
     'lightmapped_parameters_native_fixture.h','probe_lightmapped_parameters.cpp',
     'probe_lightmapped_parameters_discovery.cpp','build_lightmapped_discovery.cmd',
     'build_lightmapped_parameters.cmd','verify_lightmapped_parameters.py']
LIBS=ROOT/'outputs/material-fallback-owned-20261026/verification-0mhr77s9/copied-libraries'

def corpus():
    cases=[];files={}
    def add(label,fields=(),route=0,env=None,controls=None,expect=None):
        i=len(cases);root=node('LightmappedGeneric',list(fields));name='lightmapped/case'+str(i)
        c=dict(label=label,route=route,repeat=i%2,uncache=(i//2)%2,queued=(i//4)%2,name=name,root=root,
               env=env or [0,0,0,0,0,0,0,0,1,0,92,0],controls=controls or [0,0,0,0,0,0],expect=expect or {})
        cases.append(c)
        if route==2:files['materials/'+name+'.vmt']=render(root)
        return c
    for route in range(3):
        add('empty',route=route,expect={'empty':True})
        for config in [0,0x80,0x100,0x180]:
            for feature in range(12):
                fields=[string('$alpha','.375'),string('$color','[.25 .5 1]'),string('$color2','[.5 1 .25]'),string('$srgbtint','[.5 .25 1]')]
                if feature&1:fields+=[string('$basetexture','fixture/base'),string('$albedo','fixture/albedo')]
                if feature&2:fields+=[string('$bumpmap','fixture/normal'),string('$envmapmask','fixture/mask')]
                if feature&4:fields+=[string('$envmap','EnV_CuBeMaP')]
                if feature&8:fields+=[string('$nodiffusebumplighting','1')]
                fields+=[string('$lightwarptexture','fixture/warp'),string('$blendmodulatetexture','fixture/blend')]
                control=[feature%3-1,(feature//2)%2,(feature//3)%2,[0,1,256,257][feature%4],[0,1,256][feature%3],feature%2]
                env=[feature%3,feature%2,(feature//2)%2,0,0,0,0,config,1,feature%2,[90,92,95][feature%3],feature%2]
                add('config-feature',fields,route,env,control,dict(base=bool(feature&1),bump=bool(feature&2),env=bool(feature&4),nodiff=bool(feature&8)))
        # Individual declarations ensure the entire descriptor family is read,
        # including input types different from the declaration's nominal type.
        names=[l.split('\t')[1] for l in (OUT/'schema.txt').read_text().splitlines() if '\t' in l]
        for index,name in enumerate(names):
            if index<4:continue
            for value in ['.375','[.25 .5 1]']:
                add('declared-'+str(index),[string(name,value)],route)
        add('explicit-default-overrides',[string(n,'.625') for n in names[14:] if n not in ['$ENVMAP','$PAINTSPLATENVMAP']]+[string('$dynamic_extra','kept')],route)
        # A normal concrete shader does not follow $fallbackmaterial. The
        # alias Wireframe does, and must recollect using the Lightmapped schema.
        c=add('fallback-to-lightmapped',route=route)
        c['root']=node('Wireframe',[string('$fallbackmaterial','light_target')])
        if route==2:files['materials/'+c['name']+'.vmt']=render(c['root'])
        files['materials/light_target.vmt']='LightmappedGeneric { $alpha .5 $basetexture fixture/loaded $bumpmap fixture/normal }'
        c=add('selected-override',[string('$alpha','.25'),node('GPU<1',[string('$alpha','.75'),string('$basetexture','fixture/selected')])],route,expect={'selected':True})
        c=add('exact-shader-override',[string('$alpha','.25'),node('LightmappedGeneric_dx9',[string('$alpha','.625')])],route,env=[2,0,0,0,0,0,0,0,1,0,92,0],expect={'alpha':0x3f200000})
        c=add('paint-preserve',[string('$envmap','fixture/env'),string('$paintsplatnormalmap','fixture/paint'),string('$paintsplatenvmap','fixture/paintenv')],route,controls=[0,0,0,0,0,1],expect={'paintPreserve':True})
        add('decal-selfillum-masks',[string('$decal','1'),string('$selfillum','1'),string('$basealphaenvmapmask','1')],route,expect={'flags':0x10002})
    require(len(cases)==549,'Preserve first accepted549-case corpus')
    for route in range(3):
        for fog in [0,1]:
            add('explicit-fog',[string('$vertexfog',str(fog))],route,controls=[1,0,0,0,0,0],expect={'fog':fog})
        for enabled in [0,1]:
            add('authored-paint-disabled' if not enabled else 'paint-copy-env',[string('$envmap','fixture/paint-env'),string('$paintsplatnormalmap','fixture/normal')],route,controls=[0,0,0,0,0,enabled],expect={'paintCopy':bool(enabled)})
        add('lightmapped-keeps-fallbackmaterial',[string('$fallbackmaterial','missing_must_not_load')],route,expect={'noFallback':True})
    originals=json.loads((OUT/'benchmark-material-identities.json').read_text())
    for path,h in originals.items():
        p=Path(path);require(sha(p)==h,'Read-only benchmark material identity '+path)
        c=add('real-benchmark-material',route=2,expect={'benchmarkPath':path});files['materials/'+c['name']+'.vmt']=p.read_bytes().decode('ascii')
    return cases,files

def independent_lightmapped(cases,records):
    checks=0
    def check(b,msg):
        nonlocal checks
        require(b,msg);checks+=1
    for index,c in enumerate(cases):
        rows=[r for r in records if r['case']==index];by={r['stage']:r for r in rows};first=by[1];last=rows[-1]
        check(first['result'] and first['shader']=='LightmappedGeneric' and first['count']==len(first['values']) and first['count']>=68,'Actual Lightmapped schema publication')
        check(first['flags']==(20 if c['route']==0 else 4) and first['forward_valid'],'Source/initialized flags and forwarding')
        check(first['values'][2]['words'][2]&4,'Secondary lightmapping requirement')
        check(first['values'][9]['text']=='effects/flashlight001','Forced flashlight declaration')
        v=first['values'];expect=c['expect'];control=c['controls'];config=c['env'][7]
        check((v[66]['words'][1]&15)==(1 if control[5] else 6),'Paint normal declaration gate')
        if not control[5]:check((v[67]['words'][1]&15)==6,'Paint envmap unset when disabled')
        if expect.get('empty'):
            for i,bits in [(17,0x40800000),(20,0x3f800000),(31,0),(32,0x3f800000),(33,0x3f800000),(50,0),(52,0x3f000000),(53,0x3f000000),(56,0x3f800000),(62,0x3dcccccd),(64,0)]:check(v[i]['words'][3]==bits,'Independent registered float default '+str(i))
            check(v[21]['words'][3:7]==[0x3f800000]*4,'Detail tint defaults all four components')
            check(v[2]['words'][2]==4 and first['forward']==[0x3f800000]*7,'Empty material flags and modulation')
        if 'bump' in expect:
            b=expect['bump'];base=expect['base'];env=expect['env'];nodiff=expect['nodiff']
            check(bool(v[2]['words'][2]&8)==(b and not nodiff and not(config&0x100)),'Independent bump lighting flag')
            if b:check((v[24]['words'][1]&15)==6,'Bump unsets explicit env mask')
            if base:check(v[6]['text']==('fixture/albedo' if b and not nodiff and not(config&0x100) else 'fixture/base'),'Albedo replacement predicate')
            warn=env and bool(control[3]&255) and not(control[4]&255)
            check(first['warnings']==int(warn),'World env_cubemap diagnostic predicate')
            check((v[22]['words'][1]&15)==(1 if env and not warn and not(config&0x80 and base) else 6),'Environment map availability')
            check((v[44]['words'][1]&15)==(6 if control[1] else 1),'Lightwarp disable control')
            check((v[45]['words'][1]&15)==(6 if control[2] else 1),'Fancy blending disable control')
            check(bool(v[0]['words'][2]&0x80000000)==bool(control[0]),'Force vertex fog control')
        if expect.get('selected'):check(v[5]['words'][3]==0x3f400000 and v[6]['text']=='fixture/selected','Selected override takes precedence')
        if 'alpha' in expect:check(v[5]['words'][3]==expect['alpha'],'Shader-specific override')
        if expect.get('paintPreserve'):check(v[66]['text']=='fixture/paint' and v[67]['text']=='fixture/paintenv','Supplied paint declarations kept')
        if 'paintCopy' in expect and expect['paintCopy']:check(v[67]['text']=='fixture/paint-env','Paint envmap derives from material envmap')
        if 'fog' in expect:check(bool(v[0]['words'][2]&0x80000000)==bool(expect['fog']),'Explicit fog declaration prevents forced override')
        if expect.get('noFallback'):check(first['lookups']==['LightmappedGeneric'] and first['values'][-1]['name']=='$fallbackmaterial','Concrete shader keeps fallbackmaterial without a transition')
        if 'benchmarkPath' in expect:check(first['lookups'][0].lower()=='lightmappedgeneric' and first['warnings']==0,'Actual benchmark VMT parsed and initialized')
        if 'flags' in expect:check(v[0]['words'][2]==expect['flags'],'Absent base clears masks; decal forces no-debug-override')
        if c['repeat']:check(by[2]['values']==first['values'] and by[2]['forward']==first['forward'],'Initialized guard unchanged')
        if c['uncache']:
            check(by[3]['values']==first['values'] and by[4]['count']==0 and by[4]['forward']==first['forward'],'Preserve and clear uncache')
            check(by[5]['result'] and by[5]['shader']=='LightmappedGeneric','Reinitialize actual family')
        check(not last['alive'] and not last['values'] and not last['queued'] and last['source_freed']==int(c['route']==0),'Object/parameter/source complete teardown')
        check(first['source']==int(c['route']==0),'Only constructor source adopted')
    return checks

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    pointer=json.loads((OUT/'preflight.json').read_text())['prior'];priorPath=Path(pointer['receipt']);require(sha(priorPath)==pointer['receipt_sha256'],'Prior receipt identity');prior=json.loads(priorPath.read_text())
    allowed={str(ROOT/'work'/n) for n in CHANGED};changes={}
    for p,h in prior['sources'].items():
        if p in allowed:
            before=OUT/'before'/Path(p).relative_to(ROOT);require(sha(before)==h,'Prior frozen source '+p);changes[p]=dict(before=h,after=sha(Path(p)),diff=''.join(difflib.unified_diff(before.read_text().splitlines(True),Path(p).read_text().splitlines(True))))
        else:require(sha(Path(p))==h,'Unrelated source drift '+p)
    require(set(changes)==allowed,'Exact source edit set');(run/'source-changes.json').write_text(json.dumps(changes,indent=2))
    sources=source_closure([Path(p) for p in prior['sources']]+[ROOT/'work'/n for n in NEW]);
    for key in ['original_hashes','private_library_hashes']:
        for p,h in prior[key].items():require(sha(Path(p))==h,'Protected original '+p)
    _,ranges=check_ranges(LIBS/'stdshader_dx9.dll','stdshader_dx9-disassembly.txt',[('parameter_index_mapping',0x10028020,0x1002828f),('concrete_init',0x10028290,0x100282bb),('lightmapped_init',0x10028970,0x100290d5),('service5',0x10064b80,0x10064b8d),('service6',0x10064b90,0x10064b9d)])
    started=time.time();build=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_lightmapped_parameters.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=240);(run/'build-driver.log').write_text(build.stdout+build.stderr);require(build.returncode==0,'Fresh affected builds')
    exes=[];logs={}
    for stem,machine,tag in [('lightmapped_reference_x86',0x14c,'native'),('lightmapped_owned_x64',0x8664,'owned'),('prior_reference_x86',0x14c,'prior-native'),('prior_owned_x64',0x8664,'prior-owned'),('store_regression_x64',0x8664,'store')]:
        exe=OUT/(stem+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if machine==0x14c else 0x20b) and b'asInvoker' in raw,'Fresh architecture/manifest '+stem)
        imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require({s.lower() for s in modules}=={'kernel32.dll'},'Runtime import independence '+stem)
        exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules));log=OUT/(tag+'-build.log');require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Clean compiler '+stem);logs[str(log)]=sha(log)
    log=OUT/'asm-build.log';require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Clean assembly build');logs[str(log)]=sha(log)
    cases,files=corpus();mount=run/'mount';mount.mkdir()
    for name,contents in files.items():p=mount/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(contents,encoding='ascii')
    fixtureHashes={str(p):sha(p) for p in mount.rglob('*') if p.is_file()};query=run/'query.bin';query.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<4I',c['route'],c['repeat'],c['uncache'],c['queued'])+struct.pack('<12I',*c['env'])+text(c['name'])+packet(c['root'])+struct.pack('<6I',*[n&0xffffffff for n in c['controls']]) for c in cases));(run/'cases.json').write_text(json.dumps(cases,indent=2))
    def execute(stem,query,dest,mount):
        native=stem.endswith('x86');p=subprocess.run([str(OUT/(stem+'.exe')),str(PRIVATE if native else run/'absent-original.dll'),str(LIBS if native else run/'absent-original-directory'),str(mount),str(query),str(dest)],cwd=run,capture_output=True,text=True,timeout=180);(run/(dest.stem+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,stem+' exit='+str(p.returncode)+'; inspect log');return p.stdout+p.stderr
    reference=run/'reference.bin';execution=execute('lightmapped_reference_x86',query,reference,mount)
    require(sha(query)=='cff2e110aad3ff3937d8c5ac22044c242dd2fdd0e0012db647c779824367505c' and sha(reference)=='963e6e3f3e0a524a5cb7cc6ddb14ccf182da614c3826bdafe131432a2d7154ff','620-case input/original output pinned before safety-recorder correction')
    # Pin the original trace before owned comparison; failed runs are retained.
    (run/'reference-identity.json').write_text(json.dumps(dict(query=sha(query),reference=sha(reference),cases=len(cases)),indent=2))
    owned=run/'owned.bin';execute('lightmapped_owned_x64',query,owned,mount);a=reference.read_bytes();b=owned.read_bytes();na,ra=decode(a);nb,rb=decode(b);diff=[i for i,(x,y) in enumerate(zip(ra,rb)) if x!=y]
    (run/'comparison.json').write_text(json.dumps(dict(cases=na,records=len(ra),owned_records=len(rb),bytes=len(a),differences=len(diff),first=[dict(record=i,case=cases[ra[i]['case']],reference=ra[i],owned=rb[i]) for i in diff[:1]]),indent=2));require(na==nb==len(cases) and len(ra)==len(rb) and a==b,'Complete Lightmapped object state comparison')
    checks=independent_lightmapped(cases,ra);counts=re.search(r'native_allocations=(\d+) releases=(\d+)',execution);require(counts and counts[1]==counts[2],'Native allocations balanced')
    prefix=struct.pack('<I',549)+a[4:15271104];require(hashlib.sha256(prefix).hexdigest()=='fc367f9cebacfc8d6b1c073e002f88bb7cff0d5b513dcfea5c923a2a641c6fff','First3016 full accepted states unchanged; no native baseline replacement')
    oldrun=priorPath.parent;oldmount=oldrun/'mount';oldquery=oldrun/'query.bin';require(sha(oldquery)==prior['query_sha256'],'Prior138-case query unchanged');priorRuns=[]
    for stem in ['prior_reference_x86','prior_owned_x64']:
        dest=run/(stem+'.bin');execute(stem,oldquery,dest,oldmount);require(sha(dest)==prior['output_sha256'],'Prior800-state full corpus unchanged '+stem);priorRuns.append(dict(executable=stem,output_sha256=sha(dest)))
    safety=subprocess.run([str(OUT/'lightmapped_owned_x64.exe'),'--test'],cwd=run,capture_output=True,text=True,timeout=90);(run/'safety.log').write_text(safety.stdout+safety.stderr);require(safety.returncode==0 and 'object_parameter_safety_checks=281' in safety.stdout and 'actual_allocation_failures=254' in safety.stdout,'Prior initialization safety')
    lightSafety=subprocess.run([str(OUT/'lightmapped_owned_x64.exe'),'--test-lightmapped'],cwd=run,capture_output=True,text=True,timeout=90);(run/'lightmapped-safety.log').write_text(lightSafety.stdout+lightSafety.stderr);require(lightSafety.returncode==0 and lightSafety.stdout=='PASS lightmapped_safety_checks=400 actual_allocation_failures=389\n','Actual Lightmapped heap refusal and paint safety')
    storePrior=json.loads(Path(prior['prior_receipt']).read_text());storeQuery=Path(prior['prior_receipt']).parent/'query.txt';require(sha(storeQuery)==storePrior['query_sha256'],'Prior6064-operation input');dest=run/'store-regression.txt';execute('store_regression_x64',storeQuery,dest,mount);require(sha(dest)==storePrior['trace_sha256'],'Prior6064-operation full ownership trace unchanged')
    storeSafety=subprocess.run([str(OUT/'store_regression_x64.exe'),'--test'],cwd=run,capture_output=True,text=True,timeout=90);(run/'store-safety.log').write_text(storeSafety.stdout+storeSafety.stderr);require(storeSafety.returncode==0 and 'owned_store_safety_checks=4657' in storeSafety.stdout and 'actual_allocation_failures=6' in storeSafety.stdout,'Prior ownership safety')
    buildInfo=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());require(all(sha(ROOT/k)==h for k,h in buildInfo['editor_source_dependencies'].items()),'Editor dependencies unchanged');require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==buildInfo['executable_sha256'],'Editor unchanged')
    for p,h in sources.items():require(sha(Path(p))==h,'Source drift '+p)
    for key in ['original_hashes','private_library_hashes']:
        for p,h in prior[key].items():require(sha(Path(p))==h,'Original drift '+p)
    for p,h in fixtureHashes.items():require(sha(Path(p))==h,'Disposable input drift '+p)
    benchmarkHashes=json.loads((OUT/'benchmark-material-identities.json').read_text())
    for p,h in benchmarkHashes.items():require(sha(Path(p))==h,'Original benchmark VMT changed '+p)
    require(not (run/'absent-original.dll').exists() and not (run/'absent-original-directory').exists(),'Owned runtime absent original paths')
    capture=run/'complete-states.bin.zlib';capture.write_bytes(zlib.compress(a,9));require(zlib.decompress(capture.read_bytes())==a,'Full lossless trace evidence')
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',scope='LightmappedGeneric real-object CPU initialization and owned x64 composition',cases=len(cases),records=len(ra),exact_bytes=len(a),query_sha256=sha(query),output_sha256=sha(reference),independent_checks=checks,native_allocations=int(counts[1]),native_releases=int(counts[2]),safety=safety.stdout,prior_parameter_runs=priorRuns,store_regression=dict(operations=6064,trace_sha256=sha(dest),safety=storeSafety.stdout),sources=sources,source_changes=changes,original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],fixture_hashes=fixtureHashes,executables=exes,build_logs=logs,static_ranges=ranges,prior_receipt=str(priorPath),prior_receipt_sha256=sha(priorPath),editor_dependencies_unchanged=len(buildInfo['editor_source_dependencies']),editor_sha256=buildInfo['executable_sha256'],limits=['CPU parameter InitParams only; no texture loading, proxies, draw/GPU/editor integration or renderer equivalence','Shader-init service slots5/6 are explicit observed return inputs, not claimed reconstructed renderer services','Native getter string conversion outside existing string values refuses transactionally','Owner-thread serialization and existing proxy/format/refusal boundaries remain; complete goal incomplete','No live SFM, Blender, original writes or full editor build/run'])
    result.update(lightmapped_safety=lightSafety.stdout,benchmark_material_hashes=benchmarkHashes,benchmark_unique_materials=len(benchmarkHashes),first_549_cases_unchanged=True)
    dest=run/'results.json';dest.write_text(json.dumps(result,indent=2)+'\n');replay=OUT/('replay-'+run.name.removeprefix('verification-'));replay.mkdir()
    for p in [Path(p) for p in sources]+[Path(e['path']) for e in exes]+[Path(p) for p in logs]+[dest,query,capture,OUT/'preflight.json',OUT/'schema.txt',OUT/'discovery_x86.exe',OUT/'discovery-build.log']+list(run.glob('*.json'))+list(run.glob('*.log'))+[Path(p) for p in fixtureHashes]+[OUT/'before/work'/n for n in CHANGED]:
        t=replay/p.relative_to(ROOT);t.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,t)
    manifest={p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()};(replay/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');pointer=dict(receipt=str(dest),receipt_sha256=sha(dest),replay=str(replay),replay_manifest_sha256=sha(replay/'manifest.json'));(OUT/'latest-verification.json').write_text(json.dumps(pointer,indent=2)+'\n');print(json.dumps(dict(status='PASS',receipt=str(dest),cases=len(cases),records=len(ra),exact_bytes=len(a),checks=checks)))

if __name__=='__main__':main()
