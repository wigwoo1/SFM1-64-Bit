"""Owned complete PHY point/nonpoint traces versus unchanged native TraceBox.

The native oracle directly executes the loaded library with no observers or
supplied leaf results. Default contents1 and fresh scratch are the bounded gate.
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

from verify_world_engine_swept_hull import ROOT, TEST_ROOT, sha, f32, ENGINE, PINS, verify_installed_ranges
from verify_physics_hull_ray_preparation import guards as prior_guards, BASE, LIBRARY_SHA, raw_ray

PREPARATION=TEST_ROOT/'20261128-physics-hull-ray-preparation/verify-2sshlo97/results.json'
POINT=ROOT.parent/'outputs/phy-point-trace-20261016/verification-xfpsrv2x/results.json'


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
    for name in ('verify_phy_hull_trace.py','probe_phy_hull_trace.cpp','build_phy_hull_trace.cmd',
                 'source_phy_point_trace_candidate.h','x64_fsin_cos.asm'):visit(ROOT/name)
    return files


def guards():
    result=prior_guards();old=json.loads(PREPARATION.read_text())
    assert all(sha(ROOT/name)==value for name,value in old['source_sha256'].items())
    assert all(sha(build['executable'])==build['sha256'] for build in old['builds'].values())
    result.append(dict(receipt=str(PREPARATION),receipt_sha256=sha(PREPARATION),source_and_binary_hashes_unchanged=True))
    # This old native result is a fixed regression oracle. Its earlier owned
    # point header predates the separately accepted contents-callback overload;
    # preserve current point source through closure(), rather than rewriting it.
    old=json.loads(POINT.read_text())
    assert all(sha(Path(exe['path']))==exe['sha256'] for exe in old['executables'])
    result.append(dict(receipt=str(POINT),receipt_sha256=sha(POINT),native_point_baselines_and_binaries_preserved=True))
    return result


def corpus(model,index):
    source=Path(model['path'])/'queries.bin';assert sha(source)==model['query_sha256']
    original=source.read_bytes();assert len(original)==model['native_metadata']['cases']*52
    seeds=[struct.unpack('<12fI',original[at:at+52]) for at in range(0,len(original),52)]
    base_count=len(seeds);mdl=Path(model['mdl']);assert sha(mdl)==model['mdl_sha256']
    raw=mdl.read_bytes();lo=struct.unpack_from('<3f',raw,104);hi=struct.unpack_from('<3f',raw,116)
    center=[(a+b)*.5 for a,b in zip(lo,hi)];radius=max(64.,max(b-a for a,b in zip(lo,hi))*2)
    assert all(math.isfinite(x) for x in (*lo,*hi)) and radius<1e5
    rng=random.Random(0x2b260+index)
    for case in range(32):
        origin=[f32(rng.uniform(-512.,512.)) for _ in range(3)]
        angles=[0.,0.,0.] if case%4==0 else [0.,rng.choice((-180.,-90.,90.,180.,360.)),0.] if case%4==1 else [f32(rng.uniform(-179.,179.)) for _ in range(3)]
        start=[f32(origin[j]+center[j]+rng.uniform(-radius,radius)) for j in range(3)]
        delta=[0.,-0.,0.] if case%7==0 else [f32(rng.uniform(-2*radius,2*radius)) for _ in range(3)]
        if case%11==0:delta=[1e-7,-1e-7,-0.]
        seeds.append((*start,*delta,*origin,*angles,(0,1,2,0x4481,0xffffffff)[case%5]))
    seeds.append((1e6,1e6,1e6,0.,0.,0.,0.,0.,0.,15.,37.,-11.,1))
    rows=[];labels=[];analytic={};point_indices=[]
    for base,seed in enumerate(seeds):
        center,delta,origin,angles=seed[:3],seed[3:6],seed[6:9],seed[9:12];mask=seed[12]
        swept=int(f32(f32(delta[0]*delta[0]+delta[1]*delta[1])+delta[2]*delta[2])!=0.)
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
            rows.append(ray+struct.pack('<6fI',*origin,*angles,mask));labels.append((base,variant))
            if base<base_count and variant==0:point_indices.append(len(rows)-1)
            if not(mask&1) or base==len(seeds)-1:
                trace=bytearray(92);start=[f32(offset[j]+center[j]) for j in range(3)]
                struct.pack_into('<6f',trace,0,*start,*[f32(delta[j]+start[j]) for j in range(3)])
                struct.pack_into('<f',trace,44,1.);analytic[len(rows)-1]=bytes(trace)
    return rows,labels,analytic,point_indices


def comparison(actual,wanted,labels,model,run,architecture,repeat):
    if actual!=wanted:
        at=next((i for i,(a,b) in enumerate(zip(actual,wanted)) if a!=b),None)
        query=None if at is None else at//92
        detail=dict(model=model,architecture=architecture,repeat=repeat,actual_bytes=len(actual),expected_bytes=len(wanted),
                    first_byte=at,query=query,field=None if at is None else at%92,label=None if query is None else labels[query])
        (run/'difference.local.json').write_text(json.dumps(detail,indent=2)+'\n')
        raise AssertionError(('full92trace mismatch',detail,str(run/'difference.local.json')))


def main(directory):
    directory=directory.resolve();assert directory.is_relative_to(TEST_ROOT.resolve());directory.mkdir(parents=True,exist_ok=True)
    assert all(sha(path)==value for path,value in PINS.items())
    frozen=closure();accepted=guards();run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    old=json.loads(BASE.read_text());point=json.loads(POINT.read_text())
    assert old['status']==point['status']=='PASS' and len(old['models'])==61
    libraries=run/'libraries';libraries.mkdir();originals={}
    for library in old['libraries']:
        source=Path(library['source']);assert sha(source)==library['sha256']==LIBRARY_SHA[source.name.lower()]
        shutil.copyfile(source,libraries/source.name);assert sha(libraries/source.name)==library['sha256'];originals[str(source)]=library['sha256']
    for model in old['models']:
        for kind in ('phy','mdl'):
            path=Path(model[kind]);assert sha(path)==model[kind+'_sha256'];originals[str(path)]=model[kind+'_sha256']
    build=subprocess.run(['cmd','/c',str(ROOT/'build_phy_hull_trace.cmd'),str(run)],capture_output=True,text=True,timeout=240)
    (run/'build.local.log').write_text(build.stdout+build.stderr);assert build.returncode==0,build.stdout+build.stderr
    builds={}
    for arch,machine,magic in (('x86',0x14c,0x10b),('x64',0x8664,0x20b)):
        log=(run/f'{arch}-build.log').read_text(errors='replace');assert not re.search(r'\bwarning\b|\berror\b',log,re.I),log
        exe=run/f'phy_hull_trace_{arch}.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        assert raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
    assembly=(run/'asm-build.log').read_text(errors='replace');assert not re.search(r'\bwarning\b|\berror\b',assembly,re.I),assembly
    models=[];totals=Counter()
    for index,model in enumerate(old['models']):
        case=run/f'{index:03d}';case.mkdir();rows,labels,analytic,point_indices=corpus(model,index)
        queries=case/'queries.local.bin';queries.write_bytes(b''.join(rows));native=None;runs=[]
        point_model=next(row for row in point['models'] if row['model']==model['model'])
        old92=Path(point_model['path'])/'point_trace_native_x86-fresh.bin';assert sha(old92)==point_model['native_semantic_trace_sha256']
        old60=Path(model['path'])/'native-traces.bin';assert sha(old60)==model['trace_sha256']
        old92_bytes=old92.read_bytes();old60_bytes=old60.read_bytes()
        for arch in ('x86','x64'):
            for repeat in (1,2):
                output=case/f'{arch}-{repeat}.bin';exe=builds[arch]['executable']
                p=subprocess.run([exe,str(libraries),model['phy'],str(queries),str(output)],capture_output=True,text=True,timeout=120)
                (case/f'{arch}-{repeat}.local.log').write_text(p.stdout+p.stderr);assert p.returncode==0,(index,model['model'],arch,repeat,p.stdout,p.stderr,str(case))
                trace=output.read_bytes();assert len(trace)==len(rows)*92
                assert Path(str(output)+'.trace60').read_bytes()==b''.join(trace[i:i+60] for i in range(0,len(trace),92))
                if native is None:
                    native=trace
                    for query,wanted in analytic.items():assert trace[query*92:(query+1)*92]==wanted,('independent filtered/outside miss',model['model'],query)
                    for old_index,query in enumerate(point_indices):
                        assert trace[query*92:(query+1)*92]==old92_bytes[old_index*92:(old_index+1)*92],('accepted native point92regression',model['model'],query)
                        assert trace[query*92:query*92+60]==old60_bytes[old_index*60:(old_index+1)*60]
                comparison(trace,native,labels,model['model'],case,arch,repeat)
                runs.append(dict(architecture=arch,repeat=repeat,record_sha256=sha(output)))
        counts=Counter(cases=len(rows),accepted_point_baselines=len(point_indices),independent_analytic_misses=len(analytic))
        for query,(base,variant) in enumerate(labels):
            trace=native[query*92:(query+1)*92];counts['point' if rows[query][68] else 'hull']+=1
            counts['fraction_hits']+=struct.unpack_from('<f',trace,44)[0]<1.;counts['startsolid']+=bool(trace[55]);counts['allsolid']+=bool(trace[54])
            counts['forced_point_nonzero_extents']+=variant==1;counts['hull_zero_extents']+=variant in (2,6)
        totals.update(counts);models.append(dict(model=model['model'],path=str(case),coverage=dict(counts),runs=runs,input_sha256=sha(queries)))
        print(json.dumps(dict(model=index+1,cases=len(rows),status='PASS')),flush=True)
    assert totals['accepted_point_baselines']==7381 and totals['point']>0 and totals['hull']>0 and totals['fraction_hits']>0 and totals['startsolid']>0
    assert closure()==frozen and guards()==accepted
    assert all(sha(Path(path))==value for path,value in originals.items()) and all(sha(path)==value for path,value in PINS.items())
    for library in old['libraries']:assert sha(libraries/Path(library['source']).name)==library['sha256']
    deps=json.loads((ROOT.parent/'publication/stage/docs/build-status.json').read_text())['editor_source_dependencies']
    assert len(deps)==543 and all(sha(ROOT.parent/name)==value for name,value in deps.items())
    physics=Path(next(library['source'] for library in old['libraries'] if Path(library['source']).name.lower()=='vphysics.dll'))
    _,evidence=verify_installed_ranges(physics,run/'physics-reference.local.txt',[('complete actual TraceBox',0x1002b260,0x1002b561),('complete leaf search/hit',0x1002a340,0x1002a4e0)])
    receipt=dict(status='VERIFIED COMPLETE OWNED POINT/NONPOINT PHY TRACES',models=models,coverage=dict(totals),
        repeats_per_arch=2,builds=builds,source_sha256=frozen,accepted_regressions=accepted,
        original_sha256={**{str(path):value for path,value in PINS.items()},**originals},originals_unchanged=True,
        editor_dependencies_unchanged=543,verified_spans=evidence,
        point_baseline_receipt=str(POINT),point_baseline_receipt_sha256=sha(POINT),
        limits=['61pinned supported loose one-solid VPHY0100/type0/IVPS models; default per-convex contents1, fresh scratch only.',
                'Native complete TraceBox executes directly with original loaded libraries and no observers, patches or supplied leaf results. Owned parses actual PHY and computes geometry traversal/support/search/hits/finalization.',
                'Finite axis-aligned extent/offset inputs and finite original angles, explicit point/swept bits. Ray transform tokens are ignored by this observed TraceBox overload; actual transformed box support remains separate.',
                'Persistent scratch progression/wrap, custom contents composition, missing/archive/multisolid assets, engine dispatch leaf integration, real entity/registry/lifetime bindings, benchmark spawn and editor integration remain unverified.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'],coverage=dict(totals),receipt=str(run/'results.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);main(parser.parse_args().directory)
