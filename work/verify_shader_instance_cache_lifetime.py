"""Authentic per-instance cache lifetime and snapshot release prerequisites."""
from pathlib import Path
from datetime import datetime, timezone
import itertools, json, random, re, struct, subprocess, tempfile, time
import verify_vertexlit_generic_initial_commands as previous
from verify_material_texture_file_resolver import closure
from verify_ifm_draw_packet import check_ranges

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/shader-instance-cache-lifetime-20261028'
PARENT=ROOT/'outputs/vertexlit-generic-initial-commands-20261028/verification-bldcmbk0/results.json'
MATERIAL=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
STD=previous.previous.STD
sha=previous.sha
need=previous.need
pack=lambda words:struct.pack('<'+'I'*len(words),*words)

def corpus():
    rows=[];rng=random.Random(2026102891)
    initial=[(0,0),(1,0)]+[(2,n) for n in [1,4,16,64]]
    for (kind,capacity),n0,n1,flags,destruction in itertools.product(initial,[0,1,4,8,24],[0,1,4,8,24],range(4),range(4)):
        serial=len(rows);rows.append([serial,kind,capacity,n0,n1,(0x40000 if flags&1 else 0)|(2 if flags&2 else 0),
                                      (0x40000 if flags&2 else 0)|(2 if flags&1 else 0),destruction,serial%4,rng.getrandbits(32),rng.getrandbits(32),0])
    return rows

