"""Actual studio ClipRayToCollideable, real physics and MDL contents composition.

Independent expected records use immutable native hull geometry and original
point-engine records. No constant collision leaves are used by either program.
"""
import argparse
import ast
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile

from verify_world_engine_swept_hull import ROOT,TEST_ROOT,sha,f32,ENGINE,PINS,verify_installed_ranges
from verify_phy_hull_contents import guards as prior_guards,game_offsets
from verify_physics_hull_ray_preparation import BASE,LIBRARY_SHA,raw_ray

HULL=TEST_ROOT/'20261128-phy-hull-trace/verify-3tpl078t/results.json'
CONTENTS=TEST_ROOT/'20261128-phy-hull-contents/verify-15pqeirv/results.json'
POINT=ROOT.parent/'outputs/engine-studio-point-clip-20261016/verification-l32p5qib/results.json'


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
    for name in ('verify_engine_studio_hull_clip.py','probe_engine_studio_hull_clip.cpp',
                 'build_engine_studio_hull_clip.cmd','test_engine_studio_hull_clip.cpp',
                 'source_engine_studio_point_clip_candidate.h','x64_fsin_cos.asm'):visit(ROOT/name)
    return files


def guards():
    result=prior_guards();old=json.loads(CONTENTS.read_text())
    assert all(sha(ROOT/name)==value for name,value in old['source_sha256'].items())
    assert all(sha(build['executable'])==build['sha256'] for build in old['builds'].values())
    result.append(dict(receipt=str(CONTENTS),receipt_sha256=sha(CONTENTS),source_and_binary_hashes_unchanged=True))
    old=json.loads(POINT.read_text());assert all(sha(Path(build['path']))==build['sha256'] for build in old['executables'])
    result.append(dict(receipt=str(POINT),receipt_sha256=sha(POINT),accepted_point_results_and_binaries_preserved=True))
    return result


def mdl_contents(data):
    assert len(data)>=400 and data[:4]==b'IDST'
    count,offset=struct.unpack_from('<2I',data,156)
    assert 0<count<=32768 and offset>=400 and offset+216*count<=len(data)
    return struct.unpack_from('<I',data,332)[0],tuple(struct.unpack_from('<I',data,offset+216*i+180)[0] for i in range(count))


def is_hit(trace):return struct.unpack_from('<f',trace,44)[0]<1. or bool(trace[54]) or bool(trace[55])


def miss(row):
    ray=row[:80];center=struct.unpack_from('<3f',ray);offset=struct.unpack_from('<3f',ray,32);delta=struct.unpack_from('<3f',ray,16)
    start=[f32(offset[j]+center[j]) for j in range(3)]
    trace=bytearray(92);struct.pack_into('<6f',trace,0,*start,*[f32(delta[j]+start[j]) for j in range(3)])
    struct.pack_into('<f',trace,44,1.);return bytes(trace)


def stamped(geometry,row,model_contents):
    trace=bytearray(geometry)
    if is_hit(trace):
        handle,surface,world=struct.unpack_from('<2IQ',row,116)
        struct.pack_into('<I',trace,48,model_contents);struct.pack_into('<Q',trace,60,0xffffffffffffffff)
        struct.pack_into('<I',trace,68,surface);struct.pack_into('<Q',trace,80,world);struct.pack_into('<I',trace,88,(handle&0xffff)+1)
    return bytes(trace)


def old_point(row):
    assert len(row)==76
    start=struct.unpack_from('<3f',row);delta=struct.unpack_from('<3f',row,12)
    swept=int(f32(f32(delta[0]*delta[0]+delta[1]*delta[1])+delta[2]*delta[2])!=0.)
    ray=bytearray(80);struct.pack_into('<3f',ray,0,*start);struct.pack_into('<3f',ray,16,*delta)
    struct.pack_into('<I',ray,64,0x1234);ray[68]=1;ray[69]=swept
    return bytes(ray)+row[24:76]


def hull_row(row,index,cache=0,mask=None):
    assert len(row)==108;row=bytearray(row)
    current_mask=struct.unpack_from('<I',row,104)[0]&~0x40000000
    struct.pack_into('<I',row,104,current_mask if mask is None else mask)
    flags=(0,4,256,384,2 if row[68] else 1)[index%5]
    handle=0x40000000|((index*1031)&0xffff);surface=(0,1,32768,65535)[index%4]
    world=(0,0x12345678,0x5555000000000000+index)[index%3]
    return bytes(row)+struct.pack('<4IQ',flags,cache,handle,surface,world)


