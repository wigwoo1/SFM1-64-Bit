"""Run accepted native world corpora through the typed physics trace bridge.

Public84-byte results keep their immutable accepted hashes. A separate92-byte
semantic sidechannel is independently predicted from those native records.
"""
import argparse
import ast
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile

from pe_image import Image
from verify_prepared_world_trace_dispatch import ACCEPTED, check_records
from verify_server_simple_trace_filter import PINS
from verify_sliding_door_spawn import ENGINE, BSP

ROOT=Path(__file__).resolve().parent
TEST_ROOT=Path('D:/SFM64-development-tests')
PERM_SHA='35e914e111dddaa8e76930d29c4c2f46156130b7f2d00ff7734274cb947adbe6'

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def closure():
    result={}
    def visit(path):
        path=path.resolve();key=path.relative_to(ROOT).as_posix()
        if key in result:return
        result[key]=sha(path);text=path.read_text(encoding='utf-8-sig')
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',text,re.M):visit(path.parent/inc)
        if path.suffix=='.py':
            for node in ast.walk(ast.parse(text)):
                names=[a.name for a in node.names] if isinstance(node,ast.Import) else [node.module] if isinstance(node,ast.ImportFrom) and node.module else []
                for name in names:
                    local=ROOT/(name.split('.')[0]+'.py')
                    if local.is_file():visit(local)
    for name in ('verify_world_physics_bridge_corpora.py','build_world_physics_bridge_corpora.cmd',
                 'probe_world_physics_stationary_hull.cpp','probe_world_physics_stationary_point.cpp',
                 'probe_world_physics_swept_hull.cpp','probe_world_physics_swept_point.cpp'):visit(ROOT/name)
    return result

def public_rows(prefix,payload,cases):
    if prefix=='swept_hull':
        block=16+9632*88
        assert cases==19264
        for half in range(2):
            assert struct.unpack_from('<I',payload,half*block)[0]==half
            for i in range(9632):yield payload[half*block+16+i*88:half*block+16+(i+1)*88]
    else:
        stride=176 if prefix=='swept_point' else 160
        assert len(payload)==32+cases*stride
        assert struct.unpack_from('<I',payload,4)[0]==cases
        for i in range(cases):yield payload[32+(i+1)*stride-88:32+(i+1)*stride]

def expected_physics(prefix,payload,cases):
    records=[];counts=Counter(default_names=0,brush_names=0,shared_terrain_names=0,private_marker_omitted=0)
    for row in public_rows(prefix,payload,cases):
        assert len(row)==88
        output=bytearray(92);output[:60]=row[:60]
        name=struct.unpack_from('<I',row,60)[0]
        if name==0xffffffff:name_id=0;counts['default_names']+=1
        elif 0x10000000<=name<0x10000000+87:
            name_id=0x1111000000000000+(name-0x10000000);counts['brush_names']+=1
        else:
            assert 0x20000000<=name<0x20000000+43
            name_id=0x2222000000000000;counts['shared_terrain_names']+=1
        struct.pack_into('<Q',output,60,name_id)
        output[68:80]=row[64:76]
        entity=struct.unpack_from('<I',row,76)[0]
        struct.pack_into('<Q',output,80,entity)
        output[88:92]=row[80:84]
        # Internal displacement marker is not part of the public/semantic wire.
        counts['private_marker_omitted']+=struct.unpack_from('<I',row,84)[0]!=0
        records.append(bytes(output))
    assert len(records)==cases and counts['default_names']>0 and counts['brush_names']>0
    if prefix=='stationary_hull':
        # This immutable stationary non-ray corpus reports brush/default names;
        # its terrain overlap/normal processing does not select terrain metadata.
        assert counts['shared_terrain_names']==counts['private_marker_omitted']==0
    else:
        assert counts['shared_terrain_names']>0 and counts['private_marker_omitted']>0
    return b''.join(records),dict(counts)

def accepted_guards():
    paths=[('20261128-prepared-world-physics-trace/verify-wmzyc5nl/results.json','dfe3090fc4134dd4bb3989d43feff53473410d8232aed9d27dc8512feba06b21'),
           ('20261128-engine-hull/verify-ia7q7zyp/results.json','1197f5424c2f39e81874ccc1a5e4b7fc75a13c773c2b5d83bcad37b3ba56fb0d')]
    result=[]
    for relative,digest in paths:
        path=TEST_ROOT/relative;old=json.loads(path.read_text())
        assert old['record_sha256']==digest
        assert all(sha(ROOT/name)==value for name,value in old['source_sha256'].items())
        assert all(sha(b['executable'])==b['sha256'] for b in old['builds'].values())
        result.append(dict(receipt=str(path),receipt_sha256=sha(path),record_sha256=digest,source_and_binaries_unchanged=True))
    path=TEST_ROOT/'20261128-prepared-world-trace-dispatch/dispatch-run4/verify-bplr9h91/results.json';old=json.loads(path.read_text())
    assert all(sha(ROOT/name.removeprefix('PY:'))==value for name,value in old['source_closure_sha256'].items())
    expected={prefix:digest for prefix,_,_,_,digest,_ in ACCEPTED}
    assert len(old['components'])==4
    for component in old['components']:
        assert component['accepted_record_sha256']==expected[component['component']]
        assert all(sha(b['executable'])==b['sha256'] for b in component['builds'].values())
    result.append(dict(receipt=str(path),receipt_sha256=sha(path),record_sha256=expected,source_and_binaries_unchanged=True))
    return result

