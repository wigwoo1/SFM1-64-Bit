"""UnlitTwoTexture CPU parameters; full pilot proxies remain safely refused.

No live SFM/GPU. Private unchanged original code and disposable workspace files.
"""
from pathlib import Path
from datetime import datetime,timezone
import difflib,hashlib,json,re,shutil,struct,subprocess,tempfile,time,zlib
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_material_keyvalues_parser import DUMP
from verify_builtin_error_material_seed import source_closure
from verify_material_object_parameters import node,string,integer,packet,text,render,decode
from verify_ifm_draw_packet import check_ranges

OUT=ROOT/'outputs/unlit-two-texture-parameters-20261026'
LIBS=ROOT/'outputs/material-fallback-owned-20261026/verification-0mhr77s9/copied-libraries'
CHANGED=['material_parameter_orchestration_candidate.h','probe_material_object_parameters.cpp']
NEW=['unlit_two_texture_parameter_schema_candidate.h','unlit_two_texture_init_candidate.h',
     'unlit_two_texture_native_fixture.h','probe_unlit_two_texture_parameters.cpp',
     'probe_unlit_two_texture_discovery.cpp','build_unlit_two_texture_discovery.cmd',
     'build_unlit_two_texture_parameters.cmd','verify_unlit_two_texture_parameters.py']

def project_proxy_block(source):
    # Retain every original byte outside the one root Proxies declaration.
    # This is an explicitly labelled CPU test projection, not an import fix.
    tokens=[(m.group(),m.start(),m.end()) for m in re.finditer(r'//[^\n]*|"[^"\n]*"|[{}]|[^\s{}"]+',source) if not m.group().startswith('//')]
    depth=0;start=end=None
    for i,(token,a,b) in enumerate(tokens):
        if token=='{':depth+=1
        elif token=='}':depth-=1
        elif token.strip('"').lower()=='proxies' and depth==1:
            require(start is None and tokens[i+1][0]=='{','One root Proxies block required');start=a;nest=0
            for t,_,finish in tokens[i+1:]:
                if t=='{':nest+=1
                elif t=='}':
                    nest-=1
                    if nest==0:end=finish;break
            break
    require(start is not None and end is not None,'Complete pilot proxy projection boundary')
    result=source[:start]+source[end:];require('"Proxies"' not in result,'Proxy block isolated')
    return result,dict(start=start,end=end,removed_bytes=end-start,outside_bytes_preserved=True)

