"""Owned pre-resource material ownership versus real original material objects.

No live SFM, GPU or original writes. Local private oracle fixtures are required.
"""
from pathlib import Path
from datetime import datetime,timezone
import difflib,hashlib,json,random,re,shlex,shutil,struct,subprocess,tempfile,time,zlib
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_material_keyvalues_parser import DUMP
from verify_builtin_error_material_seed import source_closure

OUT=ROOT/'outputs/material-store-20261026'
NEW=['material_object_store_candidate.h','probe_material_store.cpp','build_material_store.cmd','verify_material_store.py']


def corpus():
    queries=[];next_id=0;tag=0;expected_names={};groups={}
    def add(*parts):queries.append(list(parts))
    def create(mode,source,name,group='Fixture group',reuse=None,expected=None):
        nonlocal next_id,tag
        tag+=1;add('C',mode,int(source),tag,name,group)
        if reuse is not None:return reuse
        next_id+=1
        if expected is not None:expected_names[next_id]=expected
        return next_id
    names=[('Fixture/Root','fixture/root'),('Fixture\\Mixed.VMT','fixture\\mixed'),('',''),('.Hidden','.hidden'),('a/.Hidden','a/'),('Fixture.Dir/File','fixture.dir/file'),('A.B.C','a.b'),('trailing.','trailing'),('Dir\\','dir\\'),('A//B.VmT','a//b'),('{Proc}.VMT','{proc}'),(' leading name .vmt',' leading name '),('A'*255,'a'*255),('A\\..\\B.VMT','a\\..\\b')]
    begin=len(queries)
    for mode in [0,1]:
        for sourced in [False,True]:
            for n,(name,expected) in enumerate(names):
                id=create(mode,sourced,name,['G','g','fixture/root'][n%3],expected=expected)
                if sourced:add('M',id,'fixture/mutated '+str(id))
                add('I',id);add('R',id)
                if mode:add('R',id)
                add('R',id);add('I',id);add('D',id)
    groups['expanded_names_and_source_aliases']=len(queries)-begin
    rng=random.Random(6407)
    for round in range(4):
        begin=len(queries);ids=[]
        for i in range(288):ids.append(create(1,i%2!=0,'fixture/'+str(i%37)))
        for n in range(37):
            for category in [0,1]:add('F',category,'fixture/'+str(n))
        # Helper hits use a unique key to make independent identity explicit;
        # the bulk duplicate lookup identity is compared to the real tree.
        procedural=create(2,True,'Fixture/Procedural','Creation group',expected='fixture/procedural')
        create(2,True,'FIXTURE\\PROCEDURAL','Ignored group',reuse=procedural)
        add('M',procedural,'helper mutation');add('D',procedural)
        orphan=create(0,True,'fixture/orphan');add('E',orphan);add('X',orphan)
        if round==1:ids.reverse()
        if round>=2:rng.shuffle(ids)
        for step,id in enumerate(ids):
            add('D',id);add('R',id)
            if round==3 and step%11==0:add('R',id)
            add('D',id)
        groups['dictionary_round_'+str(round)]=len(queries)-begin
    begin=len(queries)
    # Helper lookup preserves extensions, unlike construction: repeat .VMT
    # misses and constructs distinct extension-stripped entries. Null-source
    # helper misses stay in the non-source category, even on the next call.
    for source in [False,True]:
        temporary=[]
        for _ in range(2):
            temporary.append(create(2,source,'lookup.with.ext.VMT','Lookup group',expected='lookup.with.ext'))
        for id in temporary:add('D',id)
    for name,source in [('helper/no-source',False),('helper\\slash-spelling',True)]:
        temporary=[create(2,source,name,expected=name) for _ in range(2)]
        for id in temporary:add('D',id)
    for source in [False,True]:
        id=create(1,source,'erase/existing');add('E',id);add('D',id);add('F',int(source),'erase/existing');add('X',id)
    groups['helper_lookup_distinct_from_constructor']=len(queries)-begin
    begin=len(queries)
    for scenario in range(4):
        for sourced in [False,True]:
            for refs in [1,2,3,7]:
                id=create(1,sourced,'fixture/queued')
                for _ in range(refs-1):add('I',id)
                queued=int(scenario!=2)
                if scenario==1:add('Q',id,1,queued)
                for _ in range(refs):add('Q',id,0,queued)
                if scenario!=1:add('Q',id,1,queued)
                if scenario==3:add('H',refs)
                add('N')
                if scenario==1:add('Q',id,1,1);add('N')
                if scenario==3:add('L')
    groups['queued_32_scenarios']=len(queries)-begin
    begin=len(queries);ids=[]
    for i in range(64):
        id=create(1,i%2!=0,'mixed/'+str(i));ids.append(id)
        for _ in range(i%3):add('I',id)
    rng.shuffle(ids)
    for id in ids:
        add('Q',id,1,1)
        # References are tied to creation index, not shuffled position.
        i=id-min(ids)
        for _ in range(i%3+1):add('Q',id,0,1)
        add('Q',id,1,1)
    add('N');groups['mixed_fifo_64_targets']=len(queries)-begin
    return queries,expected_names,groups


