"""Live matrix value/write boundary; unchanged private x86 methods versus x64.
No original writes, engine startup, GPU fidelity or texture-proxy claim.
"""
import json,random,re,shutil,struct,subprocess,tempfile,time,zlib
from pathlib import Path
from datetime import datetime,timezone
import verify_material_equals_evaluation as old
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
ROOT=old.ROOT;OUT=ROOT/'outputs/material-matrix-write-20261027';sha=old.sha;require=old.require
NEW=['material_matrix_value_write_candidate.h','probe_material_matrix_value_write.cpp','test_material_matrix_write_store.cpp','build_material_matrix_write.cmd','verify_material_matrix_write.py']
CHANGED=['material_proxy_evaluation_candidate.h']
IDENTITY=[0x3f800000 if i%5==0 else 0 for i in range(16)]

def corpus():
    cases=[];rng=random.Random(2026102707)
    def add(label,kind=7,metadata=0xff40,op=9,count=0,args=None,matrix=None,initial=None,text=''):
        cases.append(dict(label=label,query=[kind|metadata,0x12345678,0x3e800000,0xbf000000,0x3f400000,0x40800000]+(initial or IDENTITY)+[op,count]+(args or [0]*4)+(matrix or IDENTITY)+[len(text.encode())],text=text))
    translated=IDENTITY.copy();translated[3]=0x3e800000;translated[7]=0xbf000000
    for kind in [0,1,2,4,6,7]:
        for metadata in [0,0x10,0x20,0x40,0x70,0x80,0xff40,0x12340040]:
            for matrix in [IDENTITY,translated,[0]*16]:add('types-and-metadata',kind,metadata,matrix=matrix)
    values=[0,0x80000000,1,0x80000001,0x3f800000,0x3f800001,0xbf800000,0x7f800000,0xff800000,0x7fc12345,0x7fa12345]
    for index in range(16):
        for bits in values:
            matrix=IDENTITY.copy();matrix[index]=bits
            add('identity-one-word-change',matrix=matrix)
            add('same-matrix-reassignment',matrix=matrix,initial=matrix)
    signed=IDENTITY.copy()
    for i in range(16):
        if i%5:signed[i]=0x80000000
    add('all-off-diagonal-negative-zero',matrix=signed)
    for op in range(8):
        for count in ([1,2,3,4] if op==5 else [0]):
            for metadata in [0,0x10,0x40,0x70,0x80,0xff40]:
                for args in [[0]*4,[0x80000000]*4,[0x3f400000,0xc0200000,0x40400000,0x40800000],[0x7fc12345,0x7fa12345,0x7f800000,0xff800000]]:
                    texts=[''] if op!=7 else ['', '.75','-2147483649','1e+','texture/name','3\0ignored','x'*128]
                    for text in texts:add('matrix-to-numeric-or-string',metadata=metadata,op=op,count=count,args=args,text=text,initial=translated)
    for _ in range(1024):
        matrix=[rng.randrange(2**32) for _ in range(16)];initial=[rng.randrange(2**32) for _ in range(16)]
        add('seeded-all-word-patterns',kind=rng.choice([0,1,2,4,6,7]),metadata=rng.choice([0,0x40,0xff40]),matrix=matrix,initial=initial)
    return cases

def decode(raw):
    records=[];at=0
    while at<len(raw):
        require(at+168<=len(raw),'Complete matrix record');words=list(struct.unpack_from('<42I',raw,at));at+=168
        require(at+words[41]<=len(raw),'Complete matrix text');records.append(dict(words=words,text=raw[at:at+words[41]].decode()));at+=words[41]
    require(at==len(raw),'Complete matrix trace');return records

