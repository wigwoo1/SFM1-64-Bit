"""Unchanged original TextureScroll numerical path versus independent x64.
No live binding, full material, GPU, editor or live-SFM fidelity claim.
"""
import hashlib, itertools, json, math, random, re, shutil, struct, subprocess, tempfile, time, zlib
from pathlib import Path
from datetime import datetime, timezone
import verify_material_texture_transform_evaluation as previous
from generate_material_sine_constants import constants, render
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges

ROOT=previous.ROOT; OUT=ROOT/'outputs/material-texture-scroll-math-20261027'
old=previous.old; sha=old.sha; require=old.require
NEW=['material_cosine_double_candidate.h','material_cosine_double_x87_candidate.inc','material_texture_scroll_value_candidate.h','probe_material_cosine_double.cpp','probe_material_texture_scroll_math.cpp','build_material_texture_scroll_math.cmd','verify_material_texture_scroll_math.py']

def corpus():
    rng=random.Random(2026102712)
    d=lambda x:struct.unpack('<Q',struct.pack('<d',x))[0]
    f=old.fbits
    cosine=[0,0x8000000000000000,1,0x8000000000000001,0x0010000000000000,0x7fefffffffffffff,0xffefffffffffffff,0x7ff0000000000000,0xfff0000000000000,0x7ff8000000000001,0x7ff0000000000001,0xfff8123456789abc,0xfff0123456789abc]
    for edge in [0x3030000000000000,0x40f5000000000000,0x40f6000000000000]:
        for delta in [-2,-1,0,1,2]:
            for sign in [0,1<<63]:cosine.append((edge+delta)|sign)
    for i in range(-4096,4097):
        x=i*math.pi/64
        for y in [x,math.nextafter(x,math.inf),math.nextafter(x,-math.inf)]:cosine.append(d(y))
    cosine += [rng.getrandbits(64) for _ in range(8192)]
    cosine += [d(rng.uniform(-1e6,1e6)) for _ in range(4096)]
    scroll=[]
    for typ in [0,1,2,4,6,7,0xff47]:
        for t in [0.,-0.,.125,-.125,1.,-1.,2.,-2.,.99999994,16777216.,-16777216.,2147483648.,-2147483648.]:
            for rate in [-1.,0.,.25,1.,2.]:
                for angle in [0.,90.,180.,270.,360.,-90.]:scroll.append([f(t),f(rate),f(angle),f(1.5),typ])
    specials=[0,0x80000000,1,0x80000001,0x00800000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc12345,0x7fa12345,0xffc12345,0xffa12345,0x7f7fffff,0xff7fffff]
    for j in range(4):
        for b in specials:
            for typ in [0,7]:
                q=[f(.375),f(1.25),f(30),f(.5),typ];q[j]=b;scroll.append(q)
    for _ in range(8192):scroll.append([rng.getrandbits(32) for _ in range(4)]+[rng.choice([0,2,7,0xff47])])
    # Preserve the first11042 exploration inputs exactly, including the retained
    # dual-NaN mismatch; expand operand provenance rather than excluding it.
    nan_operands=[0x7fc12345,0xffc54321,0x7fa23456,0xffa65432,0x7f800000,0xff800000,0,0x80000000,0x3f800000,0xbf800000]
    for t,rate,angle in itertools.product(nan_operands,repeat=3):
        for typ in [0,7]:scroll.append([t,rate,angle,f(.75),typ])
    analytical=[]
    for typ in [0,7]:
        for t,w in [(.125,.125),(-.125,.875),(1.,0.),(-1.,0.),(2147483648.,4294967296.),(-2147483648.,0.)]:
            q=[f(t),f(1),f(0),f(1.5),typ];answer=[9 if typ==7 else 3]+[0]*16
            answer[4 if typ==7 else 1]=f(w)
            if typ==7:answer[1]=answer[6]=f(1.5);answer[11]=answer[16]=f(1)
            # sin(0)*negative time retains a negative-zero Y before wrapping.
            answer[8 if typ==7 else 2]=0x80000000 if t<0 else 0
            analytical.append((len(scroll),answer));scroll.append(q)
    return cosine,scroll,analytical

