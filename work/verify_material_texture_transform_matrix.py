"""Actual TextureTransform numerical pipeline and independently checked reads.
Shared dummy-variable observations are original-only prerequisites, not owned
proxy initialization/lifetime support. Originals/live SFM are never written.
"""
import json,random,re,shutil,struct,subprocess,tempfile,time,zlib
from pathlib import Path
from datetime import datetime,timezone
import verify_material_matrix_write as previous
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
old=previous.old;ROOT=old.ROOT;OUT=ROOT/'outputs/material-texture-transform-20261027';sha=old.sha;require=old.require
NEW=['material_texture_transform_matrix_candidate.h','probe_material_texture_transform_matrix.cpp','probe_material_texture_input_reference.cpp','build_material_texture_transform_matrix.cmd','verify_material_texture_transform_matrix.py']

def corpus():
    f=old.fbits;base=[f(x) for x in [.25,.75,1.5,.5,30.,.125,-.25]];cases=[]
    def add(label,mask,values,expected=None):cases.append(dict(label=label,words=[mask]+values,expected=expected))
    identity=previous.IDENTITY
    for mask in range(16):add('optional-input-mask',mask,base,identity if mask==0 else None)
    specials=[0,0x80000000,1,0x80000001,0x00800000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc12345,0x7fa12345,0xffc54321,0xffa54321,0x7f7fffff]
    for mask in [1,2,4,8,15]:
        for index in range(7):
            for bits in specials:
                v=base.copy();v[index]=bits;add('special-cached-input',mask,v)
    for mask in range(16):
        for bits in specials:add('same-special-all-inputs',mask,[bits]*7,identity if mask==0 else None)
    for degrees in [0.,90.,-90.,180.,360.]:
        add('exact-quarter-turn',4,[f(.5),f(.5),f(1),f(1),f(degrees),0,0])
    translated=identity.copy();translated[3]=f(.125);translated[7]=f(-.25);add('analytical-translation',8,base,translated)
    scaled=identity.copy();scaled[0]=f(1.5);scaled[5]=f(.5);scaled[3]=f(-.25);scaled[7]=f(.25);add('analytical-default-center-scale',2,base,scaled)
    rng=random.Random(2026102709)
    for _ in range(4096):add('seeded-finite',rng.randrange(16),[f(rng.uniform(-1e5,1e5)) for i in range(7)])
    for _ in range(4096):add('seeded-all-bit-patterns',rng.randrange(16),[rng.randrange(2**32) for i in range(7)])
    return cases

def getter_corpus():
    rng=random.Random(2026102710);rows=[];values=[0,0x80000000,1,0x80000001,0x00800000,0x7f7fffff,0xff7fffff,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc12345,0x7fa12345,0xffc54321,0xffa54321]
    for count in range(1,5):
        for bits in values:
            for kind in [0,1,2,3,4,5,6,7]:rows.append([0xff40|kind,bits,bits,bits,bits,count])
    for _ in range(2048):rows.append([0xff00|rng.randrange(256),*[rng.randrange(2**32) for i in range(4)],rng.randrange(1,5)])
    return rows