def independent(queries,lines,expected_names):
    live={};cached=set();symbols=[];pending=[];held=None;destroyed=[];events=[];next_id=0;next_call=0;checks=0;warnings=0
    def check(b,msg):
        nonlocal checks
        require(b,msg);checks+=1
    def intern(s):
        if s not in symbols:symbols.append(s)
    def destroy(id):
        nonlocal warnings
        m=live.pop(id);cached.discard(id);destroyed.append(id)
        if m['refs']!=0:warnings+=1
        if m['tag']:events.append('S'+str(m['tag']))
    def invoke(id,op):
        if op:
            if live[id]['refs']<=0:destroy(id)
        else:live[id]['refs']-=1
    for step,(q,line) in enumerate(zip(queries,lines)):
        sections=line.split(' |');head=sections[0].split();op=q[0];result=int(head[2]);check(head[:2]==[str(step),op],'Complete step/command '+str(step))
        if op=='C':
            mode,source,tag,name,group=q[1:];lookup=name.lower().replace('\\','/')
            if mode==2:intern(lookup)
            matching=[id for id in cached if live[id]['name']==lookup and live[id]['tag']]
            if mode==2 and matching:
                check(result in matching,'Helper selects existing source-category identity')
                if source:events.append('S'+str(tag))
            else:
                next_id+=1;check(result==next_id,'Every real construction receives distinct logical identity')
                normalized=expected_names.get(result,name)
                # All remaining names in this corpus are already normalized.
                intern(normalized);actual_group='Other textures' if mode==1 else group;intern(actual_group)
                live[result]=dict(name=normalized,group=actual_group,refs=int(mode==1),tag=tag if source else 0,value='fixture/original' if source else '',nested='nested/value' if source else '',flags=(16 if source else 0)|(32 if normalized.startswith('{') else 0))
                if mode:cached.add(result)
        elif op=='F':
            category,name=q[1:];intern(name);matching=[id for id in cached if live[id]['name']==name and bool(live[id]['tag'])==bool(category)];check(result in matching if matching else result==0,'Independent lookup membership')
        elif op=='M':live[q[1]]['value']=q[2]
        elif op=='I':live[q[1]]['refs']+=1
        elif op=='R':live[q[1]]['refs']-=1
        elif op=='D':
            if live[q[1]]['refs']<=0:destroy(q[1])
        elif op=='E':cached.discard(q[1])
        elif op=='X':destroy(q[1])
        elif op=='Q':
            id,action,queued=q[1:]
            if queued:next_call+=1;pending.append([next_call,id,action,1])
            else:invoke(id,action)
        elif op=='H':held=pending[q[1]];held[3]+=1
        elif op=='L':
            check(held and held[3]==1,'Held functor survives target destruction');events.append('F'+str(held[0]));held=None
        elif op=='N':
            for call in pending:
                invoke(call[1],call[2]);call[3]-=1
                if not call[3]:events.append('F'+str(call[0]))
            pending=[]
        tokens=shlex.split(sections[1]);check(len(tokens)==14*len(live),'Every live material recorded')
        for at,id in enumerate(sorted(live)):
            row=tokens[at*14:(at+1)*14];m=live[id]
            want=['M',str(id),m['name'],m['group'],str(m['refs']),str(m['flags']),str(m['tag']),m['value'],m['nested'],'1','1','1045220557','1045220557','1045220557']
            check(row==want,'Independent full material state '+str(step)+' id='+str(id))
        actual=list(map(int,sections[2].split()[1:]));check(set(actual)==cached and len(actual)==len(cached),'Exact cache membership')
        ordered=[(not bool(live[id]['tag']),symbols.index(live[id]['name'])) for id in actual];check(ordered==sorted(ordered),'Source category and interned-symbol order')
        check(sections[3].split()[1:]==[':'.join(map(str,c)) for c in pending],'Exact copied FIFO calls/reference counts')
        check(list(map(int,sections[4].split()[1:]))==destroyed,'Exact destruction order')
        check(sections[5].split()[1:]==events,'Actual source/functor release ordering')
        check(sections[6].split()[1:]==(['1:'+str(held[3])+':1'] if held else ['0']),'Held functor lifetime')
        check(sections[7].split()[1:]==[str(warnings)],'Nonzero-reference destruction diagnostics')
    check(not live and not cached and not pending and held is None,'All logical lifetimes complete')
    return dict(checks=checks,objects=next_id,functors=next_call,source_roots=sum(e.startswith('S') for e in events),destruction_order=destroyed)


