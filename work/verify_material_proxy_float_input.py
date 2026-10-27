"""Actual shared Sine/function FloatInput Init/getter versus owned x64 input.
No time waveform, Sine math, renderer, live SFM or original writes are claimed.
"""
import hashlib,json,random,re,shutil,struct,subprocess,tempfile,time,zlib
from pathlib import Path
from datetime import datetime,timezone
import verify_material_equals_evaluation as old
from verify_material_object_parameters import decode
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
ROOT=old.ROOT;OUT=ROOT/'outputs/material-proxy-float-input-20261027';require=old.require;sha=old.sha
NEW=['material_proxy_float_input_candidate.h','material_float_input_client_native_fixture.h','probe_material_proxy_float_input.cpp','build_material_proxy_float_input.cmd','verify_material_proxy_float_input.py']

def corpus():
    cases=[]
    def add(label,value=None,kind=1,bits=0,drive='[.25 -.5 .75 1]',default=0x3f000000,extra=None,expected=None):
        declaration=old.node('FloatInputProbe',[] if value is None and kind==1 else [old.string('input',value) if kind==1 else old.node('input',kind=kind,bits=bits)])
        root=old.node('UnlitTwoTexture',[old.literal('$out','float'),old.string('$drive',drive),*(extra or []),old.node('Proxies',[declaration])]);cases.append(dict(label=label,root=root,default=default,expected=expected))
    for bits in [0,0x80000000,0x3e800000,0xbf400000,0x7f800000,0xff800000,0x7fc12345,0x7fa12345]:
        expected=bits|0x400000 if (bits&0x7f800000)==0x7f800000 and bits&0x7fffff else bits;add('missing-default',default=bits,expected=[expected]*3)
    simple={'0':0,'-0':0x80000000,'1':0x3f800000,'.5':0x3f000000,'-.75':0xbf400000,'1e':0x3f800000,'1e+':0x3f800000,'1e-':0x3f800000,'1d3':0x3f800000,'1D-3':0x3f800000,'0x1p2':0,'1.#INF':0x3f800000,'2$drive':0x40000000,'  +.5 tail':0x3f000000}
    for s,bits in simple.items():add('numeric-prefix',s,expected=[bits]*3)
    for s in ['', ' ', '+','-','.', 'nan','NaN','inf','-inf','Infinity','missing','$missing',' $drive','$drive ','[$drive]']:
        add('lookup-refused',s,expected=False)
    for name in ['nan','inf','Infinity','NAME']:
        add('nonnumeric-can-bind',name,extra=[old.string(name,'.625')],expected=[0x3f200000]*3)
    for drive in ['.375','-3','[.25]','[.25 -.5]','[.25 -.5 .75]','[.25 -.5 .75 1]']:
        for suffix in ['', '[-1]','[-999999999999]','[0]','[1]','[2]','[3]','[4]','[999999999999]','[foo]','[ 1junk','[+01]','[','[2]tail']:
            add('binding-types-components','$drive'+suffix,drive=drive)
    for suffix,expected in [('',[0x3e800000,0x3fc00000,0xbf400000]),('[2]',[0x3f400000,0,0]),('[3]',[0x3f800000,0,0]),('[1]',[0xbf000000,0xc0000000,0xbf400000]),('[foo]',[0x3e800000,0x3fc00000,0xbf400000])]:add('analytical-dynamic','$DRIVE'+suffix,expected=expected)
    for bits in [0,1,0xffffffff,0x7fffffff,0x80000000,0x80000001,16777217,16777219]:
        signed=bits if bits<0x80000000 else bits-0x100000000;expected=struct.unpack('<I',struct.pack('<f',float(signed)))[0];add('typed-integer',kind=2,bits=bits,expected=[expected]*3)
    for bits in [0,0x80000000,0x3dcccccd,0xbf400000,1,0x7f7fffff]:add('typed-float',kind=3,bits=bits,expected=[bits]*3)
    add('typed-block',kind=0,expected=[0]*3)
    # Numeric scanner scratch-growth boundaries; do not cross the native
    # component-name copy's unsafe bracket-outside-first255 boundary.
    for n in [23,24,25,26,349,350,351,698,699,700,701,1399,1400,3000]:
        for sign in ['', '-', '+']:add('scanner-growth',sign+'1'*(n-len(sign)))
    longName='$'+'x'*249
    add('long-name-index-truncation',longName+'[0001]',extra=[old.string(longName,'[.25 .5 .75 1]')],expected=[0x3f000000]*3)
    add('long-name-truncates-index-digit',longName+'[00001]',extra=[old.string(longName,'[.25 .5 .75 1]')],expected=[0x3e800000]*3)
    lastBracketName='$'+'y'*253
    add('last-safe-bracket-defaults-index-zero',lastBracketName+'[1]',extra=[old.string(lastBracketName,'[.25 .5 .75 1]')],expected=[0x3e800000]*3)
    rng=random.Random(2026102703)
    for i in range(512):
        mantissa=str(rng.randrange(-10**18,10**18));number=mantissa+rng.choice(['e','E','d','D'])+str(rng.randrange(-500,501));add('random-literal',number)
    for i in range(256):
        drive='['+' '.join(str(rng.uniform(-20,20)) for _ in range(i%4+1))+']';add('random-binding','$drive['+str(rng.randrange(-2,7))+']',drive=drive)
    return cases

