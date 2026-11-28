"""Intact outer TraceRay vs owned finite prepared hull/point policy."""
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

from verify_engine_point_trace import make_trace
from verify_server_simple_trace_filter import PINS
from verify_sliding_door_spawn import ENGINE, verify_installed_ranges

ROOT=Path(__file__).resolve().parent
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def f32(value):return struct.unpack('<f',struct.pack('<f',value))[0]
def frame(raw):return struct.pack('<I',len(raw))+raw

def ray_bytes(ray):
    center,delta,offset,extent,is_ray,swept=ray
    raw=bytearray(80)
    for at,values in ((0,center),(16,delta),(32,offset),(48,extent)):
        struct.pack_into('<3f',raw,at,*values)
    raw[68],raw[69]=is_ray,swept
    return bytes(raw)

def prediction(rawray,mode,excluded,world,candidates,counts):
    center=struct.unpack_from('<3f',rawray);delta=struct.unpack_from('<3f',rawray,16)
    offset=struct.unpack_from('<3f',rawray,32);is_ray,swept=rawray[68:70]
    result=bytearray(world);calls=[];partition=0;overflow=0;short=(0.,0.,0.);enum_ray=bytes([255])*80
    world_calls=int(mode!=2)
    counts['cases']+=1;counts['world_calls']+=world_calls
    counts['hulls']+=not is_ray;counts['points']+=is_ray;counts['stationary']+=not swept
    if mode==2:
        result=bytearray(92);struct.pack_into('<f',result,44,1.)
        start=[f32(offset[j]+center[j]) for j in range(3)]
        end=[f32(start[j]+delta[j]) for j in range(3)]
        result[:24]=struct.pack('<6f',*start,*end);counts['entities_only']+=1
    if mode!=2 and (result[55] or mode==1):
        counts['early_returns']+=1;counts['early_hulls_preserve_positions_and_exit']+=not is_ray
    else:
        partition=1;scale=struct.unpack_from('<f',result,44)[0];left=struct.unpack_from('<f',result,56)[0];exit_scale=scale
        if scale==0:
            short=(0.,0.,0.);exit_scale=left;struct.pack_into('<f',result,56,1.);counts['zero_world_fraction']+=1
        else:
            short=tuple(f32(f32(f32(delta[j]*scale)+center[j])-center[j]) for j in range(3))
            struct.pack_into('<f',result,56,f32(left/scale))
            counts['rounded_shortening']+=any(struct.pack('<f',short[j])!=struct.pack('<f',f32(delta[j]*scale)) for j in range(3))
        enum_ray=bytearray(rawray);struct.pack_into('<3f',enum_ray,16,*short);enum_ray=bytes(enum_ray)
        struct.pack_into('<f',result,44,1.);overflow=max(0,len(candidates)-1024);counts['overflowed_candidates']+=overflow
        for i,(handle,trace) in enumerate(candidates[:1024]):
            label=i+1;calls.extend(((1,label),(2,label)));static=(handle&0xffff0000)==0x40000000
            if static and mode==2:counts['entities_only_static_skips']+=1;continue
            if not static or mode==3:
                calls.append((4,label));counts['predicates']+=1
                if excluded==label:counts['filtered']+=1;continue
            elif static:
                counts['static_predicate_bypass']+=1
                counts['excluded_static_still_clips']+=excluded==label
            calls.append((3,label));counts['clipped']+=1
            frac=struct.unpack_from('<f',result,44)[0];new_frac=struct.unpack_from('<f',trace,44)[0]
            if trace[54] or trace[55] or frac>new_frac:
                old=bytes(result);result[:]=trace;counts['merged']+=1
                if old[55]:
                    result[55]=1
                    if struct.unpack_from('<f',old,56)[0]>struct.unpack_from('<f',trace,56)[0]:
                        result[56:60]=old[56:60];result[:12]=old[:12];counts['preserved_exit_start']+=1
            if result[54]:counts['allsolid_stops']+=1;break
        struct.pack_into('<f',result,44,f32(scale*struct.unpack_from('<f',result,44)[0]))
        struct.pack_into('<f',result,56,f32(exit_scale*struct.unpack_from('<f',result,56)[0]))
        if not is_ray:
            result[:12]=struct.pack('<3f',*(f32(offset[j]+center[j]) for j in range(3)))
            struct.pack_into('<f',result,56,0.);counts['hull_start_exit_resets']+=1
    payload=bytes(result)+struct.pack('<3I3fI',world_calls,partition,overflow,*short,len(calls))+b''.join(struct.pack('<2I',*c) for c in calls)
    return frame(payload),enum_ray

