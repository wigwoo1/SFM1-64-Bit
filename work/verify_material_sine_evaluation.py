"""Unchanged original Sine factory/Init/OnBind versus independent x64 math/store.
No editor/SFM startup, private instruction patch, or original writes.
"""
import json,random,re,shutil,struct,subprocess,tempfile,time,zlib
from pathlib import Path
from datetime import datetime,timezone
import verify_material_equals_evaluation as old
from verify_material_object_parameters import decode
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
from generate_material_sine_constants import constants,render
ROOT=old.ROOT;OUT=ROOT/'outputs/material-sine-evaluation-20261027';require=old.require;sha=old.sha
CHANGED=['material_proxy_evaluation_candidate.h','material_object_store_candidate.h']
NEW=['probe_material_sine_math.cpp','build_material_sine_math.cmd','generate_material_sine_constants.py','material_sine_constants_candidate.h','material_sine_double_x87_candidate.inc','material_sine_double_candidate.h','material_sine_value_candidate.h','material_sine_proxy_candidate.h','material_sine_client_native_fixture.h','probe_material_sine_evaluation.cpp','build_material_sine_evaluation.cmd','verify_material_sine_evaluation.py']
FIRST=OUT/'math-exploration-bvwfdj5f'

def math_corpus():
    first=(FIRST/'query.bin').read_bytes();require(len(first)==4663*20,'Retained first input extent');cases=[list(struct.unpack_from('<5I',first,i*20)) for i in range(4663)];f=old.fbits
    for period in [.1,7.,1.,0.,-1.,-.1]:
        for offset in [0.,.125,-3.]:
            for i in range(-24,241):cases.append([f(i/24),f(period),f(-.02),f(.02),f(offset)])
    for t in [0,0x80000000,1,0x80000001,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7fa12345]:
        for period in [0,0x80000000,1,0x3dcccccd,0xbf800000,0x7f800000,0x7fc12345]:cases.append([t,period,0,0x3f800000,0])
    rng=random.Random(2026102705)
    for _ in range(8192):
        a=rng.randrange(0x7f800000)|(rng.randrange(2)<<31);b=rng.randrange(0x7f800000)|(rng.randrange(2)<<31);c=rng.randrange(0x7f800000)|(rng.randrange(2)<<31);d=rng.randrange(0x7f800000)|(rng.randrange(2)<<31);e=rng.randrange(0x7f800000)|(rng.randrange(2)<<31);cases.append([a,b,c,d,e])
    for t in range(13790,13810):
        for period in [0x3f800000,0x3dcccccd,0x40e00000]:cases.append([f(float(t)),period,f(-.02),f(.02),0])
    return cases

