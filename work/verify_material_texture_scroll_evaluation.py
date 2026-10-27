"""Actual TextureScroll initialization and live transactional CPU evaluation.
Original methods remain unchanged RX; originals/sessions are read-only.
"""
import json,random,re,shutil,struct,subprocess,tempfile,time,zlib
from pathlib import Path
from datetime import datetime,timezone
import verify_material_texture_scroll_math as maths
from verify_builtin_error_material_seed import source_closure
from verify_material_object_parameters import decode as decode_cpu
from verify_ifm_draw_packet import check_ranges
ROOT=maths.ROOT;OUT=ROOT/'outputs/material-texture-scroll-evaluation-20261027';old=maths.old;sha=old.sha;require=old.require
CHANGED=['material_proxy_evaluation_candidate.h','material_sine_proxy_candidate.h','build_material_sine_evaluation.cmd','build_material_matrix_write.cmd','build_material_texture_transform_evaluation.cmd']
NEW=['material_texture_scroll_proxy_candidate.h','material_texture_scroll_client_native_fixture.h','probe_material_texture_scroll_initialization.cpp','probe_material_texture_scroll_evaluation.cpp','build_material_texture_scroll_evaluation.cmd','verify_material_texture_scroll_evaluation.py']

def initialization_corpus():
    cases=[]
    def add(label,fields):cases.append(dict(label=label,declaration=old.node('TextureScroll',fields)))
    for key in ['textureScrollRate','textureScrollAngle','textureScale']:
        for value in [None,'',' ','0','1.5','-.25','$drive','$drive[1]','$drive[2]','$missing','$missing[1]','$missing[999]']:
            fields=[old.string('textureScrollVar','$out')]
            if value is not None:fields.append(old.string(key,value))
            add(key+':'+str(value),fields)
        for kind,bits in [(0,0),(2,0xfffffffe),(3,0x3e800000)]:add(key+':kind'+str(kind),[old.string('textureScrollVar','$out'),old.node(key,kind=kind,bits=bits)])
    for target in [None,'','$missing','$out[1]','$out[0]','$OUT',' $out','$out ']:add('result:'+str(target),[] if target is None else [old.string('textureScrollVar',target)])
    return cases

