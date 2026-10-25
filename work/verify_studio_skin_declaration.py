"""Full installed declaration builder + token-derived signed-short weights.

Offline only. Native GPU declaration creation/selection and GPU arithmetic are
not exercised; the one hardware-level query has an explicit fixture callback.
"""
from datetime import datetime,timezone
import hashlib
import itertools
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from inspect_generic_ambient_shader import SOURCE,disassemble
from vcs6_shader_cache_candidate import Vcs6ShaderCache
from verify_generic_ambient_irradiance import instructions,source_operand,f32
from verify_ifm_draw_packet import check_ranges
from verify_studio_skin_palette import STUDIO,STUDIO_SHA

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/generic-compressed-input-20261025'
API=STUDIO.parent/'shaderapidx9.dll'
API_SHA='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
SHADERS={4:'f1e913d0aceaccb5e5c5f49a6e70df60caf9f197f36425905850c7130f813521',5:'810ff718be49e7a2be6c5a6f16639d32bba534a7b27877bdeee3e4081e73d32f',6:'7c21e780fc6140c4ef23238307dd38a000eb5b7fe0316f6ca960773ee67d0b22',7:'76fc818be3f6adf2157687740379abf8e63b6e452d9ee1e54043c3cc68ee72c8'}
def require(value,message):
    if not value:raise RuntimeError(message)
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def extract():
    b=SOURCE.read_bytes();require(hashlib.sha256(b).hexdigest()=='0e31c41484207212b069524a079270f5f820059e7d5d4dadc3869bc194d1bd97','Shader cache pin')
    count=struct.unpack_from('<I',b,20)[0];entries=[struct.unpack_from('<II',b,28+8*i) for i in range(count)];require(entries[0][0]==0,'First static block')
    payload=b[entries[0][1]:entries[1][1]]
    fixture=b[:20]+struct.pack('<II',2,struct.unpack_from('<I',b,24)[0])+struct.pack('<II',0,48)+struct.pack('<II',0xffffffff,48+len(payload))+struct.pack('<I',0)+payload
    cache=Vcs6ShaderCache(fixture);programs=[]
    for dynamic,expected in SHADERS.items():
        shader=cache.shader(0,dynamic);digest=hashlib.sha256(shader).hexdigest();require(digest==expected,'Program pin '+str(dynamic))
        stem='generic-vs20-s0-d'+str(dynamic);(OUT/(stem+'.bin')).write_bytes(shader);(OUT/(stem+'.asm')).write_text(disassemble(shader))
        programs.append(dict(dynamic=dynamic,bytecode=stem+'.bin',sha256=digest))
    require(SOURCE.read_bytes()==b,'Shader cache changed')
    result=dict(source=str(SOURCE),source_sha256=hashlib.sha256(b).hexdigest(),programs=programs,sfm_process_used=False,gpu_used=False)
    (OUT/'extraction.json').write_text(json.dumps(result,indent=2)+'\n');return result


def weight_operations(shader):
    parsed=instructions(shader)
    for op in parsed:
        if op['opcode']==2:op['sources']=[source_operand(t) for t in op['raw_operands'][1:]]
    adds=[(i,op) for i,op in enumerate(parsed) if op['opcode']==2 and op['mask']==3 and any(s['reg']==(1,1) for s in op['sources'])]
    require(len(adds)==1,'Single weight input ADD');start,add=adds[0]
    uses=[op for op in parsed[start+1:] if op['opcode']==5 and op['mask']==3 and any(s['reg']==add['dest'] and s['modifier']==0 for s in op['sources'])]
    require(uses,'Weight scale MUL');mul=uses[0]
    constants={op['dest'][1]:struct.unpack('<4f',struct.pack('<4I',*op['raw_operands'][1:])) for op in parsed if op['opcode']==81}
    for op in [add,mul]:
        require(not op['dest_modifier'] and not op['token']&0x10000000 and len(op['sources'])==2,'Weight expression modifiers')
        for s in op['sources']:require(not s['relative'] and not s['modifier'],'Weight operand')
    require(any(s['reg']==(2,0) and s['swizzle'][:2]==[1,1] for s in add['sources']),'Supplied c0.y bias')
    scale=next(s for s in mul['sources'] if s['reg'][0]==2)
    require(constants[scale['reg'][1]][scale['swizzle'][0]]==1/32768 and scale['swizzle'][0]==scale['swizzle'][1],'Exact embedded scale')
    return [add,mul],constants


