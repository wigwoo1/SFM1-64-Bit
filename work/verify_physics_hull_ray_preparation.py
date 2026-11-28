"""Point/nonpoint preparation against unchanged actual loaded TraceBox.

This checks preparation math and observation transparency, not owned hull GJK.
"""
import argparse
import ast
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import random
import re
import shutil
import struct
import subprocess
import tempfile

from verify_world_engine_swept_hull import ROOT, TEST_ROOT, sha, f32, PINS, verify_installed_ranges
from verify_engine_collideable_clip_dispatch import guards as prior_guards

BASE=ROOT.parent/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
POINT=ROOT.parent/'outputs/physics-ray-preparation-20261015/verification-2wczayud/results.json'
DISPATCH=TEST_ROOT/'20261128-engine-collideable-clip-dispatch/verify-ru40876s/results.json'
LIBRARY_SHA={'tier0.dll':'6c05ae3d1f32e9f943b1ad30005f55ae69ddfad2f00f969b11d0c6d9c7356251',
             'vphysics.dll':'bd617003edcccba3db8dfef4be8fa6a44c962ed75100dab118d3d1e9fffb3333'}
PHY_SHA='161008f6096776bb226c3fa4937ab1a9a9b68a7a26125b6dd9ca3b24c33037d9'


def closure():
    files={}
    def visit(path):
        path=path.resolve();name=path.relative_to(ROOT).as_posix()
        if name in files:return
        files[name]=sha(path);text=path.read_text(encoding='utf-8-sig')
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',text,re.M):visit(path.parent/inc)
        if path.suffix=='.py':
            for node in ast.walk(ast.parse(text)):
                names=[a.name for a in node.names] if isinstance(node,ast.Import) else [node.module] if isinstance(node,ast.ImportFrom) and node.module else []
                for name in names:
                    local=ROOT/(name.split('.')[0]+'.py')
                    if local.is_file():visit(local)
    for name in ('verify_physics_hull_ray_preparation.py','probe_physics_hull_ray_preparation.cpp',
                 'build_physics_hull_ray_preparation.cmd','x64_fsin_cos.asm'):visit(ROOT/name)
    return files


def guards():
    result=prior_guards()
    for path in (POINT,DISPATCH):
        old=json.loads(path.read_text());assert old['source_sha256']
        assert all(sha(ROOT/name)==value for name,value in old['source_sha256'].items())
        for build in old.get('builds',{}).values():assert sha(build['executable'])==build['sha256']
        for build in old.get('executables',[]):assert sha(Path(build['path']))==build['sha256']
        result.append(dict(receipt=str(path),receipt_sha256=sha(path),source_and_binary_hashes_unchanged=True))
    return result


def raw_ray(center,delta,offset,extents,is_ray,swept,transform,index):
    ray=bytearray((13*index+17*j)&255 for j in range(80))
    for at,values in ((0,center),(16,delta),(32,offset),(48,extents)):struct.pack_into('<3f',ray,at,*values)
    struct.pack_into('<I',ray,64,transform);ray[68]=is_ray;ray[69]=swept
    return bytes(ray)


def corpus(original):
    assert len(original)%52==0
    seeds=[struct.unpack('<12fI',original[at:at+52]) for at in range(0,len(original),52)]
    original_count=len(seeds)
    def add(center=(0.,0.,0.),delta=(3.,4.,0.),origin=(0.,0.,0.),angles=(0.,0.,0.),mask=1):
        seeds.append((*center,*delta,*origin,*angles,mask))
    add(center=(-0.,0.,-0.),delta=(0.,-0.,0.),origin=(-0.,0.,0.),angles=(-0.,0.,-0.))
    for axis in range(3):
        for length in (-.001,-.0004,-.00039,-.0001,-1e-7,0.,1e-7,.0001,.00039,.0004,.001):
            delta=[0.,0.,-0.];delta[axis]=length;add(delta=delta)
    add(center=(1e7,-1e7,-0.),delta=(.5,-.25,-0.),origin=(1e7,-1e7,-0.),angles=(36000.,-36000.,0.))
    rng=random.Random(0x2b45e)
    for i in range(512):
        add(center=[f32(rng.uniform(-256.,256.)) for _ in range(3)],
            delta=[0.,-0.,0.] if i%13==0 else [f32(rng.uniform(-512.,512.)) for _ in range(3)],
            origin=[f32(rng.uniform(-128.,128.)) for _ in range(3)],
            angles=[0.,0.,0.] if i%4==0 else [0.,rng.choice((-180.,-90.,90.,180.,360.)),0.] if i%4==1 else [f32(rng.uniform(-36000.,36000.)) for _ in range(3)],
            mask=(0,1,2,0x4481,0xffffffff)[i%5])
    rows=[]
    for base,seed in enumerate(seeds):
        center,delta,origin,angles=seed[:3],seed[3:6],seed[6:9],seed[9:12];mask=seed[12]
        swept=int(((f32(delta[0]*delta[0]+delta[1]*delta[1]))+delta[2]*delta[2])!=0.)
        variants=[(1,(0.,0.,0.),(0.,0.,0.),swept,0),
                  (1,(24.,24.,41.),(-.125,-2.75,-7.0625),1-swept,0xffffffff),
                  (0,(0.,0.,0.),(0.,0.,0.),swept,0),
                  (0,(1.,1.,1.),(-1.,-2.,-3.),swept,0x12345678),
                  (0,(24.,24.,41.),(0.,0.,-41.),1-swept,0xffffffff),
                  (0,(.125,2.75,7.0625),(-.125,-2.75,-7.0625),swept,0),
                  (0,(-0.,0.,-0.),(-0.,0.,-0.),1-swept,0x77778888),
                  (0,(0.,2.,0.),(-3.,0.,1.),swept,0x12345678)]
        for variant,(is_ray,extents,offset,flag,transform) in enumerate(variants):
            ray=raw_ray(center,delta,offset,extents,is_ray,flag,transform,len(rows))
            rows.append((ray+struct.pack('<6fI',*origin,*angles,mask),base,variant))
    return rows,original_count