def main():
    pre=json.loads((OUT/'preflight.json').read_text());ptr=pre['prior'];priorPath=Path(ptr['receipt']);require(sha(priorPath)==ptr['receipt_sha256'],'Prior receipt');prior=json.loads(priorPath.read_text());pf=Path(ptr['replay']);require(sha(pf/'manifest.json')==ptr['replay_manifest_sha256'],'Prior replay manifest');priorManifest=json.loads((pf/'manifest.json').read_text());require(all(sha(pf/p)==h for p,h in priorManifest.items()),'Prior replay files');require(all(sha(p)==h for p,h in prior['sources'].items()),'All182 existing sources unchanged')
    sources=source_closure([Path(p) for p in prior['sources']]+[ROOT/'work'/n for n in NEW]);protected=[prior[k] for k in ['original_hashes','private_library_hashes','benchmark_material_hashes','fixture_hashes']]
    for group in protected:require(all(sha(p)==h for p,h in group.items()),'Protected original/private/fixture inputs before')
    require(all(sha(e['path'])==e['sha256'] for e in prior['executables']),'Earlier24 executable identities')
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));mount=run/'mount';mount.mkdir();print('Verification directory: '+str(run),flush=True)
    image,ranges=check_ranges(old.CLIENT,'../outputs/material-proxy-evaluation-20261027/client-disassembly-private.txt',[
        ('TextureTransform_registration',0x10492310,0x10492329),('factory',0x10137e40,0x10137e66),('Init',0x10137ad0,0x10137baf),('OnBind',0x10137c30,0x10137e3d),('basic_result_Init',0x100e5db0,0x100e5ea9),('matrix_multiply',0x102edd70,0x102ee171),('translate',0x102ee380,0x102ee3f1),('rotate',0x102ee450,0x102ee4f1),('scale',0x102ee500,0x102ee571)])
    require([image.string(a) for a in [0x10500194,0x10500188,0x1050017c,0x1050016c]]==['centerVar','scaleVar','rotateVar','translateVar'],'Actual field names')
    require([image.word(0x105001a4+4*i) for i in range(5)]==[0x10137ad0,0x10137c30,0x10175ed0,0x100e5f90,0x10137780],'Actual proxy vtable')
    _,getterRanges=check_ranges(old.PRIVATE,'materialsystem-disassembly.txt',[('FindVar',0x10004520,0x100045e2),('GetFloat',0x10017c20,0x10017c6a),('GetVec',0x10017cc0,0x10017d77),('GetCount',0x10017d80,0x10017dd1)])
    started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_texture_transform_matrix.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=240);(run/'build-driver.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Fresh texture/getter builds')
    executables=[];logs={}
    for name,native,tag in [('texture_transform_reference_x86.exe',True,'native'),('texture_transform_owned_x64.exe',False,'owned'),('texture_input_reference_x86.exe',True,'input-native')]:
        exe=OUT/name;b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];machine=0x14c if native else 0x8664;require(exe.stat().st_mtime>=started-2 and b[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==(0x10b if native else 0x20b) and b'asInvoker' in b,'Fresh architecture '+name);imports=subprocess.run([str(old.DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require({x.lower() for x in modules}=={'kernel32.dll'},'Direct imports '+name);executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules));log=OUT/(tag+'-build.log');require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Warning-free '+tag);logs[str(log)]=sha(log)
    logs[str(OUT/'asm-build.log')]=sha(OUT/'asm-build.log')
    def execute(exe,args,label):
        p=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' exit='+str(p.returncode)+' inspect '+str(run/(label+'.log')));return p.stdout
    getters=getter_corpus();gq=run/'getter-query.bin';gq.write_bytes(b''.join(struct.pack('<6I',*q) for q in getters));go=run/'getters-native.bin';getterTrace=execute(OUT/'texture_input_reference_x86.exe',[old.PRIVATE,old.LIBS,mount,gq,go],'getters-native');raw=go.read_bytes();require(len(raw)==120+24*len(getters),'Getter trace extent');header=list(struct.unpack_from('<30I',raw));expectedHeader=[0xff44,0,0,0,0,0,0xff40,0,0x3f400000,0x3f400000,0x3f400000,0x3f400000,0xff40,0xfffffffe,0xc0000000,0xc0000000,0xc0000000,0xc0000000,0xff44,17,0x41880000,0x41880000,0x41880000,0x41880000,0xff40,0,0,0,0,0];require(header==expectedHeader,'Initial shared integer-zero dummy, persistent writes across names/materials')
    quiet=lambda x:x|0x400000 if (x&0x7fffffff)>0x7f800000 else x
    for i,q in enumerate(getters):
        expected=[quiet(q[1])]+[quiet(q[j+1]) if j<q[5] else 0xdeadbeef for j in range(4)]+[(q[0]>>4)&7];require(list(struct.unpack_from('<6I',raw,120+24*i))==expected,'Actual scalar/vector cached getter '+str(i))
    cases=corpus();query=run/'query.bin';query.write_bytes(b''.join(struct.pack('<8I',*c['words']) for c in cases));(run/'cases.json').write_text(json.dumps(cases,indent=2));outputs=[];traces=[]
    for flag in [0,1]:
        tail=['--sse2'] if flag else [];pair=[]
        for native in [True,False]:
            label=('native' if native else 'owned')+'-dispatch'+str(flag);dest=run/(label+'.bin');trace=execute(OUT/('texture_transform_reference_x86.exe' if native else 'texture_transform_owned_x64.exe'),[old.CLIENT if native else run/'absent.dll',query,dest,*tail],label);pair.append(dest);traces.append(trace)
        a,b=[p.read_bytes() for p in pair]
        if a!=b:
            diff=[i for i in range(len(cases)) if a[64*i:64*(i+1)]!=b[64*i:64*(i+1)]];(run/'difference.json').write_text(json.dumps(dict(dispatch=flag,differences=len(diff),first_case=cases[diff[0]],index=diff[0],native=list(struct.unpack_from('<16I',a,64*diff[0])),owned=list(struct.unpack_from('<16I',b,64*diff[0]))),indent=2));raise RuntimeError('TextureTransform math mismatch; inspect '+str(run/'difference.json'))
        require(len(a)==64*len(cases),'All16 output words each case');outputs.append(pair[0])
        for i,c in enumerate(cases):
            if c['expected'] is not None:require(list(struct.unpack_from('<16I',a,64*i))==c['expected'],'Analytical transform '+c['label'])
    require(outputs[0].read_bytes()==outputs[1].read_bytes(),'Both observed original math dispatches match for this corpus')
    regressions=[]
    for native in [True,False]:
        name='matrix_value_reference_x86.exe' if native else 'matrix_value_owned_x64.exe';exe=previous.OUT/name;dest=run/('prior-native.bin' if native else 'prior-owned.bin');execute(exe,[old.PRIVATE if native else run/'absent.dll',priorPath.parent/'query.bin',dest],dest.stem);require(sha(dest)==prior['output_sha256'],'Prior1929 full matrix transitions unchanged');regressions.append(dict(executable=name,cases=1929,sha256=sha(dest)))
    safety=execute(previous.OUT/'matrix_store_safety_x64.exe',[],'matrix-store-safety');require(safety==prior['safety'],'Prior37 matrix safety/17 failures unchanged')
    sine=json.loads(Path(prior['prior_receipt']).read_text())
    historyPath=Path(prior['prior_receipt'])
    while 'unlit-two-texture-parameters' not in str(historyPath.parent.parent):historyPath=Path(json.loads(historyPath.read_text())['prior_receipt'])
    pilot=historyPath.parent/'mount/materials/pilot-original.vmt';require(sha(pilot)==sine['pilot']['sha256'],'Untouched disposable pilot');refusal=execute(previous.OUT/'unlit_two_texture_parameters_owned_x64.exe',['--test-pilot-refusal',pilot],'pilot-refusal');require(refusal==prior['pilot_refusal'],'Full pilot remains refused')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());require(all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()) and sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor unchanged')
    for group in [sources,*protected]:require(all(sha(p)==h for p,h in group.items()),'Final protected inputs')
    require(all(sha(e['path'])==e['sha256'] for e in prior['executables']) and not (run/'absent.dll').exists(),'Earlier executable identities and owned absent-library independence')
    counts=re.search(r'native_allocations=(\d+) releases=(\d+)',getterTrace);require(counts and counts[1]==counts[2],'Actual lookup/getter material allocation balance');nativeCounts=[]
    for trace in [traces[0],traces[2]]:
        n=re.search(r'native_proxy_allocations=(\d+) releases=(\d+)',trace);require(n and n[1]==n[2]==str(len(cases)),'Original proxy allocation balance');nativeCounts.append(int(n[1]))
    capture=run/'complete-transform.bin.zlib';capture.write_bytes(zlib.compress(outputs[0].read_bytes(),9))
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',scope='Actual TextureTransform factory/Init/OnBind/Release numerical matrix output with resolved input-service spies; original cached getters and shared missing-variable behavior measured separately',cases=len(cases),exact_bytes=outputs[0].stat().st_size,analytical_checks=sum(c['expected'] is not None for c in cases)*2,getter_cases=len(getters),getter_exact_bytes=len(raw),missing_variable_scenarios=4,missing_variable_header=header,original_dispatch_states=[0,1],query_sha256=sha(query),output_sha256=sha(outputs[0]),getter_query_sha256=sha(gq),getter_output_sha256=sha(go),native_proxy_allocations_each_dispatch=nativeCounts,native_cpu_allocations=int(counts[1]),native_cpu_releases=int(counts[2]),safety=safety,regressions=regressions,pilot_refusal=refusal,sources=sources,new_sources=NEW,prior_sources_unchanged=len(prior['sources']),original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],benchmark_material_hashes=prior['benchmark_material_hashes'],fixture_hashes=prior['fixture_hashes'],executables=executables,reused_executables=prior['executables'],build_logs=logs,static_ranges=ranges,getter_static_ranges=getterRanges,prior_receipt=str(priorPath),prior_receipt_sha256=sha(priorPath),editor_dependencies_unchanged=len(build['editor_source_dependencies']),editor_sha256=build['executable_sha256'],limits=['Owned numerical input record is explicit cached values/absence; live TextureTransform name lookup/registry/store binding is not implemented','Original Init uses resolved named fixture inputs; numeric/wide/pointer KV conversion and optional missing-variable integration are not claimed','Actual shared dummy is material-system-wide and mutable; no per-variable zero substitute is implemented. Missing-cache snapshot/ownership interface must be resolved before wiring the proxy','Getters execute original ownerless/no-queue raw-cache methods, including NaN quieting; full resource/owner/queue ABI is not adopted','Checked round-nearest/nonflushing SSE and near53-bit x87 mode on this host; FP exception flags/other modes/other CPU architectures unproved','Native result SetMatrix is a capture service; actual setter was separately verified by prior1929-case prerequisite. No combined original engine/GPU or renderer proof','TextureScroll, weapon_invis, whole pilot, editor integration and whole SFM goal remain unfinished; no original writes/live SFM control'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');frozen=OUT/('replay-'+run.name.removeprefix('verification-'));frozen.mkdir();included=[Path(p) for p in sources]+[Path(e['path']) for e in executables]+[Path(p) for p in logs]+[receipt,query,gq,go,capture,OUT/'preflight.json']+list(run.glob('*.json'))+list(run.glob('*.log'))
    for p in included:
        target=frozen/p.relative_to(ROOT);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target)
    manifest={p.relative_to(frozen).as_posix():sha(p) for p in frozen.rglob('*') if p.is_file()};(frozen/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');pointer=dict(receipt=str(receipt),receipt_sha256=sha(receipt),replay=str(frozen),replay_manifest_sha256=sha(frozen/'manifest.json'));(OUT/'latest-verification.json').write_text(json.dumps(pointer,indent=2)+'\n');print(json.dumps(dict(status='PASS',receipt=str(receipt),cases=len(cases),getter_cases=len(getters),exact_bytes=result['exact_bytes'],scope=result['scope'])),flush=True)

if __name__=='__main__':main()