def corpus():
    rng=random.Random(0x16ece0)
    rays=[((1e7,-1e7,-0.),(.5,-.25,-0.),(-1e7,1e7,0.),(24.,24.,41.)),
          ((-0.,-0.,0.),(-0.,2.,-3.),(-0.,0.,-0.),(0.,0.,0.)),
          ((32.,-10.,100.),(1e-24,0.,0.),(-1.,2.,-41.),(.000732421875,.000244140625,0.)),
          ((.03125,-.03125,2.),(.125,.25,-.5),(-.0625,.0625,-2.),(.25,.5,1.))]
    rows=[]
    for ray in rays:
        for flags in range(4):
            full=ray_bytes((*ray,flags&1,(flags>>1)&1))
            for mode in range(4):
                for frac in (0.,.125,.5,1.):
                    for inside in (0,1,2):
                        for excluded in (0,1,2):
                            for kind in range(4):
                                world=make_trace(rng,frac,inside,0,.75)
                                candidates=[((0x40000000 if kind&1 else 0x40010000)+1,make_trace(rng,.25,kind>>1,kind>>1,.5)),
                                            (0x40000002,make_trace(rng,.125,0,0,.25))]
                                rows.append((full,0x200400b,mode,excluded,world,candidates))
    for n in (0,1,1023,1024,1025,2048):
        for mode in (0,2,3):
            for is_ray in (0,1):
                ray=ray_bytes((*rays[0],is_ray,1))
                candidates=[(0x40000000+i,make_trace(rng,1.,0,0,0.)) for i in range(n)]
                rows.append((ray,1,mode,0,make_trace(rng,1.,0,0,0.),candidates))
    for _ in range(2048):
        ray=ray_bytes((*rng.choice(rays),rng.randrange(2),rng.randrange(2)));n=rng.randrange(9)
        candidates=[(rng.choice((0,0x40000000,0x40010000,0xffff0000))+rng.randrange(65536),make_trace(rng)) for _ in range(n)]
        rows.append((ray,rng.getrandbits(32),rng.randrange(4),rng.randrange(n+1),make_trace(rng),candidates))
    return rows

def source_closure():
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
                    target=ROOT/(name.split('.')[0]+'.py')
                    if target.is_file():visit(target)
    for name in ('verify_engine_hull_trace.py','probe_engine_hull_trace.cpp','build_engine_hull_trace.cmd'):visit(ROOT/name)
    return result

def accepted_regressions():
    base=Path('D:/SFM64-development-tests')
    entries=[
        ('20261128-composed/verify-c33386xf/results.json','d2f0f33470a0196251c6d2722439687ed4bb8e2565063eac783eea51fa561f73'),
        ('20261128-server-pass-entity/verify-9zp997a0/results.json','23f6b8237a23e1184246e57f3c9d3b839245f39451046e875cc833aa732d026e'),
        ('20261128-server-standard-filter/verify-2qqwr538/results.json','6fa3ddfeabd7b1a1c3ba695ecd32d46f538e00efe7fea1d763e9f77b406d9bf4'),
        ('20261128-swept-point-world-trace/verify-r1dbiebk/results.json','b3d6513006607b089b30f8e4e4fe491bbfc30fffb205ea28c7a03c3bada82f5a')]
    result=[]
    for relative,digest in entries:
        path=base/relative;old=json.loads(path.read_text())
        assert old['record_sha256']==digest,('accepted record identity changed',str(path))
        assert old['source_sha256']
        assert all(sha(ROOT/name)==value for name,value in old['source_sha256'].items()),('accepted source changed',str(path))
        assert all(sha(build['executable'])==build['sha256'] for build in old['builds'].values()),('accepted binary changed',str(path))
        result.append(dict(receipt=str(path),receipt_sha256=sha(path),record_sha256=digest,source_and_binary_hashes_unchanged=True))
    path=base/'20261128-prepared-world-trace-dispatch/dispatch-run4/verify-bplr9h91/results.json'
    old=json.loads(path.read_text())
    for name,value in old['source_closure_sha256'].items():
        assert sha(ROOT/(name[3:] if name.startswith('PY:') else name))==value,('accepted dispatcher source changed',name)
    known={'stationary_hull':'d71e19661c5bd52c468822423d5228b250d65821daa2bea1c0e8812b8001947e',
           'stationary_point':'4d95e057f11332fb3f8bdbbbef0968b8d19cd8a569b74f22e5e396d5337e5b14',
           'swept_hull':'35ca5376f7362f4bfe909a7636ec250fcab31596b1dfb284080ebb5b6e6c4c5d',
           'swept_point':'b3d6513006607b089b30f8e4e4fe491bbfc30fffb205ea28c7a03c3bada82f5a'}
    assert len(old['components'])==4
    for component in old['components']:
        assert component['accepted_record_sha256']==known[component['component']]
        assert all(sha(b['executable'])==b['sha256'] for b in component['builds'].values())
    result.append(dict(receipt=str(path),receipt_sha256=sha(path),record_sha256=known,source_and_binary_hashes_unchanged=True))
    return result