def decode_output(raw):
    at=0
    def u():
        nonlocal at
        require(at+4<=len(raw),'FloatInput word extent');v=struct.unpack_from('<I',raw,at)[0];at+=4;return v
    def chars(n):
        nonlocal at
        require(at+n<=len(raw),'FloatInput text extent');v=raw[at:at+n].decode('ascii');at+=n;return v
    def text():return chars(u())
    result=[]
    for _ in range(u()):
        c=dict(index=u(),initialized=bool(u()),frames=[])
        if c['initialized']:
            for _ in range(3):
                f=dict(writes=[[u() for _ in range(7)] for _ in range(u())],shader=text(),values=[])
                for _ in range(u()):
                    w=[u() for _ in range(28)];f['values'].append(dict(words=w,text=chars(w[10]),name=chars(w[11])))
                c['frames'].append(f)
        result.append(c)
    require(at==len(raw),'Complete FloatInput output');return result

def main():
    pre=json.loads((OUT/'preflight.json').read_text());pointer=pre['prior'];priorPath=Path(pointer['receipt']);require(sha(priorPath)==pointer['receipt_sha256'],'Prior receipt');prior=json.loads(priorPath.read_text());priorReplay=Path(pointer['replay']);require(sha(priorReplay/'manifest.json')==pointer['replay_manifest_sha256'],'Prior frozen manifest');priorManifest=json.loads((priorReplay/'manifest.json').read_text());require(all(sha(priorReplay/p)==h for p,h in priorManifest.items()),'Prior frozen files')
    for group in [prior['sources'],prior['original_hashes'],prior['private_library_hashes'],prior['benchmark_material_hashes']]:require(all(sha(Path(p))==h for p,h in group.items()),'Protected inputs before')
    sources=source_closure([Path(p) for p in prior['sources']]+[ROOT/'work'/p for p in NEW]);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));mount=run/'mount';mount.mkdir()
    _,ranges=check_ranges(old.CLIENT,'../outputs/material-proxy-evaluation-20261027/client-disassembly-private.txt',[('FloatInput_Init',0x100e60d0,0x100e6212),('FloatInput_Get',0x100e5d20,0x100e5da7),('original_CRT_numeric_table_initializer',0x1045b659,0x1045b6b9),('sscanf_entry',0x1045dc6f,0x1045dc91),('Sine_Init_static_only',0x10136440,0x101364e4),('Sine_OnBind_static_only',0x101364f0,0x101365a7)])
    start=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_proxy_float_input.cmd')],capture_output=True,text=True,cwd=ROOT,timeout=180);(run/'build-driver.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Fresh FloatInput build')
    executables=[];logs={}
    for native in [True,False]:
        exe=OUT/('float_input_reference_x86.exe' if native else 'float_input_owned_x64.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];machine=0x14c if native else 0x8664;require(exe.stat().st_mtime>=start-2 and struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==(0x10b if native else 0x20b) and b'asInvoker' in b,'Fresh architecture');imports=subprocess.run([str(old.DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require({x.lower() for x in modules}=={'kernel32.dll'},'Direct imports');executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules));log=OUT/('native-build.log' if native else 'owned-build.log');require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Warning-free build');logs[str(log)]=sha(log)
    logs[str(OUT/'asm-build.log')]=sha(OUT/'asm-build.log')
    def execute(exe,args,label):
        p=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' exit='+str(p.returncode)+'; inspect '+str(run/(label+'.log')));return p.stdout
    cases=corpus();query=run/'query.bin';query.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<I',c['default'])+old.packet(c['root']) for c in cases));(run/'cases.json').write_text(json.dumps(cases,indent=2))
    cpuQuery=run/'cpu-query.bin';cpuQuery.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<4I',0,0,0,0)+struct.pack('<12I',0,0,0,0,0,0,0,0,1,0,92,0)+old.text('equals/case')+old.packet(c['root'])+struct.pack('<6I',0,0,0,0,0,0) for c in cases));cpuOut=run/'cpu-original.bin';cpuExe=old.OUT/'unlit_two_texture_parameters_reference_x86.exe';cpuTrace=execute(cpuExe,[old.PRIVATE,old.LIBS,mount,cpuQuery,cpuOut],'cpu-original');n,records=decode(cpuOut.read_bytes());initial=[r for r in records if r['stage']==1];require(n==len(cases)==len(initial) and all(r['result'] for r in initial),'Original CPU initial states');prepared=run/'prepared-original.bin';prepared.write_bytes(struct.pack('<I',n)+b''.join(old.text(s['shader'])+struct.pack('<I',len(s['values']))+b''.join(struct.pack('<28I',*v['words'])+v['text'].encode()+v['name'].encode() for v in s['values']) for s in initial))
    a=run/'native.bin';b=run/'owned.bin';nativeTrace=execute(OUT/'float_input_reference_x86.exe',[old.CLIENT,query,prepared,a],'native');execute(OUT/'float_input_owned_x64.exe',[run/'absent.dll',query,prepared,b],'owned')
    native=decode_output(a.read_bytes());owned=decode_output(b.read_bytes())
    if a.read_bytes()!=b.read_bytes():
        diffs=[i for i,(x,y) in enumerate(zip(native,owned)) if x!=y];(run/'difference.json').write_text(json.dumps(dict(different_cases=diffs,first_case=cases[diffs[0]],native=native[diffs[0]],owned=owned[diffs[0]]),indent=2));raise RuntimeError('FloatInput mismatch; inspect '+str(run/'difference.json'))
    checks=0;frames=0;refused=0
    for c,state in zip(cases,native):
        if c['expected'] is False:require(not state['initialized'],'Expected missing binding refusal');checks+=1
        else:require(state['initialized'],'Expected input initialization');checks+=1
        if not state['initialized']:refused+=1;continue
        for i,f in enumerate(state['frames']):
            frames+=1;values={v['name'].lower():v for v in f['values']};result=values['$out']['words'][3];require(f['writes'][-1][1]==1 and f['writes'][-1][3]==result,'Actual input result reaches output variable');checks+=1
            if c['expected'] is not None:require(result==c['expected'][i],'Analytical input result '+c['label']);checks+=1
            if i==1:require(f['writes'][0][1:3]==[5,2] and values['$drive']['words'][3:7]==[0x3fc00000,0xc0000000,0,0],'Controlled per-frame vector input');checks+=1
            if i==2:require(f['writes'][0][1]==1 and values['$drive']['words'][3:7]==[0xbf400000]*4,'Controlled per-frame scalar input retains measured count bits');checks+=1
    safety=execute(OUT/'float_input_owned_x64.exe',['--test'],'safety');require('PASS float_input_safety_checks=' in safety,'Owned FloatInput safety')
    regressions=[];priorRun=priorPath.parent;reusedNames=['unlit_two_texture_parameters_reference_x86.exe','equals_reference_x86.exe','equals_owned_x64.exe','component_reference_x86.exe','component_owned_x64.exe'];reused=[e for e in prior['executables'] if Path(e['path']).name in reusedNames];require(len(reused)==5 and all(sha(e['path'])==e['sha256'] for e in reused),'Unchanged reused oracle/probe identities')
    for nativeMode in [True,False]:
        exe=old.OUT/('equals_reference_x86.exe' if nativeMode else 'equals_owned_x64.exe');dest=run/('prior-equals-native.bin' if nativeMode else 'prior-equals-owned.bin');execute(exe,[old.CLIENT if nativeMode else run/'absent.dll',priorRun/'query.bin',priorRun/'prepared-original.bin',dest],dest.stem);require(sha(dest)==prior['output_sha256'],'All682 prior Equals cases unchanged');regressions.append(dict(name=exe.name,cases=682,sha256=sha(dest)))
        exe=old.OUT/('component_reference_x86.exe' if nativeMode else 'component_owned_x64.exe');dest=run/('prior-component-native.bin' if nativeMode else 'prior-component-owned.bin');execute(exe,[old.PRIVATE if nativeMode else run/'absent.dll',priorRun/'component-query.bin',dest],dest.stem);require(sha(dest)==prior['component_output_sha256'],'All2408 prior component cases unchanged');regressions.append(dict(name=exe.name,cases=2408,sha256=sha(dest)))
    priorSafety=execute(old.OUT/'equals_owned_x64.exe',['--test-equals'],'prior-equals-safety');require(priorSafety==prior['safety']['equals'],'Prior Equals safety unchanged')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());require(all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()) and sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor unchanged')
    for group in [sources,prior['original_hashes'],prior['private_library_hashes'],prior['benchmark_material_hashes']]:require(all(sha(Path(p))==h for p,h in group.items()),'Protected final identities')
    require(not (run/'absent.dll').exists(),'Owned absent-original argument');counts=re.search(r'native_scratch_allocations=(\d+) releases=(\d+)',nativeTrace);cpuCounts=re.search(r'native_allocations=(\d+) releases=(\d+)',cpuTrace);require(counts and counts[1]==counts[2] and cpuCounts and cpuCounts[1]==cpuCounts[2],'Actual scratch/CPU allocation balance');capture=run/'complete-input.bin.zlib';capture.write_bytes(zlib.compress(a.read_bytes(),9))
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',scope='Actual shared function/Sine FloatInput Init and live cached-value getter; Sine waveform not yet implemented',cases=len(cases),frames=frames,refused=refused,exact_bytes=a.stat().st_size,independent_checks=checks,query_sha256=sha(query),output_sha256=sha(a),cpu_query_sha256=sha(cpuQuery),cpu_output_sha256=sha(cpuOut),prepared_original_sha256=sha(prepared),native_scratch_allocations=int(counts[1]),native_scratch_releases=int(counts[2]),native_cpu_allocations=int(cpuCounts[1]),native_cpu_releases=int(cpuCounts[2]),safety=safety,prior_equals_safety=priorSafety,regressions=regressions,sources=sources,new_sources=NEW,prior_sources_unchanged=len(prior['sources']),original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],benchmark_material_hashes=prior['benchmark_material_hashes'],executables=executables,reused_executables=reused,build_logs=logs,static_ranges=ranges,prior_receipt=str(priorPath),prior_receipt_sha256=sha(priorPath),editor_dependencies_unchanged=len(build['editor_source_dependencies']),editor_sha256=build['executable_sha256'],limits=['Input KV kinds0/1/2/3 and cached variable values only; pointer/wide/color/uint64 KV getters unproved and refused','Original Init/getter/scanf execute unchanged RX with explicit service spies and separately original-seeded CMaterial CPU state; not a combined engine process','Names with bracket outside copied first255 characters refuse rather than reproduce unsafe original access','Default clock is not invented; Sine waveform, CRT sine fidelity, renderer integration and full pilot remain incomplete','No live SFM/Blender, original writes, editor build/run or full-goal completion claim'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');replay=OUT/('replay-'+run.name.removeprefix('verification-'));replay.mkdir();included=[Path(p) for p in sources]+[Path(e['path']) for e in executables]+[Path(p) for p in logs]+[receipt,query,cpuQuery,prepared,capture,OUT/'preflight.json']+list(run.glob('*.json'))+list(run.glob('*.log'))
    for p in included:
        target=replay/p.relative_to(ROOT);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target)
    manifest={p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()};(replay/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');pointer=dict(receipt=str(receipt),receipt_sha256=sha(receipt),replay=str(replay),replay_manifest_sha256=sha(replay/'manifest.json'));(OUT/'latest-verification.json').write_text(json.dumps(pointer,indent=2)+'\n');print(json.dumps(dict(status='PASS',receipt=str(receipt),cases=len(cases),frames=frames,refused=refused,exact_bytes=a.stat().st_size,safety=safety.strip())))

if __name__=='__main__':main()
