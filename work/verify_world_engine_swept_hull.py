"""Actual outer engine TraceRay plus prepared BSP world and server annotation.

The native engine/public world/annotation bodies execute unchanged. Predicted
outer outputs derive from the immutable earlier public world corpus, not from
the owned outer helper. Empty entity enumeration remains a supplied service.
"""
import argparse
from collections import Counter
import hashlib
import itertools
import json
from pathlib import Path
import struct
import subprocess
import tempfile

from pe_image import Image
from verify_server_simple_trace_filter import PINS
from verify_world_physics_bridge_corpora import accepted_guards, PERM_SHA
from verify_sliding_door_spawn import ENGINE, BSP, verify_installed_ranges

ROOT=Path(__file__).resolve().parent
TEST_ROOT=Path('D:/SFM64-development-tests')
BASELINE=TEST_ROOT/'20261127-stationary-hull-normal/swept-regression/verify-w7v2jz67/x86-1.bin'
BASELINE_SHA='35ca5376f7362f4bfe909a7636ec250fcab31596b1dfb284080ebb5b6e6c4c5d'
WORLD_ID=0x3333000000000001

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]

def closure():
    import ast,re
    files={}
    def visit(path):
        path=path.resolve();name=path.relative_to(ROOT).as_posix()
        if name in files:return
        files[name]=sha(path);text=path.read_text(encoding='utf-8-sig')
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',text,re.M):visit(path.parent/inc)
        if path.suffix=='.py':
            for node in ast.walk(ast.parse(text)):
                names=[a.name for a in node.names] if isinstance(node,ast.Import) else [node.module] if isinstance(node,ast.ImportFrom) and node.module else []
                for module in names:
                    local=ROOT/(module.split('.')[0]+'.py')
                    if local.exists():visit(local)
    for name in ('verify_world_engine_swept_hull.py','probe_world_engine_swept_hull.cpp','build_world_engine_swept_hull.cmd'):visit(ROOT/name)
    return files

def selected_public_rows():
    assert sha(BASELINE)==BASELINE_SHA
    data=BASELINE.read_bytes();block=16+9632*88;halves=[]
    for hdr in range(2):
        begin=hdr*block;header=data[begin:begin+16]
        assert struct.unpack('<4I',header)==(hdr,58 if hdr else 7,43,5312)
        case=0;selected=[]
        for mesh,q,mask,variant,compute,available in itertools.product(range(43),range(6),(0,1,0x2081,0xffffffff),range(4),(False,True),(True,False)):
            if not available and q!=0:continue
            row=data[begin+16+case*88:begin+16+(case+1)*88];case+=1
            if compute:selected.append((row,dict(mesh=mesh,query=q,mask=mask,variant=variant,available=available)))
        assert case==9632 and len(selected)==4816
        halves.append((header,selected))
    footer=data[2*block:];assert len(footer)==16+36*88
    return halves,footer

def predictions(rays):
    halves,footer=selected_public_rows();assert len(rays)==9632*80
    public=[];physics=[];counts=Counter();n=0
    for header,rows in halves:
        public.append(header)
        for original,query in rows:
            ray=rays[n*80:(n+1)*80];n+=1
            assert struct.unpack_from('<I',ray,64)[0]==0 and ray[68]==0 and ray[69]==1
            center=struct.unpack_from('<3f',ray,0);offset=struct.unpack_from('<3f',ray,32)
            extents=struct.unpack_from('<3f',ray,48)
            expected_offsets=((0.,0.,0.),(0.,0.,-41.),(-.125,-2.75,-7.0625),(-1.,-2.,-3.))
            expected_extents=((2.,24.,2.),(24.,24.,41.),(.125,2.75,7.0625),(3.,4.,5.))
            assert offset==expected_offsets[query['variant']] and extents==expected_extents[query['variant']]
            if query['query']==4:
                assert ray[16:28]==struct.pack('<3f',f32(-1e-7),0.,-0.)
            row=bytearray(original);fraction=struct.unpack_from('<f',row,44)[0]
            has_hit=fraction<1. or bool(row[54]) or bool(row[55])
            if not row[55]:
                row[:12]=struct.pack('<3f',*(f32(offset[a]+center[a]) for a in range(3)))
                row[56:60]=bytes(4);counts['outer_hull_start_exit_reset']+=1
            else:counts['world_start_solid_early_return']+=1
            # Native88-byte corpus projection keeps its pre-annotation entity
            # token zero; the complete annotation output is checked below.
            assert row[68:74]==bytes(6) and row[76:84]==bytes(8)
            public.append(bytes(row))
            out=bytearray(92);out[:60]=row[:60]
            name=struct.unpack_from('<I',row,60)[0]
            if name==0xffffffff:name_id=0;counts['default_name']+=1
            elif 0x10000000<=name<0x10000000+87:name_id=0x1111000000000000+name-0x10000000;counts['brush_name']+=1
            else:
                assert 0x20000000<=name<0x20000000+43
                name_id=0x2222000000000000;counts['shared_terrain_name']+=1
            struct.pack_into('<Q',out,60,name_id);out[68:80]=row[64:76]
            struct.pack_into('<Q',out,80,WORLD_ID if has_hit else 0)
            out[88:92]=row[80:84]
            physics.append(bytes(out));counts['annotation_hits' if has_hit else 'annotation_misses']+=1
            counts['unavailable_worlds']+=not query['available']
            counts['zero_mask']+=query['mask']==0
            counts['retained_small_delta_query']+=query['query']==4
    public.append(footer)
    assert n==9632 and counts['annotation_hits']>0 and counts['annotation_misses']>0
    assert counts['shared_terrain_name']>0 and counts['brush_name']>0
    return b''.join(public),b''.join(physics),dict(counts)

