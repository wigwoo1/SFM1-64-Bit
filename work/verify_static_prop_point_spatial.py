"""Owned BSP -> retained assets -> effective registration -> ordered partition.
Original registration and original partition execute as separate test oracles.
The owned executable receives original asset paths and rays, never oracle boxes.
"""
import collections,hashlib,json,re,shutil,struct,subprocess,tempfile
from pathlib import Path
from verify_spatial_ray_flags import parseOutput
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/static-prop-point-spatial-20261016'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
REG=ROOT/'outputs/static-prop-partition-registration-20261016/verification-or1d0we6/results.json'
SCOPE=ROOT/'outputs/studio-vcollide-read-20261016/verification-gr1jjsch/results.json'
FLAGS=ROOT/'outputs/spatial-ray-flags-20261016/verification-unn8kagu/results.json'
CLIP=ROOT/'outputs/static-prop-point-registry-20261016/verification-72v49r60/results.json'
DUMP=Path('C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def main():
    references={str(p):json.loads(p.read_text()) for p in [REG,SCOPE,FLAGS,CLIP]}
    require(all(x['status']=='PASS' for x in references.values()),'Prior proof status')
    registration,scope,flags,clip=[references[str(p)] for p in [REG,SCOPE,FLAGS,CLIP]]
    originals=flags['original_hashes']
    for p,s in originals.items():require(sha(Path(p))==s,'Original changed before test '+p)
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libs=run/'libraries';libs.mkdir()
    for name in ['tier0.dll','vphysics.dll']:shutil.copyfile(GAME/'bin'/name,libs/name)
    mounts=run/'mounts.txt';shutil.copyfile(CLIP.parent/'mounts.txt',mounts)
    executables=[]
    for name in ['registration_native_x86','partition_native_x86','spatial_owned_x64','spatial_safety_x64','registry_owned_x64','registry_safety_x64']:
        exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];machine=0x14c if name.endswith('x86') else 0x8664
        require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Executable identity '+name)
        imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout
        (run/(name+'-imports.txt')).write_text(imports)
        modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I)
        require(modules and set(x.lower() for x in modules)=={'kernel32.dll'},'Unexpected runtime dependency '+name+' '+str(modules))
        executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
    def execute(name,args,log):
        r=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=60)
        log.write_text(r.stdout+r.stderr);require(r.returncode==0,'Probe failed '+str(log)+' '+r.stdout+r.stderr);return r.stdout.strip()
    # Repeat the actual original registration corpus, including missing/empty,
    # model-box replacement, arbitrary returned handle and nonbenchmark poses.
    assets={r['model']:r for r in scope['records']};dummy=next(r['phy'] for r in scope['records'] if r['phy'])
    freshRegistrationQueries=0
    for i,record in enumerate(registration['records']):
        case=run/('registration-%03d'%i);case.mkdir();old=Path(record['path']);output=case/'native.bin';asset=assets[record['model']]
        require(sha(old/'native.bin')==record['sha256'],'Frozen registration identity')
        execute('registration_native_x86',[GAME/'bin/engine.dll',libs,asset['phy'] or dummy,old/'queries.bin',output],case/'native.log')
        require(output.read_bytes()==(old/'native.bin').read_bytes() and Path(str(output)+'.physics').read_bytes()==(old/'native.bin.physics').read_bytes(),'Fresh registration regression '+record['model'])
        freshRegistrationQueries+=record['queries']
        if (i+1)%10==0:print('PASS fresh original registration models='+str(i+1),flush=True)
    # Frozen rows are independent expected registration, now freshly revalidated.
    baselineRaw=Path(registration['path'])/'native-ordered.bin';baseline=baselineRaw.read_bytes()
    cases=json.loads((Path(registration['path'])/'cases.json').read_text());expectedStates=bytearray();objects=[];counts=collections.Counter()
    for i,c in enumerate(cases):
        if c['label']!='benchmark':continue
        events=struct.unpack_from('<8I',baseline,i*56);registered,effective,mask=events[:3];ordinal=c['ordinal'];id=((ordinal+1)<<32)|0xdecafbad
        box=baseline[i*56+32:(i+1)*56];handle=len(objects) if registered else 65535
        expectedStates+=struct.pack('<Q4I',id,c['solid'],effective,handle,registered)+box
        if registered:objects.append(struct.pack('<QI',id,mask)+box)
        counts['props']+=1;counts['registered']+=registered;counts['disk_nonsolid']+=c['solid']==0;counts['missing_disabled']+=c['solid']==6 and effective==0
    require(dict(counts)==dict(props=485,registered=145,disk_nonsolid=339,missing_disabled=1),'Benchmark registration inventory')
    header=struct.pack('<I',len(objects))+b''.join(objects)
    querySets=[]
    for name in ['prior-benchmark','directed-and-caller-shortened']:
        record=next(r for r in flags['records'] if r['name']==name and r['mode']=='explicit')
        fixture=Path(record['fixture']);require(sha(fixture)==record['fixture_sha256'],'Frozen flag input identity');wire=fixture.read_bytes();n=struct.unpack_from('<I',wire)[0];at=4+n*36
        querySets.append((name,wire[at:]))
    # Explicit prefixes at every registered box center. These are new composed
    # pipeline queries, not the frozen tree's stored results.
    prefixRows=[]
    for entity in objects:
        box=struct.unpack_from('<6f',entity,12);center=[(box[j]+box[j+3])*.5 for j in range(3)]
        for flag in [0,1]:
            for stop in [0,1,2,5]:prefixRows.append(struct.pack('<I6fIII',0x65,*center,0,0,0,0,stop,flag))
    querySets.append(('registered-centers-prefixes',struct.pack('<I',len(prefixRows))+b''.join(prefixRows)))
    queryRecords=[];totalQueries=0;nonempty=0
    for name,queries in querySets:
        case=run/name;case.mkdir();query=case/'queries.bin';query.write_bytes(queries);fixture=case/'native-fixture.bin';fixture.write_bytes(header+queries)
        native=case/'native.bin';owned=case/'owned.bin'
        execute('partition_native_x86',[GAME/'bin/engine.dll',libs,fixture,native,'explicit-swept'],case/'native.log')
        print(execute('spatial_owned_x64',[GAME/'tf_movies/maps/mtt_soldier.bsp',mounts,query,owned],case/'owned.log'),flush=True)
        require(Path(str(owned)+'.registrations').read_bytes()==expectedStates,'Owned effective registration/raw bounds mismatch '+name)
        require(owned.read_bytes()==native.read_bytes(),'Ordered composition output mismatch '+name)
        require(Path(str(owned)+'.stored-boxes').read_bytes()==Path(str(native)+'.stored-boxes').read_bytes(),'Composed stored bounds/mask/index flags mismatch '+name)
        answers=parseOutput(owned.read_bytes());allIds={struct.unpack_from('<Q',e)[0] for e in objects}
        require(all(len(a)==len(set(a)) and set(a)<=allIds for a in answers),'Returned duplicate/disabled/unknown/truncated entity')
        totalQueries+=len(answers);nonempty+=sum(bool(a) for a in answers)
        if name=='registered-centers-prefixes':
            for at in range(0,len(answers),4):
                for offset,stop in enumerate([0,1,2,5]):require(answers[at+offset]==answers[at][:stop] if stop else answers[at+offset]==answers[at],'Composed early-stop prefix')
        queryRecords.append(dict(name=name,queries=len(answers),path=str(case),queries_sha256=sha(query),native_fixture_sha256=sha(fixture),ordered_sha256=sha(owned),registration_sha256=sha(Path(str(owned)+'.registrations')),stored_boxes_sha256=sha(Path(str(owned)+'.stored-boxes'))))
    safety=execute('spatial_safety_x64',[GAME/'tf_movies/maps/mtt_soldier.bsp',mounts],run/'spatial-safety.log');print(safety,flush=True)
    registrySafety=execute('registry_safety_x64',[GAME/'tf_movies/maps/mtt_soldier.bsp',mounts],run/'registry-safety.log');print(registrySafety,flush=True)
    require(sha(CLIP.parent/'queries.bin')==clip['query_sha256'],'Frozen clip query identity')
    output=run/'registry-regression.bin';print(execute('registry_owned_x64',[GAME/'tf_movies/maps/mtt_soldier.bsp',mounts,CLIP.parent/'queries.bin',output],run/'registry-regression.log'),flush=True)
    for suffix in ['', '.bindings','.events']:require(Path(str(output)+suffix).read_bytes()==(CLIP.parent/('owned.bin'+suffix)).read_bytes(),'Per-prop registry regression '+suffix)
    for p,s in originals.items():require(sha(Path(p))==s,'Original changed after tests '+p)
    names=['source_static_prop_point_spatial_candidate.h','probe_static_prop_point_spatial.cpp','test_static_prop_point_spatial.cpp','build_static_prop_point_spatial.cmd','verify_static_prop_point_spatial.py']
    changed={str(ROOT/'work'/p):sha(ROOT/'work'/p) for p in names}
    # Local include closure binds receipts to actual compiled owned/native code.
    sources=dict(changed);pending=[ROOT/'work'/p for p in names]+[ROOT/'work'/p for p in ['probe_native_spatial_partition.cpp','probe_static_prop_partition_registration_native.cpp','probe_static_prop_point_registry.cpp','test_static_prop_point_registry.cpp','x64_fsin_cos.asm']]
    visited=set()
    while pending:
        p=pending.pop()
        if p in visited:continue
        visited.add(p);sources[str(p)]=sha(p)
        if p.suffix not in ['.h','.cpp']:continue
        for local in re.findall(r'^\s*#include\s*"([^"]+)"',p.read_text(),re.M):
            child=p.parent/local
            if child.is_file():pending.append(child)
    logs=[]
    for p in OUT.glob('*-build.log'):
        content=p.read_text();require(not re.search(r'\b(?:warning|error) [A-Z]+\d+',content),'Compiler diagnostic '+str(p));logs.append(dict(path=str(p),sha256=sha(p),warnings=0,errors=0))
    limitations=['Initial immutable solid0/solid6 one-solid configured loose-mount registration only; solid2/hulls/dynamic moves/removals/callback mutation remain unsupported.',
        'Owned preparation computes bounds from owned retained assets; no native box/state is passed to owned executable. Surface properties explicitly unresolved: broadphase only, NOT full world/prop trace integration.',
        'Actual original registration/physics and actual original partition each run, but as separate oracle processes with controlled services. Not a single full engine pipeline, live SFM or complete application workflow.',
        'Original completed missing collision scope is prior tested explicit GAME loose directories, not all Steam/archive/dynamic-provider configurations.',
        'Existing per-prop clip regression compared to pinned prior native results; native per-prop clipping not rerun this milestone.',
        'No editor integration/build, static-prop lighting guard removal, serialization, animation, renderer, save behavior or dependencies changed.']
    result=dict(status='PASS',path=str(run),benchmark=dict(counts),fresh_original_registration_queries=freshRegistrationQueries,ordered_queries=totalQueries,nonempty_queries=nonempty,registration_differences=0,stored_bounds_differences=0,ordered_differences=0,safety=safety,registry_safety=registrySafety,clip_regression_queries=clip['queries'],records=queryRecords,executables=executables,changed_sources=changed,sources=sources,build_logs=logs,original_hashes=originals,reference_receipts={str(p):sha(p) for p in [REG,SCOPE,FLAGS,CLIP]},mounts_sha256=sha(mounts),limitations=limitations)
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2))
    (OUT/'source-integrity.json').write_text(json.dumps(dict(status='PASS',authoritative_receipt=str(receipt),receipt_sha256=sha(receipt),sources=sources,changed_sources=changed,build_logs=logs,executables=executables,original_hashes=originals),indent=2))
    report=['VERIFIED: owned retained static-prop initial registration + point spatial enumeration.', 'Whole application PARTIALLY VERIFIED.',str(receipt),'',str(dict(counts)), 'Fresh original registration cases: '+str(freshRegistrationQueries),'Ordered composed point queries: '+str(totalQueries)+'; differences=0','Stored and physical bounds, masks, flags and handles: differences=0',safety,registrySafety,'18430 per-prop clipping regressions: exact full trace/events/bindings.','Original files hash-checked unchanged: '+str(len(originals)),'', 'Exact freshly built/tested executables:']
    report += [e['path']+' SHA256='+e['sha256']+' machine='+e['machine'] for e in executables]
    report += ['', 'Changed files:']+list(changed)+['','Scope:']+limitations+['','Reproduce from workspace:','cmd /c work\\build_static_prop_point_spatial.cmd','C:\\Users\\wigwo\\AppData\\Local\\Python\\bin\\python3.14.exe work\\verify_static_prop_point_spatial.py']
    (OUT/'verification.txt').write_text('\n'.join(report)+'\n')
    (OUT/'remaining-integration.txt').write_text('Next: resolve actual studio surface-property provider and retained runtime identity; then compose verified world + props + ordered enumeration with full original trace comparison.\nUse effective runtime solid state; keep disk binding unchanged. Never derive swept flag from shortened delta.\nDo not remove current static-prop lighting guard or claim editor integration from these component tests.\nGeneral solid2/hulls/dynamic spatial mutation/archive/included-model/async/multisolid scope is incomplete.\nContinue Sol High for provider reverse engineering and scene integration; goal active, no model switch or whole-project percentage.\n')
    print('PASS '+str(receipt),flush=True)
if __name__=='__main__':main()
