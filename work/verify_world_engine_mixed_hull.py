"""Intact outer TraceRay with prepared benchmark world and supplied entity clips.

Predictions begin with immutable earlier native public-world records. Candidate
traces are fixed synthetic services; this does not verify actual entity geometry.
"""
import argparse
import ast
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile

from verify_engine_hull_trace import prediction
from verify_world_engine_swept_hull import (
    ROOT, TEST_ROOT, WORLD_ID, selected_public_rows, component_guards,
    sha, f32, ENGINE, BSP, PINS, PERM_SHA, verify_installed_ranges)

ACCEPTED=TEST_ROOT/'20261128-world-engine-swept-hull/verify-o17qf620/results.json'
RAY_SHA='264b7c180bcf0d5c49a920eab837a6026b6de260a40ae73863893a5c89a0325f'


def closure():
    files={}
    def visit(path):
        path=path.resolve();name=path.relative_to(ROOT).as_posix()
        if name in files:return
        files[name]=sha(path);text=path.read_text(encoding='utf-8-sig')
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',text,re.M):visit(path.parent/inc)
        if path.suffix=='.py':
            for node in ast.walk(ast.parse(text)):
                modules=[a.name for a in node.names] if isinstance(node,ast.Import) else [node.module] if isinstance(node,ast.ImportFrom) and node.module else []
                for module in modules:
                    local=ROOT/(module.split('.')[0]+'.py')
                    if local.is_file():visit(local)
    for name in ('verify_world_engine_mixed_hull.py','probe_world_engine_mixed_hull.cpp','build_world_engine_mixed_hull.cmd'):visit(ROOT/name)
    return files


def guards():
    results=component_guards();old=json.loads(ACCEPTED.read_text())
    assert all(sha(ROOT/name)==value for name,value in old['source_sha256'].items())
    assert all(sha(build['executable'])==build['sha256'] for build in old['builds'].values())
    assert all(run['ray_sha256']==RAY_SHA for run in old['runs'])
    results.append(dict(receipt=str(ACCEPTED),receipt_sha256=sha(ACCEPTED),source_and_binary_hashes_unchanged=True))
    return results


def candidate(profile,index):
    """Independent fixture bytes, including fields omitted by the 88-byte view."""
    out=bytearray(92)
    struct.pack_into('<6f',out,0,101+index,201+index,301+index,401+index,501+index,601+index)
    struct.pack_into('<4f',out,24,.125*index,-.25*index,.5,700+index)
    out[40:44]=bytes((1+index,2+index,0x30+index,0x40+index))
    fraction=((.5,1.),(.125,1.,.25),(.75,.5,0.,0.))[profile][index-1]
    left=(.5,.25,1.,0.)[index-1] if profile==2 else 0.
    struct.pack_into('<fIHBBf',out,44,fraction,0x80000000|index,0x100+index,
                     int(profile==2 and index==3),int(profile==2 and index in (1,3)),left)
    struct.pack_into('<QIIHHQI',out,60,0x1111000000000000+10+index,
                     (0x500+index)|((0x600+index)<<16),0x7000+index,0x80+index,
                     0x20+index,0x4444000000000000+index,0x9000+index)
    is_static=index==1 if profile==0 else index==2
    return (0x40000000 if is_static else 0x40010000)+index,bytes(out)


def world_trace(original):
    out=bytearray(92);out[:60]=original[:60]
    name=struct.unpack_from('<I',original,60)[0]
    if name==0xffffffff:identity=0
    elif 0x10000000<=name<0x10000000+87:identity=0x1111000000000000+name-0x10000000
    else:
        assert 0x20000000<=name<0x20000000+43
        identity=0x2222000000000000
    struct.pack_into('<Q',out,60,identity);out[68:80]=original[64:76]
    has_hit=struct.unpack_from('<f',original,44)[0]<1. or bool(original[54]) or bool(original[55])
    struct.pack_into('<Q',out,80,WORLD_ID if has_hit else 0);out[88:92]=original[80:84]
    assert out[72:78]==bytes(6) and out[88:92]==bytes(4)
    return bytes(out)