def component_guards():
    result=accepted_guards()
    paths=[TEST_ROOT/'20261128-server-trace-annotation/verify-d9yq3916/results.json',
           TEST_ROOT/'20261128-world-physics-bridge-corpora/verify-fb8sn_jo/results.json']
    for path in paths:
        old=json.loads(path.read_text())
        assert all(sha(ROOT/name)==value for name,value in old['source_sha256'].items())
        builds=old.get('builds',{})
        for build in builds.values():assert sha(build['executable'])==build['sha256']
        for comp in old.get('components',[]):
            for build in comp['builds'].values():assert sha(build['executable'])==build['sha256']
        result.append(dict(receipt=str(path),receipt_sha256=sha(path),source_and_binary_hashes_unchanged=True))
    return result

def main(directory):
    import re
    directory=directory.resolve();assert directory.is_relative_to(TEST_ROOT.resolve())
    directory.mkdir(parents=True,exist_ok=True);assert all(sha(p)==v for p,v in PINS.items())
    frozen=closure();guards=component_guards();run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    p=subprocess.run(['cmd','/c',str(ROOT/'build_world_engine_swept_hull.cmd'),str(run)],capture_output=True,text=True,timeout=240)
    (run/'build.local.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
    image=Image(ENGINE);at=image.offset(0x104129b0);table=image.b[at:at+1024]
    assert hashlib.sha256(table).hexdigest()==PERM_SHA
    permutation=run/'private-permutation.local.bin';permutation.write_bytes(table)
    _,spans=verify_installed_ranges(ENGINE,run/'reference.local.txt',[
        ('complete outer engine TraceRay',0x1016ece0,0x1016f0d3),('complete public world trace',0x10149eb0,0x1014a12c),
        ('complete server annotation',0x1016d840,0x1016d8b6),('complete merge',0x1016ebd0,0x1016ece0)])
    builds={};runs=[];expected=None;expected_physics=None;baseline_rays=None;coverage=None
    for arch,machine,magic in (('x86',0x14c,0x10b),('x64',0x8664,0x20b)):
        log=(run/f'{arch}-build.log').read_text(errors='replace');assert not re.search(r'\bwarning\b|\berror\b',log,re.I),log
        exe=run/f'world_engine_swept_hull_{arch}.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        assert raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
        for repeat in (1,2):
            output=run/f'{arch}-{repeat}.records'
            p=subprocess.run([str(exe),str(ENGINE) if arch=='x86' else '-',str(BSP),str(permutation),str(output)],capture_output=True,text=True,timeout=240)
            (run/f'{arch}-{repeat}.local.log').write_text(p.stdout+p.stderr);assert p.returncode==0,(arch,repeat,p.stdout,p.stderr)
            current=json.loads(p.stdout);assert current['cases']==9632 and current['finish_cases']==36
            counts=json.loads(Path(str(output)+'.engine-counts.json').read_text())
            assert counts==dict(owned=9632,native=9632 if arch=='x86' else 0)
            rays=Path(str(output)+'.rays').read_bytes()
            if baseline_rays is None:
                baseline_rays=rays;expected,expected_physics,coverage=predictions(rays)
            assert rays==baseline_rays,('provided full-ray corpus changed',arch,repeat)
            assert output.read_bytes()==expected,('projected outer trace differs from immutable public baseline +independent hull reset',arch,repeat)
            owned=Path(str(output)+'.owned-physics').read_bytes()
            assert owned==expected_physics,('owned full annotation/outer physics trace differs',arch,repeat)
            native_path=Path(str(output)+'.native-physics')
            if arch=='x86':assert native_path.read_bytes()==expected_physics,('native full annotation/outer physics trace differs',repeat)
            runs.append(dict(architecture=arch,repeat=repeat,public_sha256=sha(output),physics_sha256=hashlib.sha256(owned).hexdigest(),ray_sha256=hashlib.sha256(rays).hexdigest()))
    assert closure()==frozen and component_guards()==guards
    assert all(sha(p)==v for p,v in PINS.items())
    editor=json.loads((ROOT.parent/'publication/stage/docs/build-status.json').read_text());deps=editor['editor_source_dependencies']
    assert len(deps)==543 and all(sha(ROOT.parent/name)==value for name,value in deps.items())
    receipt=dict(status='VERIFIED OUTER ENGINE WITH PREPARED BENCHMARK HULL WORLD',cases_per_run=9632,finish_cases=36,repeats_per_arch=2,
        builds=builds,runs=runs,coverage=coverage,source_sha256=frozen,accepted_regressions=guards,
        original_sha256={str(p):v for p,v in PINS.items()},originals_unchanged=True,editor_dependencies_unchanged=543,
        baseline_public_record=str(BASELINE),baseline_public_sha256=BASELINE_SHA,
        projected_record_bytes=len(expected),projected_record_sha256=hashlib.sha256(expected).hexdigest(),
        physics_record_bytes=len(expected_physics),physics_record_sha256=hashlib.sha256(expected_physics).hexdigest(),
        verified_spans=spans,
        limits=['Prepared non-ray swept benchmark world, constant Everything filter and empty entity enumeration only.',
                'Geometry/work/cache/material loading and name/world-entity identities use documented fixtures. Production resource lifetime, actual entities/static props, filtering/clipping, real spawn and editor integration remain unverified.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'],cases=9632,coverage=coverage,receipt=str(run/'results.json'))))

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);main(p.parse_args().directory)
