"""Compare intact server simple-filter orchestration with owned x64 semantics.

StandardFilterRules and PassServerEntityFilter are declared supplied services.
This verifies their caller ordering, not those two predicates or real entities.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
from verify_sliding_door_spawn import verify_installed_ranges, SERVER, ENGINE, BSP

ROOT=Path(__file__).resolve().parent
PINS={SERVER:'47a03f6bce8f519dafe1cfbcb9ba97cf0b0069d6eab57c2a52cffd61c66265d4',
      ENGINE:'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f',
      BSP:'60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea'}
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def closure():
    import ast
    result={}
    def visit(path):
        path=path.resolve();key=path.relative_to(ROOT).as_posix()
        if key in result:return
        result[key]=sha(path);text=path.read_text(encoding='utf-8-sig')
        for name in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',text,re.M):visit(path.parent/name)
        if path.suffix=='.py':
            for node in ast.walk(ast.parse(text)):
                names=[a.name for a in node.names] if isinstance(node,ast.Import) else [node.module] if isinstance(node,ast.ImportFrom) and node.module else []
                for name in names:
                    local=ROOT/(name.split('.')[0]+'.py')
                    if local.is_file():visit(local)
    for name in ('probe_server_simple_trace_filter.cpp','build_server_simple_trace_filter.cmd','verify_server_simple_trace_filter.py'):visit(ROOT/name)
    return result

def predict():
    rows=[];stops={name:0 for name in ('standard','ignored','static','base','entity','groups','extra','accepted')}
    for flags in range(512):
        b=lambda n:bool(flags&(1<<n))
        for mask in (0,1,0x200400b,0xffffffff):
            for group in (-1,0,5):
                events=[1];stop='standard';accepted=False
                if b(0):
                    if b(1):events.append(2)
                    if b(1) and not b(2):stop='ignored'
                    else:
                        events.append(3)
                        if b(3):stop='static'
                        else:
                            events.append(4)
                            if not b(4):stop='base'
                            else:
                                events.append(5)
                                if not b(5):stop='entity'
                                else:
                                    events.append(6)
                                    if not b(6):stop='groups'
                                    else:
                                        if b(7):events.append(7)
                                        accepted=not b(7) or b(8)
                                        stop='accepted' if accepted else 'extra'
                stops[stop]+=1
                rows.append(struct.pack('<IIii10I',flags,mask,group,-7 if group==5 else 31,int(accepted),len(events),*(events+[0]*(8-len(events)))))
    assert sum(stops.values())==6144 and all(stops.values())
    return b''.join(rows),stops

def main(directory):
    directory=directory.resolve()
    assert directory.is_relative_to(Path('D:/SFM64-development-tests').resolve())
    directory.mkdir(parents=True,exist_ok=True)
    assert all(sha(p)==v for p,v in PINS.items())
    frozen=closure();run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    build=subprocess.run(['cmd','/c',str(ROOT/'build_server_simple_trace_filter.cmd'),str(run)],capture_output=True,text=True,timeout=180)
    (run/'build.log').write_text(build.stdout+build.stderr)
    assert build.returncode==0,build.stdout+build.stderr
    image,evidence=verify_installed_ranges(SERVER,run/'server-filter-reference.local.txt',[
        ('complete simple ShouldHitEntity',0x102677a0,0x10267852),
        ('complete simple filter constructor',0x10267770,0x10267791),
        ('complete trace type virtual',0x10140140,0x10140143)])
    assert image.word(0x104e7390)==0x102677a0 and image.word(0x104e7394)==0x10140140
    expected,coverage=predict();builds={};runs=[]
    for arch,machine,magic in (('x86',0x14c,0x10b),('x64',0x8664,0x20b)):
        log=(run/f'{arch}-build.log').read_text(errors='replace')
        assert not re.search(r'\bwarning\b|\berror\b',log,re.I),log
        exe=run/f'simple_filter_{arch}.exe';binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0]
        assert binary[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',binary,pe+4)[0]==machine and struct.unpack_from('<H',binary,pe+24)[0]==magic
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
        for repeat in (1,2):
            output=run/f'{arch}-{repeat}.bin'
            result=subprocess.run([str(exe),str(SERVER) if arch=='x86' else '-',str(output)],capture_output=True,text=True,timeout=120)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode==0,(arch,result.stdout,result.stderr)
            assert json.loads(result.stdout)==dict(cases=6144,passed=True)
            assert output.read_bytes()==expected,('full result/callback ordering mismatch',arch,repeat)
            runs.append(dict(architecture=arch,repeat=repeat,record_sha256=sha(output)))
    assert closure()==frozen,'source changed during verification'
    assert all(sha(p)==v for p,v in PINS.items())
    editor=json.loads((ROOT.parent/'publication/stage/docs/build-status.json').read_text())
    deps=editor['editor_source_dependencies'];assert len(deps)==543
    assert all(sha(ROOT.parent/p)==v for p,v in deps.items())
    receipt=dict(status='VERIFIED BOUNDED SERVER SIMPLE FILTER ORCHESTRATION',cases_per_run=6144,repeats_per_arch=2,
        builds=builds,runs=runs,coverage=coverage,record_bytes=len(expected),record_sha256=hashlib.sha256(expected).hexdigest(),
        source_sha256=frozen,original_sha256={str(p):v for p,v in PINS.items()},originals_unchanged=True,
        editor_dependencies_unchanged=543,verified_spans=evidence,
        supplied_services=['standard rules102676b0','pass entity relationship10267610','static manager virtual+0c',
                           'handle base entity virtual+14','entity ShouldCollide virtual+44','game rules virtual+70','optional extra predicate'],
        findings=['All 512 policy combinations, four masks and three signed groups match intact original caller and independent predictions including callback order and short-circuit behavior.',
                  'Native filter/entity inputs remain unchanged and executable sections are sealed and verified unchanged.',
                  'The server filter rejects static props if called on them; the engine trace mode can bypass this predicate for static props. It must not be applied blindly to every candidate.',
                  'Owned x64 identity fixtures use values above32bits; original32bit addresses are test-only bindings.'],
        limits=['This proves decision composition only; standard rules, ignored entity relationships, virtual policy bodies, actual entity storage, engine hull enumeration/clipping and real spawn remain unverified.',
                'Prepared world dispatcher and this filter are not yet integrated into editor collision. No editor build or visual improvement is claimed.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'],coverage=coverage,receipt=str(run/'results.json'))))
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path)
    main(parser.parse_args().directory)