def box_expected(ray):
    if ray[68]:return struct.pack('<6fI',*([0.]*6),1),0.
    extents=struct.unpack_from('<3f',ray,48)
    squared=f32(f32(f32(extents[0]*extents[0])+f32(extents[1]*extents[1]))+f32(extents[2]*extents[2]))
    radius=f32(math.sqrt(squared))
    axes=[value for extent in extents for value in (extent,-extent)]
    return struct.pack('<6fI',*axes,0),radius


def independent_checks(prepared,traces,rows,original_count,old_prepared,old_traces):
    assert len(prepared)==len(rows)*196 and len(traces)==len(rows)*92
    counts=Counter();reference=None
    for index,(query,base,variant) in enumerate(rows):
        row=prepared[index*196:(index+1)*196];ray=query[:80]
        assert all(math.isfinite(value) for value in struct.unpack('<48f',row[:192]))
        axes,radius=box_expected(ray)
        assert row[168:]==axes,('independent box axes/point',index)
        assert row[124:128]==struct.pack('<f',radius),('native raw radius',index)
        other=row[:124]+row[128:168]
        if variant==0:reference=other
        else:assert other==reference,('offset/extents/point/swept/transform changed remaining preparation',index)
        trace=traces[index*92:(index+1)*92]
        center=struct.unpack_from('<3f',ray);offset=struct.unpack_from('<3f',ray,32);delta=struct.unpack_from('<3f',ray,16)
        start=[f32(offset[j]+center[j]) for j in range(3)];fraction=struct.unpack_from('<f',trace,44)[0]
        assert math.isfinite(fraction) and 0<=fraction<=1.
        assert trace[:24]==struct.pack('<6f',*start,*[f32(f32(delta[j]*fraction)+start[j]) for j in range(3)]),('native final positions',index)
        if base<original_count and variant==0:
            assert row[:168]==old_prepared[base*168:(base+1)*168],('accepted point preparation regression',base)
            assert trace[:60]==old_traces[base*60:(base+1)*60],('accepted original point trace regression',base)
            counts['accepted_point_baselines']+=1
        counts['forced_point_nonzero_extents']+=variant==1
        counts['hull_zero_extents']+=variant in (2,6)
        counts['nonzero_offsets']+=any(offset)
        counts['raw_nonzero_radius']+=radius>0
        counts['original_native_fraction_hits']+=fraction<1
        counts['original_native_startsolid']+=bool(trace[55])
        counts['point' if ray[68] else 'hull']+=1
    assert counts['accepted_point_baselines']==original_count
    assert all(counts[name]>0 for name in ('raw_nonzero_radius','original_native_fraction_hits','original_native_startsolid','forced_point_nonzero_extents','hull_zero_extents'))
    return dict(counts)


