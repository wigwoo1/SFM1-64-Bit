"""Actual native/owned PHY hull geometry with per-convex contents services."""
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

from verify_world_engine_swept_hull import ROOT, TEST_ROOT, sha, f32, PINS, verify_installed_ranges
from verify_phy_hull_trace import guards as prior_guards
from verify_physics_hull_ray_preparation import BASE, LIBRARY_SHA, raw_ray

HULL=TEST_ROOT/'20261128-phy-hull-trace/verify-3tpl078t/results.json'


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
    for name in ('verify_phy_hull_contents.py','probe_phy_hull_contents.cpp','build_phy_hull_contents.cmd','x64_fsin_cos.asm'):visit(ROOT/name)
    return files


def guards():
    result=prior_guards();old=json.loads(HULL.read_text())
    assert all(sha(ROOT/name)==value for name,value in old['source_sha256'].items())
    assert all(sha(build['executable'])==build['sha256'] for build in old['builds'].values())
    result.append(dict(receipt=str(HULL),receipt_sha256=sha(HULL),source_and_binary_hashes_unchanged=True))
    return result


def provider(mode,seed,game_data):
    if mode==0:return 1
    if mode==1:return 0
    if mode==2:return seed
    if mode==4:return 1<<((game_data+seed)&31)
    assert mode==3
    value=((game_data^seed)*0x9e3779b9)&0xffffffff;shift=(game_data+seed)&31
    return ((value<<shift)|(value>>(32-shift if shift else 32)))&0xffffffff


def miss(query):
    ray=query[:80];center=struct.unpack_from('<3f',ray);delta=struct.unpack_from('<3f',ray,16);offset=struct.unpack_from('<3f',ray,32)
    start=[f32(offset[j]+center[j]) for j in range(3)]
    result=bytearray(92);struct.pack_into('<6f',result,0,*start,*[f32(delta[j]+start[j]) for j in range(3)])
    struct.pack_into('<f',result,44,1.)
    return bytes(result)


def game_offsets(data):
    # Independent compact-tree file references, never original pointers.
    compact=struct.unpack_from('<I',data)[0]+32
    pending=[compact+struct.unpack_from('<i',data,compact+32)[0]];seen=set();offsets=[]
    while pending:
        node=pending.pop();assert 0<=node<=len(data)-28 and node not in seen;seen.add(node)
        right,relative=struct.unpack_from('<2i',data,node)
        if right:pending.extend((node+right,node+28))
        else:
            offset=node+relative+4;assert 0<=offset<=len(data)-4;offsets.append(offset)
    assert len(offsets)==len(set(offsets))
    return offsets


