"""Independent policy verification for intact ClipRayToCollideable dispatch.

Geometry leaves return supplied traces. This verifies dispatch and metadata
policy, not the physical correctness of those supplied geometry answers.
"""
import argparse
import ast
from collections import Counter
import hashlib
import itertools
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile

from verify_world_engine_swept_hull import ROOT, TEST_ROOT, sha, f32, ENGINE, PINS, verify_installed_ranges
from verify_world_engine_mixed_hull import guards as prior_guards


def frame(value):return struct.pack('<I',len(value))+value


def hit(trace):return struct.unpack_from('<f',trace,44)[0]<1. or bool(trace[54]) or bool(trace[55])


def initialized(ray):
    trace=bytearray(92);center=struct.unpack_from('<3f',ray);delta=struct.unpack_from('<3f',ray,16)
    offset=struct.unpack_from('<3f',ray,32)
    start=[f32(offset[j]+center[j]) for j in range(3)]
    end=[f32(delta[j]+start[j]) for j in range(3)]
    struct.pack_into('<6f',trace,0,*start,*end);struct.pack_into('<f',trace,44,1.)
    return trace


def expected(query,coverage):
    """Branch/field prediction based only on explicit input fixture values."""
    ray,model,contents,property_id,solid,first_flags,later_flags,root,mask,answers,annotation,annotation_box=query
    trace=initialized(ray);current=bytearray(ray);events=[]
    coverage['cases']+=1
    coverage['points' if ray[68] else 'hulls']+=1
    def event(code):events.append((code,bytes(current)))
    # Event numbering follows the host boundary protocol, not owned helper code.
    event(1)
    studio=model==3
    if studio:
        event(2)
        if not(mask&0x40000000) and not(contents&mask):
            coverage['model_contents_early_return']+=1
            return trace,events
    event(3)
    if first_flags&256:
        event(4);struct.pack_into('<I',current,64,root);coverage['root_override']+=1
    event(5)
    if solid==5:custom=True;coverage['solid5_custom']+=1
    else:
        event(3)
        custom=bool(later_flags&(1 if ray[68] else 2))
        coverage['point_flag_custom' if ray[68] else 'hull_flag_custom']+=custom
    def leaf(index):
        nonlocal trace
        event(6+index);ok,value=answers[index];trace=bytearray(value)
        coverage['leaf_'+str(index)]+=1
        coverage['false_leaf_writes']+=not ok
        return ok
    if custom:
        ignored=leaf(0);handled=True
        coverage['custom_ignored_false_return']+=not ignored
    else:
        handled=leaf(1)
        if not handled:
            if model==1:handled=leaf(2)
            if not handled:handled=leaf(3)
    hitbox=False
    if studio and mask&0x40000000 and not custom:
        hitbox=leaf(4);handled=hitbox
        coverage['hitbox_handled' if hitbox else 'hitbox_declined']+=1
    if not handled:leaf(5)
    if studio and not hitbox and hit(trace):
        if not custom or struct.unpack_from('<H',trace,68)[0]==0:
            struct.pack_into('<I',trace,48,contents)
            struct.pack_into('<Q',trace,60,0xffffffffffffffff)
            struct.pack_into('<I',trace,68,property_id)
            coverage['studio_metadata_stamp']+=1
        else:coverage['custom_keeps_nonzero_property']+=1
    if struct.unpack_from('<Q',trace,80)[0]==0 and hit(trace):
        event(12);struct.pack_into('<Q',trace,80,annotation);struct.pack_into('<I',trace,88,annotation_box);coverage['annotation']+=1
    else:coverage['annotation_skipped']+=1
    return trace,events


def record(trace,events):
    return frame(bytes(trace)+struct.pack('<I',len(events))+b''.join(struct.pack('<I',code)+ray for code,ray in events))