def main(directory):
    directory=directory.resolve();assert directory.is_relative_to(TEST_ROOT.resolve());directory.mkdir(parents=True,exist_ok=True)
    assert all(sha(path)==value for path,value in PINS.items())
    frozen=closure();accepted=guards();run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    old=json.loads(BASE.read_text());point=json.loads(POINT.read_text())
    assert old['status']==point['status']=='PASS' and point['cases']==15606
    model=next(model for model in old['models'] if model['model']=='models/props_granary/grain_sack.mdl')
    phy=Path(model['phy']);assert sha(phy)==model['phy_sha256']==PHY_SHA
    original_path=Path(model['path'])/'queries.bin';original_trace=Path(model['path'])/'native-traces.bin'
    assert sha(original_path)==model['query_sha256'] and sha(original_trace)==model['trace_sha256']
    group=next(group for group in point['groups'] if group['name']==model['model'])
    point_prepared=Path(group['path'])/'preparation_native_x86.bin';assert sha(point_prepared)==group['preparation_sha256']
    rows,original_count=corpus(original_path.read_bytes());assert original_count==121
    inputs=run/'queries.local.bin';inputs.write_bytes(b''.join(query for query,_,_ in rows))
    libraries=run/'libraries';libraries.mkdir();originals={str(phy):PHY_SHA}
    for library in old['libraries']:
        source=Path(library['source']);assert sha(source)==library['sha256']==LIBRARY_SHA[source.name.lower()]
        shutil.copyfile(source,libraries/source.name);assert sha(libraries/source.name)==library['sha256'];originals[str(source)]=library['sha256']
    build=subprocess.run(['cmd','/c',str(ROOT/'build_physics_hull_ray_preparation.cmd'),str(run)],capture_output=True,text=True,timeout=240)
    (run/'build.local.log').write_text(build.stdout+build.stderr);assert build.returncode==0,build.stdout+build.stderr
    physics=Path(next(library['source'] for library in old['libraries'] if Path(library['source']).name.lower()=='vphysics.dll'))
    _,evidence=verify_installed_ranges(physics,run/'physics-reference.local.txt',[
        ('complete actual TraceBox caller',0x1002b260,0x1002b561),('complete ray prepare',0x10027400,0x10027571),
        ('complete box radius getter',0x10027260,0x10027264),('complete collector constructor',0x100295f0,0x100296a6)])
    assembly_log=(run/'asm-build.log').read_text(errors='replace')
    assert not re.search(r'\bwarning\b|\berror\b',assembly_log,re.I),assembly_log
    builds={};runs=[];first_prepared=None;first_traces=None;coverage=None
    for arch,machine,magic in (('x86',0x14c,0x10b),('x64',0x8664,0x20b)):
        log=(run/f'{arch}-build.log').read_text(errors='replace');assert not re.search(r'\bwarning\b|\berror\b',log,re.I),log
        exe=run/f'hull_ray_preparation_{arch}.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        assert raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
        for repeat in (1,2):
            output=run/f'{arch}-{repeat}.bin'
            p=subprocess.run([str(exe),str(libraries),str(phy),str(inputs),str(output)],capture_output=True,text=True,timeout=180)
            (run/f'{arch}-{repeat}.local.log').write_text(p.stdout+p.stderr);assert p.returncode==0,(arch,repeat,p.stdout,p.stderr,str(run))
            prepared=output.read_bytes();assert len(prepared)==len(rows)*196
            if arch=='x86':
                observed=Path(str(output)+'.observed-traces').read_bytes();unobserved=Path(str(output)+'.unobserved-traces').read_bytes()
                assert observed==unobserved,('hardware observation altered full native trace',repeat)
                if first_traces is None:first_traces=observed
                assert observed==first_traces
                coverage=independent_checks(prepared,observed,rows,original_count,point_prepared.read_bytes(),original_trace.read_bytes())
            if first_prepared is None:first_prepared=prepared
            if prepared!=first_prepared:
                first=next((i for i,(a,b) in enumerate(zip(prepared,first_prepared)) if a!=b),None)
                raise AssertionError(('all196preparation bytes differ',arch,repeat,first,None if first is None else first//196,str(run)))
            runs.append(dict(architecture=arch,repeat=repeat,preparation_sha256=sha(output),native_full_trace_sha256=hashlib.sha256(first_traces).hexdigest() if arch=='x86' else None))
    assert closure()==frozen and guards()==accepted
    assert all(sha(Path(path))==value for path,value in originals.items()) and all(sha(path)==value for path,value in PINS.items())
    for library in old['libraries']:assert sha(libraries/Path(library['source']).name)==library['sha256']
    assert sha(inputs)==hashlib.sha256(b''.join(query for query,_,_ in rows)).hexdigest()
    deps=json.loads((ROOT.parent/'publication/stage/docs/build-status.json').read_text())['editor_source_dependencies']
    assert len(deps)==543 and all(sha(ROOT.parent/name)==value for name,value in deps.items())
    receipt=dict(status='VERIFIED POINT/NONPOINT PHYSICS PREPARATION',cases_per_run=len(rows),repeats_per_arch=2,
        builds=builds,runs=runs,coverage=coverage,source_sha256=frozen,accepted_regressions=accepted,
        preparation_bytes_per_result=196,preparation_sha256=hashlib.sha256(first_prepared).hexdigest(),
        native_full_trace_sha256=hashlib.sha256(first_traces).hexdigest(),input_sha256=sha(inputs),
        original_sha256={**{str(path):value for path,value in PINS.items()},**originals},originals_unchanged=True,
        editor_dependencies_unchanged=543,verified_spans=evidence,
        old_point_preparation_receipt=str(POINT),old_point_preparation_receipt_sha256=sha(POINT),
        limits=['Owned result is preparation math only; original full native collision runs for observation transparency, not owned hull search verification.',
                'One pinned one-solid grain_sack PHY, supplied axis-aligned Ray_t extents/isRay/swept/offset/padding/opaque transform values and finite angles within36000degrees. No transform pointer is dereferenced.',
                'Original ray/motion/collector/preparation/tree/search functions run unchanged; hardware postprepare observation resumes the original instruction without software patches. Model loading uses disposable copies of pinned libraries.',
                'Actual owned nonpoint support/search/tree/finalization, real entity/static-prop geometry, production bindings, spawn and editor integration remain pending.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'],cases=len(rows),receipt=str(run/'results.json'),coverage=coverage)))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);main(parser.parse_args().directory)