def model_rows(point_record,hull_record,model_contents,leaf_contents=None):
    point_path=Path(point_record['path']);point_queries=point_path/'queries.bin'
    assert sha(point_queries)==point_record['queries_sha256']
    old_trace=point_path/'studio_clip_native_x86.bin';old_events=Path(str(old_trace)+'.events')
    assert sha(old_trace)==point_record['semantic_trace_sha256'] and sha(old_events)==point_record['events_sha256']
    old_queries=point_queries.read_bytes();old_output=old_trace.read_bytes();old_meta=old_events.read_bytes()
    assert len(old_queries)%76==0 and len(old_output)==len(old_queries)//76*92
    rows=[];expected=[];events=[];labels=[]
    for i in range(len(old_queries)//76):
        rows.append(old_point(old_queries[i*76:(i+1)*76]));expected.append(old_output[i*92:(i+1)*92]);events.append(old_meta[i*8:(i+1)*8]);labels.append(('old_point',i))
    base=Path(hull_record['path']);query_path=base/'queries.local.bin';trace_path=base/'x86-1.bin'
    assert sha(query_path)==hull_record['input_sha256'] and sha(trace_path)==hull_record['runs'][0]['record_sha256']
    query=query_path.read_bytes();trace=trace_path.read_bytes();assert len(query)%108==0 and len(trace)==len(query)//108*92
    if leaf_contents is None:
        assert model_contents==1
        selected=range(len(query)//108)
    else:
        eligible=[i for i in range(len(query)//108) if struct.unpack_from('<I',query,i*108+104)[0]&1]
        hits=[i for i in eligible if is_hit(trace[i*92:(i+1)*92])]
        selected=list(dict.fromkeys(hits[:64]+eligible[::max(1,len(eligible)//64)]))[:128]
    for i in selected:
        original=query[i*108:(i+1)*108];geometry=trace[i*92:(i+1)*92]
        for mask in (None,) if leaf_contents is None else (1,2,4,6,0x4481,0x80000000):
            row=hull_row(original,len(rows),mask=mask);effective=struct.unpack_from('<I',row,104)[0]
            dispatch=bool(effective&model_contents)
            value=stamped(geometry,row,model_contents) if dispatch and (leaf_contents is None or effective&leaf_contents) else miss(row)
            rows.append(row);expected.append(value);events.append(struct.pack('<2I',dispatch,is_hit(value)));labels.append(('hull',i,mask))
    # Known absent caches are distinct explicit inputs, rather than unresolved
    # assets. Use complete miss predictions even when the normal geometry hits.
    for i in list(selected)[:16]:
        for cache in (1,2):
            row=hull_row(query[i*108:(i+1)*108],len(rows),cache=cache)
            rows.append(row);expected.append(miss(row));events.append(bytes(8));labels.append(('known_cache',i,cache))
    return rows,b''.join(expected),b''.join(events),labels


def assertions(rows,traces,events,callbacks,model_contents,bones,old_count):
    assert len(traces)==len(rows)*92 and len(events)==len(rows)*8 and len(callbacks)%12==0
    counts=Counter(queries=len(rows),accepted_old_point_queries=old_count)
    for index,row in enumerate(rows):
        trace=traces[index*92:(index+1)*92];mask,flags,cache,handle,surface,world=struct.unpack_from('<5IQ',row,104)
        dispatched,hit=struct.unpack_from('<2I',events,index*8)
        assert dispatched==int(cache==0 and bool(mask&model_contents)) and hit==int(is_hit(trace))
        if not dispatched:assert trace==miss(row)
        if hit:
            assert struct.unpack_from('<I',trace,48)[0]==model_contents
            assert struct.unpack_from('<QI',trace,60)==(0xffffffffffffffff,surface)
            assert struct.unpack_from('<QI',trace,80)==(world,(handle&0xffff)+1)
        counts['physics_dispatches']+=dispatched;counts['hits']+=hit;counts['hulls']+=row[68]==0;counts['points']+=row[68]!=0
        counts['missing_caches']+=cache==1;counts['empty_caches']+=cache==2
        counts['wide_world_hit']+=bool(hit and world>0xffffffff)
        counts['opposite_custom_flag']+=bool(flags&(2 if row[68] else 1))
    for at in range(0,len(callbacks),12):
        index,game_data,contents=struct.unpack_from('<3I',callbacks,at);assert index<len(rows)
        mask,_,cache=struct.unpack_from('<3I',rows[index],104);assert not cache and mask&model_contents
        assert game_data<=len(bones);assert contents==(model_contents if game_data==0 else bones[game_data-1])
        counts['contents_callbacks']+=1;counts['model_contents_callbacks']+=game_data==0;counts['bone_contents_callbacks']+=game_data!=0
        counts['different_bone_contents_callbacks']+=game_data!=0 and contents!=model_contents
    return counts


def main(directory):
    directory=directory.resolve();assert directory.is_relative_to(TEST_ROOT.resolve());directory.mkdir(parents=True,exist_ok=True)
    assert all(sha(path)==value for path,value in PINS.items());frozen=closure();accepted=guards()
    run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory));old=json.loads(BASE.read_text());points=json.loads(POINT.read_text());hulls=json.loads(HULL.read_text())
    assert len(old['models'])==len(points['models'])==len(hulls['models'])==61
    libraries=run/'libraries';libraries.mkdir();originals={}
    for library in old['libraries']:
        source=Path(library['source']);assert sha(source)==library['sha256']==LIBRARY_SHA[source.name.lower()]
        shutil.copyfile(source,libraries/source.name);assert sha(libraries/source.name)==library['sha256'];originals[str(source)]=library['sha256']
    build=subprocess.run(['cmd','/c',str(ROOT/'build_engine_studio_hull_clip.cmd'),str(run)],capture_output=True,text=True,timeout=240)
    (run/'build.local.log').write_text(build.stdout+build.stderr);assert build.returncode==0,build.stdout+build.stderr
    builds={}
    for arch,machine,magic in (('x86',0x14c,0x10b),('x64',0x8664,0x20b)):
        log=(run/f'{arch}-build.log').read_text(errors='replace');assert not re.search(r'\bwarning\b|\berror\b',log,re.I),log
        exe=run/f'engine_studio_hull_clip_{arch}.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        assert raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
    assembly=(run/'asm-build.log').read_text(errors='replace');assert not re.search(r'\bwarning\b|\berror\b',assembly,re.I),assembly
    safety_log=(run/'safety-build.log').read_text(errors='replace')
    assert not re.search(r'\bwarning\b|\berror\b',safety_log,re.I),safety_log
    safety_exe=run/'engine_studio_hull_clip_safety_x64.exe';raw=safety_exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
    assert raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==0x8664 and struct.unpack_from('<H',raw,pe+24)[0]==0x20b
    builds['safety']=dict(executable=str(safety_exe),sha256=sha(safety_exe),machine='0x8664',warnings=0)
    first_model=old['models'][0]
    assert all(sha(Path(first_model[kind]))==first_model[kind+'_sha256'] for kind in ('phy','mdl'))
    safety=subprocess.run([str(safety_exe),first_model['phy'],first_model['mdl']],capture_output=True,text=True,timeout=30)
    (run/'safety.local.log').write_text(safety.stdout+safety.stderr)
    assert safety.returncode==0 and re.fullmatch(r'PASS studio hull binding safety checks=\d+',safety.stdout.strip()),safety.stdout+safety.stderr
    results=[];totals=Counter()
    for index in range(63):
        source_index=index if index<61 else 0;model=old['models'][source_index];point_record=points['models'][source_index];hull_record=hulls['models'][source_index]
        assert model['model']==point_record['model']==hull_record['model']
        case=run/f'{index:03d}';case.mkdir();phy=Path(model['phy']);mdl=Path(model['mdl'])
        for kind in ('phy','mdl'):
            source=Path(model[kind]);assert sha(source)==model[kind+'_sha256'];originals[str(source)]=model[kind+'_sha256']
        data=mdl.read_bytes();model_content,bones=mdl_contents(data);leaf_content=None
        if index>=61:
            game_data=index-61;point_record=points['controlled_contents_variations'][game_data]
            original_phy=phy.read_bytes();altered_phy=bytearray(original_phy);offsets=game_offsets(altered_phy)
            for at in offsets:struct.pack_into('<I',altered_phy,at,game_data)
            altered_mdl=bytearray(data);bone_offset=struct.unpack_from('<I',data,160)[0]
            struct.pack_into('<I',altered_mdl,332,6);struct.pack_into('<I',altered_mdl,bone_offset+180,4)
            assert all(a==b or i in {at+j for at in offsets for j in range(4)} for i,(a,b) in enumerate(zip(original_phy,altered_phy)))
            assert all(a==b or i in {*range(332,336),*range(bone_offset+180,bone_offset+184)} for i,(a,b) in enumerate(zip(data,altered_mdl)))
            phy=case/'controlled.local.phy';mdl=case/'controlled.local.mdl';phy.write_bytes(altered_phy);mdl.write_bytes(altered_mdl)
            model_content,bones=mdl_contents(altered_mdl);leaf_content=model_content if game_data==0 else bones[0]
        else:assert model_content==1 and bones==(1,)
        rows,wanted,wanted_events,labels=model_rows(point_record,hull_record,model_content,leaf_content)
        queries=case/'queries.local.bin';queries.write_bytes(b''.join(rows));reference_callbacks=None;runs=[]
        for arch in ('x86','x64'):
            for repeat in (1,2):
                output=case/f'{arch}-{repeat}.bin'
                p=subprocess.run([builds[arch]['executable'],str(ENGINE) if arch=='x86' else '-',str(libraries),str(phy),str(mdl),str(queries),str(output)],capture_output=True,text=True,timeout=180)
                (case/f'{arch}-{repeat}.local.log').write_text(p.stdout+p.stderr);assert p.returncode==0,(index,arch,repeat,p.stdout,p.stderr,str(case))
                actual=output.read_bytes();events=Path(str(output)+'.events').read_bytes();callbacks=Path(str(output)+'.contents').read_bytes()
                if actual!=wanted or events!=wanted_events:
                    first=next((i for i,(a,b) in enumerate(zip(actual,wanted)) if a!=b),None)
                    detail=dict(corpus=index,architecture=arch,repeat=repeat,actual_bytes=len(actual),expected_bytes=len(wanted),first_byte=first,
                        query=None if first is None else first//92,label=None if first is None else labels[first//92],events_match=events==wanted_events)
                    (case/'difference.local.json').write_text(json.dumps(detail,indent=2)+'\n');raise AssertionError(('complete independent studio result/events',str(case/'difference.local.json'),detail))
                if reference_callbacks is None:reference_callbacks=callbacks
                assert callbacks==reference_callbacks,('original/owned MDL callback sequence',index,arch,repeat,str(case))
                runs.append(dict(architecture=arch,repeat=repeat,record_sha256=sha(output),events_sha256=hashlib.sha256(events).hexdigest(),contents_sha256=hashlib.sha256(callbacks).hexdigest()))
        counts=assertions(rows,wanted,wanted_events,reference_callbacks,model_content,bones,len(Path(point_record['path']).joinpath('queries.bin').read_bytes())//76)
        totals.update(counts);results.append(dict(model=model['model'],controlled_game_data=None if index<61 else index-61,path=str(case),coverage=dict(counts),input_sha256=sha(queries),runs=runs))
        print(json.dumps(dict(model=index+1,cases=len(rows),status='PASS')),flush=True)
    assert totals['accepted_old_point_queries']==25079 and totals['hulls']>0 and totals['different_bone_contents_callbacks']>0 and totals['wide_world_hit']>0
    assert closure()==frozen and guards()==accepted
    assert all(sha(Path(path))==value for path,value in originals.items()) and all(sha(path)==value for path,value in PINS.items())
    for library in old['libraries']:assert sha(libraries/Path(library['source']).name)==library['sha256']
    deps=json.loads((ROOT.parent/'publication/stage/docs/build-status.json').read_text())['editor_source_dependencies']
    assert len(deps)==543 and all(sha(ROOT.parent/name)==value for name,value in deps.items())
    image,evidence=verify_installed_ranges(ENGINE,run/'engine-reference.local.txt',[
        ('complete ClipRayToCollideable',0x1016f300,0x1016f54c),('complete physics dispatch',0x1016e170,0x1016e2c2),
        ('complete actual studio contents provider',0x1016d6c0,0x1016d6ee),('complete client entity annotation',0x1016d7b0,0x1016d839)])
    assert image.word(0x10354470)==0x1016d6c0 and image.string(0x103545f8)=='**studio**'
    receipt=dict(status='VERIFIED ACTUAL ENGINE STUDIO POINT/HULL PHYSICS COMPOSITION',models=results,coverage=dict(totals),repeats_per_arch=2,
        builds=builds,safety=safety.stdout.strip(),source_sha256=frozen,accepted_regressions=accepted,verified_spans=evidence,
        original_sha256={**{str(path):value for path,value in PINS.items()},**originals},originals_unchanged=True,editor_dependencies_unchanged=543,
        limits=['61pinned one-solid studio/solid6 static-prop models; known missing/empty cache states, supplied resolved surface/world identities and caller-established identity root only.',
                'Original complete engine1016F300/1016E170/1016D6C0/client1016D7B0 and actual full VPhysicsTraceBox execute unchanged. No constant geometry leaves or native-state replay.',
                'Model/cache/pose/root/entity-list providers remain explicit host fixtures; material-registry/asset-cache ownership and real partition/entity population are not initialized.',
                'Custom/hitbox/other solid paths, nonidentity transforms, multisolid/missing/archive assets, production spawn and editor integration remain unverified. Fresh scratch only.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'],coverage=dict(totals),receipt=str(run/'results.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);main(parser.parse_args().directory)