def main(directory):
    directory=directory.resolve();assert directory.is_relative_to(Path('D:/SFM64-development-tests').resolve())
    directory.mkdir(parents=True,exist_ok=True);assert all(sha(p)==v for p,v in PINS.items())
    frozen=source_closure();regressions=accepted_regressions()
    run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory));rows=corpus();counts=Counter()
    inputs=run/'input.local.bin';inputs.write_bytes(b''.join(ray+struct.pack('<3I',mask,mode,excluded)+world+struct.pack('<I',len(candidates))+b''.join(struct.pack('<I',h)+trace for h,trace in candidates) for ray,mask,mode,excluded,world,candidates in rows))
    predicted=[prediction(ray,mode,excluded,world,candidates,counts) for ray,mask,mode,excluded,world,candidates in rows]
    wanted=b''.join(p[0] for p in predicted);wanted_rays=b''.join(p[1] for p in predicted)
    assert all(counts[k]>0 for k in ('hulls','points','stationary','entities_only','early_returns','early_hulls_preserve_positions_and_exit','zero_world_fraction','rounded_shortening','overflowed_candidates','entities_only_static_skips','static_predicate_bypass','excluded_static_still_clips','preserved_exit_start','allsolid_stops','hull_start_exit_resets')),counts
    build=subprocess.run(['cmd','/c',str(ROOT/'build_engine_hull_trace.cmd'),str(run)],capture_output=True,text=True,timeout=240)
    (run/'build.local.log').write_text(build.stdout+build.stderr);assert build.returncode==0,build.stdout+build.stderr
    _,evidence=verify_installed_ranges(ENGINE,run/'engine-reference.local.txt',[
        ('complete engine TraceRay',0x1016ece0,0x1016f0d3),('complete merge',0x1016ebd0,0x1016ece0),
        ('complete default trace initializer',0x10145f40,0x10145f7d),('complete collector',0x1016d8f0,0x1016d923)])
    builds={};runs=[]
    for arch,machine,magic in (('x86',0x14c,0x10b),('x64',0x8664,0x20b)):
        log=(run/f'{arch}-build.log').read_text(errors='replace');assert not re.search(r'\bwarning\b|\berror\b',log,re.I),log
        exe=run/f'engine_hull_{arch}.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        assert raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
        for repeat in (1,2):
            output=run/f'{arch}-{repeat}.bin';p=subprocess.run([str(exe),str(ENGINE) if arch=='x86' else '-',str(inputs),str(output)],capture_output=True,text=True,timeout=180)
            (run/f'{arch}-{repeat}.local.log').write_text(p.stdout+p.stderr);assert p.returncode==0,(arch,repeat,p.stdout,p.stderr)
            actual=output.read_bytes();actual_rays=Path(str(output)+'.ray').read_bytes()
            if actual!=wanted:
                first=next(i for i,(a,b) in enumerate(zip(actual,wanted)) if a!=b) if len(actual)==len(wanted) else None
                raise AssertionError(('full trace/events differ',arch,repeat,len(actual),len(wanted),first,str(run)))
            assert actual_rays==wanted_rays,('full80byte enumeration rays differ',arch,repeat,str(run))
            runs.append(dict(architecture=arch,repeat=repeat,record_sha256=sha(output),ray_sha256=hashlib.sha256(actual_rays).hexdigest()))
    assert source_closure()==frozen,'source changed during verification'
    assert accepted_regressions()==regressions
    assert all(sha(p)==v for p,v in PINS.items())
    editor=json.loads((ROOT.parent/'publication/stage/docs/build-status.json').read_text());deps=editor['editor_source_dependencies']
    assert len(deps)==543 and all(sha(ROOT.parent/p)==v for p,v in deps.items())
    receipt=dict(status='VERIFIED PREPARED ENGINE HULL/POINT POLICY',cases_per_run=len(rows),repeats_per_arch=2,
        builds=builds,runs=runs,coverage=dict(counts),source_sha256=frozen,record_bytes=len(wanted),record_sha256=hashlib.sha256(wanted).hexdigest(),
        ray_bytes=len(wanted_rays),ray_sha256=hashlib.sha256(wanted_rays).hexdigest(),input_sha256=sha(inputs),
        original_sha256={str(p):v for p,v in PINS.items()},originals_unchanged=True,editor_dependencies_unchanged=543,
        accepted_regressions=regressions,
        verified_spans=evidence,
        limits=['Finite prepared axis-aligned rays and constant filter modes only. World/partition/resolution/clipping/predicate results are explicit services.',
                'All80enumerated Ray_t bytes compare; probe checks full copied rays at clip. Null transform and fixed padding only. Actual spatial query, collisions, production loading/lifetime/spawn/editor integration remain unverified.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'],cases=len(rows),coverage=dict(counts),receipt=str(run/'results.json'))))

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);main(p.parse_args().directory)