def fixtures(base_queries,base_trace,full=True,all_leaf_query=None):
    assert len(base_queries)%108==0 and len(base_trace)==len(base_queries)//108*92
    count=len(base_queries)//108;eligible=[i for i in range(count) if struct.unpack_from('<I',base_queries,i*108+104)[0]&1]
    hits=[i for i in eligible if struct.unpack_from('<f',base_trace,i*92+44)[0]<1. or base_trace[i*92+55]]
    chosen=list(dict.fromkeys(hits[:8]+eligible[::max(1,len(eligible)//24)]))[:32]
    assert len(chosen)>=24
    rows=[];expected={};labels=[]
    def add(query,mode,seed,label,wanted=None):
        index=len(rows);rows.append(query+struct.pack('<2I',mode,seed));labels.append(label)
        if wanted is not None:expected[index]=wanted
    for i in range(count) if full else chosen:
        add(base_queries[i*108:(i+1)*108],0,0,('accepted_default',i),base_trace[i*92:(i+1)*92])
    for i in chosen:
        original=base_queries[i*108:(i+1)*108];trace=base_trace[i*92:(i+1)*92]
        add(original,1,0,('constant_zero',i),miss(original))
        for seed in (0,2,6,0x80000000,0xffffffff):
            for overlap in (False,True):
                query=bytearray(original);mask=(seed&-seed) if overlap and seed else 0xffffffff if overlap else (~seed)&0xffffffff
                struct.pack_into('<I',query,104,mask)
                if seed&mask:
                    wanted=bytearray(trace);contents=struct.unpack_from('<I',wanted,48)[0];assert contents in (0,1)
                    struct.pack_into('<I',wanted,48,seed if contents else 0);wanted=bytes(wanted)
                else:wanted=miss(query)
                add(bytes(query),2,seed,('constant_seed',i,seed,overlap),wanted)
        for mode in (3,4):
            for seed in (0,1,0x80000000,0xffffffff):
                query=bytearray(original);struct.pack_into('<I',query,104,0xffffffff)
                add(bytes(query),mode,seed,('per_game_data',i,mode,seed))
    if all_leaf_query is not None:
        for mode in (3,4):add(all_leaf_query,mode,1,('all_leaf_mask_zero',mode),miss(all_leaf_query))
    return rows,expected,labels


def callback_checks(callbacks,rows,allowed):
    assert len(callbacks)%12==0;counts=Counter();seen=set()
    for at in range(0,len(callbacks),12):
        query,game_data,result=struct.unpack_from('<3I',callbacks,at)
        assert query<len(rows) and game_data in allowed
        mode,seed=struct.unpack_from('<2I',rows[query],108)
        assert result==provider(mode,seed,game_data),('independent callback formula',query,game_data,mode,seed,result)
        counts['callbacks']+=1;counts['mode_'+str(mode)]+=1;counts['mask_zero_callbacks']+=struct.unpack_from('<I',rows[query],104)[0]==0
        counts['zero_result_callbacks']+=result==0;counts['high_bit_result_callbacks']+=bool(result&0x80000000)
        seen.add(game_data)
    return counts,seen


def main(directory):
    directory=directory.resolve();assert directory.is_relative_to(TEST_ROOT.resolve());directory.mkdir(parents=True,exist_ok=True)
    assert all(sha(path)==value for path,value in PINS.items())
    frozen=closure();accepted=guards();run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    old=json.loads(BASE.read_text());prior=json.loads(HULL.read_text());assert len(old['models'])==len(prior['models'])==61
    libraries=run/'libraries';libraries.mkdir();originals={}
    for library in old['libraries']:
        source=Path(library['source']);assert sha(source)==library['sha256']==LIBRARY_SHA[source.name.lower()]
        shutil.copyfile(source,libraries/source.name);assert sha(libraries/source.name)==library['sha256'];originals[str(source)]=library['sha256']
    build=subprocess.run(['cmd','/c',str(ROOT/'build_phy_hull_contents.cmd'),str(run)],capture_output=True,text=True,timeout=240)
    (run/'build.local.log').write_text(build.stdout+build.stderr);assert build.returncode==0,build.stdout+build.stderr
    builds={}
    for arch,machine,magic in (('x86',0x14c,0x10b),('x64',0x8664,0x20b)):
        log=(run/f'{arch}-build.log').read_text(errors='replace');assert not re.search(r'\bwarning\b|\berror\b',log,re.I),log
        exe=run/f'phy_hull_contents_{arch}.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        assert raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
    assembly=(run/'asm-build.log').read_text(errors='replace');assert not re.search(r'\bwarning\b|\berror\b',assembly,re.I),assembly
    models=[];totals=Counter();seen_all=set()
    for index in range(63):
        source_index=index if index<61 else (21 if index==61 else 23)
        model=old['models'][source_index];baseline=prior['models'][source_index]
        assert model['model']==baseline['model']
        phy=Path(model['phy']);assert sha(phy)==model['phy_sha256'];originals[str(phy)]=model['phy_sha256']
        base_query_path=Path(baseline['path'])/'queries.local.bin';assert sha(base_query_path)==baseline['input_sha256']
        base_trace_path=Path(baseline['path'])/'x86-1.bin';assert sha(base_trace_path)==baseline['runs'][0]['record_sha256']
        data=phy.read_bytes();offsets=game_offsets(data);case=run/f'{index:03d}';case.mkdir();changed=False;all_leaf=None
        if index>=61:
            controlled=bytearray(data);values=(0,1,0x80000000,0xffffffff)
            for i,at in enumerate(offsets):struct.pack_into('<I',controlled,at,values[i%4] if len(offsets)>1 else values[index-61+2])
            assert all(a==b or at in {position+j for position in offsets for j in range(4)} for at,(a,b) in enumerate(zip(data,controlled)))
            phy=case/'controlled-game-data.local.phy';phy.write_bytes(controlled);data=bytes(controlled);changed=True
            mdl=Path(model['mdl']);assert sha(mdl)==model['mdl_sha256'];raw=mdl.read_bytes();lo=struct.unpack_from('<3f',raw,104);hi=struct.unpack_from('<3f',raw,116)
            center=[f32((lo[i]+hi[i])*.5) for i in range(3)]
            ray=raw_ray(center,(0.,0.,0.),(0.,0.,0.),(10000.,10000.,10000.),0,0,0,index)
            all_leaf=ray+struct.pack('<6fI',0.,0.,0.,0.,0.,0.,0)
        allowed={struct.unpack_from('<I',data,at)[0] for at in offsets}
        rows,expected,labels=fixtures(base_query_path.read_bytes(),base_trace_path.read_bytes(),not changed,all_leaf)
        queries=case/'queries.local.bin';queries.write_bytes(b''.join(rows));reference=None;reference_callbacks=None;runs=[]
        for arch in ('x86','x64'):
            for repeat in (1,2):
                output=case/f'{arch}-{repeat}.bin'
                p=subprocess.run([builds[arch]['executable'],str(libraries),str(phy),str(queries),str(output)],capture_output=True,text=True,timeout=120)
                (case/f'{arch}-{repeat}.local.log').write_text(p.stdout+p.stderr);assert p.returncode==0,(index,arch,repeat,p.stdout,p.stderr,str(case))
                traces=output.read_bytes();callbacks=Path(str(output)+'.contents').read_bytes();assert len(traces)==len(rows)*92
                assert Path(str(output)+'.trace60').read_bytes()==b''.join(traces[i:i+60] for i in range(0,len(traces),92))
                if reference is None:
                    reference=traces;reference_callbacks=callbacks
                    for query,wanted in expected.items():assert traces[query*92:(query+1)*92]==wanted,('independent constant/filtered/baseline prediction',index,query,labels[query],str(case))
                if traces!=reference or callbacks!=reference_callbacks:
                    at=next((i for i,(a,b) in enumerate(zip(traces,reference)) if a!=b),None)
                    raise AssertionError(('native/owned full trace or callback sequence',index,arch,repeat,at,None if at is None else labels[at//92],callbacks==reference_callbacks,str(case)))
                runs.append(dict(architecture=arch,repeat=repeat,record_sha256=sha(output),callback_sha256=hashlib.sha256(callbacks).hexdigest()))
        counts,seen=callback_checks(reference_callbacks,rows,allowed);seen_all.update(seen)
        if index==62:assert seen==allowed=={0,1,0x80000000,0xffffffff}
        counts['queries']=len(rows);counts['independent_predictions']=len(expected)
        counts['default_baseline_queries']=len(base_query_path.read_bytes())//108 if not changed else 0
        counts['fraction_hits']=sum(struct.unpack_from('<f',reference,i*92+44)[0]<1. for i in range(len(rows)))
        totals.update(counts);models.append(dict(model=model['model'],path=str(case),controlled_game_data=changed,
            input_sha256=sha(queries),phy_sha256=sha(phy),coverage=dict(counts),game_data_seen=sorted(seen),runs=runs))
        print(json.dumps(dict(model=index+1,cases=len(rows),status='PASS')),flush=True)
    assert totals['default_baseline_queries']==75152 and all(totals['mode_'+str(mode)]>0 for mode in range(5))
    assert totals['mask_zero_callbacks']>0 and totals['high_bit_result_callbacks']>0 and totals['zero_result_callbacks']>0
    assert closure()==frozen and guards()==accepted
    assert all(sha(Path(path))==value for path,value in originals.items()) and all(sha(path)==value for path,value in PINS.items())
    for library in old['libraries']:assert sha(libraries/Path(library['source']).name)==library['sha256']
    deps=json.loads((ROOT.parent/'publication/stage/docs/build-status.json').read_text())['editor_source_dependencies']
    assert len(deps)==543 and all(sha(ROOT.parent/name)==value for name,value in deps.items())
    physics=Path(next(library['source'] for library in old['libraries'] if Path(library['source']).name.lower()=='vphysics.dll'))
    _,evidence=verify_installed_ranges(physics,run/'physics-reference.local.txt',[('complete leaf contents/filter/search',0x1002a340,0x1002a4e0),('complete TraceBox',0x1002b260,0x1002b561)])
    receipt=dict(status='VERIFIED PER-CONVEX CONTENTS WITH ACTUAL PHY HULL GEOMETRY',models=models,coverage=dict(totals),game_data_seen=sorted(seen_all),
        repeats_per_arch=2,builds=builds,source_sha256=frozen,accepted_regressions=accepted,
        original_sha256={**{str(path):value for path,value in PINS.items()},**originals},originals_unchanged=True,
        editor_dependencies_unchanged=543,verified_spans=evidence,
        limits=['61pinned supported loose one-solid compact PHY assets plus two disposable copies with only convex gameData changed; original geometry files stay read-only.',
                'Native full geometry executes unchanged without observers/patches; per-convex contents providers are explicit host functions, independently formula-checked. Complete92results and ordered natural callback logs compare.',
                'Fresh scratch only, finite axis-aligned inputs. Actual engine studio/MDL provider composition, persistent scratch, transforms, multisolid/archive/missing assets, production bindings, spawn and editor integration remain separate gates.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'],coverage=dict(totals),receipt=str(run/'results.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);main(parser.parse_args().directory)
