"""Original weapon_invis dependency chain and independent CPU value proof.
No live scene/entity adapter, proxy registration or full-pilot claim.
"""
import hashlib,json,random,re,shutil,struct,subprocess,tempfile,time,zlib
from pathlib import Path
from datetime import datetime,timezone
import verify_material_texture_scroll_evaluation as previous
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
ROOT=previous.ROOT;OUT=ROOT/'outputs/material-weapon-invis-contract-20261027';old=previous.old;sha=old.sha;require=old.require
NEW=['material_weapon_invis_value_candidate.h','material_weapon_invis_client_native_fixture.h','probe_material_weapon_invis_contract.cpp','build_material_weapon_invis_contract.cmd','verify_material_weapon_invis_contract.py']

def corpus():
    rows=[];meta=[];f=old.fbits
    def add(label,flags=255,kind=0xff40,handle=0x12340001,serial=0x1234,owner=3,viewer=2,mode=0,value=None,limit=None,slot=0,cache=None,expected='unchecked'):
        rows.append([flags,kind,handle,serial,owner&0xffffffff,viewer&0xffffffff,mode&0xffffffff,f(.75) if value is None else value,f(.5) if limit is None else limit,slot,f(.125) if cache is None else cache]);meta.append(dict(label=label,expected=expected))
    # Retain all5120 initial original observations exactly.
    for flags in range(256):
        for owner,viewer in [(2,2),(3,2),(2,3),(3,3),(0,2)]:
            for mode in [0,1,2,3]:add('all-flags-and-selector-paths',flags=flags,owner=owner,viewer=viewer,mode=mode)
    for kind in [0,1,2,4,6,7]:
        for metadata in [0,0x40,0xff40,0x12340040]:
            for flags in [0,1,3,7,15,31,63,127,255]:add('output-types-cache-preservation',flags=flags,kind=metadata|kind)
    for handle in [0xffffffff,0,0x00010000,0xffff0000,0xfffffffe,0x1234ffff]:
        for serial in [handle>>16,(handle>>16)^1,0x10000,0xffffffff]:
            for flags in [255,255&~8,255&~16]:add('handle-index-serial-presence',handle=handle,serial=serial,flags=flags)
    special=[0,0x80000000,1,0x80000001,0x00800000,0x3f000000,0x3f800000,0xbf800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7fa12345,0xffc54321,0xffa54321]
    for value in special:
        for limit in special:
            for flags,owner,viewer,mode in [(255,2,2,0),(255,3,2,0),(255,3,2,1),(255,3,2,2),(255&~128,3,2,1),(255&~32,3,2,0),(255&~64,3,2,0),(255,3,1,0)]:add('nonfinite-ordered-clamp',flags=flags,owner=owner,viewer=viewer,mode=mode,value=value,limit=limit)
    rng=random.Random(2026102714)
    for _ in range(8192):
        handle=rng.getrandbits(32);add('seeded-service-snapshots',flags=rng.randrange(256),kind=rng.choice([0,1,2,4,6,7])|rng.choice([0,0xff40,0x12340040]),handle=handle,serial=rng.choice([handle>>16,rng.getrandbits(32)]),owner=rng.choice([0,1,2,3,-1,-2147483648]),viewer=rng.choice([0,1,2,3,-1,-2147483648]),mode=rng.choice([0,1,2,3,4,0xffffffff]),value=rng.getrandbits(32),limit=rng.getrandbits(32),slot=rng.randrange(2),cache=rng.getrandbits(32))
    add('analytical-enemy-unobserved',expected=f(.75));add('analytical-same-selector-clamp',owner=2,expected=f(.5));add('analytical-observer-mode1',mode=1,expected=f(.5));add('analytical-observer-mode2',mode=2,expected=f(.5));add('analytical-observer-other-target',flags=255&~128,mode=1,expected=f(.75))
    for flag in [1,2,4]:add('analytical-no-write',flags=255&~flag,expected=None)
    for args in [dict(handle=0xffffffff),dict(serial=0x4321),dict(flags=255&~8),dict(flags=255&~16)]:add('analytical-zero-write',expected=0,**args)
    add('analytical-no-viewer-clamp',flags=255&~32,expected=f(.5));add('analytical-nonplayer-viewer-clamp',flags=255&~64,expected=f(.5));add('analytical-negative-zero-preserved',owner=2,value=0x80000000,limit=0,expected=0x80000000);add('analytical-positive-zero-preserved',owner=2,value=0,limit=0x80000000,expected=0)
    return rows,meta

