"""Offline native optional-tangent writers and owned x64/Phong handoff.

Original machine code is mapped privately by the x86 test only, without
DllMain, import initialization, patches or any SFM process access. All output
is fresh workspace evidence. GPU behavior and active shader choice unverified.
"""
from datetime import datetime,timezone
from pathlib import Path
import hashlib
import itertools
import json
import math
import random
import struct
import subprocess
import tempfile
from verify_ifm_draw_packet import check_ranges
from verify_studio_skin_palette import STUDIO,STUDIO_SHA
from verify_generic_ambient_irradiance import f32
import verify_phong_shader_vertex_skin as phong

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/studio-vertex-tangent-20261025'
def require(b,s):
    if not b:raise RuntimeError(s)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def fixture(vertex,opt,tangent,compressed,stored,has=True):
    require(len(vertex)==48 and len(opt)==9 and len(tangent)==16,'Query input sizes')
    return bytes(vertex)+bytes(opt)+bytes([compressed,stored,int(has)])+bytes.fromhex('1234567812345678')+tangent

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));before={str(STUDIO):sha(STUDIO)}
    require(before[str(STUDIO)]==STUDIO_SHA,'Studiorender pin')
    image,evidence=check_ranges(STUDIO,'studiorender-disassembly.txt',[
        ('complete_compressed_writer',0x10055aa0,0x100560be),('complete_uncompressed_writer',0x100560c0,0x10056518),
        ('normal_packer',0x1000a100,0x1000a210),('repair_normalizer',0x10058a60,0x10058acd),
        ('builder_advance',0x1000a210,0x1000a28d),('cookie_success',0x1005bdd1,0x1005bddb)])
    constants={0x10070294:0x3dcccccd,0x10071418:0x3f333333,0x1007076c:0x34000000,0x100701f8:0x3f800000,0x100701f4:0xbf800000}
    require(all(image.word(a)==v for a,v in constants.items()),'Native repair constants')
    binaries=[]
    for tag,name,machine in [('native','vertex_tangent_native_x86',0x14c),('owned','vertex_tangent_owned_x64',0x8664),('safety','vertex_tangent_safety_x64',0x8664),('handoff','phong_vertex_skin_handoff_x64',0x8664)]:
        log=(OUT/(tag+'-build.log')).read_text();require('warning ' not in log.lower() and 'error ' not in log.lower(),'Compiler diagnostics '+tag)
        p=OUT/(name+'.exe');b=p.read_bytes();pe=struct.unpack_from('<I',b,60)[0]
        require(b[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==(0x10b if machine==0x14c else 0x20b),'Build architecture')
        binaries.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    def invoke(name,args,label):
        result=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=120)
        (run/(label+'.log')).write_text(result.stdout+result.stderr);require(result.returncode==0,name+' failed '+str(result.returncode)+' '+result.stdout+result.stderr);return result.stdout
    safety=invoke('vertex_tangent_safety_x64',[],'safety');require('checks=6 refusals=9' in safety,'Safety coverage')
    profiles,cache_inventory=phong.load_profiles(run,before)
    contexts,palettes,prior=phong.benchmark_contexts(before)
    rows=[];synthetic_tangents=[];rng=random.Random(2026102513)
    for axis,sign in itertools.product(range(3),[-1.0,1.0]):
        n=[0.0]*3;n[axis]=sign
        for hand in [-1.0,1.0]:
            synthetic_tangents.extend((n,[0.0,0.0,0.0,hand]) for _ in range(1))
            synthetic_tangents.append((n,[*n,hand]))
            for bits in [0x3dcccccb,0x3dcccccc,0x3dcccccd,0x3dccccce,0x3dcccccf]:
                t=[0.0]*3;t[(axis+1)%3]=struct.unpack('<f',struct.pack('<I',bits))[0];synthetic_tangents.append((n,[*t,hand]))
    for bits,sign in itertools.product(range(0x3f333330,0x3f333337),[-1.0,1.0]):
        nx=sign*struct.unpack('<f',struct.pack('<I',bits))[0]
        for nz in [-0.2,0.0,0.2]:
            n=[nx,f32(math.sqrt(1-nx*nx-nz*nz)),nz]
            for hand in [-1.0,1.0]:synthetic_tangents.append((n,[0,0,0,hand]))
    for i in range(2048):
        n=[rng.uniform(-1,1) for _ in range(3)];length=math.sqrt(sum(x*x for x in n));n=[f32(x/length) for x in n];hand=-1 if i&1 else 1
        if i%4==0:t=[0.0]*3
        elif i%4==1:t=[f32(x*rng.choice([-0.5,1.0,2.0])) for x in n]
        elif i%4==2:t=[f32(rng.uniform(-2,2)) for _ in range(3)]
        else:t=[f32(n[j]+rng.uniform(-0.03,0.03)) for j in range(3)]
        synthetic_tangents.append((n,[*t,hand]))
    modes=[(0,2),(1,1),(1,2)]
    for i,(n,t) in enumerate(synthetic_tangents):
        vertex=bytearray(48);weights=(0.25,0.5,0.25) if i%7 else (0.2,0.2,0.2)
        struct.pack_into('<3f',vertex,0,*weights);vertex[12:16]=bytes([0,1,2,3]);struct.pack_into('<3f',vertex,16,1,2,3);struct.pack_into('<3f',vertex,28,*n)
        opt=bytearray([2,0,1,i%4,0,0,52,0,26]);struct.pack_into('<H',opt,4,[0,1,17][i%3]);tangent=struct.pack('<4f',*t)
        for compressed,stored in modes:rows.append(fixture(vertex,opt,tangent,compressed,stored))
        if i%32==0:
            poison=struct.pack('<4I',*([0x7fc00001]*4))
            for compressed,stored in modes:rows.append(fixture(vertex,opt,poison,compressed,stored,False))
    synthetic_count=len(rows);benchmark_count=0
    # model_contexts provides the raw optimized record; pair by ordered source
    # coverage rather than reverse-solving a weight/index result.
    context_opts=[]
    for stem in [phong.GAME/'usermod/models/player/hwm/pyro',phong.GAME/'usermod/models/weapons/w_models/w_flamethrower']:
        data=[Path(str(stem)+ext).read_bytes() for ext in ['.mdl','.vvd','.dx90.vtx']]
        context_opts.extend((v,opt,m) for v,opt,m in phong.model_contexts(*data))
    require(len(context_opts)==len(contexts),'Benchmark context count')
    benchmark_keys={}
    for i,((bones,vertex,tangent,mapping,old_weight),(raw,opt,map2)) in enumerate(zip(contexts,context_opts)):
        require(vertex==raw and mapping==map2,'Ordered VTX context identity')
        for compressed,stored in modes:
            for has in [False,True]:
                q=fixture(vertex,opt,tangent,compressed,stored,has);rows.append(q);benchmark_count+=1
                if has and stored==2:benchmark_keys[i,compressed]=q
    query=run/'queries.bin';query.write_bytes(b''.join(rows));outputs=[]
    for tag in ['native_x86','owned_x64']:
        target=run/(tag+'.bin');invoke('vertex_tangent_'+tag,[STUDIO,query,target],tag);outputs.append(target.read_bytes())
    require(len(outputs[0])==len(rows)*48 and len(outputs[1])==len(outputs[0]),'Writer output framing')
    if outputs[0]!=outputs[1]:
        different=next(i for i in range(len(rows)) if outputs[0][i*48:i*48+48]!=outputs[1][i*48:i*48+48])
        raise RuntimeError('Native/owned tangent differs at '+str(different)+' query='+rows[different].hex()+' native='+outputs[0][different*48:different*48+48].hex()+' owned='+outputs[1][different*48:different*48+48].hex())
    print('Native/owned exact writer cases='+str(len(rows)),flush=True)
    lookup={};repaired=0;benchmark_repaired=0;benchmark_zero=0;changed_handedness=0
    for i,q in enumerate(rows):
        result=outputs[0][i*48:i*48+48]
        if q in lookup:require(lookup[q]==result,'Duplicate writer query changed')
        lookup[q]=result
        if not q[57] and q[59]:
            changed=q[68:84]!=result[28:44];repaired+=changed;benchmark_repaired+=(i>=synthetic_count and changed)
            if i>=synthetic_count and q[68:80]==bytes(12):benchmark_zero+=1;require(changed,'Zero benchmark tangent not repaired')
            changed_handedness+=q[80:84]!=result[40:44]
        require(struct.unpack_from('<I',result,44)[0]==1,'Vertex advancement')
    require(changed_handedness==0 and benchmark_zero==7 and benchmark_repaired>=benchmark_zero,'Native repair evidence')
    prior_receipt=ROOT/'outputs/phong-vertex-contract-20261025/verification-m67x4ebf/results.json';prior_phong=json.loads(prior_receipt.read_text());decoder=prior_receipt.parent/'decode-token-reference.bin'
    require(sha(decoder)==prior_phong['decoder_fixture_sha256'],'Prior packed decoder evidence')
    for name,pin in prior_phong['sources_sha256'].items():require(sha(ROOT/'work'/name)==pin,'Prior Phong source changed')
    handoff=run/'phong-native-handoff-token-reference.bin';world_cases=0;zero_handoff_cases=0
    with handoff.open('wb') as f:
        for tick in [50000,50417]:
            for i,(bones,vertex,tangent,mapping,old_weight) in enumerate(contexts):
                source=palettes[tick,bones];palette=[source[mapping[j]] if j in mapping else [[0.0]*4 for _ in range(3)] for j in range(53)]
                for version in [20,30]:
                    for dynamic in [2,3]:
                        compressed=dynamic&1;row=lookup[benchmark_keys[i,compressed]];require(row[12]==1,'Benchmark native weights invalid');indices=list(row[8:11]);require(all(j in mapping for j in indices),'Native slot unmapped')
                        p=profiles[version,dynamic];registers=p['parsed']['inputs'];weights=row[:8];normal=row[16:28];native_tangent=row[28:44];position=struct.unpack_from('<3f',vertex,16)
                        w=struct.unpack('<2h' if compressed else '<2f',weights[:4] if compressed else weights)
                        n=list(normal[:4]) if compressed else list(struct.unpack('<3f',normal))
                        inputs={registers[0,0]:[*position,1],registers[1,0]:[*w,0,0],registers[2,0]:[f32(indices[2]/255),f32(indices[1]/255),f32(indices[0]/255),0],registers[3,0]:n,registers[0,1]:[0,0,0,0],registers[3,1]:[0,0,0,0]}
                        if not compressed:inputs[registers[6,0]]=struct.unpack('<4f',native_tangent)
                        expected=phong.execute(p['full'],inputs,palette,0);selected=[x for j in indices for matrow in palette[j] for x in matrow]
                        profile_id=(0 if version==20 else 2)+compressed
                        f.write(struct.pack('<I36f',profile_id,*selected)+weights+normal+native_tangent+struct.pack('<10f',*position,*([0.0]*7))+struct.pack('<12f',*expected));world_cases+=1
                        zero_handoff_cases+=tangent[:12]==bytes(12)
    print('Native handoff Phong vertex cases='+str(world_cases),flush=True)
    handoff_log=invoke('phong_vertex_skin_handoff_x64',[decoder,handoff],'handoff')
    require(f'packed_frame_cases=131072 vertex_frame_cases={world_cases} degenerate_frame_refusals=0' in handoff_log and 'checks=6 refusals=12' in handoff_log,'Handoff coverage')
    for path,pin in before.items():require(sha(path)==pin,'Original changed')
    tracked=['studio_vertex_tangent_candidate.h','probe_studio_vertex_tangent.cpp','test_studio_vertex_tangent.cpp','build_studio_vertex_tangent.cmd','verify_studio_vertex_tangent.py']
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(rows),synthetic_cases=synthetic_count,benchmark_writer_cases=benchmark_count,exact_native_writer_bytes=len(outputs[0]),native_observed_uncompressed_repairs=repaired,benchmark_uncompressed_repairs=benchmark_repaired,benchmark_zero_tangents_repaired=benchmark_zero,native_sign_changes=changed_handedness,
        handoff_world_vertex_cases=world_cases,zero_source_tangent_handoff_cases=zero_handoff_cases,exact_token_world_frame_bytes=world_cases*48,packed_decoder_regression_cases=131072,safety=safety.strip(),executables=binaries,sources_sha256={name:sha(ROOT/'work'/name) for name in tracked},originals_sha256=before,originals_unchanged=True,
        static_ranges=evidence,constant_bits={hex(a):hex(v) for a,v in constants.items()},query_sha256=sha(query),native_output_sha256=sha(run/'native_x86.bin'),handoff_fixture_sha256=sha(handoff),prior_evidence=prior,prior_phong_receipt=str(prior_receipt),cache_inventory=cache_inventory,
        computer_use=False,sfm_process_used=False,gpu_used=False,editor_changed=False,
        limitations='Full original compressed/uncompressed writers execute unpatched, optional tangent absent/present, bone-bearing branches with finite nonzero normal, bounded finite tangent XYZ, handedness +/-1. Compare emitted weights/index/validity, normal/packed frame, raw tangent and advance. Original runtime declaration/variant choice, strip upload callbacks and GPU precision remain UNVERIFIED. All 9610 benchmark strip vertices pass explicit VS2/VS3 static0/dynamic2,3 using actual native tangent output at frames120/121, zero flex and retained skin palettes. Token calculations use the documented CPU float convention, not native GPU parity. No viewport integration or full lighting/material rendering parity.')
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(status='PASS',receipt=str(run/'results.json'),cases=len(rows),benchmark_zero_tangents_repaired=benchmark_zero,handoff_world_vertex_cases=world_cases)))

if __name__=='__main__':main()
