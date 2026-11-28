"""Actual server trace annotation versus owned x64 semantic identities."""
import argparse
import ast
from collections import Counter
import hashlib
import json
from pathlib import Path
import random
import re
import struct
import subprocess
import tempfile
from verify_server_simple_trace_filter import PINS, sha
from verify_sliding_door_spawn import ENGINE, verify_installed_ranges
ROOT=Path(__file__).resolve().parent

def closure():
    files={}
    def visit(path):
        path=path.resolve();name=path.relative_to(ROOT).as_posix()
        if name in files:return
        files[name]=sha(path);s=path.read_text(encoding='utf-8-sig')
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',s,re.M):visit(path.parent/inc)
        if path.suffix=='.py':
            for n in ast.walk(ast.parse(s)):
                names=[a.name for a in n.names] if isinstance(n,ast.Import) else [n.module] if isinstance(n,ast.ImportFrom) and n.module else []
                for module in names:
                    candidate=ROOT/(module.split('.')[0]+'.py')
                    if candidate.is_file():visit(candidate)
    for n in ('probe_server_trace_annotation.cpp','build_server_trace_annotation.cmd','verify_server_trace_annotation.py'):visit(ROOT/n)
    return files

def corpus():
    rng=random.Random(0x1016d840);inputs=[];outputs=[];coverage=Counter()
    for handle in (0,1,0x0000ffff,0x40000000,0x40000001,0x4000ffff,0x40010001,0xc0000001,0xffffffff):
        for flags in (0,1,2,3,4,5,7,255):
            for present in (0,1):
                for fraction in (0.,.5,1.):
                    for allsolid in (0,1,2):
                        for startsolid in (0,1,2):
                            for prior in (0,0x12345678,0xffffffff):
                                row=bytearray(struct.pack('<10f4sfIHBBfQIIHHQI',
                                    *[rng.randrange(-10000,10001)/16. for _ in range(10)],
                                    bytes(rng.randrange(256) for _ in range(4)),fraction,
                                    rng.getrandbits(32),rng.getrandbits(16),allsolid,startsolid,.125,
                                    rng.choice((0,1234,0x100000000)),rng.getrandbits(32),rng.getrandbits(32),
                                    rng.getrandbits(16),rng.getrandbits(16),prior,rng.getrandbits(32)))
                                assert len(row)==92
                                inputs.append(struct.pack('<3I',handle,flags,present)+row)
                                events=[]
                                if fraction<1 or allsolid or startsolid:
                                    events=[1,2]
                                    static=handle&0xffff0000==0x40000000
                                    identity=(0x300000001 if flags&4 and present else 0) if static else 0x200000001
                                    struct.pack_into('<Q',row,80,identity)
                                    if static:
                                        events.append(2);struct.pack_into('<I',row,88,(handle&0xffff)+1)
                                    coverage['static' if static else 'nonstatic']+=1
                                    coverage['prior_nonzero_replaced']+=prior!=0
                                    coverage['wide_identity_outputs']+=identity>0xffffffff
                                    coverage['static_world_present']+=static and bool(identity)
                                    coverage['static_world_absent']+=static and not identity
                                    coverage['static_index65535']+=static and (handle&0xffff)==65535
                                else:coverage['miss_unchanged']+=1
                                outputs.append(bytes(row)+struct.pack('<I',len(events))+b''.join(struct.pack('<I',x) for x in events))
    assert all(coverage.values())
    return b''.join(inputs),b''.join(outputs),coverage,len(inputs)

def main(directory):
    directory=directory.resolve();assert directory.is_relative_to(Path('D:/SFM64-development-tests').resolve())
    directory.mkdir(parents=True,exist_ok=True);assert all(sha(p)==v for p,v in PINS.items())
    frozen=closure();run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory));query,want,coverage,count=corpus()
    source=run/'queries.bin';source.write_bytes(query)
    build=subprocess.run(['cmd','/c',str(ROOT/'build_server_trace_annotation.cmd'),str(run)],capture_output=True,text=True,timeout=180)
    (run/'build.log').write_text(build.stdout+build.stderr);assert build.returncode==0,build.stdout+build.stderr
    image,evidence=verify_installed_ranges(ENGINE,run/'annotation-reference.local.txt',[
        ('server annotation',0x1016d840,0x1016d8b6),('static manager getter',0x101c46e0,0x101c46e6),
        ('static entity classification',0x101c4700,0x101c472e),('static entity index',0x101c4750,0x101c4776)])
    assert image.word(0x1035456c+26*4)==0x1016d840
    assert image.word(0x1040b598+0) and image.word(image.word(0x1040b598)+11*4)==0x101c4700
    assert image.word(image.word(0x1040b598)+12*4)==0x101c4750
    builds={}
    for arch,machine,magic in (('x86',0x14c,0x10b),('x64',0x8664,0x20b)):
        log=(run/f'{arch}-build.log').read_text(errors='replace');assert not re.search(r'warning|error',log,re.I),log
        exe=run/f'annotation_{arch}.exe';data=exe.read_bytes();pe=struct.unpack_from('<I',data,60)[0]
        assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0]==machine and struct.unpack_from('<H',data,pe+24)[0]==magic
        for repeat in (1,2):
            output=run/f'{arch}-{repeat}.bin'
            p=subprocess.run([str(exe),str(ENGINE) if arch=='x86' else '-',str(source),str(output)],capture_output=True,text=True,timeout=120)
            (run/f'{arch}-{repeat}.log').write_text(p.stdout+p.stderr)
            assert p.returncode==0,(arch,p.stdout,p.stderr)
            assert p.stdout.strip()==f'PASS annotation cases={count}'
            assert output.read_bytes()==want,('annotation full trace/events differ',arch,repeat)
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
    assert closure()==frozen and all(sha(p)==v for p,v in PINS.items())
    editor=json.loads((ROOT.parent/'publication/stage/docs/build-status.json').read_text())
    deps=editor['editor_source_dependencies'];assert len(deps)==543 and all(sha(ROOT.parent/p)==v for p,v in deps.items())
    receipt=dict(status='VERIFIED BOUNDED SERVER TRACE ANNOTATION',cases_per_run=count,repeats_per_arch=2,
        coverage=dict(coverage),builds=builds,record_bytes=len(want),record_sha256=hashlib.sha256(want).hexdigest(),
        query_sha256=sha(source),source_sha256=frozen,verified_spans=evidence,
        original_sha256={str(p):v for p,v in PINS.items()},originals_unchanged=True,editor_dependencies_unchanged=543,
        supplied_services=['valid nonnull collideable virtual0 entity binding','entity GetRefEHandle virtual+8','prepared nonnull world-record storage'],
        findings=['Complete original server1016d840 and original static manager getter/classification/index routines execute unchanged in a sealed private image.',
                  'All92semantic trace bytes and callback order match independent predictions, including no-hit preservation, entity replacement, world flag4 gating and static index+1.',
                  'Native addresses normalize only to distinguishing wide semantic entity/world identities. Original trace guards, unrelated84-byte trace fields, world record and manager remain unchanged.'],
        limits=['Finite prepared trace states, valid nonnull collideable/entity/world-record storage only; malformed pointer behavior and production storage lifetime are not established.',
                'This is server annotation only. The client has different null/nonstatic/world resolution semantics and is not covered.',
                'Actual clipping geometry, full world loading, filtering composition and real spawn/editor behavior remain separate gates.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'],cases=count,receipt=str(run/'results.json'),coverage=coverage)))

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);main(parser.parse_args().directory)
