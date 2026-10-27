"""Actual client Equals and scalar-result helper, separately verified vector
component setter, and transactional native-x64 material-store evaluation.
Private unchanged original child oracles only; never a live SFM instance.
"""
import hashlib,json,random,re,shutil,struct,subprocess,tempfile,time,zlib
from pathlib import Path
from datetime import datetime,timezone
from verify_material_numeric_value import ROOT,PRIVATE,require,sha
from verify_material_keyvalues_parser import DUMP
from verify_builtin_error_material_seed import source_closure
from verify_material_object_parameters import node,string,packet,text,decode
from verify_ifm_draw_packet import check_ranges
OUT=ROOT/'outputs/material-proxy-evaluation-20261027'
LIBS=ROOT/'outputs/material-fallback-owned-20261026/verification-0mhr77s9/copied-libraries'
CLIENT=OUT/'copied-bin/client.dll'
CLIENT_ORIGINAL=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/tf/bin/client.dll')
CLIENT_SHA='91dd1062227b095f1338312aa7268121894970c3e0a5b0d314e42c77f2aba73b'
CHANGED=['material_proxy_lifetime_candidate.h','material_object_parameter_initialization_candidate.h','material_object_store_candidate.h']
NEW=['material_vector_component_candidate.h','material_proxy_evaluation_candidate.h','material_equals_proxy_candidate.h','material_equals_client_native_fixture.h','probe_material_equals_evaluation.cpp','probe_material_proxy_component.cpp','build_material_equals_evaluation.cmd','verify_material_equals_evaluation.py']

def fbits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def literal(name,kind):
    if kind=='float':return node(name,kind=3,bits=fbits(.375))
    if kind=='int':return node(name,kind=2,bits=0xfffffffd)
    if kind=='string':return string(name,'12.75 trailing')
    if kind=='vector':return string(name,'[.25 -.5 .75 1]')
    if kind.startswith('vec'):return string(name,'['+' '.join(['.125','.25','.5','1'][:int(kind[-1])])+']')
    raise RuntimeError(kind)
def declaration(result,source='$src',second=''):
    return node('Equals',[string('resultVar',result),string('srcVar1',source)]+([string('srcVar2',second)] if second else []))