def prediction(rows):
    raw=bytearray();total_alloc=total_free=event_count=0;grow=reuse=created=0
    for serial,kind,capacity,n0,n1,f0,f1,destruction,slot,poison,seed,failure in rows:
        need(failure==0,'normal model only')
        blocks={};entry=None;events=[];states=[];allocations=releases=0;recording=0
        def make(size):
            ident=len(blocks)+1;blocks[ident]=dict(size=size,alive=1,data=[poison]*(size//4));return ident
        def event(kind,ident,size):
            events.append([kind,ident,size,entry['id'] if entry else 0,entry['table'] if entry else 0,
                           entry['buffer'] if entry else 0,entry['capacity'] if entry else 0,recording,allocations])
        def alloc(size):
            nonlocal allocations
            allocations+=1;ident=make(size);event(1,ident,size);return ident
        def free(ident):
            nonlocal releases
            need(blocks[ident]['alive'],'no model double free');event(2,ident,blocks[ident]['size']);blocks[ident]['alive']=0;releases+=1
        def snapshot():
            buffer=entry['buffer'] if entry else 0
            data=blocks[buffer]['data'] if buffer and blocks[buffer]['alive'] else []
            states.extend([entry['id'] if entry else 0,entry['table'] if entry else 0,buffer,
                           entry['capacity'] if entry else 0,blocks[entry['id']]['alive'] if entry else 0,
                           blocks[buffer]['alive'] if buffer else 0,recording,len(data),*data])
        if kind:
            entry=dict(id=make(12),table=2,buffer=0,capacity=0)
            if kind==2 and capacity:entry.update(buffer=make(capacity*4),capacity=capacity*4)
        for phase,(n,flags) in enumerate([(n0,f0),(n1,f1)]):
            words=[(seed^(phase<<24)^(i*0x9e3779b9))&0xffffffff for i in range(n)]
            if flags&0x40000:words.append(3)
            if flags&2:words.append(4)
            words.append(0);recording=0;size=4*len(words)
            if entry is None:
                ident=alloc(12);entry=dict(id=ident,table=2,buffer=0,capacity=0);created+=1
            if entry['capacity']<size:
                if entry['buffer']:free(entry['buffer'])
                ident=alloc(size);entry.update(buffer=ident,capacity=size);grow+=1
            else:reuse+=1
            blocks[entry['buffer']]['data'][:len(words)]=words;snapshot()
        entry['table']=2
        if entry['buffer']:free(entry['buffer'])
        entry['table']=1
        if destruction&1:free(entry['id'])
        snapshot();entry=None;snapshot()
        header=[serial,allocations,releases,sum(b['alive'] for b in blocks.values()),len(states),len(events)]
        raw+=pack(header+states+[v for e in events for v in e]);total_alloc+=allocations;total_free+=releases;event_count+=len(events)
    return bytes(raw),dict(cases=len(rows),allocations=total_alloc,releases=total_free,events=event_count,
                          new_entries=created,buffer_allocations=grow,buffer_reuse=reuse)

def owner_prediction(rows):
    result=bytearray();deleting=pools=0
    for serial,hardware,pattern,passes,salt,null_array in rows:
        events=[];pool=0
        if not null_array:
            events.append([1,hardware,0,0]);count=128 if hardware&2 else 32 if hardware&1 else 16
            for r in range(count):
                for p in range(passes):
                    if (r+p+pattern)&1:events.append([2,r*8+p*2+1,1,0]);deleting+=1
                    if (r*3+p+pattern)&2:events.append([2,r*8+p*2+2,1,0]);deleting+=1
            events.append([1,hardware,0,0]);pool=2 if hardware&3 else 1;events.append([3,pool,0,0]);pools+=1
        result+=pack([serial,len(events),pool]+[v for e in events for v in e])
    return bytes(result),dict(cases=len(rows),deleting_calls=deleting,pool_releases=pools)

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
    need(sha(PARENT)=='e969931e0f3e76e10024d36b382e4027765cb54d35cf40c65a68a6fb47021b1b','accepted receipt')
    prior=json.loads(PARENT.read_text());protected={**prior['protected'],**prior['source'],str(PARENT):sha(PARENT)}
    for e in prior['executables'].values():protected[e['path']]=e['sha256']
    originals={**prior['original_inputs'],str(MATERIAL):'c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8'}
    need(all(sha(p)==h for p,h in protected.items()) and all(sha(p)==h for p,h in originals.items()),'protected/original hashes before')
    sources=closure([ROOT/'work/probe_shader_instance_cache_lifetime.cpp',ROOT/'work/probe_shader_snapshot_instance_release.cpp',ROOT/'work/build_shader_instance_cache_lifetime.cmd',Path(__file__)])
    image,ranges=check_ranges(STD,'stdshader_dx9-disassembly.txt',[
        ('whole_allocator_wrapper',0x10030950,0x10030967),('whole_free_wrapper',0x10030a40,0x10030a57),
        ('whole_instance_deleting_destructor',0x10065260,0x10065298),('whole_base_instance_deleting_destructor',0x100649d0,0x100649f2),
        ('whole_finalize',0x10065980,0x10065a6b),('whole_base_draw_borrowing',0x100652c0,0x1006534b)])
    material,mranges=check_ranges(MATERIAL,'materialsystem-disassembly.txt',[
        ('whole_snapshot_context_release',0x10002b00,0x10002bba),('material_destructor_release_call',0x10006818,0x10006823)])
    need(image.word(0x1009ea58)==0x10065260 and image.word(0x1009ea50)==0x100649d0,'actual deleting vtables')
    need(image.string(image.word(image.word(0x1009ea54)+12)+8)=='.?AVCPerInstanceContextData@@','actual RTTI instance type')
    (run/'static-ranges.private.json').write_text(json.dumps(dict(shader=ranges,material=mranges),indent=2)+'\n')
    rows=corpus();expected,coverage=prediction(rows);query=run/'queries.bin';query.write_bytes(b''.join(pack(r) for r in rows));(run/'analytical.bin').write_bytes(expected)
    owner_rows=[[i,h,pattern,p,0xa5a5a5a5^i,null] for i,(h,pattern,p,null) in enumerate(itertools.product(range(4),range(8),range(5),range(2)))]
    owner_expected,owner_coverage=owner_prediction(owner_rows);owner_query=run/'owner-queries.bin';owner_query.write_bytes(b''.join(pack(r) for r in owner_rows));(run/'owner-analytical.bin').write_bytes(owner_expected)
    started=time.time();build=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_shader_instance_cache_lifetime.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180)
    (run/'build-driver.log').write_text(build.stdout+build.stderr);need(build.returncode==0,'eight fresh builds; inspect logs')
    builds=[('lifetime','probe_shader_instance_cache_lifetime','x86'),('owner','probe_shader_snapshot_instance_release','x86'),
            ('commands_reference','probe_vertexlit_generic_initial_commands','x86'),('commands_owned','probe_vertexlit_generic_initial_commands','x64'),
            ('preparation_reference','probe_vertexlit_generic_preparation','x86'),('preparation_owned','probe_vertexlit_generic_preparation','x64'),
            ('observer','probe_private_boundary_observer_self_test','x86'),('blend','probe_generic_blend_resource_reference','x64')]
    exes={}
    for tag,stem,arch in builds:
        exe=OUT/(stem+'_'+arch+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];machine,magic=(0x14c,0x10b) if arch=='x86' else (0x8664,0x20b)
        need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic and b'asInvoker' in raw,'fresh architecture/manifest '+tag)
        log=(OUT/(stem+'_'+arch+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log);need(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log,re.I),'warning-free '+tag)
        imports=subprocess.run([str(previous.previous.DUMP),'/imports',str(exe)],capture_output=True,text=True,timeout=30);dlls=sorted(set(re.findall(r'^\s+([^\s]+\.dll)\s*$',imports.stdout,re.I|re.M)))
        need(imports.returncode==0 and dlls==['KERNEL32.dll'],'system imports '+tag);(run/(tag+'-imports.txt')).write_text(imports.stdout)
        exes[tag]=dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=dlls)
    def execute(tag,args,label,code=0):
        p=subprocess.run([exes[tag]['path'],*map(str,args)],cwd=run,capture_output=True,text=True,timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        need(p.returncode==code,label+' exit='+str(p.returncode)+' '+p.stdout+p.stderr);return p
    runs=[]
    for tag,original,queries,wanted in [('lifetime',STD,query,expected),('owner',MATERIAL,owner_query,owner_expected)]:
        for repeat in range(2):
            trace=run/(tag+'-'+str(repeat)+'.bin');p=execute(tag,[original,queries,trace],trace.stem);actual=trace.read_bytes()
            if actual!=wanted:
                at=next((i for i,(a,b) in enumerate(zip(actual,wanted)) if a!=b),min(len(actual),len(wanted)))
                raise RuntimeError(tag+' trace mismatch at byte '+str(at))
            runs.append(dict(tag=tag,path=str(trace),sha256=sha(trace),stdout=p.stdout.strip()))
    # Characterize the original unsafe null behavior, do not turn it into a
    # claimed successful cleanup or a proposed owned-code failure policy.
    failures=[]
    for name,kind,capacity,nth,diagnostic in [
        ('new-object',0,0,1,'allocation=1 releases=0 eip=10065a35 address=4 slot=0 capacity=0 recording=0'),
        ('new-buffer',0,0,2,'allocation=2 releases=0 eip=10067467 address=0 slot=1 capacity=36 recording=0'),
        ('grow-buffer',2,1,1,'allocation=1 releases=1 eip=10067467 address=0 slot=1 capacity=36 recording=0')]:
        row=[0,kind,capacity,8,1,0,0,1,0,0xa5a5a5a5,0x12345678,nth];fq=run/(name+'-query.bin');fq.write_bytes(pack(row));trace=run/(name+'-fault.bin')
        p=execute('lifetime',[STD,fq,trace],name,238)
        need(not p.stdout and p.stderr.strip()=='EXPECTED_UNSAFE_NULL_ALLOCATION '+diagnostic,'exact allocation-failure child instruction/state')
        failures.append(dict(name=name,stdout=p.stdout.strip(),stderr=p.stderr.strip(),exit_code=238,cleanup='original cleanup NOT RUN; disposable child terminated'))
    observer=execute('observer',[],'observer').stdout.strip();need('PASS private_thread_boundaries=4' in observer,'observer self-test')
    regressions=[];prep_path=Path(prior['parent']);prep=json.loads(prep_path.read_text());blend_path=Path(prep['parent']);blend=json.loads(blend_path.read_text())
    for tag,parent,data in [('commands_reference',PARENT,prior),('commands_owned',PARENT,prior),('preparation_reference',prep_path,prep),('preparation_owned',prep_path,prep),('blend',blend_path,blend)]:
        trace=run/(tag+'.bin');args=([STD] if tag.endswith('reference') else [])+[parent.parent/'mapping.bin',parent.parent/'queries.bin',trace]
        # Initial-command receipt borrows mapping from its parent.
        if tag.startswith('commands'):args[-3]=prep_path.parent/'mapping.bin'
        p=execute(tag,args,tag);need(sha(trace)==data['analytical_sha256'],'exact accepted regression '+tag);regressions.append(dict(tag=tag,sha256=sha(trace),stdout=p.stdout.strip()))
    need(all(sha(p)==h for p,h in protected.items()) and all(sha(p)==h for p,h in sources.items()),'protected/source unchanged after')
    need(all(sha(p)==h for p,h in originals.items()),'originals unchanged after')
    result=dict(status='VERIFIED original instance-cache lifetime and snapshot-release contracts; owned cache NOT IMPLEMENTED',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',
        parent=str(PARENT),parent_sha256=sha(PARENT),source=sources,protected=protected,original_inputs=originals,executables=exes,runs=runs,
        coverage=coverage,owner_coverage=owner_coverage,failures=failures,compiler_warnings=0,regressions=regressions,observer=observer,
        trace_sha256=sha(run/'analytical.bin'),owner_trace_sha256=sha(run/'owner-analytical.bin'),
        editor=prior['editor'],editor_sha256=prior['editor_sha256'],editor_dependencies=prior['editor_dependencies'],editor_rebuilt=False,
        limits=['Reference investigation only. No owned x64 cache/lifetime integration, GPU execution, full generic cleanup or visual fidelity claim.',
                'Whole original finalizer/destructor execute against explicit tracked allocator virtual services. Host keeps released blocks physically allocated for diagnostic snapshots and then frees all blocks; this is not engine heap implementation.',
                'Whole material snapshot release executes with controlled hardware-query/context-destructor services and an intercepted pool-release boundary. Original stdshader destructors and material owner are separate reference processes, not one combined pipeline.',
                'Fault children establish unsafe original null allocations only; original cleanup is not claimed. A safe owned failure policy must explicitly distinguish refusal from native success equivalence.',
                'Material destructor callsite is authenticated static evidence only; other destructor work, snapshot-pool allocator internals, concurrency and external aliases are unverified.',
                'Original inputs and editor dependencies/binary unchanged; no live SFM/process control, original writes, dependency changes, editor build or publication.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt)),indent=2)+'\n')
    print(json.dumps(dict(status=result['status'],receipt=str(receipt),coverage=coverage,owner_coverage=owner_coverage,failures=failures),indent=2),flush=True)

if __name__=='__main__':main()