def main():
    pre=json.loads((OUT/'preflight.json').read_text());pointer=pre['prior'];priorPath=Path(pointer['receipt']);require(sha(priorPath)==pointer['receipt_sha256'],'Pinned prior receipt');prior=json.loads(priorPath.read_text());replay=Path(pointer['replay']);require(sha(replay/'manifest.json')==pointer['replay_manifest_sha256'],'Prior manifest');require(all(sha(replay/p)==h for p,h in json.loads((replay/'manifest.json').read_text()).items()),'Prior frozen evidence unchanged')
    changed=[Path(p).name for p,h in prior['sources'].items() if sha(p)!=h];require(set(changed)==set(CHANGED),'Only matrix evaluation boundary changed');require(all(sha(OUT/'before/work'/p)==prior['sources'][str(ROOT/'work'/p)] for p in CHANGED),'Before snapshot identity')
    sources=source_closure([Path(p) for p in prior['sources']]+[ROOT/'work'/n for n in NEW]);protected=[prior['original_hashes'],prior['private_library_hashes'],prior['benchmark_material_hashes']]
    for group in protected:require(all(sha(p)==h for p,h in group.items()),'Original/private/material identities before')
    historyPath=priorPath;history=prior;fixtureHashes={}
    while True:
        fixtureHashes.update(history.get('fixture_hashes',{}))
        if 'prior_receipt' not in history:break
        historyPath=Path(history['prior_receipt']);history=json.loads(historyPath.read_text())
    require(len(fixtureHashes)==2198 and all(sha(p)==h for p,h in fixtureHashes.items()),'All2198 linked historical synthetic fixtures unchanged')
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
    image,ranges=check_ranges(old.PRIVATE,'materialsystem-disassembly.txt',[
        ('matrix_identity',0x10017430,0x1001753f),('free_backing',0x10017ad0,0x10017b46),
        ('matrix_identity_getter',0x10018430,0x10018443),('matrix_getter',0x10018d60,0x10018e5c),
        ('matrix_setter',0x10019f70,0x1001a09f),('string_setter',0x1001a580,0x1001a6e1)])
    slots={3:0x10019350,4:0x10019460,5:0x1001a580,9:0x10019d50,10:0x10019d10,11:0x10019cd0,12:0x10019b00,18:0x1001a0a0,19:0x10019f70,20:0x10018d60,21:0x10018430}
    require(all(image.word(0x10098f34+4*i)==v for i,v in slots.items()),'Actual vtable routes, including slot5 SetString')
    started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_matrix_write.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=600);(run/'build-driver.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Fresh24 build; inspect driver/logs')
    stems=['sine_math','sine','float_input','equals','component','material_proxy_lifetime','unlit_two_texture_parameters','world_transition_parameters','lightmapped_parameters','material_object_parameters','matrix_value']
    tags={'sine_math':'sine-math','sine':'sine','float_input':'float-input','equals':'equals','component':'component','matrix_value':'matrix-value'}
    targets=[(s,n,tags.get(s,s)+('-native' if n else '-owned')) for s in stems for n in [True,False]]+[('material_store',False,'material_store-owned'),('matrix_store_safety',False,'matrix-store')]
    executables=[];logs={}
    for stem,native,tag in targets:
        exe=OUT/(stem+('_reference_x86.exe' if native else '_owned_x64.exe'));exe=OUT/'matrix_store_safety_x64.exe' if stem=='matrix_store_safety' else exe
        raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];machine=0x14c if native else 0x8664
        require(exe.stat().st_mtime>=started-2 and raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if native else 0x20b) and b'asInvoker' in raw,'Fresh PE architecture '+exe.name)
        imports=subprocess.run([str(old.DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require({x.lower() for x in modules}=={'kernel32.dll'},'Independent direct imports '+exe.name)
        executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules));log=OUT/(tag+'-build.log');require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Warning-free '+tag);logs[str(log)]=sha(log)
    logs[str(OUT/'asm-build.log')]=sha(OUT/'asm-build.log')
    def execute(stem,native,args,label):
        exe=OUT/(stem+('_reference_x86.exe' if native else '_owned_x64.exe'));exe=OUT/'matrix_store_safety_x64.exe' if stem=='matrix_store_safety' else exe
        p=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' exit='+str(p.returncode)+'; inspect '+str(run/(label+'.log')));return p.stdout
    cases=corpus();query=run/'query.bin';query.write_bytes(b''.join(struct.pack('<45I',*c['query'])+c['text'].encode() for c in cases));(run/'cases.json').write_text(json.dumps(cases,indent=2));traces=[]
    for native in [True,False]:
        dest=run/('native.bin' if native else 'owned.bin');traces.append(execute('matrix_value',native,[old.PRIVATE if native else run/'absent.dll',query,dest],dest.stem))
    a=decode((run/'native.bin').read_bytes());b=decode((run/'owned.bin').read_bytes());require(len(a)==len(b)==len(cases),'Case count')
    if a!=b:
        diffs=[i for i,(x,y) in enumerate(zip(a,b)) if x!=y];i=diffs[0];(run/'difference.json').write_text(json.dumps(dict(count=len(diffs),first_index=i,case=cases[i],native=a[i],owned=b[i]),indent=2));raise RuntimeError('Matrix values differ; inspect '+str(run/'difference.json'))
    checks=0
    def identity(matrix):return all((bits&0x7fffffff)==0 if i%5 else bits==0x3f800000 for i,bits in enumerate(matrix))
    for c,s in zip(cases,a):
        q=c['query'];w=s['words']
        if q[22]==9:
            require(w[0]==((q[0]&~15)|7) and w[1:6]==[0]*5,'Matrix clears caches and preserves metadata');checks+=1
            require(w[6]==1 and w[7]==w[8]==int(identity(q[28:44])) and w[9:25]==w[25:41]==q[28:44] and not s['text'],'Full sixteen exact matrix words and ordered identity');checks+=1
        else:
            require(w[6:9]==[0,0,1] and w[9:25]==[0]*16 and w[25:41]==IDENTITY,'Detached matrix and identity fallback');checks+=1
            if q[22]==6:require(w[:6]==[(q[0]&~15)|6]+q[1:6],'Undefined preserves cached numbers');checks+=1
    safety=execute('matrix_store_safety',False,[],'matrix-store-safety');require('PASS matrix_store_safety_checks=' in safety,'Matrix safety')
    regressions=[];historyPath=priorPath;history=prior
    stages=[('sine',True),('float_input',True),('equals',True),('material_proxy_lifetime',False),('unlit_two_texture_parameters',False),('world_transition_parameters',False),('lightmapped_parameters',False),('material_object_parameters',False)]
    for stem,prepared in stages:
        for native,tail in [(True,[]),(False,[])]+([(True,['--sse2'])] if stem=='sine' else []):
            label=stem+('-prior-native' if native else '-prior-owned')+('-sse2' if tail else '');dest=run/(label+'.bin')
            args=[old.CLIENT if native else run/'absent.dll',historyPath.parent/'query.bin',historyPath.parent/'prepared-original.bin',dest,*tail] if prepared else [old.PRIVATE if native else run/'absent.dll',old.LIBS if native else run/'absent-directory',historyPath.parent/'mount',historyPath.parent/'query.bin',dest]
            execute(stem,native,args,label);require(sha(dest)==history['output_sha256'],'Full prior corpus '+label);regressions.append(dict(stem=stem,native=native,dispatch=tail,cases=history['cases'],sha256=sha(dest)))
        historyPath=Path(history['prior_receipt']);history=json.loads(historyPath.read_text())
    mount=run/'mount';mount.mkdir();storeDest=run/'store-prior.txt';execute('material_store',False,[run/'absent.dll',run/'absent-directory',mount,historyPath.parent/'query.txt',storeDest],'store-prior');require(sha(storeDest)==history['trace_sha256'],'6064 ownership operations unchanged')
    floatPath=Path(prior['prior_receipt']);floatReceipt=json.loads(floatPath.read_text());eqPath=Path(floatReceipt['prior_receipt']);eq=json.loads(eqPath.read_text())
    for native in [True,False]:
        dest=run/('component-prior-native.bin' if native else 'component-prior-owned.bin');execute('component',native,[old.PRIVATE if native else run/'absent.dll',eqPath.parent/'component-query.bin',dest],dest.stem);require(sha(dest)==eq['component_output_sha256'],'2408 components unchanged')
    for native,tail,label in [(True,[],'math-native'),(True,['--sse2'],'math-native-sse2'),(False,[],'math-owned')]:
        dest=run/(label+'.bin');execute('sine_math',native,[old.CLIENT if native else run/'absent.dll',priorPath.parent/'math-query.bin',dest,*tail],label);require(sha(dest)==prior['math_output_sha256'],'17762 exact Sine math results '+label)
    oldSafety={}
    for stem,arg,key in [('sine','--test-sine','sine'),('float_input','--test','float_input'),('equals','--test-equals','equals'),('material_proxy_lifetime','--test-proxies','proxies'),('unlit_two_texture_parameters','--test-unlit-two-texture','two'),('world_transition_parameters','--test-world-transition','world'),('lightmapped_parameters','--test-lightmapped','lightmapped'),('material_object_parameters','--test','parameters'),('material_store','--test','ownership')]:oldSafety[key]=execute(stem,False,[arg],key+'-safety')
    require(oldSafety==prior['safety'],'All earlier safety traces/counts unchanged')
    life=json.loads(Path(eq['prior_receipt']).read_text());twoPath=Path(life['prior_receipt']);two=json.loads(twoPath.read_text());pilot=twoPath.parent/'mount/materials/pilot-original.vmt';require(sha(pilot)==prior['pilot']['sha256'],'Disposable pilot unchanged');refusal=execute('unlit_two_texture_parameters',False,['--test-pilot-refusal',pilot],'whole-pilot-refusal');require(refusal==prior['pilot_refusal'],'Whole pilot refusal unchanged')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());require(all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()) and sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor310 graph/executable unchanged')
    for group in [sources,*protected,fixtureHashes]:require(all(sha(p)==h for p,h in group.items()),'Final protected identities')
    require(not (run/'absent.dll').exists() and not (run/'absent-directory').exists(),'Owned original arguments absent');counts=re.search(r'native_allocations=(\d+) releases=(\d+)',traces[0]);require(counts and counts[1]==counts[2],'Original backing allocation balance')
    capture=run/'complete-matrix.bin.zlib';capture.write_bytes(zlib.compress((run/'native.bin').read_bytes(),9))
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',scope='Resource-free/string/matrix ownerless immediate SetMatrix/GetMatrix/IsMatrixIdentity and matrix-to-scalar/vector/string/undefined values; authenticated transactional16-word write payload',cases=len(cases),exact_bytes=(run/'native.bin').stat().st_size,independent_checks=checks,query_sha256=sha(query),output_sha256=sha(run/'native.bin'),native_allocations=int(counts[1]),native_releases=int(counts[2]),safety=safety,prior_safety=oldSafety,regressions=regressions,math_regression_cases=17762,component_regression_cases=2408,ownership_regression_operations=6064,pilot_refusal=refusal,sources=sources,new_sources=NEW,changed_sources=CHANGED,original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],benchmark_material_hashes=prior['benchmark_material_hashes'],fixture_hashes=fixtureHashes,executables=executables,build_logs=logs,static_ranges=ranges,prior_receipt=str(priorPath),prior_receipt_sha256=sha(priorPath),editor_dependencies_unchanged=len(build['editor_source_dependencies']),editor_sha256=build['executable_sha256'],limits=['Explicit raw ownerless/no-queue CMaterialVar states, not full material owner/resource/queued notification ABI','Owned copy/get-by-value and alias-safe writes intentionally avoid native pointer alias use-after-free; original allocation failures are unsafe and not executed','Actual TextureTransform/TextureScroll are not implemented or verified here; no clock/lookup/cosine semantics guessed','No GPU upload, editor integration, original writes, live SFM control or whole-goal completion claim'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');frozen=OUT/('replay-'+run.name.removeprefix('verification-'));frozen.mkdir()
    included=[Path(p) for p in sources]+[Path(e['path']) for e in executables]+[Path(p) for p in logs]+[receipt,query,capture,OUT/'preflight.json']+list(run.glob('*.json'))+list(run.glob('*.log'))+[OUT/'before/work'/n for n in CHANGED]
    for p in included:
        target=frozen/p.relative_to(ROOT);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target)
    manifest={p.relative_to(frozen).as_posix():sha(p) for p in frozen.rglob('*') if p.is_file()};(frozen/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');pointer=dict(receipt=str(receipt),receipt_sha256=sha(receipt),replay=str(frozen),replay_manifest_sha256=sha(frozen/'manifest.json'));(OUT/'latest-verification.json').write_text(json.dumps(pointer,indent=2)+'\n');print(json.dumps(dict(status='PASS',receipt=str(receipt),cases=len(cases),exact_bytes=result['exact_bytes'],safety=safety.strip())),flush=True)

if __name__=='__main__':main()