def expected_services(q):
    s=[0]*9;flags=q[0]
    if not flags&1 or not flags&2:return s
    s[0]=s[1]=1
    if not flags&4 or q[2]==0xffffffff or q[2]>>16!=q[3] or not flags&8:return s
    s[2]=1
    if not flags&16:return s
    s[2]=2;s[8]=1
    if not flags&32:return s
    s[3]=1
    if not flags&64:return s
    s[4]=1
    if q[5] not in [2,3]:return s
    s[5]=1
    if not ((q[5]==2 and q[4]==3) or (q[5]==3 and q[4]==2)):return s
    s[8]+=1;s[3]+=1;s[6]=1
    if q[6] in [1,2]:s[7]=1
    return s

def decode(raw):
    at=0;rows=[]
    def word():
        nonlocal at
        require(at+4<=len(raw),'Weapon trace word extent');v=struct.unpack_from('<I',raw,at)[0];at+=4;return v
    def text(n=None):
        nonlocal at
        if n is None:n=word()
        require(at+n<=len(raw),'Weapon trace text extent');v=raw[at:at+n].decode('ascii');at+=n;return v
    while at<len(raw):
        head=[word() for _ in range(5)];shader=text();require(word()==1,'Exactly one material output variable');value=[word() for _ in range(28)];rows.append(dict(header=head,shader=shader,value=value,text=text(value[10]),name=text(value[11])))
    require(at==len(raw),'Complete weapon output');return rows