def execute_weight(operations,constants,packed):
    values=struct.unpack('<2h',packed);registers={((1,1),c):float(values[c]) for c in range(2)}
    def read(s,lane):
        register=s['reg'];c=s['swizzle'][lane]
        if register[0]==2:
            if register[1]==0:require(c==1,'Unprovided constant');return 1.0
            return constants[register[1]][c]
        return registers[(register,c)]
    for op in operations:
        updated={}
        for c in range(2):
            a,b=[read(s,c) for s in op['sources']];updated[(op['dest'],c)]=f32(a+b if op['opcode']==2 else a*b)
        registers.update(updated)
    return [registers[(operations[-1]['dest'],c)] for c in range(2)]


def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    before={str(p):sha(p) for p in [API,STUDIO,SOURCE]};require(before[str(API)]==API_SHA and before[str(STUDIO)]==STUDIO_SHA,'Installed binary pins')
    image,evidence=check_ranges(API,'shaderapidx9-disassembly.txt',[
        ('complete_declaration_builder',0x10031660,0x10032002),('complete_type_helper',0x10031510,0x10031570),
        ('declaration_builder_to_GPU_creation_call',0x10033681,0x100336cc)])
    require([image.word(0x1016e1fc+i*4) for i in range(2)]==[255,17],'Native declaration END')
    extraction=extract();binaries=[]
    for tag,name,machine in [('native','skin_declaration_native_x86',0x14c),('owned','skin_declaration_owned_x64',0x8664),('input','compressed_weight_input_x64',0x8664),('regression','vertex_weights_regression_x86',0x14c)]:
        log=(OUT/(tag+'-build.log')).read_text();require('warning ' not in log.lower() and 'error ' not in log.lower(),'Build diagnostics '+tag)
        path=OUT/(name+'.exe');b=path.read_bytes();pe=struct.unpack_from('<I',b,60)[0]
        require(b[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==(0x20b if machine==0x8664 else 0x10b),'Binary architecture')
        binaries.append(dict(path=str(path),sha256=sha(path),machine=hex(machine)))
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=120)
        (run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,name+' failed '+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout
    queries=[];rng=random.Random(2026102509)
    for compressed,weights,extra,user,level,flex in itertools.product(range(2),[1,2],range(16),[0,4],[79,91,92,100],range(2)):
        flags=(extra&3)*4|((extra>>2)&3)*16
        f=0x83|(compressed<<10)|(weights<<12)|(user<<15)|(2<<18)|flags
        queries.append(struct.pack('<4I',f,0,level,flex))
    for _ in range(2048):
        f=0x83|(rng.randrange(2)<<10)|(rng.randrange(1,3)<<12)|(rng.choice([0,4])<<15)|(rng.randrange(2)<<9)
        for i in range(8):f|=rng.randrange(5)<<(18+3*i)
        queries.append(struct.pack('<4I',f&0xffffffff,f>>32,rng.choice([91,92,95]),rng.randrange(2)))
    query=run/'declaration-queries.bin';query.write_bytes(b''.join(queries));outputs=[]
    for tag in ['native_x86','owned_x64']:
        path=run/(tag+'.bin');execute('skin_declaration_'+tag,[API,query,path],tag);outputs.append(path.read_bytes())
    require(outputs[0]==outputs[1],'Native and owned declarations differ')
    at=0;decls=[]
    for _ in queries:
        n=struct.unpack_from('<I',outputs[0],at)[0];at+=4;decls.append([struct.unpack_from('<HH4B',outputs[0],at+i*8) for i in range(n)]);at+=n*8
    require(at==len(outputs[0]),'Declaration output framing')
    for q,elements in zip(queries,decls):
        compressed=bool(struct.unpack_from('<I',q)[0]&0x400)
        require(next(e for e in elements if e[0]==0 and e[4]==1)[2]==(6 if compressed else 1),'Weight declaration type')
        require(next(e for e in elements if e[0]==0 and e[4]==2)[2]==4,'Bone-index D3DCOLOR declaration')
        require(next(e for e in elements if e[0]==0 and e[4]==3)[2]==(5 if compressed else 2),'Normal declaration type')
    fixture=[];token_info=[]
    for dynamic in [5,7]:
        operations,constants=weight_operations((OUT/('generic-vs20-s0-d'+str(dynamic)+'.bin')).read_bytes())
        token_info.append(dict(dynamic=dynamic,word_offsets=[o['offset_words'] for o in operations],opcodes=[o['opcode'] for o in operations]))
        for code in range(65536):
            # 251 is coprime to 65536: both signed-short lanes exhaust their
            # entire encoding domain independently, including negative codes.
            packed=struct.pack('<2H',code,(code*251+32768)&65535)
            fixture.append(packed+struct.pack('<2f',*execute_weight(operations,constants,packed)))
    weight_query=run/'weight-token-reference.bin';weight_query.write_bytes(b''.join(fixture))
    input_test=execute('compressed_weight_input_x64',[weight_query],'input');require('checks=2 refusals=5 shader_input_cases=131072' in input_test,'Weight input coverage')
    # Shared fixture default minimum remains unchanged; freshly execute the
    # prior 36,414 writer cases with the newly compiled default constructor.
    prior_dir=ROOT/'outputs/studio-vertex-weights-20261025/verification-f_onyd_0';prior=json.loads((prior_dir/'results.json').read_text())
    require(sha(prior_dir/'queries.bin')==prior['query_sha256'],'Previous regression query pin')
    regression=run/'weight-regression.bin';execute('vertex_weights_regression_x86',[STUDIO,prior_dir/'queries.bin',regression],'regression')
    require(sha(regression)==prior['output_sha256'],'Default fixture writer regression')
    for p,digest in before.items():require(sha(p)==digest,'Original file changed')
    tracked=['studio_skin_declaration_candidate.h','generic_compressed_weight_input_candidate.h','probe_studio_skin_declaration.cpp','test_generic_compressed_weight_input.cpp','build_studio_skin_declaration.cmd','verify_studio_skin_declaration.py','native_private_world_image_fixture.h']
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),declaration_cases=len(queries),exact_declaration_output_bytes=len(outputs[0]),
        weight_shader_cases=len(fixture),exact_decoded_weight_bytes=len(fixture)*8,analytical=2,guard_refusals=5,regression_writer_cases=prior['cases'],
        declaration_types=dict(uncompressed_weights=1,compressed_weights=6,bone_indices=4,compressed_normals=5),
        token_operations=token_info,shader_extraction=extraction,static_ranges=evidence,executables=binaries,sources_sha256={name:sha(ROOT/'work'/name) for name in tracked},
        originals_sha256=before,originals_unchanged=True,computer_use=False,sfm_process_used=False,gpu_used=False,editor_changed=False,
        limitations='Explicit skinned standard-float3 formats, no extra/override/instance/vertex-ID/color1 streams; hardware-level callback supplied. Exact declarations and separate-float32 token weight-input reference only. Compressed normals declared but not decoded here. Active declaration/combo selection, native GPU conversion/rounding, strip submission and full skinning integration remain unverified.')
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(status='PASS',receipt=str(run/'results.json'),declaration_cases=len(queries),exact_declaration_output_bytes=len(outputs[0]),weight_shader_cases=len(fixture),regression_writer_cases=prior['cases'])))


if __name__=='__main__':main()