def corpus(prior):
    cases=[]
    def add(label,outKind='float',srcKind='float',result='$out',second='',mode=0,bits=fbits(-.75),extra=None,proxies=None,source='$src'):
        fields=[]
        if outKind not in ['undefined','matrix','color']:fields.append(literal('$out',outKind))
        if srcKind!='undefined':fields.append(literal('$src',srcKind))
        if extra:fields+=extra
        if outKind in ['undefined','matrix','color'] and result=='$out':result={'undefined':'$texture2','matrix':'$basetexturetransform','color':'$color'}[outKind]
        if srcKind=='undefined' and source=='$src':source='$basetexture'
        fields.append(node('Proxies',proxies if proxies is not None else [declaration(result,source,second)]));cases.append(dict(label=label,root=node('UnlitTwoTexture',fields),mode=mode,bits=bits,outKind=outKind,srcKind=srcKind,result=result))
    for outKind in ['float','int','string','vec1','vec2','vec3','vec4','undefined','matrix','color']:
        for srcKind in ['float','int','string','vector','undefined']:add('typed-'+outKind+'-'+srcKind,outKind,srcKind)
    for n in range(1,5):
        for suffix in ['[-7]','[-1]','[0]','[1]','[2]','[3]','[foo]','[ 1junk','[+01]']:
            for source in ['float','int','string','vector','undefined']:add('component-'+str(n)+suffix+'-'+source,'vec'+str(n),source,'$out'+suffix)
    values=[0,0x80000000,fbits(.375),fbits(-.75),fbits(1),0x7f7fffff,0x7fc12345]
    for outKind in ['float','int','string','vec1','vec2','vec3','vec4','undefined','color']:
        for bits in values:add('shared-scalar-'+outKind,outKind,mode=1,bits=bits)
    for n in range(1,5):
        for component in range(4):
            for bits in values:add('shared-component-'+str(n)+'-'+str(component),'vec'+str(n),result='$out['+str(component)+']',mode=1,bits=bits)
    for secondary in ['float','int','string','vector','undefined']:
        extra=[] if secondary=='undefined' else [literal('$secondary',secondary)];add('secondary-type-'+secondary,'undefined','undefined',second='$texture2' if secondary=='undefined' else '$secondary',extra=extra)
    for result,source,second in [('$missing','$src',''),('$out','$missing',''),('$out','$src[0]',''),('$out','$src','$missing'),('','$src',''),('$out','','')]:add('init-refusal',result=result,source=source,second=second)
    for outKind in ['float','int','vec2']:add('self-alias',outKind,source='$out')
    for component in [0,1,2,3]:add('declaration-order-chain','vec2',extra=[literal('$next','vec4')],proxies=[declaration('$out['+str(component)+']'),declaration('$next','$out')])
    rng=random.Random(2026102701)
    for i in range(256):
        add('seeded-component','vec'+str(i%4+1),result='$out['+str(rng.randrange(4))+']',extra=[node('$src',kind=3,bits=fbits(rng.uniform(-4096,4096)))],srcKind='undefined',source='$src')
    pilot=next(Path(p) for p in prior['benchmark_material_hashes'] if p.lower().endswith('/pilotlight.vmt') or p.lower().endswith('\\pilotlight.vmt'));raw=pilot.read_bytes();configs=[]
    for block in re.findall(r'\bEquals\s*\{([^{}]*)\}',raw.decode('ascii'),re.I):
        fields={m[0].lower():(m[1] or m[2]) for m in re.findall(r'(srcVar1|resultVar)\s+(?:"([^"]*)"|([^\s{}]+))',block,re.I)};require(set(fields)=={'srcvar1','resultvar'},'Actual pilot Equals declaration fields');configs.append(declaration(fields['resultvar'],fields['srcvar1']))
    require(len(configs)==2,'Two actual pilot Equals declarations')
    for a,b in [(0.,0.),(-.02,.005),(.02,-.005)]:
        add('actual-pilot-equals-controlled-inputs','vec2',extra=[string('$temp','[0 0]'),string('$pilot',str(a)),string('$pilot2',str(b))],proxies=configs)
    return cases,dict(path=str(pilot),sha256=sha(pilot),equals_declarations=configs,scope='Actual Equals declarations with controlled scalar inputs; Sine, transforms, scroll and weapon_invis are not evaluated')

def component_corpus():
    cases=[];values=[0,0x80000000,0x3ec00000,0xbf400000,0x7f800000,0xff800000,0x7fc12345,0x7fa12345,0x7f7fffff]
    for count in range(5):
        for index in range(4):
            for bits in values:
                for vector in [[0,0,0,0],[0x80000000,0x7fc12345,0x3f800000,0xbf800000]]:cases.append([0xdeadff02|(count<<4),0xa5a5a5a5,*vector,index,bits])
    rng=random.Random(2026102702)
    for _ in range(2048):cases.append([0x1234ff02|(rng.randrange(5)<<4),rng.getrandbits(32),*[rng.getrandbits(32) for _ in range(4)],rng.randrange(4),rng.getrandbits(32)])
    return cases

def decode_frames(raw):
    at=0
    def u():
        nonlocal at
        require(at+4<=len(raw),'Frame word extent');v=struct.unpack_from('<I',raw,at)[0];at+=4;return v
    def chars(n):
        nonlocal at
        require(at+n<=len(raw),'Frame text extent');v=raw[at:at+n].decode('ascii');at+=n;return v
    def s():return chars(u())
    count=u();cases=[]
    for _ in range(count):
        c=dict(index=u(),initialized=bool(u()),frames=[])
        if c['initialized']:
            for frame in range(3):
                writes=[[u() for _ in range(7)] for _ in range(u())];shader=s();values=[]
                for _ in range(u()):q=[u() for _ in range(28)];values.append(dict(words=q,text=chars(q[10]),name=chars(q[11])))
                c['frames'].append(dict(writes=writes,shader=shader,values=values,forward=[u() for _ in range(7)]))
        cases.append(c)
    require(at==len(raw),'Complete frame decoding');return cases