def corpus():
    cases=[];f=old.fbits
    def scroll(target='$out',**fields):return old.node('TextureScroll',[old.string('textureScrollVar',target)]+[old.string(k,v) if isinstance(v,str) else old.node(k,kind=3,bits=f(v)) for k,v in fields.items()])
    def sine(target,**fields):return old.node('Sine',[old.string('resultVar',target)]+[old.string(k,v) if isinstance(v,str) else old.node(k,kind=3,bits=f(v)) for k,v in fields.items()])
    def transform(target='$out',**fields):return old.node('TextureTransform',[old.string('resultVar',target)]+[old.string(k,v) for k,v in fields.items()])
    def add(label,proxies=None,out='float',extra=None,expected=True,counts=None):
        values=[old.string('$out','[1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1]') if out=='matrix' else old.literal('$out',out),old.string('$drive','[.25 -.5 .75 1]'),old.string('$scale','[1.5 .5]'),old.string('$temp','[0 0]'),old.string('$pilot','0.0'),old.string('$pilot2','0.0'),*(extra or []),old.node('Proxies',proxies or [scroll()])]
        cases.append(dict(label=label,root=old.node('UnlitTwoTexture',values),times=[f(0),f(.25),f(-.125)],missing=[[0]*4,[f(.25),f(-.5),f(.75),f(1)],[f(-.5),f(2),0,0]],counts=counts or [4,2,0],expected=expected))
    fields=['textureScrollRate','textureScrollAngle','textureScale']
    for out in ['float','int','string','vec1','vec2','vec3','vec4','matrix']:
        for mask in range(8):add('optional-input-mask',[scroll(**{fields[i]:[.75,30.,1.5][i] for i in range(3) if mask&(1<<i)})],out)
    for key in fields:
        for value in ['',' ','0','1.5','-.25','1e+','nan','inf','$drive','$DRIVE[1]','$drive[2]','$drive[3]','$drive[4]','$missing','$missing[0]','$missing[1]','$missing[3]','$missing[4]','$missing[999]',' $drive','$drive ']:
            for out in ['float','matrix']:add('input-default-live-or-missing',[scroll(**{key:value})],out)
        for kind,bits in [(0,0),(2,0xfffffffe),(3,0x3e800000)]:
            d=scroll();d['children'].append(old.node(key,kind=kind,bits=bits));add('typed-input',[d])
    for target in ['$missing','','$out[0]',' $out','$out ']:add('literal-result-refusal',[scroll(target)],expected=False)
    add('missing-result-key',[old.node('TextureScroll')],expected=False)
    add('literal-bracket-result',[scroll('$out[1]')],extra=[old.string('$out[1]','[.25 .5]')])
    for target in ['$OUT','$alpha','$color','$basetexture','$texture2','$basetexturetransform']:add('other-found-results',[scroll(target,textureScrollRate=.75)])
    for size in [255,256,301,511,1023]:
        name='$'+'r'*(size-1);add('long-literal-result',[scroll(name)],extra=[old.node(name,kind=3,bits=f(.25))])
    for key in fields:add('live-driver',[sine('$drive',sinemin=.25,sinemax=1.5),scroll(**{key:'$drive'})],out='matrix')
    add('equals-live-rate',[sine('$pilot',sinemin=.25,sinemax=.75),old.declaration('$drive','$pilot'),scroll(textureScrollRate='$drive')])
    add('current-result-type',[transform(scaleVar='$scale'),scroll(),sine('$out',sinemin=.25,sinemax=.75),scroll()])
    add('vector-to-matrix',[scroll(),transform(scaleVar='$scale'),scroll()])
    for key in fields:add('result-input-alias',[scroll(**{key:'$out'})],out='vec2')
    add('all-shared-inputs',[scroll(**{key:'$missing'+str(i)+'[1]' for i,key in enumerate(fields)})],out='matrix')
    for count in range(5):add('shared-component-count',[scroll(textureScrollRate='$missing[1]')],out='matrix',counts=[count]*3)
    rng=random.Random(2026102713)
    for _ in range(256):add('seeded-live-values',[sine('$pilot',sinemin=-1.,sinemax=1.),scroll(textureScrollRate=rng.uniform(-8,8),textureScrollAngle=rng.uniform(-720,720),textureScale=rng.uniform(-4,4))],out=rng.choice(['float','matrix']))
    pilot=next(Path(n) for n in json.loads(Path(json.loads((maths.OUT/'latest-verification.json').read_text())['receipt']).read_text())['benchmark_material_hashes'] if n.lower().endswith('pilotlight.vmt'));prefix=[]
    for name,block in re.findall(r'"(Sine|Equals|TextureTransform|TextureScroll)"\s*\{([^{}]*)\}',pilot.read_text()):
        entries=[]
        for key,quoted,token in re.findall(r'"([^"\r\n]+)"\s+(?:"([^"\r\n]*)"|([^\s{}]+))',block):entries.append(old.string(key,quoted) if quoted else old.node(key,kind=3,bits=f(float(token))))
        prefix.append(old.node(name,entries))
    require([p['key'] for p in prefix]==['Sine','Sine','Equals','Equals','TextureTransform','TextureScroll'],'Actual six-declaration pilot prefix');add('actual-six-proxy-pilot-prefix',prefix)
    return cases

