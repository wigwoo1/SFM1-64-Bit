"""Live checked TextureTransform + Sine/Equals declaration-order evaluation.
Original methods execute unchanged with explicit cached-value services; the
missing-variable cache is a supplied shared snapshot, never invented zeroes.
"""
import json,random,re,shutil,struct,subprocess,tempfile,time,zlib
from pathlib import Path
from datetime import datetime,timezone
import verify_material_texture_transform_matrix as maths
from verify_builtin_error_material_seed import source_closure
from verify_material_object_parameters import decode as decode_cpu
from verify_ifm_draw_packet import check_ranges
old=maths.old;ROOT=old.ROOT;OUT=ROOT/'outputs/material-texture-transform-evaluation-20261027';sha=old.sha;require=old.require
CHANGED=['material_proxy_evaluation_candidate.h','material_sine_proxy_candidate.h','material_object_parameter_initialization_candidate.h','material_parameter_orchestration_candidate.h','source_material_patch_files_candidate.h','material_object_store_candidate.h']
NEW=['material_texture_transform_proxy_candidate.h','material_texture_transform_client_native_fixture.h','probe_material_texture_transform_evaluation.cpp','build_material_texture_transform_evaluation.cmd','verify_material_texture_transform_evaluation.py']

def corpus():
    cases=[];f=old.fbits
    def texture(result='$out',**fields):return old.node('TextureTransform',[old.string('resultVar',result)]+[old.string(k,v) for k,v in fields.items()])
    def sine(result,**fields):return old.node('Sine',[old.string('resultVar',result)]+[old.string(k,v) if isinstance(v,str) else old.node(k,kind=3,bits=f(v)) for k,v in fields.items()])
    def add(label,proxies=None,out='float',extra=None,expected=True,missing=None):
        values=[old.string('$out','[1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1]') if out=='matrix' else old.literal('$out',out),old.string('$center','[.25 .75]'),old.string('$scale','[1.5 .5]'),old.node('$rotate',kind=3,bits=f(30)),old.string('$translate','[.125 -.25]'),old.node('$driver',kind=3,bits=f(.5)),*(extra or []),old.node('Proxies',proxies or [texture()])]
        cases.append(dict(label=label,root=old.node('UnlitTwoTexture',values),times=[f(0),f(.25),f(.5)],missing=missing or [[0]*4,[f(.25),f(-.5),f(.75),f(1)],[f(-.5),f(2),0,0]],expected=expected))
    fields=['centerVar','scaleVar','rotateVar','translateVar'];names=['$center','$scale','$rotate','$translate']
    for mask in range(16):
        for out in ['float','int','string','vec1','vec2','vec3','vec4','matrix']:add('optional-masks',[texture(**{fields[i]:names[i] for i in range(4) if mask&(1<<i)})],out)
    for field in fields:
        for kind in ['float','int','string','vec1','vec2','vec3','vec4']:
            add('cached-input-types',[texture(**{field:'$typed'})],extra=[old.literal('$typed',kind)])
        for name in ['$texture2','$basetexturetransform','$basetexture','$COLOR','$missing','$missing[1]',' $scale','$scale ','']:
            add('cached-and-missing-inputs',[texture(**{field:name})])
    for suffix in ['[-999999999999]','[-1]','[0]','[3]','[4]','[999999999999]','[foo]','[ 1tail','[']:
        add('result-component-ignored',[texture('$out'+suffix,scaleVar='$scale')])
    for field in fields:add('result-input-alias',[texture(**{field:'$out'})],out='vec2')
    add('all-shared-missing',[texture(**{field:'$missing'+str(i) for i,field in enumerate(fields)})])
    add('empty-fields',[texture(**{field:'' for field in fields})])
    add('sine-drives-translation',[sine('$translate',sinemin=-.25,sinemax=.75),texture(translateVar='$translate')])
    add('matrix-then-sine-then-matrix',[texture(scaleVar='$scale'),sine('$out',sinemin=.125,sinemax=.5),texture(translateVar='$out')])
    add('equals-drives-scale',[old.declaration('$scale','$driver'),texture(scaleVar='$scale')])
    add('texture-then-alias-texture',[texture(translateVar='$translate'),texture(centerVar='$out',scaleVar='$scale')])
    for result in ['$basetexture','$texture2','$basetexturetransform','$color']:add('other-found-results',[texture(result,scaleVar='$scale')])
    add('sine-equals-texture-chain',[sine('$driver',sinemin=.5,sinemax=2),old.declaration('$scale','$driver'),texture(centerVar='$center',scaleVar='$scale',rotateVar='$rotate',translateVar='$translate')])
    long='$'+'x'*253;add('last-safe-result-bracket',[texture(long+'[123456789]')],extra=[old.node(long,kind=3,bits=f(.25))])
    for size in [255,256,301,511,1023]:
        name='$'+'y'*(size-1);add('long-result-no-bracket',[texture(name)],extra=[old.node(name,kind=3,bits=f(.25))])
    for result in ['$missing','', '$not-there[0]']:add('result-init-refusal',[texture(result)],expected=False)
    add('missing-result-key',[old.node('TextureTransform')],expected=False)
    rng=random.Random(2026102711)
    for _ in range(256):
        numbers=[str(rng.uniform(-10,10)) for _ in range(7)];extra=[old.string('$rcenter','['+' '.join(numbers[:2])+']'),old.string('$rscale','['+' '.join(numbers[2:4])+']'),old.string('$rrotate',numbers[4]),old.string('$rtranslate','['+' '.join(numbers[5:])+']')]
        add('seeded-live-inputs',[sine('$driver',sinemin=-1,sinemax=1),texture(centerVar='$rcenter',scaleVar='$rscale',rotateVar='$rrotate',translateVar='$rtranslate')],extra=extra)
    pilot=next(Path(p) for p in json.loads(Path(json.loads((maths.OUT/'latest-verification.json').read_text())['receipt']).read_text())['benchmark_material_hashes'] if p.lower().endswith('pilotlight.vmt'))
    prefix=[]
    for name,block in re.findall(r'"(Sine|Equals|TextureTransform)"\s*\{([^{}]*)\}',pilot.read_text()):
        fields=[]
        for key,quoted,token in re.findall(r'"([^"\r\n]+)"\s+(?:"([^"\r\n]*)"|([^\s{}]+))',block):fields.append(old.string(key,quoted) if quoted else old.node(key,kind=3,bits=f(float(token))))
        prefix.append(old.node(name,fields))
    require([p['key'] for p in prefix]==['Sine','Sine','Equals','Equals','TextureTransform'],'Actual five-proxy pilot prefix')
    add('actual-five-proxy-pilot-prefix',prefix,extra=[old.string('$temp','[0 0]'),old.string('$pilot','0.0'),old.string('$pilot2','0.0')])
    return cases

