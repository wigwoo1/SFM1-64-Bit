"""Compare composed standard/pass/outer predicates against intact server code."""
import argparse
import hashlib
import json
import random
import re
import struct
import subprocess
import tempfile
from pathlib import Path
from verify_server_simple_trace_filter import PINS, sha
from verify_sliding_door_spawn import verify_installed_ranges

ROOT=Path(__file__).resolve().parent
def closure():
    import ast
    result={}
    def visit(p):
        p=p.resolve();key=p.relative_to(ROOT).as_posix()
        if key in result:return
        result[key]=sha(p);s=p.read_text(encoding='utf-8-sig')
        for name in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',s,re.M):visit(p.parent/name)
        if p.suffix=='.py':
            for node in ast.walk(ast.parse(s)):
                names=[x.name for x in node.names] if isinstance(node,ast.Import) else [node.module] if isinstance(node,ast.ImportFrom) and node.module else []
                for name in names:
                    p2=ROOT/(name.split('.')[0]+'.py')
                    if p2.is_file():visit(p2)
    for n in ('verify_server_composed_trace_filter.py','probe_server_composed_trace_filter.cpp','build_server_composed_trace_filter.cmd'):visit(ROOT/n)
    return result

def predict(q):
    f,mode,model,solid,move,first,second,mask,group,policy,owner,serial=q
    e=[]
    # Independent decision from the inputs. Record all externally observable
    # callbacks; direct native owner field/slot reads have no callback event.
    def result(answer,gate):return struct.pack('<26I',answer,len(e),*e,*([0]*(24-len(e)))),gate
    e.append(1)
    if not f&1:
        e.append(2)
        if f&2:
            e.extend((3,4,5,6))
            if model!=1 or solid not in (1,6):
                if not mask&0x2000000:return result(0,'standard_model')
            if not mask&2:
                e.extend((7,8))
                if first&255 and not second&255:return result(0,'standard_flags')
            if not mask&0x4000 and move==7:return result(0,'standard_move')
    if mode:
        if mode==1:return result(0,'same_handle')
        e.append(1)
        if not f&1:e.append(2)
        e.append(9)
        if not f&4:e.append(10)
        if f&2 and not f&1 and f&8 and not f&4 and serial==0 and owner:
            return result(0,'owner')
    e.append(1)
    if f&1:return result(0,'static')
    e.append(2)
    if not f&2:return result(0,'missing_base')
    e.append(11)
    if not policy&1:return result(0,'entity_rule')
    e.append(12)
    if not policy&2:return result(0,'group_rule')
    if f&16:
        e.append(13)
        if not policy&4:return result(0,'extra')
    return result(1,'accepted')

def main(directory):
    directory=directory.resolve();assert directory.is_relative_to(Path('D:/SFM64-development-tests').resolve())
    directory.mkdir(parents=True,exist_ok=True);assert all(sha(p)==v for p,v in PINS.items())
    frozen=closure();run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    rows=[]
    for flags in range(32):
        for mode in range(3):
            for policy in range(8):
                for owner in range(3):
                    for serial in range(3):
                        for mask in (0,2,0x4000,0x2000000,0x200400b,0xffffffff):
                            rows.append((flags,mode,1,1,0,0,0,mask,5,policy,owner,serial))
    rng=random.Random(20261128)
    for _ in range(12000):
        rows.append((rng.randrange(32),rng.randrange(3),rng.choice((0,1,2)),rng.choice((0,1,6)),rng.choice((0,7)),
                     rng.choice((0,1,0x100,0x101)),rng.choice((0,1,0x100,0x101)),rng.choice((0,2,0x2000000,0x4000,0x200400b)),
                     rng.choice((0,5,0xffffffff)),rng.randrange(8),rng.randrange(3),rng.randrange(3)))
    input_path=run/'input.bin';input_path.write_bytes(b''.join(struct.pack('<12I',*q) for q in rows))
    predictions=[predict(q) for q in rows];want=b''.join(p[0] for p in predictions)
    from collections import Counter
    coverage=dict(Counter(p[1] for p in predictions))
    assert len(coverage)==11,coverage
    build=subprocess.run(['cmd','/c',str(ROOT/'build_server_composed_trace_filter.cmd'),str(run)],capture_output=True,text=True,timeout=180)
    assert build.returncode==0,build.stdout
    server=next(p for p in PINS if p.name=='server.dll')
    _,spans=verify_installed_ranges(server,run/'reference.local.txt',[("outer",0x102677a0,0x10267852),("standard",0x102676b0,0x1026776d),("pass",0x10267610,0x102676a3),("owner",0x10010090,0x100100bc),("model getter",0x100b4aa0,0x100b4abd)])
    builds={}
    for arch in ('x86','x64'):
        log=(run/f'{arch}-build.log').read_text();assert not re.search(r'warning|error',log,re.I)
        exe=run/f'composed_{arch}.exe'
        raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        assert raw[pe:pe+4]==b'PE\0\0'
        assert struct.unpack_from('<H',raw,pe+4)[0]==(0x14c if arch=='x86' else 0x8664)
        assert struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if arch=='x86' else 0x20b)
        for repeat in range(2):
            out=run/f'{arch}-{repeat}.bin';p=subprocess.run([str(exe),str(server) if arch=='x86' else '-',str(input_path),str(out)],capture_output=True,timeout=120)
            assert p.returncode==0,p.stderr
            assert int(p.stdout)==len(rows)
            assert out.read_bytes()==want
        builds[arch]={'executable':str(exe),'sha256':sha(exe),'warnings':0,'machine':hex(0x14c if arch=='x86' else 0x8664)}
    assert closure()==frozen
    assert all(sha(p)==v for p,v in PINS.items())
    editor=json.loads((ROOT.parent/'publication/stage/docs/build-status.json').read_text())
    deps=editor['editor_source_dependencies']
    assert len(deps)==543 and all(sha(ROOT.parent/p)==v for p,v in deps.items())
    r={'status':'VERIFIED COMPOSED SERVER FILTER','builds':builds,'source_sha256':frozen,
       'cases':len(rows),'repeats':2,'record_sha256':hashlib.sha256(want).hexdigest()}
    (run/'results.json').write_text(json.dumps(r))
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('directory',type=Path);main(p.parse_args().directory)