def corpus(prior):
    cases=[];f=old.fbits
    def sine(result='$out',**fields):return old.node('Sine',[old.string('resultVar',result)]+[old.string(k,v) if isinstance(v,str) else old.node(k,kind=3,bits=f(v)) for k,v in fields.items()])
    def equals(result,source):return old.declaration(result,source)
    def add(label,proxies=None,out='float',times=None,extra=None,expected=None):
        values=[old.literal('$out',out),old.string('$drive','[.25 -.5 .75 1]')]+(extra or []);values.append(old.node('Proxies',proxies if proxies is not None else [sine()]));cases.append(dict(label=label,root=old.node('UnlitTwoTexture',values),times=times or [f(0),f(.25),f(5)],expected=expected))
    add('analytical-default',times=[f(0),f(.25),f(.75)],expected=[0x3f000000,0x3f800000,0])
    for p in ['0','-0','1','-1','.1','7','1e-40','1e30']:
        for lo,hi in [(0.,1.),(-.02,.02),(.005,-.005),(2.,2.)]:
            for output in ['float','int','string','vec1','vec2','vec3','vec4']:add('typed-results',[sine(sineperiod=p,sinemin=lo,sinemax=hi)],out=output)
    for result in ['$out[-1]','$out[0]','$out[1]','$out[2]','$out[3]','$out[foo]','$out[ 1tail']:
        for n in range(1,5):add('output-components',[sine(result,sineperiod='.1',sinemin=-.02,sinemax=.02)],out='vec'+str(n))
    for field in ['sineperiod','sinemin','sinemax','timeOffset']:
        for source in ['$drive','$drive[0]','$drive[1]','$drive[2]','$drive[3]','$drive[4]','$drive[-1]','$drive[foo]']:
            add('bound-inputs',[sine('$drive[1]',sineperiod='.1',sinemin=.125,sinemax=2.),sine(**{field:source})])
    add('self-alias',[sine(sineperiod='$out',sinemin=-1.,sinemax=2.)])
    add('declaration-order',[sine('$drive[3]',sineperiod='7'),sine(sinemin='$drive[3]',sinemax='$drive[0]'),equals('$drive[1]','$out')])
    for name in ['$color','$texture2','$flags']:add('other-cached-results',[sine(name,sineperiod='.1')])
    for field in ['sineperiod','sinemin','sinemax','timeOffset']:add('init-refusal',[sine(**{field:'$missing'})],expected=False)
    add('init-refusal',[sine('$missing')],expected=False)
    for t in [0x7f800000,0xff800000,0x7fc12345,0x7fa12345,0x7f7fffff]:add('clock-extremes',times=[t,f(0),f(.25)])
    rng=random.Random(2026102706)
    for _ in range(256):
        fields=dict(sineperiod=str(rng.uniform(-10,10)),sinemin=rng.uniform(-2,2),sinemax=rng.uniform(-2,2),timeOffset=str(rng.uniform(-20,20)));add('seeded-waveform',[sine(**fields)],times=[f(rng.uniform(-1e6,1e6)) for _ in range(3)])
    pilotPath=next(Path(p) for p in prior['benchmark_material_hashes'] if p.lower().endswith('pilotlight.vmt'));raw=pilotPath.read_bytes();prefix=[]
    for name,block in re.findall(r'"(Sine|Equals)"\s*\{([^{}]*)\}',raw.decode('ascii')):
        fields=[]
        for key,quoted,token in re.findall(r'"([^"\r\n]+)"\s+(?:"([^"\r\n]*)"|([^\s{}]+))',block):fields.append(old.string(key,quoted) if quoted else old.node(key,kind=3,bits=f(float(token))))
        prefix.append(old.node(name,fields))
    require([p['key'] for p in prefix]==['Sine','Sine','Equals','Equals'],'Actual four-proxy pilot prefix')
    add('actual-pilot-prefix-controlled-clock',prefix,extra=[old.string('$temp','[0 0]'),old.string('$pilot','0.0'),old.string('$pilot2','0.0')],times=[f(0),f(.025),f(5)])
    return cases,dict(path=str(pilotPath),sha256=sha(pilotPath),prefix=prefix,scope='Four actual declarations in a synthetic CPU fixture only; TextureTransform/TextureScroll/weapon_invis and full material remain incomplete')