def ray_bytes(index,is_ray):
    # Padding, opaque transforms and supplied isRay/swept bits all survive.
    raw=bytearray((index*13+j*17)&255 for j in range(80))
    options=[((1e7,-1e7,-0.),(.5,-.25,-0.),(-1e7,1e7,0.),(24.,24.,41.)),
             ((-0.,-0.,0.),(-0.,2.,-3.),(-0.,0.,-0.),(0.,0.,0.)),
             ((32.,-10.,100.),(1e-24,0.,-0.),(-1.,2.,-41.),(.000732421875,.000244140625,0.)),
             ((.03125,-.03125,2.),(.125,.25,-.5),(-.0625,.0625,-2.),(.25,.5,1.))]
    for offset,values in zip((0,16,32,48),options[index%len(options)]):struct.pack_into('<3f',raw,offset,*values)
    struct.pack_into('<I',raw,64,(0,0x12345678,0xffffffff)[index%3])
    raw[68]=is_ray;raw[69]=(index//2)%2
    return bytes(raw)


def leaf_trace(case,index):
    out=bytearray(92);tag=case*6+index+1
    struct.pack_into('<6f',out,0,101+index,201+index,301+index,401+index,501+index,601+index)
    struct.pack_into('<4f',out,24,.125*(index+1),-.25*(index+1),.5,700+index)
    out[40:44]=bytes((index+1,index+2,0x30+index,0x40+index))
    struct.pack_into('<fIHBBf',out,44,(0.,.25,1.,1.25)[(case+index)%4],0x80000000|tag,
                     0x100+index,int((case+index)%7==0),int((case+index)%5==0),(.0,.5,1.)[tag%3])
    surface=(0,0xffffffffffffffff,0x1111000000000000+tag)[tag%3]
    metadata=((0x600+index)<<16)|((0,1,32768,65535)[(case+index)%4])
    entity=0 if (case+index)%3 else 0x4444000000000000+tag
    struct.pack_into('<QIIHHQI',out,60,surface,metadata,0x7000+index,0x80+index,0x20+index,entity,0x9000+index)
    return bytes(out)


def corpus():
    patterns=(0,1,2,3,4,6,8,10,16,18,32,63)
    for case,(model,solid,is_ray,first,later,mask,contents,returns) in enumerate(itertools.product(
            (0xffffffff,0,1,3,7),(0,3,5,6),(0,1),(0,1,256,258),(0,1,2,3),
            (0,1,0x40000000,0x40000001),(0,1),patterns)):
        ray=ray_bytes(case,is_ray);property_id=(0,1,32768,65535)[case%4]
        root=(0,0x77778888,0xffffffff)[(case//3)%3]
        answers=[(bool(returns&(1<<index)),leaf_trace(case,index)) for index in range(6)]
        annotation=0 if case%5==0 else 0x5555000000000000+case
        yield (ray,model,contents,property_id,solid,first,later,root,mask,answers,annotation,0xa0000000+case)


def encode(query):
    ray,model,contents,property_id,solid,first,later,root,mask,answers,annotation,box=query
    return ray+struct.pack('<8IQI',model,solid,first,later,contents,property_id,root,mask,annotation,box)+b''.join(
        struct.pack('<I',returned)+trace for returned,trace in answers)


def closure():
    import ast
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
    for name in ('verify_engine_collideable_clip_dispatch.py','probe_engine_collideable_clip_dispatch.cpp','build_engine_collideable_clip_dispatch.cmd'):visit(ROOT/name)
    return files


def guards():
    result=prior_guards()
    path=TEST_ROOT/'20261128-world-engine-mixed-hull/verify-4p451lcw/results.json';old=json.loads(path.read_text())
    assert all(sha(ROOT/name)==value for name,value in old['source_sha256'].items())
    assert all(sha(build['executable'])==build['sha256'] for build in old['builds'].values())
    result.append(dict(receipt=str(path),receipt_sha256=sha(path),source_and_binary_hashes_unchanged=True))
    return result


def check(actual,wanted,arch,repeat,run):
    if actual==wanted:return
    first=next((i for i,(a,b) in enumerate(zip(actual,wanted)) if a!=b),None)
    case=0;at=0
    while first is not None and at+4<=len(wanted):
        size=struct.unpack_from('<I',wanted,at)[0]+4
        if at+size>first:break
        at+=size;case+=1
    raise AssertionError(('complete trace/event/ray frame mismatch',arch,repeat,len(actual),len(wanted),first,case,str(run)))


def main(directory):
    directory=directory.resolve();assert directory.is_relative_to(TEST_ROOT.resolve())
    directory.mkdir(parents=True,exist_ok=True)
    assert all(sha(path)==value for path,value in PINS.items())
    frozen=closure();accepted=guards();run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    counts=Counter();rows=list(corpus());assert len(rows)==61440
    inputs=run/'input.local.bin';inputs.write_bytes(b''.join(map(encode,rows)))
    wanted=b''.join(record(*expected(query,counts)) for query in rows)
    assert all(counts[key]>0 for key in ('model_contents_early_return','root_override','solid5_custom','point_flag_custom',
        'hull_flag_custom','false_leaf_writes','custom_ignored_false_return','hitbox_handled','hitbox_declined',
        'studio_metadata_stamp','custom_keeps_nonzero_property','annotation','annotation_skipped','points','hulls',
        *(f'leaf_{index}' for index in range(6)))),counts
    build=subprocess.run(['cmd','/c',str(ROOT/'build_engine_collideable_clip_dispatch.cmd'),str(run)],capture_output=True,text=True,timeout=240)
    (run/'build.local.log').write_text(build.stdout+build.stderr);assert build.returncode==0,build.stdout+build.stderr
    _,evidence=verify_installed_ranges(ENGINE,run/'engine-reference.local.txt',[
        ('complete ClipRayToCollideable dispatch',0x1016f300,0x1016f54c),
        ('complete default trace initializer',0x10145f40,0x10145f7d)])
    builds={};runs=[]
    for arch,machine,magic in (('x86',0x14c,0x10b),('x64',0x8664,0x20b)):
        log=(run/f'{arch}-build.log').read_text(errors='replace');assert not re.search(r'\bwarning\b|\berror\b',log,re.I),log
        exe=run/f'engine_collideable_clip_dispatch_{arch}.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        assert raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
        for repeat in (1,2):
            output=run/f'{arch}-{repeat}.bin'
            p=subprocess.run([str(exe),str(ENGINE) if arch=='x86' else '-',str(inputs),str(output)],capture_output=True,text=True,timeout=180)
            (run/f'{arch}-{repeat}.local.log').write_text(p.stdout+p.stderr)
            assert p.returncode==0,(arch,repeat,p.stdout,p.stderr,str(run))
            assert p.stdout.strip()=='PASS collideable clip dispatch cases=61440'
            check(output.read_bytes(),wanted,arch,repeat,run)
            runs.append(dict(architecture=arch,repeat=repeat,record_sha256=sha(output)))
    assert closure()==frozen and guards()==accepted
    assert all(sha(path)==value for path,value in PINS.items())
    assert inputs.read_bytes()==b''.join(map(encode,rows))
    deps=json.loads((ROOT.parent/'publication/stage/docs/build-status.json').read_text())['editor_source_dependencies']
    assert len(deps)==543 and all(sha(ROOT.parent/name)==value for name,value in deps.items())
    receipt=dict(status='VERIFIED PREPARED COLLIDEABLE CLIP DISPATCH',cases_per_run=len(rows),repeats_per_arch=2,
        builds=builds,runs=runs,coverage=dict(counts),source_sha256=frozen,accepted_regressions=accepted,
        record_bytes=len(wanted),record_sha256=hashlib.sha256(wanted).hexdigest(),input_sha256=sha(inputs),
        original_sha256={str(path):value for path,value in PINS.items()},originals_unchanged=True,
        editor_dependencies_unchanged=543,verified_spans=evidence,
        limits=['Complete original1016F300 and initializer execute unchanged in a private sealed image; geometry leaves are supplied hardware-boundary/vtable services, not native geometry execution.',
                'Finite axis-aligned Ray_t field values, supplied point/hull/swept bits and opaque32-bit transform tokens. All80bytes are compared at every callback and restored afterward; transforms are not dereferenced.',
                'Model/studio/flags/root metadata, custom/physics/brush/OBB/hitbox/bounds results and annotation are explicit services. Real entity/static-prop geometry, production bindings, server filter composition, spawn and editor integration remain unverified.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'],cases=len(rows),receipt=str(run/'results.json'),coverage=dict(counts))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);main(parser.parse_args().directory)