def projected(trace,original):
    out=bytearray(88);out[:60]=trace[:60];out[64:68]=trace[68:72]
    out[74:76]=trace[78:80];out[84:88]=original[84:88]
    name=struct.unpack_from('<Q',trace,60)[0]
    if name==0:token=0xffffffff
    elif name==0x2222000000000000:token=struct.unpack_from('<I',original,60)[0]
    else:
        assert 0x1111000000000000<=name<0x1111000000010000
        token=0x10000000+name-0x1111000000000000
    struct.pack_into('<I',out,60,token)
    return bytes(out)


def expected(profile,rays):
    assert len(rays)==9632*80 and hashlib.sha256(rays).hexdigest()==RAY_SHA
    halves,footer=selected_public_rows();physics=[];events=[];enumerated=[];public=[];counts=Counter();outer=Counter();n=0
    candidates=[candidate(profile,i) for i in range(1,(2,3,4)[profile]+1)]
    for header,rows in halves:
        public.append(header)
        for original,query in rows:
            ray=rays[n*80:(n+1)*80];n+=1
            assert ray[68:70]==bytes((0,1)) and struct.unpack_from('<I',ray,64)[0]==0
            world=world_trace(original)
            framed,short=prediction(ray,0,1 if profile in (0,1) else 0,world,candidates,outer)
            payload=framed[4:];assert len(payload)==struct.unpack_from('<I',framed)[0]
            trace=payload[:92];ev=payload[116:]
            count=struct.unpack_from('<I',ev)[0];assert len(ev)==4+8*count
            physics.append(trace);events.append(ev);enumerated.append(short);public.append(projected(trace,original))
            entity=struct.unpack_from('<Q',trace,80)[0]
            if entity==WORLD_ID:counts['world_selected']+=1
            elif entity==0:counts['miss_selected']+=1
            else:
                assert 0x4444000000000001<=entity<=0x4444000000000004
                counts['entity_'+str(entity-0x4444000000000000)+'_selected']+=1
            counts['unavailable_worlds']+=not query['available'];counts['zero_mask']+=query['mask']==0
            counts['retained_small_delta_query']+=query['query']==4
            counts['early_world_startsolid']+=bool(world[55])
            counts['world_fraction_zero']+=struct.unpack_from('<f',world,44)[0]==0.
            counts['initial_world_hit']+=struct.unpack_from('<Q',world,80)[0]==WORLD_ID
    assert n==9632 and outer['early_returns']==1104 and outer['hull_start_exit_resets']==8528
    assert outer['static_predicate_bypass']>0 and outer['predicates']>0 and outer['clipped']>0
    if profile==0:assert outer['excluded_static_still_clips']>0 and counts['entity_1_selected']>0
    if profile==1:assert outer['filtered']>0 and counts['entity_3_selected']>0
    if profile==2:assert outer['preserved_exit_start']>0 and outer['allsolid_stops']>0 and counts['entity_3_selected']==8528
    public.append(footer)
    return dict(physics=b''.join(physics),events=b''.join(events),enumerated=b''.join(enumerated),public=b''.join(public),
                coverage=dict(counts),outer_coverage=dict(outer))


def compare(path,wanted,label):
    actual=path.read_bytes()
    if actual!=wanted:
        first=next((i for i,(a,b) in enumerate(zip(actual,wanted)) if a!=b),None)
        raise AssertionError((label,str(path),len(actual),len(wanted),first))
    return hashlib.sha256(actual).hexdigest()


