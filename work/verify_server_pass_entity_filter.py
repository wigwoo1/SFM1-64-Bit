"""Compare ignored-entity and owner resolution through intact original calls."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
from verify_server_simple_trace_filter import PINS, SERVER, sha
from verify_sliding_door_spawn import verify_installed_ranges

ROOT=Path(__file__).resolve().parent
def closure():
    import ast
    files={}
    def visit(path):
        path=path.resolve();key=path.relative_to(ROOT).as_posix()
        if key in files:return
        files[key]=sha(path);s=path.read_text(encoding='utf-8-sig')
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',s,re.M):visit(path.parent/inc)
        if path.suffix=='.py':
            for node in ast.walk(ast.parse(s)):
                names=[x.name for x in node.names] if isinstance(node,ast.Import) else [node.module] if isinstance(node,ast.ImportFrom) and node.module else []
                for name in names:
                    p=ROOT/(name.split('.')[0]+'.py')
                    if p.exists():visit(p)
    for name in ('probe_server_pass_entity_filter.cpp','build_server_pass_entity_filter.cmd','verify_server_pass_entity_filter.py'):visit(ROOT/name)
    return files

def expected():
    rows=[];counts={x:0 for x in ('null','same','first_owner','second_owner','no_relationship','stale','sentinel','null_owner','aliased')}
    for flags in range(32):
        for mode in range(3):
            for a in range(7):
                for b in range(7):
                    for gen in (0,1,65534,65535):
                        for ix in range(2):
                            owners=[]
                            for n,state in enumerate((a,b)):
                                index=(65535,65534)[n] if ix else n
                                h=(gen<<16)|index
                                # Compare full serial; no truncation to16bits.
                                if state==0 or h==0xffffffff:
                                    owners.append(0);counts['sentinel']+=1
                                elif state in (4,5):owners.append(0);counts['stale']+=1
                                elif state==1 or state==6:owners.append(2-n)
                                elif state==2:owners.append(n+1)
                                elif state==3:owners.append(3)
                            events=[];answer=True
                            if mode==0:counts['null']+=1
                            elif mode==1:answer=False;counts['same']+=1
                            else:
                                events.append(1)
                                if not flags&1:events.append(2)
                                events.append(3)
                                if not flags&4:events.append(4)
                                base=1 if flags&2 else 0
                                other=1 if flags&16 else 2
                                if not flags&8:other=0
                                if flags&1:base=0
                                if flags&4:other=0
                                if base and other:
                                    events.append(5)
                                    if owners[0]==other:answer=False;counts['first_owner']+=1
                                    else:
                                        events.append(6 if other==2 else 5)
                                        answer=owners[other-1]!=base
                                        counts['second_owner']+=not answer
                                    counts['aliased']+=base==other
                                counts['no_relationship']+=answer
                            counts['null_owner']+=owners.count(0)
                            rows.append(struct.pack('<16I',flags,mode,a,b,gen,ix,int(answer),len(events),*(events+[0]*(6-len(events))),*owners))
    assert len(rows)==37632 and all(counts.values())
    return b''.join(rows),counts

def main(directory):
    directory=directory.resolve();assert directory.is_relative_to(Path('D:/SFM64-development-tests').resolve())
    directory.mkdir(parents=True,exist_ok=True);assert all(sha(p)==v for p,v in PINS.items())
    frozen=closure();run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    p=subprocess.run(['cmd','/c',str(ROOT/'build_server_pass_entity_filter.cmd'),str(run)],capture_output=True,text=True,timeout=180)
    (run/'build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stderr
    _,e=verify_installed_ranges(SERVER,run/'reference.local.txt',[("full pass",0x10267610,0x102676a3),("full owner",0x10010090,0x100100bc)])
    want,counts=expected();builds={}
    for arch,m in (('x86',0x14c),('x64',0x8664)):
        log=(run/f'{arch}-build.log').read_text();assert not re.search(r'warning|error',log,re.I)
        exe=run/f'pass_entity_{arch}.exe';b=exe.read_bytes();o=struct.unpack_from('<I',b,60)[0]
        assert b[o:o+4]==b'PE\0\0' and struct.unpack_from('<H',b,o+4)[0]==m and struct.unpack_from('<H',b,o+24)[0]==(0x10b if arch=='x86' else 0x20b)
        for n in (1,2):
            out=run/f'{arch}-{n}.bin';p=subprocess.run([str(exe),str(SERVER) if arch=='x86' else '-',str(out)],capture_output=True,text=True,timeout=120)
            assert p.returncode==0,p.stderr
            assert json.loads(p.stdout)==dict(cases=37632,passed=True)
            assert out.read_bytes()==want
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),warnings=0,machine=hex(m))
    assert closure()==frozen and all(sha(p)==v for p,v in PINS.items())
    ed=json.loads((ROOT.parent/'publication/stage/docs/build-status.json').read_text())
    assert len(ed['editor_source_dependencies'])==543 and all(sha(ROOT.parent/p)==v for p,v in ed['editor_source_dependencies'].items())
    r=dict(status='VERIFIED BOUNDED SERVER PASS ENTITY',builds=builds,record_sha256=hashlib.sha256(want).hexdigest(),source_sha256=frozen,
           cases_per_run=37632,repeats_per_arch=2,record_bytes=len(want),coverage=counts,
           original_sha256={str(p):v for p,v in PINS.items()},originals_unchanged=True,editor_dependencies_unchanged=543,
           verified_spans=e,supplied_services=['manager classification','base resolution'],
           limits=['Complete original owner getter executes against prepared slots; no production lifetime verified.'])
    (run/'results.json').write_text(json.dumps(r));print(r)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('directory',type=Path);main(p.parse_args().directory)