def main():
    pre=json.loads((OUT/'preflight.json').read_text());priorPointer=pre['prior'];priorPath=Path(priorPointer['receipt']);require(sha(priorPath)==priorPointer['receipt_sha256'],'Accepted prior receipt');prior=json.loads(priorPath.read_text());priorReplay=Path(priorPointer['replay']);require(sha(priorReplay/'manifest.json')==priorPointer['replay_manifest_sha256'],'Prior frozen manifest');require(all(sha(priorReplay/p)==h for p,h in json.loads((priorReplay/'manifest.json').read_text()).items()),'Prior frozen files')
    changes=[Path(p).name for p,h in prior['sources'].items() if sha(Path(p))!=h];require(set(changes)==set(CHANGED),'Bounded production edits');require(all(sha(OUT/'before/work'/n)==prior['sources'][str(ROOT/'work'/n)] for n in CHANGED),'Before snapshots');sources=source_closure([Path(p) for p in prior['sources']]+[ROOT/'work'/n for n in NEW]);require(sha(CLIENT)==sha(CLIENT_ORIGINAL)==CLIENT_SHA,'Exact original client copy')
    original=dict(prior['original_hashes'],**{str(CLIENT_ORIGINAL):CLIENT_SHA});private=dict(prior['private_library_hashes'],**{str(CLIENT):CLIENT_SHA})
    for group in [original,private,prior['benchmark_material_hashes']]:require(all(sha(Path(p))==h for p,h in group.items()),'Protected originals before execution')
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));mount=run/'mount';mount.mkdir()
    # Byte-checked static provenance, including actual registration, factories,
    # Init/OnBind/Release and shared helper. Sine ranges are discovery ONLY.
    image,ranges=check_ranges(CLIENT,'../outputs/material-proxy-evaluation-20261027/client-disassembly-private.txt',[
        ('Equals_registration',0x10492190,0x104921a9),('Equals_factory',0x10137430,0x10137456),('Equals_Init',0x100e5fa0,0x100e6033),('Equals_OnBind',0x101373a0,0x1013742d),('basic_result_Init',0x100e5db0,0x100e5ea9),('scalar_result_write',0x100e5eb0,0x100e5f52),('effective_output_type',0x100e6040,0x100e60cc),('GetMaterial',0x100e5f90,0x100e5f9a),('Release',0x10175ed0,0x10175ede),('destructor',0x10137780,0x101377a1),('Sine_registration_static_only',0x10492170,0x10492189),('Sine_Init_static_only',0x10136440,0x101364e4),('Sine_OnBind_static_only',0x101364f0,0x101365a7)])
    require(image.string(0x105000cc)=='Equals' and image.string(0x105000c4)=='Sine' and [image.word(0x104fff3c+4*i) for i in range(5)]==[0x100e5fa0,0x101373a0,0x10175ed0,0x100e5f90,0x10137780],'Actual client registration/vtable identity')
    _,componentRanges=check_ranges(PRIVATE,'materialsystem-disassembly.txt',[('SetVecComponentValue',0x10019d90,0x10019f6f)])
    started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_equals_evaluation.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=360);(run/'build-driver.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Fresh Equals/component/prior builds')
    targets=[('equals',True,'equals-native'),('equals',False,'equals-owned'),('component',True,'component-native'),('component',False,'component-owned')]+[(stem,native,stem+('-native' if native else '-owned')) for stem in ['material_proxy_lifetime','unlit_two_texture_parameters','world_transition_parameters','lightmapped_parameters','material_object_parameters'] for native in [True,False]]+[('material_store',False,'material_store-owned')];exes=[];logs={}
    for stem,native,tag in targets:
        exe=OUT/(stem+('_reference_x86.exe' if native else '_owned_x64.exe'));b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];machine=0x14c if native else 0x8664;require(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==(0x10b if native else 0x20b) and b'asInvoker' in b,'Fresh executable architecture '+exe.name);imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require({x.lower() for x in modules}=={'kernel32.dll'},'Direct imports '+exe.name);exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules));log=OUT/(tag+'-build.log');require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Clean build '+tag);logs[str(log)]=sha(log)
    logs[str(OUT/'asm-build.log')]=sha(OUT/'asm-build.log')
    def execute(stem,native,args,label):
        exe=OUT/(stem+('_reference_x86.exe' if native else '_owned_x64.exe'));p=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,stem+' exit='+str(p.returncode)+'; inspect '+label+'.log');return p.stdout
    comps=component_corpus();componentQuery=run/'component-query.bin';componentQuery.write_bytes(b''.join(struct.pack('<8I',*x) for x in comps));componentA=run/'component-reference.bin';componentB=run/'component-owned.bin';execute('component',True,[PRIVATE,componentQuery,componentA],'component-reference');execute('component',False,[run/'absent-original.dll',componentQuery,componentB],'component-owned');require(componentA.read_bytes()==componentB.read_bytes(),'Actual component setter complete states');componentChecks=0
    for i,q in enumerate(comps):
        v=struct.unpack_from('<7I',componentA.read_bytes(),i*28);require(v[1]==q[1],'Component write preserves integer cache');componentChecks+=1
        if ((q[0]>>4)&7)==0 and q[6]==3 and q[7]==0x3ec00000:require(v[0]&0x70==0x30 and v[2:5]==(0,0,0) and v[5]==q[7],'Measured count=index growth and gap fill');componentChecks+=1
    cases,pilot=corpus(prior);query=run/'query.bin';query.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<2I',c['mode'],c['bits'])+packet(c['root']) for c in cases));(run/'cases.json').write_text(json.dumps(cases,indent=2));(run/'pilot-equals-scope.json').write_text(json.dumps(pilot,indent=2))
    # Independent initial states come from the real normally constructed
    # materialsystem objects with its deliberately null proxy factory. Full
    # proxy declarations stay intact; no original asset is rewritten.
    cpuQuery=run/'cpu-query.bin';cpuQuery.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<4I',0,0,0,0)+struct.pack('<12I',0,0,0,0,0,0,0,0,1,0,92,0)+text('equals/case')+packet(c['root'])+struct.pack('<6I',0,0,0,0,0,0) for c in cases));cpuOut=run/'cpu-original.bin';cpuExecution=execute('unlit_two_texture_parameters',True,[PRIVATE,LIBS,mount,cpuQuery,cpuOut],'cpu-original');n,records=decode(cpuOut.read_bytes());initial=[r for r in records if r['stage']==1];require(n==len(cases)==len(initial) and all(r['result'] for r in initial),'Actual material CPU initial states')
    prepared=run/'prepared-original.bin';prepared.write_bytes(struct.pack('<I',n)+b''.join(text(s['shader'])+struct.pack('<I',len(s['values']))+b''.join(struct.pack('<28I',*v['words'])+v['text'].encode('ascii')+v['name'].encode('ascii') for v in s['values']) for s in initial));a=run/'equals-reference.bin';b=run/'equals-owned.bin';nativeExecution=execute('equals',True,[CLIENT,query,prepared,a],'equals-reference');execute('equals',False,[run/'absent-original.dll',query,prepared,b],'equals-owned');require(a.read_bytes()==b.read_bytes(),'Actual client Equals/scalar-helper dispatch and complete typed states');decoded=decode_frames(a.read_bytes());checks=0;frames=0;refused=0
    for c,state in zip(cases,decoded):
        require(state['initialized']!=(c['label']=='init-refusal'),'Expected Init result');checks+=1
        if not state['initialized']:refused+=1;continue
        require(len(state['frames'])==3,'Three evaluated frames');checks+=1;frames+=3
        for frame in state['frames']:
            values={v['name'].lower():v for v in frame['values']};require(frame['forward'][0]==values['$alpha']['words'][3] and frame['forward'][1:4]==values['$color']['words'][3:6],'Forwarding snapshot updated from evaluated caches');checks+=1
            if c['mode']==0 and c['outKind'] in ['string','matrix']:require(not frame['writes'],'Actual string/matrix Equals no-op');checks+=1
            if c['label']=='actual-pilot-equals-controlled-inputs':require(len(frame['writes'])==2 and all(w[1]==8 for w in frame['writes']) and [w[2] for w in frame['writes']]==[0,1],'Actual pilot declarations use two component writes');checks+=1
            if c['label']=='declaration-order-chain':require(len(frame['writes'])==2 and values['$next']['words'][3]==values['$out']['words'][3],'Later proxy reads staged earlier write');checks+=1
    nativeCounts=re.search(r'native_proxy_allocations=(\d+) releases=(\d+)',nativeExecution);cpuCounts=re.search(r'native_allocations=(\d+) releases=(\d+)',cpuExecution);require(nativeCounts and nativeCounts[1]==nativeCounts[2] and cpuCounts and cpuCounts[1]==cpuCounts[2],'Original client proxy/material CPU cleanup balanced')
    regressions=[]
    for native in [True,False]:
        dest=run/('proxy-prior-reference.txt' if native else 'proxy-prior-owned.txt');execute('material_proxy_lifetime',native,[PRIVATE if native else run/'absent-original.dll',LIBS if native else run/'absent-directory',priorPath.parent/'mount',priorPath.parent/'query.bin',dest],dest.stem);require(sha(dest)==prior['output_sha256'],'All210 accepted lifetime cases unchanged');regressions.append(dict(stem='material_proxy_lifetime',native=native,cases=prior['cases'],output_sha256=sha(dest)))
    previousPath=Path(prior['prior_receipt']);previous=json.loads(previousPath.read_text())
    for stem in ['unlit_two_texture_parameters','world_transition_parameters','lightmapped_parameters','material_object_parameters']:
        oldrun=previousPath.parent;oldquery=oldrun/'query.bin';require(sha(oldquery)==previous['query_sha256'],'Prior input identity')
        for native in [True,False]:
            dest=run/(stem+('-reference.bin' if native else '-owned.bin'));execute(stem,native,[PRIVATE if native else run/'absent-original.dll',LIBS if native else run/'absent-directory',oldrun/'mount',oldquery,dest],dest.stem);require(sha(dest)==previous['output_sha256'],'Whole prior material corpus unchanged '+dest.name);regressions.append(dict(stem=stem,native=native,cases=previous['cases'],output_sha256=sha(dest)))
        previousPath=Path(previous['prior_receipt']);previous=json.loads(previousPath.read_text())
    storeQuery=previousPath.parent/'query.txt';require(sha(storeQuery)==previous['query_sha256'],'Ownership input identity');storeOut=run/'store-regression.txt';execute('material_store',False,[run/'absent-original.dll',run/'absent-directory',mount,storeQuery,storeOut],'store-regression');require(sha(storeOut)==previous['trace_sha256'],'6064-operation ownership trace unchanged')
    safety={}
    for stem,arg,key in [('equals','--test-equals','equals'),('material_proxy_lifetime','--test-proxies','proxies'),('unlit_two_texture_parameters','--test-unlit-two-texture','two'),('world_transition_parameters','--test-world-transition','world'),('lightmapped_parameters','--test-lightmapped','lightmapped'),('material_object_parameters','--test','parameters'),('material_store','--test','ownership')]:safety[key]=execute(stem,False,[arg],key+'-safety')
    require(all(safety[k]==prior['safety'][k] for k in prior['safety']),'Every previous safety count unchanged')
    twoPath=Path(prior['prior_receipt']);two=json.loads(twoPath.read_text());pilotCopy=twoPath.parent/'mount/materials/pilot-original.vmt';require(sha(pilotCopy)==pilot['sha256'],'Untouched disposable full pilot');pilotRefusal=execute('unlit_two_texture_parameters',False,['--test-pilot-refusal',pilotCopy],'pilot-full-refusal');require(pilotRefusal==two['pilot_refusal'],'Complete pilot still safely refused by default')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());graph=set()
    def visit(p):
        name=p.relative_to(ROOT).as_posix()
        if name in graph:return
        graph.add(name)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            q=p.parent/inc
            if q.is_file():visit(q.resolve())
    visit((ROOT/'work/probe_vulkan_win32_surface_x64.cpp').resolve());require(graph==set(build['editor_source_dependencies']) and all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()),'Whole editor graph unchanged');require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor unchanged')
    for group in [sources,original,private,prior['benchmark_material_hashes']]:require(all(sha(Path(p))==h for p,h in group.items()),'Protected final identities')
    require(not (run/'absent-original.dll').exists() and not (run/'absent-directory').exists(),'Owned native independence arguments absent');capture=run/'complete-equals.bin.zlib';capture.write_bytes(zlib.compress(a.read_bytes(),9));componentCapture=run/'complete-components.bin.zlib';componentCapture.write_bytes(zlib.compress(componentA.read_bytes(),9))
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',scope='Actual Equals and shared scalar-result helper, checked component writes, and owned transactional material-store evaluation',cases=len(cases),frames=frames,init_refused=refused,exact_bytes=a.stat().st_size,query_sha256=sha(query),output_sha256=sha(a),independent_checks=checks,component_cases=len(comps),component_exact_bytes=componentA.stat().st_size,component_checks=componentChecks,component_query_sha256=sha(componentQuery),component_output_sha256=sha(componentA),native_proxy_allocations=int(nativeCounts[1]),native_proxy_releases=int(nativeCounts[2]),native_cpu_allocations=int(cpuCounts[1]),native_cpu_releases=int(cpuCounts[2]),cpu_query_sha256=sha(cpuQuery),cpu_original_output_sha256=sha(cpuOut),prepared_original_sha256=sha(prepared),safety=safety,regressions=regressions,store_regression=dict(operations=6064,trace_sha256=sha(storeOut)),sources=sources,changed_sources=CHANGED,new_sources=NEW,original_hashes=original,private_library_hashes=private,benchmark_material_hashes=prior['benchmark_material_hashes'],executables=exes,build_logs=logs,client_static_ranges=ranges,component_static_ranges=componentRanges,pilot=pilot,pilot_refusal=pilotRefusal,prior_receipt=str(priorPath),prior_receipt_sha256=sha(priorPath),editor_dependencies_unchanged=len(graph),editor_sha256=build['executable_sha256'],limits=['Equals string KV configuration and exact factory spelling; alpha-result Init requiring renderer flag/snapshot side effects explicitly refuses; unsafe component index>3 refuses','Original client proxy methods execute unchanged against explicit material/variable service spies seeded by independently executed real original materialsystem CPU states; component setter separately executes real original materialsystem method','Values/getter caches and other numeric setters retain earlier proof gates; this is not a combined original client/materialsystem process or complete engine/renderer proof','Evaluation is synchronous owner-thread and atomic at owned publication; partial failed native writes are deliberately not reproduced','Sine is located statically only; TextureTransform, TextureScroll and weapon_invis remain unfinished; whole original pilot is not yet supported','Editor graph/build unchanged; no live SFM, Blender, original writes or claimed whole-project completion'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');replay=OUT/('replay-'+run.name.removeprefix('verification-'));replay.mkdir();included=[Path(p) for p in sources]+[Path(e['path']) for e in exes]+[Path(p) for p in logs]+[receipt,query,componentQuery,cpuQuery,prepared,capture,componentCapture,OUT/'preflight.json']+list(run.glob('*.json'))+list(run.glob('*.log'))+[OUT/'before/work'/n for n in CHANGED]
    for p in included:t=replay/p.relative_to(ROOT);t.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,t)
    manifest={p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()};(replay/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');pointer=dict(receipt=str(receipt),receipt_sha256=sha(receipt),replay=str(replay),replay_manifest_sha256=sha(replay/'manifest.json'));(OUT/'latest-verification.json').write_text(json.dumps(pointer,indent=2)+'\n');print(json.dumps(dict(status='PASS',receipt=str(receipt),cases=len(cases),frames=frames,component_cases=len(comps),exact_bytes=a.stat().st_size,safety=safety['equals'])))

if __name__=='__main__':main()