def corpus():
    cases=[];files={}
    def add(label,shader,route,fields=(),env=None,controls=None,expect=None):
        i=len(cases);root=node(shader,list(fields));name='two/case'+str(i)
        c=dict(label=label,route=route,repeat=i%2,uncache=(i//2)%2,queued=(i//4)%2,name=name,root=root,env=env or [0,0,0,0,0,0,0,0,1,0,92,0],controls=controls or [0,0,0,0,0,0],expect=expect or {},shader='UnlitTwoTexture_DX9');cases.append(c)
        if route==2:files['materials/'+name+'.vmt']=render(root)
        return c
    for route in range(3):
        for shader in ['UnlitTwoTexture','UnlitTwoTexture_DX9']:
            add('empty',shader,route,expect={'enabled':False,'empty':True})
            for enabledValue,enabled in [('0',False),('1',True),('-1',True),('.5',False),('256',True),('garbage',False),('[1 .5 0]',True),('0.000',False)]:
                for supplied in range(8):
                    fields=[string('$basetexture','fixture/base'),string('$texture2','fixture/second'),string('$alpha','.375'),string('$color','[.25 .5 1]'),string('$color2','[.5 1 .25]'),string('$srgbtint','[.5 .25 1]'),string('$cloakpassenabled',enabledValue)]
                    if supplied&1:fields+=[string('$cloakfactor','.25')]
                    if supplied&2:fields+=[string('$cloakcolortint','[.25 .5 1 .75]')]
                    if supplied&4:fields+=[string('$refractamount','.625')]
                    env=[supplied%3,supplied%2,(supplied//2)%2,0,0,0,0,[0,0x80,0x100,0x180][supplied%4],1,1,[80,90,92,95][supplied%4],supplied%2]
                    add('cloak-grid',shader,route,fields,env,[1,1,1,1,0,1],dict(enabled=enabled,supplied=supplied,base=True))
            names=[l.split('\t')[1] for l in (OUT/'schema.txt').read_text().split('shader UnlitTwoTexture_DX9')[1].splitlines() if '\t' in l]
            for index,name in enumerate(names):
                if index<4:continue
                for value in ['.375','[.25 .5 1]']:add('declared-'+str(index),shader,route,[string(name,value)])
            add('typed-cloak-int',shader,route,[integer('$cloakpassenabled',1)],expect={'enabled':True})
            add('second-transform',shader,route,[string('$basetexturetransform','center .5 .5 scale 2 3 rotate 30 translate .1 .2'),string('$texture2transform','center 0 0 scale 3 2 rotate -15 translate .2 .3')],expect={'transforms':True})
            add('dynamic-names',shader,route,[string('$basetexture2','fixture/not-texture2'),string('$phong','1'),string('$bumpmap','fixture/normal')],expect={'dynamic':True})
            add('flags-preserved',shader,route,[string('$additive','1'),string('$selfillum','1'),string('$basealphaenvmapmask','1'),string('$vertexfog','0')],controls=[1,0,0,0,0,0],expect={'flags':0x1000c0})
            add('selected-override',shader,route,[string('$alpha','.25'),node('GPU<1',[string('$alpha','.75'),string('$texture2','fixture/selected')])],expect={'alpha':0x3f400000})
            if shader=='UnlitTwoTexture':add('concrete-override',shader,route,[string('$alpha','.25'),node('UnlitTwoTexture_DX9',[string('$alpha','.625')])],env=[2,0,0,0,0,0,0,0,1,0,92,0],expect={'alpha':0x3f200000})
            else:add('concrete-ignores-fallback',shader,route,[string('$fallbackmaterial','must_not_load')],expect={'noFallback':True})
        for request,target in [('two_target','UnlitTwoTexture_DX9'),('two_chain','UnlitTwoTexture_DX9'),('unlit_target','UnlitGeneric'),('missing_two_fallback','UnlitGeneric')]:
            c=add('alias-fallback-'+request,'UnlitTwoTexture',route,[string('$fallbackmaterial',request)]);c['shader']=target
        owner='two/self'+str(route);c=add('alias-self-fallback','UnlitTwoTexture',route,[string('$fallbackmaterial',owner)]);c['name']=owner;c['shader']='UnlitGeneric'
        if route==2:del files['materials/two/case'+str(len(cases)-1)+'.vmt'];files['materials/'+owner+'.vmt']=render(c['root'])
    files['materials/two_target.vmt']='UnlitTwoTexture_DX9 { $alpha .5 $basetexture fixture/loaded $texture2 fixture/second $cloakpassenabled 1 }'
    files['materials/two_chain.vmt']='UnlitTwoTexture { $fallbackmaterial two_target }'
    files['materials/unlit_target.vmt']='UnlitGeneric { $alpha .25 }'
    identities=json.loads((OUT/'preflight.json').read_text())['benchmark_material_hashes'];require(len(identities)==1,'One actual pilot source')
    path,h=next(iter(identities.items()));original=Path(path);require(sha(original)==h,'Original pilot identity');source=original.read_bytes().decode('ascii');projection,extent=project_proxy_block(source)
    c=add('pilot-cpu-projection','UnlitTwoTexture',2,expect={'pilotProjection':True,'enabled':True});files['materials/'+c['name']+'.vmt']=projection
    return cases,files,source,dict(original=str(original),original_sha256=h,projection_sha256=hashlib.sha256(projection.encode('ascii')).hexdigest(),projection_extent=extent,full_material_supported=False,reason='Nonempty Proxies remains transactionally refused; projection tests CPU declarations only')

def independent_two(cases,records):
    checks=0
    def check(b,msg):
        nonlocal checks
        require(b,msg);checks+=1
    for index,c in enumerate(cases):
        rows=[r for r in records if r['case']==index];by={r['stage']:r for r in rows};first=by[1];last=rows[-1];v=first['values'];expect=c['expect'];two=c['shader']=='UnlitTwoTexture_DX9';alias=c['root']['key']=='UnlitTwoTexture'
        check(first['result'] and first['shader']==c['shader'] and first['count']==len(v) and first['count']>=(20 if two else 68),'Concrete schema publication')
        check(first['flags']==(20 if c['route']==0 else 4) and first['forward_valid'],'Initialized/source flags and forwarding')
        if two:
            check(v[2]['words'][2]&0x40000,'Per-frame snapshot requirement')
            if alias:check(first['lookups']==(['UnlitTwoTexture','UnlitTwoTexture','UnlitTwoTexture_DX9'] if c['label']=='alias-fallback-two_chain' else ['UnlitTwoTexture','UnlitTwoTexture_DX9']),'Alias recollects every loaded root')
            else:check(first['lookups']==['UnlitTwoTexture_DX9'],'Concrete shader does not transition')
        if 'enabled' in expect:
            enabled=expect['enabled'];supplied=expect.get('supplied',0)
            check(v[2]['words'][2]==(0x40040 if enabled else 0x40000),'Cloak tangent flag')
            check(bool(v[0]['words'][2]&0x800)==(enabled or bool(expect.get('pilotProjection'))),'Enabled cloak adds model flag')
            check(v[17]['words'][3]==(0x3e800000 if supplied&1 else 0),'Factor supplied/default')
            check(v[19]['words'][3]==(0x3f200000 if supplied&4 else 0x3dcccccd if enabled else 0),'Refract supplied/default conditional')
            if not supplied&2:check(v[18]['words'][3:7]==([0x3f800000]*4 if enabled else [0x3f800000]*3+[0]),'Cloak vec4 versus completion vec3 default')
        if expect.get('empty'):check(first['count']==20 and first['forward']==[0x3f800000]*7 and (v[13]['words'][1]&15)==6,'Empty material defaults, second texture remains undefined')
        if expect.get('base'):check(v[6]['text']=='fixture/base' and v[13]['text']=='fixture/second','Both texture declarations retained independently')
        if expect.get('transforms'):check(v[8]['words'][8]==1 and v[15]['words'][8]==1 and v[8]['words'][12:]!=v[15]['words'][12:],'Separate typed transforms preserved')
        if expect.get('dynamic'):check(first['count']==23 and (v[13]['words'][1]&15)==6 and [x['name'] for x in v[20:]]==['$basetexture2','$phong','$bumpmap'],'Dynamic names cannot become declared fields')
        if 'flags' in expect:check(v[0]['words'][2]==expect['flags'],'Family preserves authored flags without unrelated generic fog/mask changes')
        if 'alpha' in expect:check(v[5]['words'][3]==expect['alpha'],'Selected override alpha')
        if expect.get('noFallback'):check(v[-1]['name']=='$fallbackmaterial','Concrete keeps unused fallback declaration')
        if expect.get('pilotProjection'):check(v[6]['text']=='models/effects/pyro/pilotlight' and v[13]['text']=='models/effects/pyro/pilotlight_motion' and v[0]['words'][2]&0x80,'CPU projection retains actual texture names and additive flag')
        if c['repeat']:check(by[2]['values']==v and by[2]['forward']==first['forward'],'Initialized guard')
        if c['uncache']:check(by[3]['values']==v and by[4]['count']==0 and by[4]['forward']==first['forward'] and by[5]['shader']==c['shader'],'Uncache/reinitialization')
        check(not last['alive'] and not last['values'] and not last['queued'] and last['source_freed']==int(c['route']==0),'Complete material/parameter/source lifetime')
    return checks

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    preflight=json.loads((OUT/'preflight.json').read_text());pointer=preflight['prior'];priorPath=Path(pointer['receipt']);require(sha(priorPath)==pointer['receipt_sha256'],'Prior world receipt');prior=json.loads(priorPath.read_text());changes={};allowed={str(ROOT/'work'/n) for n in CHANGED}
    for p,h in prior['sources'].items():
        if p in allowed:
            before=OUT/'before'/Path(p).relative_to(ROOT);require(sha(before)==h,'Prior frozen source');changes[p]=dict(before=h,after=sha(Path(p)),diff=''.join(difflib.unified_diff(before.read_text().splitlines(True),Path(p).read_text().splitlines(True))))
        else:require(sha(Path(p))==h,'Unrelated source '+p)
    require(set(changes)==allowed,'Exact edit set');(run/'source-changes.json').write_text(json.dumps(changes,indent=2));sources=source_closure([Path(p) for p in prior['sources']]+[ROOT/'work'/n for n in NEW])
    benchmark=dict(prior['benchmark_material_hashes'],**preflight['benchmark_material_hashes'])
    for group in [prior['original_hashes'],prior['private_library_hashes'],benchmark]:
        for p,h in group.items():require(sha(Path(p))==h,'Protected original '+p)
    _,ranges=check_ranges(LIBS/'stdshader_dx9.dll','stdshader_dx9-disassembly.txt',[('alias_noop_init',0x1000f6e0,0x1000f6e3),('alias_fallback',0x1004ccc0,0x1004ccc8),('concrete_no_fallback',0x10041370,0x10041375),('two_texture_initializer',0x1004cdb0,0x1004ce54),('cloak_parameter_helper',0x10008fa0,0x100090e2)])
    started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_unlit_two_texture_parameters.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=240);(run/'build-driver.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Fresh two-texture/prior builds');exes=[];logs={}
    targets=[('two_reference_x86',0x14c,'native'),('two_owned_x64',0x8664,'owned'),('world_reference_x86',0x14c,'world-native'),('world_owned_x64',0x8664,'world-owned'),('prior_reference_x86',0x14c,'prior-native'),('prior_owned_x64',0x8664,'prior-owned'),('light_reference_x86',0x14c,'light-native'),('light_owned_x64',0x8664,'light-owned'),('store_regression_x64',0x8664,'store')]
    for stem,machine,tag in targets:
        exe=OUT/(stem+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if machine==0x14c else 0x20b) and b'asInvoker' in raw,'Fresh PE architecture '+stem)
        imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require({s.lower() for s in modules}=={'kernel32.dll'},'Runtime imports '+stem);exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
        log=OUT/(tag+'-build.log');require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Clean compiler '+stem);logs[str(log)]=sha(log)
    log=OUT/'asm-build.log';require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Clean assembly');logs[str(log)]=sha(log)
    cases,files,pilotSource,pilotProjection=corpus();mount=run/'mount';mount.mkdir()
    for name,contents in files.items():p=mount/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(contents.encode('ascii'))
    pilotCopy=mount/'materials/pilot-original.vmt';pilotCopy.write_bytes(pilotSource.encode('ascii'))
    projectionCase=next(c for c in cases if c['label']=='pilot-cpu-projection');projectionFile=mount/('materials/'+projectionCase['name']+'.vmt')
    require(sha(pilotCopy)==pilotProjection['original_sha256'] and sha(projectionFile)==pilotProjection['projection_sha256'],'Pilot copy and projection preserve exact source bytes, including CRLF')
    pilotProjection.update(disposable_full_copy_sha256=sha(pilotCopy),disposable_projection_sha256=sha(projectionFile),full_copy_byte_identical=True)
    (run/'pilot-projection.json').write_text(json.dumps(pilotProjection,indent=2));fixtureHashes={str(p):sha(p) for p in mount.rglob('*') if p.is_file()};query=run/'query.bin';query.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<4I',c['route'],c['repeat'],c['uncache'],c['queued'])+struct.pack('<12I',*c['env'])+text(c['name'])+packet(c['root'])+struct.pack('<6I',*[n&0xffffffff for n in c['controls']]) for c in cases));(run/'cases.json').write_text(json.dumps(cases,indent=2))
    def execute(stem,query,dest,mount):
        native=stem.endswith('x86');p=subprocess.run([str(OUT/(stem+'.exe')),str(PRIVATE if native else run/'absent-original.dll'),str(LIBS if native else run/'absent-original-directory'),str(mount),str(query),str(dest)],cwd=run,capture_output=True,text=True,timeout=180);(run/(dest.stem+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,stem+' exit='+str(p.returncode)+'; inspect log');return p.stdout+p.stderr
    reference=run/'reference.bin';execution=execute('two_reference_x86',query,reference,mount);(run/'reference-identity.json').write_text(json.dumps(dict(query=sha(query),reference=sha(reference),cases=len(cases)),indent=2))
    require(sha(query)=='2e0cceac10a3522dbe7bd7621139acf2822542986ffc9fc99c97c73c70d2981e' and sha(reference)=='7acb2bb5fa925795c02d894ddf3d3e54210e216afba4f853e7cb08383ccf4659','Original634-case input/output preserved after byte-copy correction')
    owned=run/'owned.bin';execute('two_owned_x64',query,owned,mount);a=reference.read_bytes();b=owned.read_bytes();na,ra=decode(a);nb,rb=decode(b);diff=[i for i,(x,y) in enumerate(zip(ra,rb)) if x!=y]
    (run/'comparison.json').write_text(json.dumps(dict(cases=na,records=len(ra),owned_records=len(rb),bytes=len(a),differences=len(diff),first=[dict(record=i,case=cases[ra[i]['case']],reference=ra[i],owned=rb[i]) for i in diff[:1]]),indent=2));require(na==nb==len(cases) and len(ra)==len(rb) and a==b,'Complete UnlitTwoTexture alias/concrete CPU states')
    checks=independent_two(cases,ra);counts=re.search(r'native_allocations=(\d+) releases=(\d+)',execution);require(counts and counts[1]==counts[2],'Native allocations balanced')
    regressions=[]
    lightPath=Path(prior['prior_receipt']);light=json.loads(lightPath.read_text());priorParametersPath=Path(light['prior_receipt']);priorParameters=json.loads(priorParametersPath.read_text())
    for previous,previousPath,stems in [(prior,priorPath,['world_reference_x86','world_owned_x64']),(light,lightPath,['light_reference_x86','light_owned_x64']),(priorParameters,priorParametersPath,['prior_reference_x86','prior_owned_x64'])]:
        oldrun=previousPath.parent;oldquery=oldrun/'query.bin';require(sha(oldquery)==previous['query_sha256'],'Prior input identity')
        for stem in stems:
            dest=run/(stem+'.bin');execute(stem,oldquery,dest,oldrun/'mount');require(sha(dest)==previous['output_sha256'],'Entire prior logical corpus unchanged '+stem);regressions.append(dict(executable=stem,cases=previous['cases'],records=previous['records'],output_sha256=sha(dest)))
    safety={}
    for exe,arg,key in [('two_owned_x64','--test-unlit-two-texture','two'),('world_owned_x64','--test-world-transition','world'),('light_owned_x64','--test-lightmapped','lightmapped'),('prior_owned_x64','--test','parameters'),('store_regression_x64','--test','ownership')]:
        p=subprocess.run([str(OUT/(exe+'.exe')),arg],cwd=run,capture_output=True,text=True,timeout=120);(run/(key+'-safety.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,'Owned safety '+key);safety[key]=p.stdout
    require(safety['two']=='PASS unlit_two_texture_safety_checks=499 actual_allocation_failures=488\n' and all(safety[k]==prior['safety'][k] for k in ['world','lightmapped','parameters','ownership']),'Exact affected safety counts')
    pilotRefusal=subprocess.run([str(OUT/'two_owned_x64.exe'),'--test-pilot-refusal',str(pilotCopy)],cwd=run,capture_output=True,text=True,timeout=30);(run/'pilot-refusal.log').write_text(pilotRefusal.stdout+pilotRefusal.stderr);require(pilotRefusal.returncode==0 and pilotRefusal.stdout=='PASS untouched_pilot_proxy_refusal=1\n','Complete untouched pilot declaration is read and refused without mutation')
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
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',scope='UnlitTwoTexture alias/concrete CPU initialization; original pilot proxies remain refused',cases=len(cases),records=len(ra),exact_bytes=len(a),query_sha256=sha(query),output_sha256=sha(reference),independent_checks=checks,native_allocations=int(counts[1]),native_releases=int(counts[2]),safety=safety,regressions=regressions,store_regression=dict(operations=6064,trace_sha256=sha(dest)),sources=sources,source_changes=changes,original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],benchmark_material_hashes=benchmark,pilot_material_hashes=preflight['benchmark_material_hashes'],pilot_projection=pilotProjection,pilot_refusal=pilotRefusal.stdout,fixture_hashes=fixtureHashes,executables=exes,build_logs=logs,static_ranges=ranges,prior_receipt=str(priorPath),prior_receipt_sha256=sha(priorPath),editor_dependencies_unchanged=len(graph),editor_sha256=buildInfo['executable_sha256'],limits=['CPU InitParams only; no GPU textures/proxies/resource Init, editor/render integration or visual parity','Actual13-field alias/20-field concrete and enabled cloak CPU defaults; no proxy animation, additive draw or visible cloak implementation','Original text remains RX/unchanged with existing controlled host/file/shader-init/cvar services; slot5/6 semantic renderer integration still unproven','Full original pilot material has nonempty Proxies and still refuses; only explicitly isolated CPU declarations are compared','Owner-thread serialized API, existing proxy and file-format limits; full goal incomplete; no live SFM/Blender or original writes'])
    dest=run/'results.json';dest.write_text(json.dumps(result,indent=2)+'\n');replay=OUT/('replay-'+run.name.removeprefix('verification-'));replay.mkdir()
    for p in [Path(p) for p in sources]+[Path(e['path']) for e in exes]+[Path(p) for p in logs]+[dest,query,capture,OUT/'preflight.json',OUT/'schema.txt',OUT/'discovery_x86.exe',OUT/'discovery-build.log']+list(run.glob('*.json'))+list(run.glob('*.log'))+[Path(p) for p in fixtureHashes]+[OUT/'before/work'/n for n in CHANGED]:
        t=replay/p.relative_to(ROOT);t.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,t)
    manifest={p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()};(replay/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');pointer=dict(receipt=str(dest),receipt_sha256=sha(dest),replay=str(replay),replay_manifest_sha256=sha(replay/'manifest.json'));(OUT/'latest-verification.json').write_text(json.dumps(pointer,indent=2)+'\n');print(json.dumps(dict(status='PASS',receipt=str(dest),cases=len(cases),records=len(ra),exact_bytes=len(a),checks=checks)))

if __name__=='__main__':main()