def main():
    pre=json.loads((OUT/'preflight.json').read_text());ptr=pre['prior'];parent=Path(ptr['receipt']);require(sha(parent)==ptr['receipt_sha256'],'Prior accepted receipt');prior=json.loads(parent.read_text());frozen=Path(ptr['replay']);require(sha(frozen/'manifest.json')==ptr['replay_manifest_sha256'],'Prior frozen manifest');require(all(sha(frozen/n)==h for n,h in json.loads((frozen/'manifest.json').read_text()).items()),'All prior frozen files')
    sources=source_closure([Path(n) for n in prior['sources']]+[ROOT/'work'/n for n in NEW]);protected=[prior[k] for k in ['sources','original_hashes','private_library_hashes','benchmark_material_hashes','fixture_hashes']]
    for group in protected:require(all(sha(n)==h for n,h in group.items()),'Protected sources/originals/fixtures before')
    require(all(sha(e['path'])==e['sha256'] for e in prior['executables']),'All earlier36 executable identities')
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
    image,ranges=check_ranges(old.CLIENT,'../outputs/material-proxy-evaluation-20261027/client-disassembly-private.txt',[
        ('registration',0x104a21b0,0x104a21c9),('factory',0x1028c5a0,0x1028c5bf),('Init',0x10265100,0x10265126),('bind_renderable_wrapper',0x10175ee0,0x10175f0f),('bind_owner_body',0x1028c510,0x1028c5a0),('GetMaterial',0x101b3950,0x101b3963),('destructor',0x10227110,0x10227132),('player_cast',0x1000d450,0x1000d475),('cloak_getter',0x10226450,0x102264fb),('selector_relation',0x10224d40,0x10224dab),('raw_cloak_read',0x1025f880,0x1025f887),('local_viewer_lookup',0x100564f0,0x10056514)])
    require(image.string(0x10576dc4)=='weapon_invis' and image.string(0x105354c8)=='$cloakfactor','Actual registration/result names');require([image.word(0x10576dac+4*i) for i in range(6)]==[0x10265100,0x10175ee0,0x10175ed0,0x101b3950,0x10227110,0x1028c510],'Actual complete proxy vtable')
    start=time.time();b=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_weapon_invis_contract.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=240);(run/'build-driver.log').write_text(b.stdout+b.stderr);require(b.returncode==0,'Fresh native/owned build')
    executables=[];logs={}
    for native,tag in [(True,'native'),(False,'owned')]:
        exe=OUT/('weapon_invis_reference_x86.exe' if native else 'weapon_invis_owned_x64.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];machine=0x14c if native else 0x8664;require(exe.stat().st_mtime>=start-2 and raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if native else 0x20b) and b'asInvoker' in raw,'Fresh architecture '+exe.name);imp=subprocess.run([str(old.DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imp,re.I|re.M);require({s.lower() for s in modules}=={'kernel32.dll'},'Direct imports '+exe.name);executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules));log=OUT/(tag+'-build.log');require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Clean build '+tag);logs[str(log)]=sha(log)
    logs[str(OUT/'asm-build.log')]=sha(OUT/'asm-build.log')
    def execute(exe,args,label):
        x=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(x.stdout+x.stderr);require(x.returncode==0,label+' exit='+str(x.returncode)+' inspect '+str(run/(label+'.log')));return x.stdout
    rows,meta=corpus();query=run/'query.bin';query.write_bytes(b''.join(struct.pack('<11I',*q) for q in rows));(run/'cases.json').write_text(json.dumps(meta,indent=2));require(query.read_bytes()[:5120*44]==(OUT/'exploration/query.bin').read_bytes(),'Retained first5120 input observations')
    native=run/'native.bin';owned=run/'owned.bin';services=run/'services.bin';native_trace=execute(Path(executables[0]['path']),[old.CLIENT,query,native,services],'native');execute(Path(executables[1]['path']),[run/'absent-original.dll',query,owned],'owned');a,b=decode(native.read_bytes()),decode(owned.read_bytes());require(len(a)==len(b)==len(rows),'All weapon cases recorded')
    if a!=b:
        diffs=[i for i,(x,y) in enumerate(zip(a,b)) if x!=y];i=diffs[0];(run/'difference.json').write_text(json.dumps(dict(differences=len(diffs),first_index=i,input=rows[i],case=meta[i],native=a[i],owned=b[i]),indent=2));raise RuntimeError('Weapon contract mismatch; inspect '+str(run/'difference.json'))
    require(native.read_bytes()[:(OUT/'exploration/native.bin').stat().st_size]==(OUT/'exploration/native.bin').read_bytes(),'All first5120 complete original outputs unchanged')
    calls=list(struct.iter_unpack('<9I',services.read_bytes()));require(len(calls)==len(rows),'Complete original external-service trace');checks=0;writes=no_write=refused=0
    for i,(q,m,state,service) in enumerate(zip(rows,meta,a,calls)):
        h=state['header'];v=state['value'];require(h[0]==i and bool(h[1])==bool(q[0]&1) and h[1]==h[2] and h[3]<=1,'Init/GetMaterial/write count');checks+=1;require(list(service)==expected_services(q),'Exact actual dependency call counts '+str(i));checks+=1
        if not h[1]:refused+=1
        if not h[3]:no_write+=1;require(v[1]==q[1] and v[3]==q[10] and h[4]==0,'No-write preserves cached state');checks+=1
        else:writes+=1;require(v[1]==(q[1]&~15) and v[3:7]==[h[4]]*4 and not v[8] and state['text']=='','SetFloat cached projection/matrix detach');checks+=1
        if m['expected']!='unchecked':require((h[4] if h[3] else None)==m['expected'],'Independent analytical branch '+m['label']);checks+=1
    totals=[sum(s[j] for s in calls) for j in range(9)];require(all(totals),'Every external service branch exercised');counts=re.search(r'native_proxy_allocations=(\d+) releases=(\d+)',native_trace);require(counts and counts[1]==counts[2]==str(len(rows)),'Native proxy allocation balance')
    # No earlier source changed. Reuse pinned old36 binaries, and rerun the
    # complete latest live corpus and rollback tests rather than rebuilding
    # unchanged editor/history/rendering systems.
    prior_native=next(Path(e['path']) for e in prior['executables'] if Path(e['path']).name=='scroll_evaluation_reference_x86.exe');prior_owned=next(Path(e['path']) for e in prior['executables'] if Path(e['path']).name=='scroll_evaluation_owned_x64.exe')
    for native_flag,tail,label in [(True,[],'native'),(True,['--sse2'],'native-sse2'),(False,[],'owned')]:
        dest=run/('prior-live-'+label+'.bin');execute(prior_native if native_flag else prior_owned,[old.CLIENT if native_flag else run/'absent-original.dll',parent.parent/'query.bin',parent.parent/'prepared-original.bin',dest,*tail],dest.stem);require(sha(dest)==prior['output_sha256'],'All489 prior live scroll cases unchanged')
    safety=execute(prior_owned,['--test-scroll'],'prior-scroll-safety');require(safety==prior['safety'],'Prior37 rollback checks/19 actual allocation failures')
    two_path=parent
    while 'unlit-two-texture-parameters' not in str(two_path.parent.parent):two_path=Path(json.loads(two_path.read_text())['prior_receipt'])
    pilot=two_path.parent/'mount/materials/pilot-original.vmt';pilot_exe=next(Path(e['path']) for e in prior['executables'] if Path(e['path']).name=='unlit_two_texture_parameters_owned_x64.exe');pilot_refusal=execute(pilot_exe,['--test-pilot-refusal',pilot],'pilot-refusal');require(pilot_refusal==prior['pilot_refusal'],'Whole pilot remains refused')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());require(all(sha(ROOT/n)==h for n,h in build['editor_source_dependencies'].items()) and sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor graph/executable unchanged')
    for group in [sources,*protected]:require(all(sha(n)==h for n,h in group.items()),'Protected final identities')
    require(all(sha(e['path'])==e['sha256'] for e in prior['executables']) and not (run/'absent-original.dll').exists(),'Old36 identities and owned absent-original independence');capture=run/'complete-weapon.bin.zlib';capture.write_bytes(zlib.compress(native.read_bytes(),9))
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',scope='Actual weapon_invis factory/Init/GetMaterial/OnBind/Release plus unchanged original renderable-to-entity, serial-checked owner lookup, player cast, local-viewer selection and cloak-clamp helper chain under explicit controlled services; independent x64 action/value prerequisite',cases=len(rows),writes=writes,no_write=no_write,init_refused=refused,exact_bytes=native.stat().st_size,independent_checks=checks,analytical_cases=sum(m['expected']!='unchecked' for m in meta),external_service_checks=len(rows)*9,external_service_totals=totals,native_proxy_allocations=int(counts[1]),native_proxy_releases=int(counts[2]),query_sha256=sha(query),output_sha256=sha(native),services_sha256=sha(services),prior_safety=safety,pilot_refusal=pilot_refusal,sources=sources,new_sources=NEW,prior_sources_unchanged=len(prior['sources']),original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],benchmark_material_hashes=prior['benchmark_material_hashes'],fixture_hashes=prior['fixture_hashes'],executables=executables,reused_executables=prior['executables'],build_logs=logs,static_ranges=ranges,prior_receipt=str(parent),prior_receipt_sha256=sha(parent),editor_dependencies_unchanged=len(build['editor_source_dependencies']),editor_sha256=build['executable_sha256'],limits=['Explicit raw service/data snapshots, not a live entity provider or reconstructed engine ownership/handle allocator','No proxy registration or material-store/frame integration; missing real scene context must not be silently replaced by this controlled fixture','Opaque entity selector/mode numbers follow actual branches; broader game semantics and current SFM scene values require adapter proof','Reference code executes unchanged RX with explicit virtual services and mutable data only; scalar setter is a previously proved value service, not combined owner/resource/renderer notification ABI','No live SFM, original writes, fullpilot acceptance, GPU/editor/rendering or whole-goal completion claim; FP exception flags/alternate modes unproved'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');frozen=OUT/('replay-'+run.name.removeprefix('verification-'));frozen.mkdir();included=[Path(n) for n in sources]+[Path(e['path']) for e in executables]+[Path(n) for n in logs]+[receipt,query,services,capture,OUT/'preflight.json']+list(run.glob('*.json'))+list(run.glob('*.log'))
    for p in included:
        dest=frozen/p.relative_to(ROOT);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,dest)
    manifest={p.relative_to(frozen).as_posix():sha(p) for p in frozen.rglob('*') if p.is_file()};(frozen/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt),replay=str(frozen),replay_manifest_sha256=sha(frozen/'manifest.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','writes','no_write','init_refused','exact_bytes','independent_checks']}),flush=True)

if __name__=='__main__':main()
