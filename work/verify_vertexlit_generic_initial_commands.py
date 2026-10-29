"""Initial installed VertexLitGeneric command construction, existing cache only."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, itertools, json, re, struct, subprocess, tempfile, time
import verify_vertexlit_generic_preparation as previous
from verify_material_texture_file_resolver import closure
from verify_ifm_draw_packet import check_ranges

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/vertexlit-generic-initial-commands-20261028'
PARENT=ROOT/'outputs/vertexlit-generic-preparation-correction-20261028/verification-63dui0b0/results.json'
sha=previous.sha
need=previous.need

def corpus():
    rows=previous.corpus()
    for row in rows:
        serial=row[0]
        row.extend([3 if serial&2 else 4,3 if serial&4 else 4,serial%3,serial%3,0xa5a5a5a5^serial])
    seed=[0,1,6,17,1,0,0x40,0,3,4,4,0x101,0x100,0,0,
          0,0,1,0,0,0x3f800000,4,4,1,0,4,4,0,0,0xdecafbad]
    flag_bits=[0x400,0x100000,0x200000,0x40000,2]
    for normal,bump,color,mode,bits,mutation in itertools.product(
            [3,4,19],[3,4,19],[0,1],[0,1],range(32),[0,1,8,11]):
        row=seed.copy();row[0]=len(rows);row[7]=sum(v for i,v in enumerate(flag_bits) if bits&(1<<i))
        row[14]=mutation;row[15]=mode;row[25:]=[normal,bump,color,len(rows)%3,0xdecafbad^len(rows)]
        rows.append(row)
    return rows

def prediction(rows):
    old,coverage=previous.prediction([r[:25] for r in rows])
    result=bytearray();at=static_lists=0;opcodes=set();normal_cases={};color_cases={};flag_pairs=set()
    for row in rows:
        header=struct.unpack_from('<8I',old,at);size=32+72*header[6]+32*header[7]
        part=old[at:at+size];at+=size
        if not header[6]:result+=part;continue
        state=struct.unpack_from('<18I',part,32)
        commands=[];extra=[0]*6
        if row[23]:
            static_lists+=1
            normal=row[25]&15==3;bump=not state[16] and row[26]&15==3
            secondary=row[7]^(0x400000 if header[2] else 0)
            if normal or bump:commands += [7,5,5,13];opcodes.update([7,5])
            commands += [6];opcodes.add(6)
            if row[27]==0:commands += [17,1];opcodes.add(17);kind='disabled'
            else:
                opcode=13 if state[3] else 11;commands += [opcode,1,0x3f800000,0x3f800000,0x3f800000]
                opcodes.add(opcode);kind=str(opcode)
            if secondary&0x40000:commands += [3];opcodes.add(3)
            if secondary&2:commands += [4];opcodes.add(4)
            commands += [0];opcodes.add(0)
            pair=(row[7]>>20)&3;flag_pairs.add(pair)
            extra=[1,int(normal),int(bump),pair,len(commands),0]
            key=f'normal={int(normal)},bump={int(bump)},flash={state[16]}'
            normal_cases[key]=normal_cases.get(key,0)+1;color_cases[kind]=color_cases.get(kind,0)+1
        result+=part[:104]+struct.pack('<6I',*extra)+struct.pack('<'+'I'*len(commands),*commands)+part[104:]
    need(at==len(old) and static_lists>0 and flag_pairs=={0,1,2,3},'exact framing and secondary feature coverage')
    need(opcodes=={0,3,4,5,6,7,11,13,17} and set(color_cases)=={'disabled','11','13'},'all installed command choices')
    coverage.update(static_lists=static_lists,normal_cases=normal_cases,color_cases=color_cases,
                    secondary_pairs=sorted(flag_pairs),command_opcodes=sorted(opcodes),new_cases=len(rows)-6204)
    return bytes(result),coverage

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
    need(sha(PARENT)=='339f792207fb4ff59b3f36e4ec51d96075b5e7e71e92d3d088b1c5a2991a4777','accepted parent receipt')
    prior=json.loads(PARENT.read_text())
    protected={**prior['protected'],**prior['source'],**prior['editor_dependencies'],str(PARENT):sha(PARENT),prior['editor']:prior['editor_sha256']}
    for e in prior['executables'].values():protected[e['path']]=e['sha256']
    originals=prior['original_inputs']
    need(all(sha(p)==h for p,h in protected.items()),'accepted artifacts/source unchanged before')
    need(all(sha(p)==h for p,h in originals.items()),'original inputs unchanged before')
    sources=closure([ROOT/'work/probe_vertexlit_generic_initial_commands.cpp',ROOT/'work/build_vertexlit_generic_initial_commands.cmd',Path(__file__),ROOT/'work/verify_vertexlit_generic_preparation.py'])
    image,ranges=check_ranges(previous.STD,'stdshader_dx9-disassembly.txt',[
        ('initial_static_setup',0x100557b0,0x100558be),
        ('whole_begin_command_list',0x100654a0,0x100654b2),
        ('whole_emit7',0x100654c0,0x100654eb),('whole_emit5',0x100654f0,0x1006551b),
        ('whole_emit6',0x10065520,0x10065533),('whole_emit13',0x100656f0,0x1006573e),
        ('whole_emit11',0x10065740,0x1006578e),('whole_emit17',0x10065850,0x1006587b),
        ('whole_finalize_command_list',0x10065980,0x10065a6b),
        ('memcpy_dispatch_prefix',0x10067410,0x10067470)])
    need(image.word(0x1008b308)==0x3f800000,'literal float one')
    (run/'static-ranges.private.json').write_text(json.dumps(ranges,indent=2)+'\n')
    rows=corpus();expected,coverage=prediction(rows)
    query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<30I',*r) for r in rows))
    (run/'analytical.bin').write_bytes(expected)
    mapping=prior and PARENT.parent/'mapping.bin'
    started=time.time();build=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_vertexlit_generic_initial_commands.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180)
    (run/'build-driver.log').write_text(build.stdout+build.stderr);need(build.returncode==0,'fresh six builds; inspect build logs')
    executables={}
    builds=[('reference','probe_vertexlit_generic_initial_commands','x86'),('owned','probe_vertexlit_generic_initial_commands','x64'),
            ('previous_reference','probe_vertexlit_generic_preparation','x86'),('previous_owned','probe_vertexlit_generic_preparation','x64'),
            ('observer','probe_private_boundary_observer_self_test','x86'),('blend_regression','probe_generic_blend_resource_reference','x64')]
    for tag,stem,arch in builds:
        exe=OUT/(stem+'_'+arch+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        machine,magic=(0x14c,0x10b) if arch=='x86' else (0x8664,0x20b)
        need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic and b'asInvoker' in raw,'fresh architecture/manifest '+tag)
        log=(OUT/(stem+'_'+arch+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log)
        need(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log,re.I),'warning-free '+tag)
        imports=subprocess.run([str(previous.DUMP),'/imports',str(exe)],capture_output=True,text=True,timeout=30)
        dlls=sorted(set(re.findall(r'^\s+([^\s]+\.dll)\s*$',imports.stdout,re.I|re.M)))
        need(imports.returncode==0 and dlls==['KERNEL32.dll'],'system-only imports '+tag)
        (run/(tag+'-imports.txt')).write_text(imports.stdout)
        executables[tag]=dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=dlls)
    def execute(tag,args,label):
        p=subprocess.run([executables[tag]['path'],*map(str,args)],cwd=run,capture_output=True,text=True,timeout=120)
        (run/(label+'.log')).write_text(p.stdout+p.stderr)
        need(p.returncode==0,label+' exit='+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout.strip()
    observed=execute('observer',[],'observer')
    need('PASS private_thread_boundaries=4 callee_bodies_skipped=4 caller_stack=INTACT disarmed_original_body=PASS' in observed,'observer self-test')
    runs=[]
    for i,tag in enumerate(['reference','owned','reference','owned']):
        trace=run/(tag+'-'+str(i)+'.bin')
        stdout=execute(tag,([previous.STD] if tag=='reference' else [])+[mapping,query,trace],trace.stem)
        actual=trace.read_bytes()
        if actual!=expected:
            at=next((j for j,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
            raise RuntimeError(tag+' trace mismatch at byte '+str(at))
        need(f'PASS initial_command_cases={len(rows)} events={coverage["events"]} generic_prefix_cases={coverage["generic_prefix_cases"]} static_lists={coverage["static_lists"]}' in stdout,'exact connected counts '+tag)
        runs.append(dict(tag=tag,path=str(trace),sha256=sha(trace),stdout=stdout))
    regressions=[]
    for tag in ['previous_reference','previous_owned']:
        trace=run/(tag+'.bin');stdout=execute(tag,([previous.STD] if tag.endswith('reference') else [])+[mapping,PARENT.parent/'queries.bin',trace],tag)
        need(sha(trace)==prior['analytical_sha256'] and 'PASS extended_preparation_cases=6204' in stdout,'fresh 6204-case preparation regression '+tag)
        regressions.append(dict(tag=tag,stdout=stdout,sha256=sha(trace)))
    blend_parent=Path(prior['parent']);blend=json.loads(blend_parent.read_text());trace=run/'blend-regression.bin'
    stdout=execute('blend_regression',[blend_parent.parent/'mapping.bin',blend_parent.parent/'queries.bin',trace],'blend-regression')
    need(sha(trace)==blend['analytical_sha256'] and 'PASS blend_resource_cases=5088' in stdout,'fresh 5088-case regression')
    regressions.append(dict(tag='blend_regression',stdout=stdout,sha256=sha(trace)))
    need(all(sha(p)==h for p,h in protected.items()) and all(sha(p)==h for p,h in sources.items()),'prior/source/editor identities unchanged after')
    need(all(sha(p)==h for p,h in originals.items()),'original inputs unchanged after')
    result=dict(status='VERIFIED initial installed command construction/finalization with existing sufficient cache; complete renderer INCOMPLETE',
                utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',parent=str(PARENT),parent_sha256=sha(PARENT),
                source=sources,protected=protected,original_inputs=originals,executables=executables,runs=runs,coverage=coverage,
                trace_bytes=len(expected),analytical_sha256=sha(run/'analytical.bin'),query_sha256=sha(query),mapping_sha256=sha(mapping),
                compiler_warnings=0,regressions=regressions,observer=observed,
                static_ranges=[{k:v for k,v in r.items() if k!='instructions'} for r in ranges],
                editor=prior['editor'],editor_sha256=prior['editor_sha256'],editor_dependencies=prior['editor_dependencies'],editor_rebuilt=False,
                limitations=['Original concrete14/router/generic prefix continues through558BE only on static path; dynamic path stops at557B0. Remaining static state, full cleanup and GPU/image fidelity NOT verified.',
                             'Whole original command helpers and finalizer execute. Cache uses three existing sufficiently sized controlled entries; allocation/growth/release branches are NOT verified. Original memcpy executes for the small bounded lists.',
                             'Installed infoA4 is absent; hardware query/explicit scalar color variants cannot execute here. Other schemas, unlit routes, nonzero flashlight modes, hardware detail modes and context reuse remain unverified.',
                             'Owned component produces words only. No editor integration, cache ownership or command execution is claimed. Original process access/writes, dependency changes and publication were not performed.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n')
    (OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt)),indent=2)+'\n')
    print(json.dumps(dict(status=result['status'],receipt=str(receipt),coverage=coverage,trace_bytes=len(expected)),indent=2),flush=True)

if __name__=='__main__':main()