def decode(raw):
    at=0
    def u():
        nonlocal at
        require(at+4<=len(raw),'Texture word extent');v=struct.unpack_from('<I',raw,at)[0];at+=4;return v
    def chars(n):
        nonlocal at
        require(at+n<=len(raw),'Texture string extent');s=raw[at:at+n].decode('ascii');at+=n;return s
    def text():return chars(u())
    result=[]
    for _ in range(u()):
        c=dict(index=u(),initialized=bool(u()),frames=[])
        if c['initialized']:
            for _ in range(3):
                writes=[[u() for _ in range(24)] for _ in range(u())];shader=text();values=[]
                for _ in range(u()):
                    w=[u() for _ in range(28)];values.append(dict(words=w,text=chars(w[10]),name=chars(w[11])))
                c['frames'].append(dict(writes=writes,shader=shader,values=values,forward=[u() for _ in range(7)]))
        result.append(c)
    require(at==len(raw),'Complete texture trace');return result

def main():
    pre=json.loads((OUT/'preflight.json').read_text());ptr=pre['prior'];priorPath=Path(ptr['receipt']);require(sha(priorPath)==ptr['receipt_sha256'],'Prior receipt');prior=json.loads(priorPath.read_text());priorReplay=Path(ptr['replay']);require(sha(priorReplay/'manifest.json')==ptr['replay_manifest_sha256'],'Prior manifest');require(all(sha(priorReplay/p)==h for p,h in json.loads((priorReplay/'manifest.json').read_text()).items()),'Prior frozen files')
    changed=[Path(p).name for p,h in prior['sources'].items() if sha(p)!=h];require(set(changed)==set(CHANGED),'Only bounded context/registry edits');require(all(sha(OUT/'before/work'/n)==prior['sources'][str(ROOT/'work'/n)] for n in CHANGED),'Before copies');sources=source_closure([Path(p) for p in prior['sources']]+[ROOT/'work'/n for n in NEW]);protected=[prior[k] for k in ['original_hashes','private_library_hashes','benchmark_material_hashes','fixture_hashes']]
    for group in protected:require(all(sha(p)==h for p,h in group.items()),'Protected inputs before')
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));mount=run/'mount';mount.mkdir();print('Verification directory: '+str(run),flush=True)
    _,ranges=check_ranges(old.CLIENT,'../outputs/material-proxy-evaluation-20261027/client-disassembly-private.txt',[('factory',0x10137e40,0x10137e66),('Init',0x10137ad0,0x10137baf),('OnBind',0x10137c30,0x10137e3d),('basic_result_Init',0x100e5db0,0x100e5ea9)])
    started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_texture_transform_evaluation.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=600);(run/'build-driver.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Fresh29 builds')
    tags={'sine_math':'sine-math','sine':'sine','float_input':'float-input','equals':'equals','component':'component','matrix_value':'matrix-value','texture_evaluation':'texture','texture_transform':'texture-math'};stems=['sine_math','sine','float_input','equals','component','material_proxy_lifetime','unlit_two_texture_parameters','world_transition_parameters','lightmapped_parameters','material_object_parameters','matrix_value','texture_evaluation','texture_transform'];targets=[(s,n,tags.get(s,s)+('-native' if n else '-owned')) for s in stems for n in [True,False]]+[('material_store',False,'material_store-owned'),('matrix_store_safety',False,'matrix-store'),('texture_input',True,'texture-input-native')];executables=[];logs={}
    def exe_path(stem,native):return OUT/('matrix_store_safety_x64.exe' if stem=='matrix_store_safety' else stem+('_reference_x86.exe' if native else '_owned_x64.exe'))
    for stem,native,tag in targets:
        exe=exe_path(stem,native);raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];machine=0x14c if native else 0x8664;require(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if native else 0x20b) and b'asInvoker' in raw,'Fresh PE architecture '+exe.name);imports=subprocess.run([str(old.DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require({x.lower() for x in modules}=={'kernel32.dll'},'Direct imports '+exe.name);executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules));log=OUT/(tag+'-build.log');require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Warning-free '+tag);logs[str(log)]=sha(log)
    logs[str(OUT/'asm-build.log')]=sha(OUT/'asm-build.log')
    def execute(stem,native,args,label):
        p=subprocess.run([str(exe_path(stem,native)),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' exit='+str(p.returncode)+' inspect '+str(run/(label+'.log')));return p.stdout
    cases=corpus();query=run/'query.bin';query.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<15I',*(c['times']+sum(c['missing'],[])))+old.packet(c['root']) for c in cases));(run/'cases.json').write_text(json.dumps(cases,indent=2));cpuQuery=run/'cpu-query.bin';cpuQuery.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<4I',0,0,0,0)+struct.pack('<12I',0,0,0,0,0,0,0,0,1,0,92,0)+old.text('equals/case')+old.packet(c['root'])+struct.pack('<6I',0,0,0,0,0,0) for c in cases));cpuOut=run/'cpu-original.bin';cpuTrace=execute('unlit_two_texture_parameters',True,[old.PRIVATE,old.LIBS,mount,cpuQuery,cpuOut],'cpu-original');n,records=decode_cpu(cpuOut.read_bytes());initial=[r for r in records if r['stage']==1];require(n==len(cases)==len(initial) and all(r['result'] for r in initial),'Original CMaterial CPU state');prepared=run/'prepared-original.bin';prepared.write_bytes(struct.pack('<I',n)+b''.join(old.text(s['shader'])+struct.pack('<I',len(s['values']))+b''.join(struct.pack('<28I',*v['words'])+v['text'].encode()+v['name'].encode() for v in s['values']) for s in initial))
    outputs=[];nativeTrace=''
    for native,tail,label in [(True,[],'native'),(True,['--sse2'],'native-sse2'),(False,[],'owned')]:
        dest=run/(label+'.bin');trace=execute('texture_evaluation',native,[old.CLIENT if native else run/'absent.dll',query,prepared,dest,*tail],label);outputs.append(dest)
        if label=='native':nativeTrace=trace
    if not all(p.read_bytes()==outputs[0].read_bytes() for p in outputs):
        a,b=decode(outputs[0].read_bytes()),decode(outputs[2].read_bytes());diff=[i for i,(x,y) in enumerate(zip(a,b)) if x!=y];i=diff[0] if diff else 0;(run/'difference.json').write_text(json.dumps(dict(indices=diff,case=cases[i],native=a[i],owned=b[i]),indent=2));raise RuntimeError('Texture live mismatch; inspect '+str(run/'difference.json'))
    checks=0;frames=0;refused=0
    for c,state in zip(cases,decode(outputs[0].read_bytes())):
        require(state['initialized']==c['expected'],'Expected initialization '+c['label']);checks+=1
        if not state['initialized']:refused+=1;continue
        for frame in state['frames']:
            frames+=1;values={v['name'].lower():v for v in frame['values']};require(any(w[1]==9 for w in frame['writes']),'Full matrix write present');checks+=1
            for w in frame['writes']:
                if w[1]==9:require(w[2]==16 and w[7]==1 and w[3:7]==[0]*4,'Complete matrix event');checks+=1
                else:require(w[7:]==[0]*17,'Old scalar events have no matrix payload');checks+=1
            require(frame['forward'][0]==values['$alpha']['words'][3] and frame['forward'][1:4]==values['$color']['words'][3:6],'Forwarding updated with frame');checks+=1
        if c['label']=='cached-and-missing-inputs' and '$missing' in json.dumps(c['root']):
            field=c['root']['children'][-1]['children'][0]['children'][1]['key']
            if field=='centerVar':
                for f in state['frames']:require(next(v for v in f['values'] if v['name']=='$out')['words'][12:28]==maths.previous.IDENTITY,'Center cancels without scale/rotation/translation');checks+=1
            else:require(state['frames'][0]['values']!=state['frames'][1]['values'],'Explicit shared snapshot changes active transform');checks+=1
    safety=execute('texture_evaluation',False,['--test-texture'],'texture-safety');require('PASS texture_safety_checks=' in safety,'Owned texture safety')
    # Numerical/getter prerequisites are rerun with newly compiled programs.
    for native,tail,label in [(True,[],'transform-math-native'),(True,['--sse2'],'transform-math-native-sse2'),(False,[],'transform-math-owned'),(False,['--sse2'],'transform-math-owned-sse2')]:
        dest=run/(label+'.bin');execute('texture_transform',native,[old.CLIENT if native else run/'absent.dll',priorPath.parent/'query.bin',dest,*tail],label);require(sha(dest)==prior['output_sha256'],'All8929 numerical cases unchanged')
    getterOut=run/'getter-prior.bin';execute('texture_input',True,[old.PRIVATE,old.LIBS,mount,priorPath.parent/'getter-query.bin',getterOut],'getter-prior');require(sha(getterOut)==prior['getter_output_sha256'],'All2528 getter/missing-variable observations unchanged')
    matrixPath=Path(prior['prior_receipt']);matrix=json.loads(matrixPath.read_text());sinePath=Path(matrix['prior_receipt']);sine=json.loads(sinePath.read_text());regressions=[]
    for native in [True,False]:
        dest=run/('matrix-prior-native.bin' if native else 'matrix-prior-owned.bin');execute('matrix_value',native,[old.PRIVATE if native else run/'absent.dll',matrixPath.parent/'query.bin',dest],dest.stem);require(sha(dest)==matrix['output_sha256'],'Prior1929 matrix values')
    historyPath=sinePath;history=sine
    for stem,preparedInput in [('sine',True),('float_input',True),('equals',True),('material_proxy_lifetime',False),('unlit_two_texture_parameters',False),('world_transition_parameters',False),('lightmapped_parameters',False),('material_object_parameters',False)]:
        for native in [True,False]:
            label=stem+('-prior-native' if native else '-prior-owned');dest=run/(label+'.bin');args=[old.CLIENT if native else run/'absent.dll',historyPath.parent/'query.bin',historyPath.parent/'prepared-original.bin',dest] if preparedInput else [old.PRIVATE if native else run/'absent.dll',old.LIBS if native else run/'absent-directory',historyPath.parent/'mount',historyPath.parent/'query.bin',dest];execute(stem,native,args,label);require(sha(dest)==history['output_sha256'],'Complete prior trace '+label);regressions.append(dict(stem=stem,native=native,cases=history['cases'],sha256=sha(dest)))
        historyPath=Path(history['prior_receipt']);history=json.loads(historyPath.read_text())
    storeOut=run/'store-prior.txt';execute('material_store',False,[run/'absent.dll',run/'absent-directory',mount,historyPath.parent/'query.txt',storeOut],'store-prior');require(sha(storeOut)==history['trace_sha256'],'6064 ownership operations unchanged')
    floatReceipt=json.loads(Path(sine['prior_receipt']).read_text());eqPath=Path(floatReceipt['prior_receipt']);eq=json.loads(eqPath.read_text())
    for native in [True,False]:
        dest=run/('component-prior-native.bin' if native else 'component-prior-owned.bin');execute('component',native,[old.PRIVATE if native else run/'absent.dll',eqPath.parent/'component-query.bin',dest],dest.stem);require(sha(dest)==eq['component_output_sha256'],'2408 components unchanged')
    for native,tail,label in [(True,[],'math-native'),(True,['--sse2'],'math-native-sse2'),(False,[],'math-owned')]:
        dest=run/(label+'.bin');execute('sine_math',native,[old.CLIENT if native else run/'absent.dll',sinePath.parent/'math-query.bin',dest,*tail],label);require(sha(dest)==sine['math_output_sha256'],'17762 Sine math cases unchanged')
    priorSafety={}
    for stem,arg,key in [('sine','--test-sine','sine'),('float_input','--test','float_input'),('equals','--test-equals','equals'),('material_proxy_lifetime','--test-proxies','proxies'),('unlit_two_texture_parameters','--test-unlit-two-texture','two'),('world_transition_parameters','--test-world-transition','world'),('lightmapped_parameters','--test-lightmapped','lightmapped'),('material_object_parameters','--test','parameters'),('material_store','--test','ownership')]:priorSafety[key]=execute(stem,False,[arg],key+'-safety')
    require(priorSafety==sine['safety'],'All old safety outputs unchanged');require(execute('matrix_store_safety',False,[],'matrix-safety')==matrix['safety'],'Matrix safety unchanged')
    twoPath=sinePath
    while 'unlit-two-texture-parameters' not in str(twoPath.parent.parent):twoPath=Path(json.loads(twoPath.read_text())['prior_receipt'])
    pilot=twoPath.parent/'mount/materials/pilot-original.vmt';require(sha(pilot)==sine['pilot']['sha256'],'Untouched pilot');refusal=execute('unlit_two_texture_parameters',False,['--test-pilot-refusal',pilot],'pilot-refusal');require(refusal==prior['pilot_refusal'],'Whole pilot still refused')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());require(all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()) and sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor unchanged')
    for group in [sources,*protected]:require(all(sha(p)==h for p,h in group.items()),'Protected final identities')
    require(not (run/'absent.dll').exists() and not (run/'absent-directory').exists(),'Owned original-library arguments absent');counts=re.search(r'native_proxy_allocations=(\d+) releases=(\d+)',nativeTrace);cpuCounts=re.search(r'native_allocations=(\d+) releases=(\d+)',cpuTrace);require(counts and counts[1]==counts[2] and cpuCounts and cpuCounts[1]==cpuCounts[2],'Native proxy/CPU allocations balanced');capture=run/'complete-evaluation.bin.zlib';capture.write_bytes(zlib.compress(outputs[0].read_bytes(),9))
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',scope='Opt-in TextureTransform string-declaration initialization, live authenticated cached inputs, supplied shared-missing cache snapshots, declaration-order matrix transactions and rollback; checked CPU/store contract',cases=len(cases),frames=frames,refused=refused,exact_bytes=outputs[0].stat().st_size,independent_checks=checks,query_sha256=sha(query),output_sha256=sha(outputs[0]),cpu_query_sha256=sha(cpuQuery),cpu_output_sha256=sha(cpuOut),prepared_original_sha256=sha(prepared),native_proxy_allocations=int(counts[1]),native_proxy_releases=int(counts[2]),native_cpu_allocations=int(cpuCounts[1]),native_cpu_releases=int(cpuCounts[2]),safety=safety,prior_safety=priorSafety,regressions=regressions,pilot_refusal=refusal,sources=sources,changed_sources=CHANGED,new_sources=NEW,original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],benchmark_material_hashes=prior['benchmark_material_hashes'],fixture_hashes=prior['fixture_hashes'],executables=executables,build_logs=logs,static_ranges=ranges,prior_receipt=str(priorPath),prior_receipt_sha256=sha(priorPath),editor_dependencies_unchanged=len(build['editor_source_dependencies']),editor_sha256=build['executable_sha256'],limits=['String-valued proxy declarations; typed/wide/pointer GetString conversion unproved and refused','Shared missing-variable snapshot is explicitly supplied per evaluation; no per-name zero default or owned complete material-system dummy allocator/ABI claim','Alpha-result renderer flags/snapshots, resource outputs and unsafe bracket-outside-first255 boundary refuse','Original proxy methods unchanged RX with explicit variable/KV/material/heap/TLS services and separately original-seeded CMaterial CPU state; setter/getter/missing-cache domains proved by earlier independent oracles, not combined engine/GPU proof','Owner-thread staged evaluation; cached-state snapshot consistency only, no concurrently mutable external engine state contract','TextureScroll/weapon_invis, full pilot, editor integration, visual fidelity and complete SFM goal remain unfinished; no original writes or live SFM control'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');frozen=OUT/('replay-'+run.name.removeprefix('verification-'));frozen.mkdir();included=[Path(p) for p in sources]+[Path(e['path']) for e in executables]+[Path(p) for p in logs]+[receipt,query,cpuQuery,prepared,capture,OUT/'preflight.json']+list(run.glob('*.json'))+list(run.glob('*.log'))+[OUT/'before/work'/n for n in CHANGED]
    for p in included:
        target=frozen/p.relative_to(ROOT);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target)
    manifest={p.relative_to(frozen).as_posix():sha(p) for p in frozen.rglob('*') if p.is_file()};(frozen/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');pointer=dict(receipt=str(receipt),receipt_sha256=sha(receipt),replay=str(frozen),replay_manifest_sha256=sha(frozen/'manifest.json'));(OUT/'latest-verification.json').write_text(json.dumps(pointer,indent=2)+'\n');print(json.dumps(dict(status='PASS',receipt=str(receipt),cases=len(cases),frames=frames,refused=refused,exact_bytes=result['exact_bytes'],safety=safety.strip())),flush=True)

if __name__=='__main__':main()