def main(directory):
    directory=directory.resolve();assert directory.is_relative_to(TEST_ROOT.resolve())
    directory.mkdir(parents=True,exist_ok=True)
    assert all(sha(p)==value for p,value in PINS.items())
    guards=accepted_guards();frozen=closure();run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    p=subprocess.run(['cmd','/c',str(ROOT/'build_world_physics_bridge_corpora.cmd'),str(run/'bin')],capture_output=True,text=True,timeout=600)
    (run/'build.local.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
    image=Image(ENGINE);at=image.offset(0x104129b0);table=image.b[at:at+1024]
    assert hashlib.sha256(table).hexdigest()==PERM_SHA
    permutation=run/'private-permutation.local.bin';permutation.write_bytes(table)
    bsp=BSP.read_bytes();components=[]
    for prefix,_,old_relative,cases,record_digest,argstyle in ACCEPTED:
        old_path=TEST_ROOT/old_relative;old=json.loads(old_path.read_text())
        assert old['record_sha256']==record_digest and old['counts']['cases']==cases
        builds={};runs=[];baseline=None;expected=None;name_counts=None
        for arch,machine,magic in (('x86',0x14c,0x10b),('x64',0x8664,0x20b)):
            log=(run/'bin'/f'{prefix}-{arch}-build.log').read_text(errors='replace')
            assert not re.search(r'\bwarning\b|\berror\b',log,re.I),log
            exe=run/'bin'/f'world_physics_{prefix}_{arch}.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
            assert raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic
            builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
            log=(run/'bin'/f'{prefix}-{arch}-build.log').read_text(errors='replace')
            assert not re.search(r'\bwarning\b|\berror\b',log,re.I),log
            builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
            for repeat in (1,2):
                output=run/f'{prefix}-{arch}-{repeat}.bin'
                args=[str(exe),str(ENGINE) if arch=='x86' else '-',str(BSP)]
                if argstyle=='table':args.append(str(permutation))
                args.append(str(output))
                result=subprocess.run(args,capture_output=True,text=True,timeout=300)
                (run/f'{prefix}-{arch}-{repeat}.local.log').write_text(result.stdout+result.stderr)
                assert result.returncode==0,(prefix,arch,repeat,result.stdout,result.stderr)
                payload=output.read_bytes();assert hashlib.sha256(payload).hexdigest()==record_digest,('accepted public geometry changed',prefix,arch,repeat)
                check_records(prefix,payload,bsp)
                current=json.loads(result.stdout);assert current['cases']==cases
                if prefix=='swept_hull':assert current['finish_cases']==36
                if baseline is None:
                    baseline=payload;expected,name_counts=expected_physics(prefix,payload,cases)
                assert payload==baseline
                side=Path(str(output)+'.physics');actual=side.read_bytes()
                assert len(actual)==cases*92,(prefix,arch,repeat,len(actual),cases*92)
                assert actual==expected,('typed semantic trace differs from independent native-public conversion',prefix,arch,repeat)
                callback_counts=json.loads(Path(str(output)+'.physics-counts.json').read_text())
                assert callback_counts==dict(calls=cases,default=name_counts['default_names'],
                                             terrain=name_counts['shared_terrain_names'],brush=name_counts['brush_names'])
                runs.append(dict(architecture=arch,repeat=repeat,accepted_public_sha256=record_digest,
                                 physics_record_sha256=sha(side),physics_record_bytes=len(actual)))
        components.append(dict(component=prefix,cases=cases,builds=builds,runs=runs,accepted_receipt=str(old_path),
                               accepted_public_sha256=record_digest,physics_record_sha256=hashlib.sha256(expected).hexdigest(),
                               independent_name_mapping=name_counts))
    assert closure()==frozen and accepted_guards()==guards
    assert all(sha(p)==value for p,value in PINS.items())
    editor=json.loads((ROOT.parent/'publication/stage/docs/build-status.json').read_text());deps=editor['editor_source_dependencies']
    assert len(deps)==543 and all(sha(ROOT.parent/name)==value for name,value in deps.items())
    receipt=dict(status='VERIFIED PREPARED WORLD PHYSICS TRACE CORPORA',components=components,total_cases=sum(c['cases'] for c in components),
        repeats_per_arch=2,warnings=0,source_sha256=frozen,accepted_regressions=guards,
        original_sha256={str(p):value for p,value in PINS.items()},originals_unchanged=True,editor_dependencies_unchanged=543,
        supplied_name_binding=dict(default=0,brush_base='0x1111000000000000',shared_terrain='0x2222000000000000'),
        findings=['All four accepted public geometry corpora retain immutable full-record hashes through new wrapper calls.',
                  'The typed92-byte semantic sidechannel matches an independent conversion of the native84-byte public trace, excluding the private marker.',
                  'Default, brush and terrain names use explicit fixture identities; every terrain mesh shares one name token while material properties remain independent.'],
        limits=['Original prepared geometry/work/cache/material services remain fixtures; no production names, effective physics material registry or full world loading is established.',
                'This verifies the world public-result conversion. Outer engine annotation/entity metadata, composed engine trace with real geometry, entity/static-prop clipping and actual spawn/editor integration remain separate.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'],cases=receipt['total_cases'],receipt=str(run/'results.json'))))

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);main(p.parse_args().directory)