def main(directory):
    directory=directory.resolve();assert directory.is_relative_to(TEST_ROOT.resolve())
    directory.mkdir(parents=True,exist_ok=True)
    assert all(sha(path)==value for path,value in PINS.items())
    frozen=closure();accepted=guards();run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    build=subprocess.run(['cmd','/c',str(ROOT/'build_world_engine_mixed_hull.cmd'),str(run)],capture_output=True,text=True,timeout=240)
    (run/'build.local.log').write_text(build.stdout+build.stderr);assert build.returncode==0,build.stdout+build.stderr
    image,evidence=verify_installed_ranges(ENGINE,run/'engine-reference.local.txt',[
        ('complete outer engine TraceRay',0x1016ece0,0x1016f0d3),('complete merge',0x1016ebd0,0x1016ece0),
        ('complete actual public world',0x10149eb0,0x1014a12c),('complete annotation',0x1016d840,0x1016d8b6),
        ('complete collector',0x1016d8f0,0x1016d923),('complete static classification',0x101c4700,0x101c472e)])
    raw=ENGINE.read_bytes();off=image.offset(0x104129b0);permutation=run/'permutation.private.bin'
    permutation.write_bytes(raw[off:off+1024]);assert sha(permutation)==PERM_SHA
    builds={};runs=[];predictions={}
    for arch,machine,magic in (('x86',0x14c,0x10b),('x64',0x8664,0x20b)):
        log=(run/f'{arch}-build.log').read_text(errors='replace');assert not re.search(r'\bwarning\b|\berror\b',log,re.I),log
        exe=run/f'world_engine_mixed_hull_{arch}.exe';binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0]
        assert binary[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',binary,pe+4)[0]==machine and struct.unpack_from('<H',binary,pe+24)[0]==magic
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
        for profile in range(3):
            for repeat in (1,2):
                output=run/f'{arch}-profile{profile}-{repeat}.bin';env=os.environ.copy();env['SFM64_MIXED_ENTITY_PROFILE']=str(profile)
                p=subprocess.run([str(exe),str(ENGINE) if arch=='x86' else '-',str(BSP),str(permutation),str(output)],capture_output=True,text=True,env=env,timeout=240)
                (run/f'{arch}-profile{profile}-{repeat}.local.log').write_text(p.stdout+p.stderr)
                assert p.returncode==0,(arch,profile,repeat,p.stdout,p.stderr,str(run))
                current=json.loads(p.stdout);assert current['cases']==9632 and current['finish_cases']==36
                counts=json.loads(Path(str(output)+'.engine-counts.json').read_text());assert counts==dict(owned=9632,native=9632 if arch=='x86' else 0)
                rays=Path(str(output)+'.rays').read_bytes()
                if profile not in predictions:predictions[profile]=expected(profile,rays)
                wanted=predictions[profile];assert hashlib.sha256(rays).hexdigest()==RAY_SHA
                hashes=dict(public=compare(output,wanted['public'],'legacy projection'),
                    physics=compare(Path(str(output)+'.owned-physics'),wanted['physics'],'owned full92 trace'),
                    events=compare(Path(str(output)+'.owned-events'),wanted['events'],'owned ordered callbacks'),
                    enumeration=compare(Path(str(output)+'.owned-enumerated-rays'),wanted['enumerated'],'owned full80 shortened ray'))
                if arch=='x86':
                    for suffix,key in (('physics','physics'),('events','events'),('enumerated-rays','enumerated')):
                        compare(Path(str(output)+'.native-'+suffix),wanted[key],'actual native '+suffix)
                else:
                    for suffix in ('physics','events','enumerated-rays'):assert Path(str(output)+'.native-'+suffix).stat().st_size==0
                runs.append(dict(architecture=arch,profile=profile,repeat=repeat,hashes=hashes,ray_sha256=RAY_SHA))
    assert closure()==frozen and guards()==accepted
    assert all(sha(path)==value for path,value in PINS.items())
    deps=json.loads((ROOT.parent/'publication/stage/docs/build-status.json').read_text())['editor_source_dependencies']
    assert len(deps)==543 and all(sha(ROOT.parent/name)==value for name,value in deps.items())
    receipt=dict(status='VERIFIED MIXED SUPPLIED ENTITY CLIPS WITH ACTUAL PREPARED WORLD',cases_per_profile=9632,profiles=3,
        cases_per_run=28896,finish_cases_per_profile=36,repeats_per_arch=2,builds=builds,runs=runs,
        coverage={str(profile):dict(geometry=p['coverage'],outer=p['outer_coverage']) for profile,p in predictions.items()},
        source_sha256=frozen,accepted_regressions=accepted,original_sha256={str(p):v for p,v in PINS.items()},
        originals_unchanged=True,editor_dependencies_unchanged=543,verified_spans=evidence,
        limits=['Mode0 Everything; swept nonray axis-aligned null-transform queries; prepared benchmark world only.',
                'Candidate enumeration, resolution, predicate and clip results are supplied services. Actual entity/static-prop geometry, native spatial partition, server filter composition, spawn, production lifetime and editor integration remain unverified.',
                'Nonray final start/exit reset hides intermediate preservation; the independent prior11300-case point/hull corpus separately verifies that policy. No original machine code is embedded in owned source.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'],cases=28896,receipt=str(run/'results.json'),coverage=receipt['coverage'])))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);main(parser.parse_args().directory)