def main():
    pre=json.loads((OUT/'preflight.json').read_text());ptr=pre['prior'];parent=Path(ptr['receipt']);require(sha(parent)==ptr['receipt_sha256'],'Pinned prior receipt');prior=json.loads(parent.read_text());replay=Path(ptr['replay']);require(sha(replay/'manifest.json')==ptr['replay_manifest_sha256'],'Prior frozen manifest');require(all(sha(replay/n)==h for n,h in json.loads((replay/'manifest.json').read_text()).items()),'All prior frozen files')
    sources=source_closure([Path(p) for p in prior['sources']]+[ROOT/'work'/n for n in NEW]);protected=[prior[k] for k in ['sources','original_hashes','private_library_hashes','benchmark_material_hashes','fixture_hashes']]
    for group in protected:require(all(sha(n)==h for n,h in group.items()),'All protected inputs before')
    require(all(sha(e['path'])==e['sha256'] for e in prior['executables']),'All prior29 executable identities')
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
    image,ranges=check_ranges(old.CLIENT,'../outputs/material-proxy-evaluation-20261027/client-disassembly-private.txt',[
        ('scroll_Init',0x10198ab0,0x10198b58),('scroll_factory',0x10198b60,0x10198b7f),('scroll_OnBind',0x10198b80,0x10198d40),('scroll_destructor',0x10227110,0x10227132),('SSE_cosine',0x1045cc7b,0x1045ce26),('x87_cosine_fallback',0x10466d50,0x10466e79)])
    require([image.string(a) for a in [0x104d574c,0x1050d43c,0x1050d428,0x1050d418]]==['textureScrollVar','textureScrollRate','textureScrollAngle','textureScale'],'Actual fields')
    require([image.word(0x1050d454+4*i) for i in range(5)]==[0x10198ab0,0x10198b80,0x10175ed0,0x100e5f90,0x10227110],'Actual vtable')
    table,split=constants();require((ROOT/'work/material_sine_constants_candidate.h').read_text()==render(),'Independent generated table unchanged')
    def original(a,n):return image.b[image.offset(a):image.offset(a)+n]
    require(all(struct.pack('<d',v)==original(0x105ee640+32*i+8*j,8) for i,row in enumerate(table) for j,v in enumerate(row)),'All256 independent cosine table words match')
    require(all(struct.pack('<d',v)==original(a,8) for a,v in zip([0x105eee90,0x105eee80,0x105eee98,0x105eeea0],split)),'Independent pi reduction data match')
    require(original(0x104d2200,8)==struct.pack('<d',math.pi/180) and original(0x105eeeb0,8)==struct.pack('<d',1.) and original(0x105eeeb8,8)==struct.pack('<Q',1<<63),'Degrees/tiny/nonfinite constants')
    start=time.time();b=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_texture_scroll_math.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=240);(run/'build-driver.log').write_text(b.stdout+b.stderr);require(b.returncode==0,'Fresh4 builds')
    executables=[];logs={}
    for stem,tag in [('cosine_double','cosine'),('texture_scroll','scroll')]:
        for native in [True,False]:
            exe=OUT/(stem+('_reference_x86.exe' if native else '_owned_x64.exe'));raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];machine=0x14c if native else 0x8664
            require(exe.stat().st_mtime>=start-2 and raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if native else 0x20b) and b'asInvoker' in raw,'Fresh architecture '+exe.name)
            imp=subprocess.run([str(old.DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imp,re.I|re.M);require({s.lower() for s in modules}=={'kernel32.dll'},'Direct imports '+exe.name);executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules));log=OUT/(tag+('-native' if native else '-owned')+'-build.log');require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Warning-free '+log.name);logs[str(log)]=sha(log)
    logs[str(OUT/'asm-build.log')]=sha(OUT/'asm-build.log')
    def execute(exe,args,label):
        r=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(r.stdout+r.stderr);require(r.returncode==0,label+' failed; inspect '+str(run/(label+'.log')));return r.stdout
    cosine,scroll,analytical=corpus();cq=run/'cosine-query.bin';sq=run/'scroll-query.bin';cq.write_bytes(b''.join(struct.pack('<Q',q) for q in cosine));sq.write_bytes(b''.join(struct.pack('<5I',*q) for q in scroll));(run/'analytical.json').write_text(json.dumps(analytical,indent=2));traces={};outputs={}
    for stem,query,stride,count in [('cosine_double',cq,8,len(cosine)),('texture_scroll',sq,68,len(scroll))]:
        paths=[]
        for native,tail,label in [(True,[],'native'),(True,['--sse2'],'native-sse2'),(False,[],'owned')]:
            exe=OUT/(stem+('_reference_x86.exe' if native else '_owned_x64.exe'));dest=run/(stem+'-'+label+'.bin');traces[stem+'-'+label]=execute(exe,[old.CLIENT if native else run/'absent.dll',query,dest,*tail],dest.stem);paths.append(dest)
        a=paths[0].read_bytes();require(len(a)==stride*count,'Complete numerical trace')
        for dest in paths[1:]:
            b=dest.read_bytes()
            if a!=b:
                diff=[i for i in range(count) if a[i*stride:(i+1)*stride]!=b[i*stride:(i+1)*stride]];(run/(stem+'-difference.json')).write_text(json.dumps(dict(compared=str(dest),differences=len(diff),first_indices=diff[:100],first_query=(cosine if stem=='cosine_double' else scroll)[diff[0]],native=a[diff[0]*stride:(diff[0]+1)*stride].hex(),other=b[diff[0]*stride:(diff[0]+1)*stride].hex()),indent=2));raise RuntimeError('Numerical mismatch: '+stem+'; evidence in '+str(run))
        outputs[stem]=paths[0]
    raw=outputs['texture_scroll'].read_bytes()
    for i,answer in analytical:require(list(struct.unpack_from('<17I',raw,i*68))==answer,'Analytical wrap and matrix/vector branch '+str(i))
    require(outputs['cosine_double'].read_bytes()[:16]==struct.pack('<2d',1.,1.),'Cosine of both signed zeros')
    counts=[]
    for label in ['native','native-sse2']:
        m=re.search(r'native_proxy_allocations=(\d+) releases=(\d+)',traces['texture_scroll-'+label]);require(m and m[1]==m[2]==str(len(scroll)),'Actual proxy allocation balance');counts.append(int(m[1]))
    # Existing source is unmodified; retain old build identities and replay its
    # exact accepted full live input/store contract with both original flags.
    prior_owned=next(Path(e['path']) for e in prior['executables'] if Path(e['path']).name=='texture_evaluation_owned_x64.exe');prior_native=next(Path(e['path']) for e in prior['executables'] if Path(e['path']).name=='texture_evaluation_reference_x86.exe')
    for native,tail,label in [(True,[],'native'),(True,['--sse2'],'native-sse2'),(False,[],'owned')]:
        dest=run/('prior-live-'+label+'.bin');execute(prior_native if native else prior_owned,[old.CLIENT if native else run/'absent.dll',parent.parent/'query.bin',parent.parent/'prepared-original.bin',dest,*tail],dest.stem);require(sha(dest)==prior['output_sha256'],'All483 prior live cases unchanged')
    safety=execute(prior_owned,['--test-texture'],'prior-texture-safety');require(safety==prior['safety'],'Prior36 safety/18 actual allocation failures unchanged')
    pilot_exe=next(Path(e['path']) for e in prior['executables'] if Path(e['path']).name=='unlit_two_texture_parameters_owned_x64.exe');two_path=parent
    while 'unlit-two-texture-parameters' not in str(two_path.parent.parent):two_path=Path(json.loads(two_path.read_text())['prior_receipt'])
    pilot=two_path.parent/'mount/materials/pilot-original.vmt';pilot_refusal=execute(pilot_exe,['--test-pilot-refusal',pilot],'pilot-refusal');require(pilot_refusal==prior['pilot_refusal'],'Whole pilot still refused')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());require(all(sha(ROOT/n)==h for n,h in build['editor_source_dependencies'].items()) and sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor graph/executable unchanged')
    for group in [sources,*protected]:require(all(sha(n)==h for n,h in group.items()),'Protected final identities')
    require(all(sha(e['path'])==e['sha256'] for e in prior['executables']) and not (run/'absent.dll').exists(),'Earlier executable identities / owned absent-library independence')
    captures=[]
    for stem,dest in outputs.items():
        capture=run/(stem+'-complete.bin.zlib');capture.write_bytes(zlib.compress(dest.read_bytes(),9));captures.append(capture)
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',scope='Direct original double cosine and actual TextureScroll numeric Init/OnBind/Release with typed constant input services, explicit clock and full vector/matrix output capture',cosine_cases=len(cosine),cosine_exact_bytes=8*len(cosine),scroll_cases=len(scroll),scroll_exact_bytes=68*len(scroll),analytical_checks=len(analytical)+2,original_dispatch_states=[0,1],native_proxy_allocations_each=counts,cosine_query_sha256=sha(cq),cosine_output_sha256=sha(outputs['cosine_double']),scroll_query_sha256=sha(sq),scroll_output_sha256=sha(outputs['texture_scroll']),prior_safety=safety,pilot_refusal=pilot_refusal,prior_live_cases=483,sources=sources,new_sources=NEW,prior_sources_unchanged=len(prior['sources']),original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],benchmark_material_hashes=prior['benchmark_material_hashes'],fixture_hashes=prior['fixture_hashes'],executables=executables,reused_executables=prior['executables'],build_logs=logs,static_ranges=ranges,prior_receipt=str(parent),prior_receipt_sha256=sha(parent),editor_dependencies_unchanged=len(build['editor_source_dependencies']),editor_sha256=build['executable_sha256'],limits=['Typed numeric constant inputs only; missing/default/live FloatInput initialization and return handling not yet proved for TextureScroll','Original methods unchanged RX with fixture heap/KV/variable/clock services, output setter capture; previous independent setters are not a combined material-engine proof','Round-nearest nonflushing SSE and near53-bit x87 on this host; exception flags, other modes/architectures and resource/owner side effects are not covered','Owned numerical functions only; no TextureScroll proxy registration, live transactions, weapon_invis, whole pilot, GPU/editor integration or visual fidelity claim','Originals read-only; no live SFM control'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');frozen=OUT/('replay-'+run.name.removeprefix('verification-'));frozen.mkdir();included=[Path(n) for n in sources]+[Path(e['path']) for e in executables]+[Path(n) for n in logs]+[receipt,cq,sq,*captures,OUT/'preflight.json']+list(run.glob('*.log'))+list(run.glob('*.json'))
    for p in included:
        dest=frozen/p.relative_to(ROOT);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,dest)
    manifest={p.relative_to(frozen).as_posix():sha(p) for p in frozen.rglob('*') if p.is_file()};(frozen/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt),replay=str(frozen),replay_manifest_sha256=sha(frozen/'manifest.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cosine_cases','scroll_cases','analytical_checks','scope']}),flush=True)

if __name__=='__main__':main()
