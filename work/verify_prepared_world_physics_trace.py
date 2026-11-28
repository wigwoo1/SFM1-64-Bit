"""Verify typed prepared public trace conversion; no native geometry claim."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parent
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def closure():
    result={}
    def visit(p):
        p=p.resolve();key=p.relative_to(ROOT).as_posix()
        if key in result:return
        result[key]=sha(p)
        for include in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(encoding='utf-8-sig'),re.M):visit(p.parent/include)
    for name in ('probe_prepared_world_physics_trace.cpp','build_prepared_world_physics_trace.cmd','verify_prepared_world_physics_trace.py'):visit(ROOT/name)
    return result

def expected():
    patterns=(0,0x80000000,0x3f800000,0xbf800000,0x7f7fffff,0x00800000,1,0x7fc12345)
    output=[]
    for seed in range(8):
        for kind in range(5):
            for solid in range(4):
                for variant in range(4):
                    b=bytearray(92);values=[patterns[(seed+n)%8] for n in range(12)]
                    struct.pack_into('<10I',b,0,*values[:10]);b[40:44]=bytes((seed,0x80,kind,0xff-variant))
                    struct.pack_into('<IIHBBI',b,44,values[10],0xa5c30000|(seed<<8)|kind,0xf000+variant,solid&1,(solid>>1)&1,values[11])
                    name=0 if kind==0 and variant==0 else 0x1234567800000000|(kind<<32)|(variant*193+seed)
                    prop=0 if variant==0 else 0xffff if variant==1 else 0x8000+seed
                    flags=0xffff if variant==0 else 0 if variant==1 else 0x3333+kind
                    struct.pack_into('<QHH',b,60,name,prop,flags)
                    struct.pack_into('<H',b,78,0xffff if variant==3 else variant*256+seed)
                    output.append(bytes(b))
    assert len(output)==640
    return b''.join(output)

def main(directory):
    directory=directory.resolve();assert directory.is_relative_to(Path('D:/SFM64-development-tests').resolve())
    directory.mkdir(parents=True,exist_ok=True);frozen=closure();run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    result=subprocess.run(['cmd','/c',str(ROOT/'build_prepared_world_physics_trace.cmd'),str(run)],capture_output=True,text=True,timeout=180)
    (run/'build.local.log').write_text(result.stdout+result.stderr);assert result.returncode==0,result.stdout+result.stderr
    wanted=expected();builds={}
    for arch,machine,magic in (('x86',0x14c,0x10b),('x64',0x8664,0x20b)):
        log=(run/f'{arch}-build.log').read_text(errors='replace');assert not re.search(r'\bwarning\b|\berror\b',log,re.I),log
        exe=run/f'prepared_world_physics_trace_{arch}.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        assert raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic
        for repeat in (1,2):
            path=run/f'{arch}-{repeat}.bin';p=subprocess.run([str(exe),str(path)],capture_output=True,text=True,timeout=60)
            assert p.returncode==0,p.stdout+p.stderr
            assert json.loads(p.stdout)==dict(cases=640,high_names=608,default_names=32,resolver_exceptions=640,passed=True)
            assert path.read_bytes()==wanted,('typed bridge bytes differ',arch,repeat)
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
    assert closure()==frozen
    editor=json.loads((ROOT.parent/'publication/stage/docs/build-status.json').read_text());deps=editor['editor_source_dependencies']
    assert len(deps)==543 and all(sha(ROOT.parent/name)==value for name,value in deps.items())
    receipt=dict(status='VERIFIED TYPED PREPARED WORLD PHYSICS TRACE BRIDGE',cases_per_run=640,repeats_per_arch=2,
        builds=builds,source_sha256=frozen,record_bytes=len(wanted),record_sha256=hashlib.sha256(wanted).hexdigest(),editor_dependencies_unchanged=543,
        findings=['Every public field copies into the92-byte semantic wire independently on both architectures, including signed zero and quietNaN float bits.',
                  'Name resolver is called for every semantic kind including Default; zero or >32bit name identities are preserved independently of packed properties and worldSurfaceIndex.',
                  'Private displacement marker and optional source do not leak; unspecified pre-annotation hit fields remain zero. Resolver exceptions propagate.'],
        limits=['Typed conversion only. This does not execute native world geometry or world entity annotation, load materials, or verify editor integration.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'],receipt=str(run/'results.json'),builds=builds,record_sha256=receipt['record_sha256'])))

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);main(p.parse_args().directory)