def main():
    pre=json.loads((OUT/'preflight.json').read_text());pointer=pre['prior'];priorPath=Path(pointer['receipt']);require(sha(priorPath)==pointer['receipt_sha256'],'Prior receipt');prior=json.loads(priorPath.read_text());priorReplay=Path(pointer['replay']);require(sha(priorReplay/'manifest.json')==pointer['replay_manifest_sha256'],'Prior frozen manifest');require(all(sha(priorReplay/p)==h for p,h in json.loads((priorReplay/'manifest.json').read_text()).items()),'Prior frozen files')
    changed=[Path(p).name for p,h in prior['sources'].items() if sha(p)!=h];require(set(changed)==set(CHANGED),'Only two bounded context edits');require(all(sha(OUT/'before/work'/p)==prior['sources'][str(ROOT/'work'/p)] for p in CHANGED),'Before snapshots');sources=source_closure([Path(p) for p in prior['sources']]+[ROOT/'work'/p for p in NEW])
    for group in [prior['original_hashes'],prior['private_library_hashes'],prior['benchmark_material_hashes']]:require(all(sha(p)==h for p,h in group.items()),'Protected inputs before')
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));mount=run/'mount';mount.mkdir()
    image,ranges=check_ranges(old.CLIENT,'../outputs/material-proxy-evaluation-20261027/client-disassembly-private.txt',[('Sine_registration',0x10492170,0x10492189),('Sine_factory',0x10137370,0x10137396),('Sine_Init',0x10136440,0x101364e4),('Sine_OnBind',0x101364f0,0x101365a7),('SSE2_sine',0x1045dcb3,0x1045de6b),('x87_fallback',0x10466c20,0x10466d49),('alternate_dispatch_large_return',0x1047dd02,0x1047dd07)])
    table,split=constants();require((ROOT/'work/material_sine_constants_candidate.h').read_text()==render(),'Deterministic independent constants');require(all(struct.pack('<d',v)==image.b[image.offset(0x105f1150+32*i+8*j):image.offset(0x105f1150+32*i+8*j)+8] for i,row in enumerate(table) for j,v in enumerate(row)),'All256 independently generated lookup values equal original data');require(all(struct.pack('<d',v)==image.b[image.offset(a):image.offset(a)+8] for v,a in zip(split,[0x105f19a0,0x105f1990,0x105f19a8,0x105f19b0])),'Independent pi splits/inverse equal original')
    started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_sine_evaluation.cmd')],capture_output=True,text=True,cwd=ROOT,timeout=360);(run/'build-driver.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Fresh21 Sine/prior builds')
    targets=[('sine_math',True,'sine-math-native'),('sine_math',False,'sine-math-owned'),('sine',True,'sine-native'),('sine',False,'sine-owned'),('float_input',True,'float-input-native'),('float_input',False,'float-input-owned'),('equals',True,'equals-native'),('equals',False,'equals-owned'),('component',True,'component-native'),('component',False,'component-owned')]+[(stem,n,stem+('-native' if n else '-owned')) for stem in ['material_proxy_lifetime','unlit_two_texture_parameters','world_transition_parameters','lightmapped_parameters','material_object_parameters'] for n in [True,False]]+[('material_store',False,'material_store-owned')];executables=[];logs={}
    for stem,native,tag in targets:
        exe=OUT/(stem+('_reference_x86.exe' if native else '_owned_x64.exe'));raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==(0x14c if native else 0x8664) and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if native else 0x20b) and b'asInvoker' in raw,'Fresh architecture '+exe.name);imports=subprocess.run([str(old.DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require({x.lower() for x in modules}=={'kernel32.dll'},'Direct imports '+exe.name);executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(0x14c if native else 0x8664),direct_imports=modules));log=OUT/(tag+'-build.log');require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Clean build '+tag);logs[str(log)]=sha(log)
    logs[str(OUT/'asm-build.log')]=sha(OUT/'asm-build.log')
    def execute(stem,native,args,label):
        exe=OUT/(stem+('_reference_x86.exe' if native else '_owned_x64.exe'));p=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' exit='+str(p.returncode)+'; inspect '+str(run/(label+'.log')));return p.stdout
    maths=math_corpus();mq=run/'math-query.bin';mq.write_bytes(b''.join(struct.pack('<5I',*q) for q in maths));mathOutputs=[]
    for label,native,tail in [('math-native',True,[]),('math-native-sse2',True,['--sse2']),('math-owned',False,[])]:
        dest=run/(label+'.bin');execute('sine_math',native,[old.CLIENT if native else run/'absent.dll',mq,dest,*tail],label);mathOutputs.append(dest)
    require(mathOutputs[0].read_bytes()[:4663*28]==(FIRST/'native.bin').read_bytes(),'Retained first native4663 observations unchanged')
    if not all(p.read_bytes()==mathOutputs[0].read_bytes() for p in mathOutputs):
        a=mathOutputs[0].read_bytes();b=mathOutputs[2].read_bytes();diff=[i for i in range(len(maths)) if a[i*28:(i+1)*28]!=b[i*28:(i+1)*28]];(run/'math-difference.json').write_text(json.dumps(dict(differences=len(diff),indices=diff[:100],first_query=maths[diff[0]] if diff else None),indent=2));raise RuntimeError('Math comparison mismatch; inspect '+str(run/'math-difference.json'))
    cases,pilot=corpus(prior);query=run/'query.bin';query.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<3I',*c['times'])+old.packet(c['root']) for c in cases));(run/'cases.json').write_text(json.dumps(cases,indent=2));(run/'pilot-prefix-scope.json').write_text(json.dumps(pilot,indent=2))
    cpuQuery=run/'cpu-query.bin';cpuQuery.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<4I',0,0,0,0)+struct.pack('<12I',0,0,0,0,0,0,0,0,1,0,92,0)+old.text('equals/case')+old.packet(c['root'])+struct.pack('<6I',0,0,0,0,0,0) for c in cases));cpuOut=run/'cpu-original.bin';cpuTrace=execute('unlit_two_texture_parameters',True,[old.PRIVATE,old.LIBS,mount,cpuQuery,cpuOut],'cpu-original');n,records=decode(cpuOut.read_bytes());initial=[r for r in records if r['stage']==1];require(n==len(cases)==len(initial) and all(r['result'] for r in initial),'Original CPU initial states');prepared=run/'prepared-original.bin';prepared.write_bytes(struct.pack('<I',n)+b''.join(old.text(s['shader'])+struct.pack('<I',len(s['values']))+b''.join(struct.pack('<28I',*v['words'])+v['text'].encode()+v['name'].encode() for v in s['values']) for s in initial))
    outputs=[];nativeTrace=''
    for label,native,tail in [('native',True,[]),('native-sse2',True,['--sse2']),('owned',False,[])]:
        dest=run/(label+'.bin');trace=execute('sine',native,[old.CLIENT if native else run/'absent.dll',query,prepared,dest,*tail],label);outputs.append(dest)
        if label=='native':nativeTrace=trace
    if not all(p.read_bytes()==outputs[0].read_bytes() for p in outputs):
        a=old.decode_frames(outputs[0].read_bytes());b=old.decode_frames(outputs[2].read_bytes());diff=[i for i,(x,y) in enumerate(zip(a,b)) if x!=y];(run/'difference.json').write_text(json.dumps(dict(indices=diff,first_case=cases[diff[0]] if diff else None,native=a[diff[0]] if diff else None,owned=b[diff[0]] if diff else None),indent=2));raise RuntimeError('Sine comparison mismatch; inspect '+str(run/'difference.json'))
    decoded=old.decode_frames(outputs[0].read_bytes());checks=0;frames=0;refused=0
    for c,state in zip(cases,decoded):
        require(state['initialized']!=(c['expected'] is False),'Expected Sine Init result');checks+=1
        if not state['initialized']:refused+=1;continue
        for index,frame in enumerate(state['frames']):
            frames+=1;values={v['name'].lower():v for v in frame['values']};require(frame['forward'][0]==values['$alpha']['words'][3] and frame['forward'][1:4]==values['$color']['words'][3:6],'Updated forwarding cache');checks+=1
            if isinstance(c['expected'],list):require(frame['writes'][-1][3]==c['expected'][index],'Analytical quarter-period samples');checks+=1
            if c['label']=='actual-pilot-prefix-controlled-clock':require(len(frame['writes'])==4 and [w[1] for w in frame['writes']]==[1,1,8,8] and [w[2] for w in frame['writes'][2:]]==[0,1],'Actual two Sines feed actual two Equals components');checks+=1
    regressions=[];historyPath=priorPath;history=prior
    # All pre-context accepted corpora, including958 FloatInput,682 Equals,
    # 210 lifetime,634/995/620/138 parameter cases and6064 ownership operations.
    stages=[('float_input',False),('equals',True),('material_proxy_lifetime',False),('unlit_two_texture_parameters',False),('world_transition_parameters',False),('lightmapped_parameters',False),('material_object_parameters',False)]
    for stem,preparedInput in stages:
        for native in [True,False]:
            dest=run/(stem+('-prior-native.bin' if native else '-prior-owned.bin'))
            if stem=='float_input':args=[old.CLIENT if native else run/'absent.dll',historyPath.parent/'query.bin',historyPath.parent/'prepared-original.bin',dest]
            elif preparedInput:args=[old.CLIENT if native else run/'absent.dll',historyPath.parent/'query.bin',historyPath.parent/'prepared-original.bin',dest]
            else:args=[old.PRIVATE if native else run/'absent.dll',old.LIBS if native else run/'absent-directory',historyPath.parent/'mount',historyPath.parent/'query.bin',dest]
            execute(stem,native,args,dest.stem);require(sha(dest)==history['output_sha256'],'Complete prior corpus '+dest.name);regressions.append(dict(stem=stem,native=native,cases=history['cases'],sha256=sha(dest)))
        historyPath=Path(history['prior_receipt']);history=json.loads(historyPath.read_text())
    storeDest=run/'store-prior.txt';execute('material_store',False,[run/'absent.dll',run/'absent-directory',mount,historyPath.parent/'query.txt',storeDest],'store-prior');require(sha(storeDest)==history['trace_sha256'],'6064 ownership operations unchanged')
    eq=json.loads(Path(prior['prior_receipt']).read_text());componentQuery=Path(prior['prior_receipt']).parent/'component-query.bin'
    for native in [True,False]:
        dest=run/('component-prior-native.bin' if native else 'component-prior-owned.bin');execute('component',native,[old.PRIVATE if native else run/'absent.dll',componentQuery,dest],dest.stem);require(sha(dest)==eq['component_output_sha256'],'2408 component cases unchanged')
    safety={}
    for stem,arg,key in [('sine','--test-sine','sine'),('float_input','--test','float_input'),('equals','--test-equals','equals'),('material_proxy_lifetime','--test-proxies','proxies'),('unlit_two_texture_parameters','--test-unlit-two-texture','two'),('world_transition_parameters','--test-world-transition','world'),('lightmapped_parameters','--test-lightmapped','lightmapped'),('material_object_parameters','--test','parameters'),('material_store','--test','ownership')]:safety[key]=execute(stem,False,[arg],key+'-safety')
    require(safety['float_input']==prior['safety'] and all(safety[k]==v for k,v in eq['safety'].items()),'All earlier safety counts unchanged')
    twoPath=Path(json.loads(Path(eq['prior_receipt']).read_text())['prior_receipt']);two=json.loads(twoPath.read_text());pilotCopy=twoPath.parent/'mount/materials/pilot-original.vmt';require(sha(pilotCopy)==pilot['sha256'],'Untouched full pilot identity');refusal=execute('unlit_two_texture_parameters',False,['--test-pilot-refusal',pilotCopy],'whole-pilot-refusal');require(refusal==two['pilot_refusal'],'Full pilot default refusal unchanged')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());require(all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()) and sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Full editor graph/exe unchanged')
    for group in [sources,prior['original_hashes'],prior['private_library_hashes'],prior['benchmark_material_hashes']]:require(all(sha(p)==h for p,h in group.items()),'Protected final identities')
    require(not (run/'absent.dll').exists() and not (run/'absent-directory').exists(),'Owned independence arguments absent');nativeCounts=re.search(r'native_proxy_allocations=(\d+) releases=(\d+)',nativeTrace);cpuCounts=re.search(r'native_allocations=(\d+) releases=(\d+)',cpuTrace);require(nativeCounts and nativeCounts[1]==nativeCounts[2] and cpuCounts and cpuCounts[1]==cpuCounts[2],'Original allocation balances')
    for name,data in [('complete-sine.bin.zlib',outputs[0].read_bytes()),('complete-math.bin.zlib',mathOutputs[0].read_bytes())]:(run/name).write_bytes(zlib.compress(data,9))
    rejected=json.loads((FIRST/'comparison.json').read_text());require(rejected['different_cases']==180 and sha(FIRST/'native.bin')==sha(FIRST/'reconstructed.bin'),'Rejected first approach preserved; reconstructed prefix exact')
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',scope='Actual Sine factory/Init/OnBind with explicit clock and independent SSE2/x87 final-value reconstruction; checked CPU/store integration',cases=len(cases),frames=frames,refused=refused,exact_bytes=outputs[0].stat().st_size,independent_checks=checks,math_cases=len(maths),math_exact_bytes=mathOutputs[0].stat().st_size,math_query_sha256=sha(mq),math_output_sha256=sha(mathOutputs[0]),original_dispatch_states=[0,1],independent_lookup_values=256,independent_pi_constants=4,query_sha256=sha(query),output_sha256=sha(outputs[0]),cpu_query_sha256=sha(cpuQuery),cpu_output_sha256=sha(cpuOut),prepared_original_sha256=sha(prepared),native_proxy_allocations=int(nativeCounts[1]),native_proxy_releases=int(nativeCounts[2]),native_cpu_allocations=int(cpuCounts[1]),native_cpu_releases=int(cpuCounts[2]),safety=safety,regressions=regressions,store_regression=dict(operations=6064,trace_sha256=sha(storeDest)),sources=sources,changed_sources=CHANGED,new_sources=NEW,original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],benchmark_material_hashes=prior['benchmark_material_hashes'],executables=executables,build_logs=logs,static_ranges=ranges,pilot=pilot,pilot_refusal=refusal,rejected_approach=dict(path=str(FIRST),cases=4663,differences=180,query_sha256=sha(FIRST/'query.bin'),original_sha256=sha(FIRST/'native.bin'),rejected_output_sha256=sha(FIRST/'owned.bin'),rejected_executable_sha256=sha(FIRST/'modern_sin_rejected_x64.exe')),prior_receipt=str(priorPath),prior_receipt_sha256=sha(priorPath),editor_dependencies_unchanged=len(build['editor_source_dependencies']),editor_sha256=build['executable_sha256'],limits=['Explicit supplied f32 clock; no wall time or missing-clock zero substitution; owner-thread staged evaluation','Original methods execute unchanged RX with explicit material/variable/KV/heap/TLS spies and separately original CMaterial CPU initial states; not combined original engine/GPU proof','Checked round-nearest/nonflushing SSE and x87 path on this x86-64 host; floating exception flags, alternate FP modes and arbitrary platform sin library compatibility unproved','Sine scalar/string/vector/undefined cached outputs only; alpha-result renderer flags/snapshots, resource/matrix output and unsafe component indices refuse','Four actual pilot prefix declarations checked in a synthetic CPU fixture; TextureTransform/TextureScroll/weapon_invis, full pilot, editor integration and visual parity remain incomplete','No original writes/live SFM/Blender/editor run or whole-project completion claim'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');replay=OUT/('replay-'+run.name.removeprefix('verification-'));replay.mkdir();included=[Path(p) for p in sources]+[Path(e['path']) for e in executables]+[Path(p) for p in logs]+[receipt,query,mq,cpuQuery,prepared,OUT/'preflight.json']+list(run.glob('*.json'))+list(run.glob('*.log'))+list(run.glob('*.zlib'))+[OUT/'before/work'/n for n in CHANGED]+[FIRST/p for p in ['query.bin','native.bin','owned.bin','comparison.json','reconstructed.bin','reconstructed-comparison.json','modern_sin_rejected_x64.exe']]
    for p in included:
        target=replay/p.relative_to(ROOT);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target)
    manifest={p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()};(replay/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');pointer=dict(receipt=str(receipt),receipt_sha256=sha(receipt),replay=str(replay),replay_manifest_sha256=sha(replay/'manifest.json'));(OUT/'latest-verification.json').write_text(json.dumps(pointer,indent=2)+'\n');print(json.dumps(dict(status='PASS',receipt=str(receipt),cases=len(cases),frames=frames,math_cases=len(maths),exact_bytes=outputs[0].stat().st_size,safety=safety['sine'].strip())))

if __name__=='__main__':main()