def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    ptr=json.loads((OUT/'preflight.json').read_text())['prior'];receipt=Path(ptr['receipt']);require(sha(receipt)==ptr['receipt_sha256'],'Prior tree receipt');prior=json.loads(receipt.read_text())
    modified=ROOT/'work/probe_queued_material_lifetime.cpp';before=OUT/'before/work/probe_queued_material_lifetime.cpp';require(sha(before)==prior['sources'][str(modified)],'Pinned preceding fixture')
    old=before.read_text();expected=old.replace('static bool expectedNullFailure=false;static unsigned failureSlot=0;','#ifndef SFM64_QUEUED_MATERIAL_LIFETIME_FIXTURE_ONLY\nstatic bool expectedNullFailure=false;static unsigned failureSlot=0;',1)+'#endif\n';require(modified.read_text()==expected,'Only optional fixture guard changed')
    delta=dict(before=sha(before),after=sha(modified),diff=''.join(difflib.unified_diff(old.splitlines(True),expected.splitlines(True),fromfile='before',tofile='after')));(run/'source-change.json').write_text(json.dumps(delta,indent=2))
    for p,h in prior['sources'].items():
        if p!=str(modified):require(sha(Path(p))==h,'Unrelated accepted source '+p)
    for k in ['original_hashes','private_library_hashes']:
        for p,h in prior[k].items():require(sha(Path(p))==h,'Protected library '+p)
    require(sha(LIB)==sha(PRIVATE)==PIN,'Original PE identity')
    replay=Path(ptr['replay']);require(sha(replay/'manifest.json')==ptr['replay_manifest_sha256'] and all(sha(replay/k)==h for k,h in json.loads((replay/'manifest.json').read_text()).items()),'Prior immutable replay')
    sources=source_closure([Path(p) for p in prior['sources']]+[ROOT/'work'/n for n in NEW])
    started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_store.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=150);(run/'build-driver.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Fresh owned/reference/affected regression builds')
    exes=[];logs={}
    for name,machine,log in [('store_reference_x86',0x14c,'native'),('store_owned_x64',0x8664,'owned'),('queue_regression_x86',0x14c,'queue-regression')]:
        exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if machine==0x14c else 0x20b) and b'asInvoker' in raw,'Fresh architecture/manifest '+name)
        text=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',text,re.M|re.I);require({m.lower() for m in modules}=={'kernel32.dll'},'Direct imports '+name);exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
        f=OUT/(log+'-build.log');require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',f.read_text(),re.I),'Clean compile '+name);logs[str(f)]=sha(f)
    q,names,groups=corpus();query=run/'query.txt';query.write_text('\n'.join(' '.join(json.dumps(v) if isinstance(v,str) and i else str(v) for i,v in enumerate(row)) for row in q)+'\n',encoding='ascii');(run/'groups.json').write_text(json.dumps(dict(groups=groups,queries=len(q),expected_names=names),indent=2))
    libs=ROOT/'outputs/material-fallback-owned-20261026/verification-0mhr77s9/copied-libraries';mount=run/'empty-mount';mount.mkdir();traces=[];executions=[]
    for name,material,directory in [('store_reference_x86',PRIVATE,libs),('store_owned_x64',run/'no-original.dll',run/'no-original-directory')]:
        dest=run/(name+'.txt');p=subprocess.run([str(OUT/(name+'.exe')),str(material),str(directory),str(mount),str(query),str(dest)],cwd=run,capture_output=True,text=True,timeout=120);(run/(name+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,name+' execution exit='+str(p.returncode)+'; inspect log');traces.append(dest);executions.append(p.stdout+p.stderr)
    native=traces[0].read_bytes();owned=traces[1].read_bytes();lines=native.decode().splitlines();other=owned.decode().splitlines();require(len(lines)==len(other)==len(q),'Complete logical traces')
    diff=[i for i,(a,b) in enumerate(zip(lines,other)) if a!=b];(run/'comparison.json').write_text(json.dumps(dict(operations=len(q),bytes=len(native),differences=len(diff),first=[dict(step=i,query=q[i],native=lines[i],owned=other[i]) for i in diff[:1]]),indent=2));require(not diff and native==owned,'Exact real-object/owned logical traces')
    model=independent(q,lines,names);(run/'independent.json').write_text(json.dumps(model,indent=2));counts=re.search(r'native_allocations=(\d+) releases=(\d+)',executions[0]);require(counts and counts[1]==counts[2],'Actual native heap cleanup')
    safety=subprocess.run([str(OUT/'store_owned_x64.exe'),'--test'],cwd=run,capture_output=True,text=True,timeout=60);(run/'safety.log').write_text(safety.stdout+safety.stderr);require(safety.returncode==0 and 'real_queue_capacity=4096 stale_target_refused=1' in safety.stdout,'Owned transaction/real append failure/stale target policies')
    queued=json.loads(Path(json.loads((ROOT/'outputs/queued-material-lifetime-20261026/latest-verification.json').read_text())['receipt']).read_text());regression=run/'queue-regression.txt';p=subprocess.run([str(OUT/'queue_regression_x86.exe'),str(PRIVATE),str(libs),str(mount),str(regression)],cwd=run,capture_output=True,text=True,timeout=60);(run/'queue-regression.log').write_text(p.stdout+p.stderr);require(p.returncode==0 and sha(regression)==queued['runs'][0]['records_sha256'],'All affected33 queued scenarios unchanged');require('functors=365 retained_functors=8 independent_checks=654 allocations=582 releases=582' in p.stdout,'Affected regression exact accounting')
    failures=[]
    for slot in [13,51]:
        dest=run/('failure-'+str(slot)+'.txt');p=subprocess.run([str(OUT/'queue_regression_x86.exe'),str(PRIVATE),str(libs),str(mount),str(dest),str(slot)],cwd=run,capture_output=True,text=True,timeout=30);(run/('failure-'+str(slot)+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==238 and p.stderr.strip()==f'EXPECTED_UNSAFE_NULL_ENQUEUE slot={slot} failed_allocations=1 eip=1000f717 address=0','Prior expected-fault path unchanged');failures.append(dict(slot=slot,expected_exit=238,cleanup='Not claimed; disposable original-reference child terminated by observer'))
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());graph=set()
    def visit(p):
        k=p.relative_to(ROOT).as_posix()
        if k in graph:return
        graph.add(k)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            child=p.parent/inc
            if child.is_file():visit(child.resolve())
    visit((ROOT/'work/probe_vulkan_win32_surface_x64.cpp').resolve());require(graph==set(build['editor_source_dependencies']) and all(sha(ROOT/k)==h for k,h in build['editor_source_dependencies'].items()),'Whole editor graph unchanged');require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor unchanged')
    for p,h in sources.items():require(sha(Path(p))==h,'Source drift '+p)
    for k in ['original_hashes','private_library_hashes']:
        for p,h in prior[k].items():require(sha(Path(p))==h,'Original drift '+p)
    require(not (run/'no-original.dll').exists() and not (run/'no-original-directory').exists(),'Owned execution absent original paths')
    compressed=run/'complete-logical-trace.txt.zlib';compressed.write_bytes(zlib.compress(native,9));require(zlib.decompress(compressed.read_bytes())==native,'Lossless trace evidence')
    limits=['Standalone owned pre-resource objects/source adoption/cache/references/forwarding/FIFO lifetime only; no complete IMaterial ABI, renderer or editor integration',
      'Typed owned source uses existing SourceMaterialPatchNodeCandidate; nested string trees and pointer identity/mutation/lifetime tested, not arbitrary original KeyValues metadata or unsupported encodings',
      'ASCII names/groups limited to4096 bytes by explicit owned safety policy; symbol/node domains stay16-bit, object IDs/payloads/pointers are native width',
      'Owner-thread serialized API; atomic reference fields do not establish general multithread safety. Queue buffer implementation reused; destructor drains pending valid or safely refused stale calls before releasing local objects',
      'Safe OOM/append refusal, checked stale IDs and monotonic identity are explicit owned policies; native allocation failure is unsafe and remains a separate expected-fault reference observation',
      'Original material constructor/source/raw+forward refs, cache, procedural helper and FIFO execute unchanged RX with controlled host allocator/KV symbol/lock/context/mode/unbind services; not full original render-context initialization',
      'No textures/proxies/shaders/resources, general FindMaterial, actual GPU, live SFM, Blender, original writes or editor build/run; full goal remains incomplete']
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),scope='Owned x64 pre-resource material ownership and queued lifetime',previous_goal_turn='progress',operations=len(q),trace_bytes=len(native),trace_sha256=hashlib.sha256(native).hexdigest(),query_sha256=sha(query),model=model,groups=groups,native_allocations=int(counts[1]),native_releases=int(counts[2]),safety=safety.stdout,executables=exes,build_logs=logs,source_change=delta,sources=sources,original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],prior_receipt=str(receipt),prior_receipt_sha256=sha(receipt),queued_regression=dict(cases=33,functors=365,records_sha256=sha(regression)),expected_unsafe_failures=failures,editor_dependencies_unchanged=len(graph),editor_sha256=build['executable_sha256'],limits=limits)
    dest=run/'results.json';dest.write_text(json.dumps(result,indent=2)+'\n');replay=OUT/('replay-'+run.name.removeprefix('verification-'));replay.mkdir()
    for p in [Path(p) for p in sources]+[Path(e['path']) for e in exes]+[Path(p) for p in logs]+[dest,before,query,compressed]+list(run.glob('*.log'))+list(run.glob('*.json'))+[regression]:
        t=replay/p.relative_to(ROOT);t.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,t)
    manifest={p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()};(replay/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');pointer=dict(receipt=str(dest),receipt_sha256=sha(dest),replay=str(replay),replay_manifest_sha256=sha(replay/'manifest.json'));(OUT/'latest-verification.json').write_text(json.dumps(pointer,indent=2)+'\n');print(json.dumps(dict(status=result['status'],receipt=str(dest),operations=len(q),trace_bytes=len(native),independent_checks=model['checks'],safety=safety.stdout)))


if __name__=='__main__':main()