def main():
    pre=json.loads((OUT/'preflight.json').read_text());ptr=pre['prior'];parent=Path(ptr['receipt']);require(sha(parent)==ptr['receipt_sha256'],'Pinned prior receipt');prior=json.loads(parent.read_text());frozen=Path(ptr['replay']);require(sha(frozen/'manifest.json')==ptr['replay_manifest_sha256'],'Prior frozen manifest');require(all(sha(frozen/n)==h for n,h in json.loads((frozen/'manifest.json').read_text()).items()),'Prior frozen files')
    changed=[Path(n).name for n,h in prior['sources'].items() if sha(n)!=h];require(set(changed)==set(CHANGED),'Only bounded frame/factory/build-link changes');require(all(sha(OUT/'before/work'/n)==prior['sources'][str(ROOT/'work'/n)] for n in CHANGED),'Before snapshots');sources=source_closure([Path(n) for n in prior['sources']]+[ROOT/'work'/n for n in NEW]);protected=[prior[k] for k in ['original_hashes','private_library_hashes','benchmark_material_hashes','fixture_hashes']]
    for group in protected:require(all(sha(n)==h for n,h in group.items()),'Protected inputs before')
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));mount=run/'mount';mount.mkdir();print('Verification directory: '+str(run),flush=True)
    _,ranges=check_ranges(old.CLIENT,'../outputs/material-proxy-evaluation-20261027/client-disassembly-private.txt',[('scroll_Init',0x10198ab0,0x10198b58),('scroll_OnBind',0x10198b80,0x10198d40),('FloatInput_Init',0x100e60d0,0x100e6212),('FloatInput_Get',0x100e5d20,0x100e5da7)])
    started=time.time();b=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_texture_scroll_evaluation.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=720);(run/'build-driver.log').write_text(b.stdout+b.stderr);require(b.returncode==0,'Fresh36 test builds')
    tags={'sine_math':'sine-math','sine':'sine','float_input':'float-input','equals':'equals','component':'component','matrix_value':'matrix-value','texture_evaluation':'texture','texture_transform':'texture-math','scroll_evaluation':'scroll','cosine_double':'cosine','texture_scroll':'scroll-math'}
    stems=['sine_math','sine','float_input','equals','component','material_proxy_lifetime','unlit_two_texture_parameters','world_transition_parameters','lightmapped_parameters','material_object_parameters','matrix_value','texture_evaluation','texture_transform','scroll_evaluation','cosine_double','texture_scroll'];targets=[(s,n,tags.get(s,s)+('-native' if n else '-owned')) for s in stems for n in [True,False]]+[('material_store',False,'material_store-owned'),('matrix_store_safety',False,'matrix-store'),('texture_input',True,'texture-input-native'),('scroll_initialization',True,'scroll-init-native')];executables=[];logs={}
    def exe(stem,native):return OUT/('matrix_store_safety_x64.exe' if stem=='matrix_store_safety' else stem+('_reference_x86.exe' if native else '_owned_x64.exe'))
    for stem,native,tag in targets:
        p=exe(stem,native);raw=p.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];machine=0x14c if native else 0x8664;require(p.stat().st_mtime>=started-2 and raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if native else 0x20b) and b'asInvoker' in raw,'Fresh PE architecture '+p.name);imp=subprocess.run([str(old.DUMP),'/imports',str(p)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imp,re.I|re.M);require({s.lower() for s in modules}=={'kernel32.dll'},'Direct imports '+p.name);executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine),direct_imports=modules));log=OUT/(tag+'-build.log');require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Warning-free '+log.name);logs[str(log)]=sha(log)
    logs[str(OUT/'asm-build.log')]=sha(OUT/'asm-build.log')
    def execute(stem,native,args,label):
        p=subprocess.run([str(exe(stem,native)),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' exit='+str(p.returncode)+' inspect '+str(run/(label+'.log')));return p.stdout
    ic=initialization_corpus();iq=run/'initialization-query.bin';iq.write_bytes(struct.pack('<I',len(ic))+b''.join(old.packet(c['declaration']) for c in ic));require(iq.read_bytes()==(OUT/'init-exploration/query.bin').read_bytes(),'Retained initialization corpus');io=run/'initialization-native.bin';init_trace=execute('scroll_initialization',True,[old.CLIENT,iq,io],'initialization-native');require(io.read_bytes()==(OUT/'init-exploration/native.bin').read_bytes(),'All53 original initialization observations unchanged');require('initialized=47 ignored_input_failures=15 allocations=53 releases=53' in init_trace,'Defaults/ignored-failure initialization contract')
    cases=corpus();query=run/'query.bin';query.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<18I',*(c['times']+sum(c['missing'],[])+c['counts']))+old.packet(c['root']) for c in cases));(run/'cases.json').write_text(json.dumps(cases,indent=2));cpu_query=run/'cpu-query.bin';cpu_query.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<4I',0,0,0,0)+struct.pack('<12I',0,0,0,0,0,0,0,0,1,0,92,0)+old.text('equals/case')+old.packet(c['root'])+struct.pack('<6I',0,0,0,0,0,0) for c in cases));cpu_out=run/'cpu-original.bin';cpu_trace=execute('unlit_two_texture_parameters',True,[old.PRIVATE,old.LIBS,mount,cpu_query,cpu_out],'cpu-original');n,records=decode_cpu(cpu_out.read_bytes());initial=[r for r in records if r['stage']==1];require(n==len(cases)==len(initial) and all(s['result'] for s in initial),'All separately original CMaterial CPU states');prepared=run/'prepared-original.bin';prepared.write_bytes(struct.pack('<I',n)+b''.join(old.text(s['shader'])+struct.pack('<I',len(s['values']))+b''.join(struct.pack('<28I',*v['words'])+v['text'].encode()+v['name'].encode() for v in s['values']) for s in initial))
    paths=[];native_trace=''
    for native,tail,label in [(True,[],'native'),(True,['--sse2'],'native-sse2'),(False,[],'owned')]:
        dest=run/(label+'.bin');trace=execute('scroll_evaluation',native,[old.CLIENT if native else run/'absent.dll',query,prepared,dest,*tail],label);paths.append(dest)
        if label=='native':native_trace=trace
    if not all(p.read_bytes()==paths[0].read_bytes() for p in paths):
        a,b=maths.previous.decode(paths[0].read_bytes()),maths.previous.decode(paths[2].read_bytes());diff=[i for i,(x,y) in enumerate(zip(a,b)) if x!=y];i=diff[0] if diff else 0;(run/'difference.json').write_text(json.dumps(dict(indices=diff,case=cases[i],native=a[i],owned=b[i]),indent=2));raise RuntimeError('Live TextureScroll mismatch; inspect '+str(run/'difference.json'))
    frames=0;refused=0;checks=0;states=maths.previous.decode(paths[0].read_bytes())
    for c,state in zip(cases,states):
        require(state['initialized']==c['expected'],'Expected initialization '+c['label']);checks+=1
        if not state['initialized']:refused+=1;continue
        for frame in state['frames']:
            frames+=1;values={v['name'].lower():v['words'] for v in frame['values']};require(frame['forward'][0]==values['$alpha'][3] and frame['forward'][1:4]==values['$color'][3:6],'Frame forwarding');checks+=1
            for w in frame['writes']:
                require((w[1]==9 and w[2]==16 and w[7]==1 and w[3:7]==[0]*4) or (w[1]!=9 and w[7:]==[0]*17),'Complete journal payload');checks+=1
            if c['label']=='current-result-type':require([w[1] for w in frame['writes']]==[9,9,1,3],'Current staged output type controls branch');checks+=1
            if c['label']=='actual-six-proxy-pilot-prefix':require([w[1] for w in frame['writes']]==[1,1,8,8,9,9],'Actual six-declaration prefix');checks+=1
    safety=execute('scroll_evaluation',False,['--test-scroll'],'scroll-safety');require('PASS scroll_safety_checks=' in safety,'Owned scroll safety')
    for stem,key,prefix in [('cosine_double','cosine','cosine'),('texture_scroll','scroll','scroll')]:
        for native,tail,label in [(True,[],'native'),(True,['--sse2'],'native-sse2'),(False,[],'owned')]:
            dest=run/(stem+'-prior-'+label+'.bin');execute(stem,native,[old.CLIENT if native else run/'absent.dll',parent.parent/(prefix+'-query.bin'),dest,*tail],dest.stem);require(sha(dest)==prior[key+'_output_sha256'],'Complete numerical prerequisite '+stem)
    texture_path=Path(prior['prior_receipt']);texture=json.loads(texture_path.read_text())
    for native,tail,label in [(True,[],'native'),(True,['--sse2'],'native-sse2'),(False,[],'owned')]:
        dest=run/('texture-prior-'+label+'.bin');execute('texture_evaluation',native,[old.CLIENT if native else run/'absent.dll',texture_path.parent/'query.bin',texture_path.parent/'prepared-original.bin',dest,*tail],dest.stem);require(sha(dest)==texture['output_sha256'],'All483 prior live TextureTransform cases')
    transform_path=Path(texture['prior_receipt']);transform=json.loads(transform_path.read_text());matrix_path=Path(transform['prior_receipt']);matrix=json.loads(matrix_path.read_text());sine_path=Path(matrix['prior_receipt']);sine=json.loads(sine_path.read_text());regressions=[]
    for native,tail,label in [(True,[],'native'),(True,['--sse2'],'native-sse2'),(False,[],'owned'),(False,['--sse2'],'owned-sse2')]:
        dest=run/('transform-prior-'+label+'.bin');execute('texture_transform',native,[old.CLIENT if native else run/'absent.dll',transform_path.parent/'query.bin',dest,*tail],dest.stem);require(sha(dest)==transform['output_sha256'],'All8929 transform values')
    dest=run/'getter-prior.bin';execute('texture_input',True,[old.PRIVATE,old.LIBS,mount,transform_path.parent/'getter-query.bin',dest],'getter-prior');require(sha(dest)==transform['getter_output_sha256'],'All2528 getter observations')
    for native in [True,False]:
        dest=run/('matrix-prior-'+str(native)+'.bin');execute('matrix_value',native,[old.PRIVATE if native else run/'absent.dll',matrix_path.parent/'query.bin',dest],dest.stem);require(sha(dest)==matrix['output_sha256'],'All1929 matrix transitions')
    history_path=sine_path;history=sine
    for stem,prepared_input in [('sine',True),('float_input',True),('equals',True),('material_proxy_lifetime',False),('unlit_two_texture_parameters',False),('world_transition_parameters',False),('lightmapped_parameters',False),('material_object_parameters',False)]:
        for native in [True,False]:
            dest=run/(stem+'-prior-'+str(native)+'.bin');args=[old.CLIENT if native else run/'absent.dll',history_path.parent/'query.bin',history_path.parent/'prepared-original.bin',dest] if prepared_input else [old.PRIVATE if native else run/'absent.dll',old.LIBS if native else run/'absent-directory',history_path.parent/'mount',history_path.parent/'query.bin',dest];execute(stem,native,args,dest.stem);require(sha(dest)==history['output_sha256'],'Complete earlier corpus '+stem);regressions.append(dict(stem=stem,native=native,cases=history['cases'],sha256=sha(dest)))
        history_path=Path(history['prior_receipt']);history=json.loads(history_path.read_text())
    store_dest=run/'store-prior.txt';execute('material_store',False,[run/'absent.dll',run/'absent-directory',mount,history_path.parent/'query.txt',store_dest],'store-prior');require(sha(store_dest)==history['trace_sha256'],'6064 ownership operations')
    eq_path=Path(json.loads(Path(sine['prior_receipt']).read_text())['prior_receipt']);eq=json.loads(eq_path.read_text())
    for native in [True,False]:
        dest=run/('component-prior-'+str(native)+'.bin');execute('component',native,[old.PRIVATE if native else run/'absent.dll',eq_path.parent/'component-query.bin',dest],dest.stem);require(sha(dest)==eq['component_output_sha256'],'2408 component values')
    for native,tail,label in [(True,[],'native'),(True,['--sse2'],'native-sse2'),(False,[],'owned')]:
        dest=run/('sine-math-prior-'+label+'.bin');execute('sine_math',native,[old.CLIENT if native else run/'absent.dll',sine_path.parent/'math-query.bin',dest,*tail],dest.stem);require(sha(dest)==sine['math_output_sha256'],'17762 Sine values')
    old_safety={}
    for stem,arg,key in [('sine','--test-sine','sine'),('float_input','--test','float_input'),('equals','--test-equals','equals'),('material_proxy_lifetime','--test-proxies','proxies'),('unlit_two_texture_parameters','--test-unlit-two-texture','two'),('world_transition_parameters','--test-world-transition','world'),('lightmapped_parameters','--test-lightmapped','lightmapped'),('material_object_parameters','--test','parameters'),('material_store','--test','ownership')]:old_safety[key]=execute(stem,False,[arg],key+'-safety')
    require(old_safety==texture['prior_safety'],'All historical safety outputs unchanged');require(execute('texture_evaluation',False,['--test-texture'],'texture-safety')==texture['safety'],'Texture36 safety/18 failures');require(execute('matrix_store_safety',False,[],'matrix-safety')==matrix['safety'],'Matrix37 safety/17 failures')
    two_path=sine_path
    while 'unlit-two-texture-parameters' not in str(two_path.parent.parent):two_path=Path(json.loads(two_path.read_text())['prior_receipt'])
    pilot=two_path.parent/'mount/materials/pilot-original.vmt';pilot_refusal=execute('unlit_two_texture_parameters',False,['--test-pilot-refusal',pilot],'pilot-refusal');require(pilot_refusal==texture['pilot_refusal'],'Whole pilot remains refused')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());require(all(sha(ROOT/n)==h for n,h in build['editor_source_dependencies'].items()) and sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor unchanged')
    for group in [sources,*protected]:require(all(sha(n)==h for n,h in group.items()),'Final protected identities')
    require(not (run/'absent.dll').exists() and not (run/'absent-directory').exists(),'Owned absent-library arguments');counts=re.search(r'native_proxy_allocations=(\d+) releases=(\d+)',native_trace);cpu_counts=re.search(r'native_allocations=(\d+) releases=(\d+)',cpu_trace);require(counts and counts[1]==counts[2] and cpu_counts and cpu_counts[1]==cpu_counts[2],'Native allocation balance');capture=run/'complete-evaluation.bin.zlib';capture.write_bytes(zlib.compress(paths[0].read_bytes(),9))
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',scope='Actual TextureScroll defaults/ignored missing-input initialization and opt-in live FloatInput binding, shared-cache component count snapshots, staged matrix/vector transactions',cases=len(cases),frames=frames,refused=refused,exact_bytes=paths[0].stat().st_size,independent_checks=checks,initialization_cases=53,ignored_input_failures=15,initialization_trace=init_trace,query_sha256=sha(query),output_sha256=sha(paths[0]),prepared_original_sha256=sha(prepared),native_proxy_allocations=int(counts[1]),native_proxy_releases=int(counts[2]),native_cpu_allocations=int(cpu_counts[1]),native_cpu_releases=int(cpu_counts[2]),safety=safety,prior_safety=old_safety,regressions=regressions,pilot_refusal=pilot_refusal,sources=sources,changed_sources=CHANGED,new_sources=NEW,original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],benchmark_material_hashes=prior['benchmark_material_hashes'],fixture_hashes=prior['fixture_hashes'],executables=executables,build_logs=logs,static_ranges=ranges,prior_receipt=str(parent),prior_receipt_sha256=sha(parent),editor_dependencies_unchanged=len(build['editor_source_dependencies']),editor_sha256=build['executable_sha256'],limits=['String output-name declarations and checked FloatInput kinds0/1/2/3; other KeyValues conversion domains remain refused','Shared missing-variable float values and optional component count must describe the same explicit snapshot; unknown required fields cause rollback, no implicit zero/count4','Native component count0..4 only; unsafe original vector scratch extents beyond4 not executed; owned reads refuse unsafe in-range component access','Actual proxy methods unchanged RX with explicit heap/KV/variable/clock services, independently original CMaterial initial state and separately proved setters/getters; no combined engine/GPU proof','Owner-thread staged evaluation; exception flags, alternative FP modes, resource/owner notifications, complete global dummy lifecycle, weapon_invis, whole pilot, editor/renderer integration and visual fidelity remain incomplete','Originals read-only; no live SFM control'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');frozen=OUT/('replay-'+run.name.removeprefix('verification-'));frozen.mkdir();included=[Path(n) for n in sources]+[Path(e['path']) for e in executables]+[Path(n) for n in logs]+[receipt,query,prepared,capture,iq,io,OUT/'preflight.json']+list(run.glob('*.json'))+list(run.glob('*.log'))+[OUT/'before/work'/n for n in CHANGED]
    for p in included:
        dest=frozen/p.relative_to(ROOT);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,dest)
    manifest={p.relative_to(frozen).as_posix():sha(p) for p in frozen.rglob('*') if p.is_file()};(frozen/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt),replay=str(frozen),replay_manifest_sha256=sha(frozen/'manifest.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','frames','refused','exact_bytes','safety']}),flush=True)

if __name__=='__main__':main()
