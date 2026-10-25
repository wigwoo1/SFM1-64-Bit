"""Installed VS20 token-derived matrix blend CPU reference (not GPU parity).

Never opens SFM. No claim about live shader selection, VTX remapping, packed
weight conversion, flex, dot-product rounding, or normalized final normals.
"""
from collections import Counter
from datetime import datetime, timezone
import hashlib
import itertools
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from verify_generic_ambient_irradiance import instructions, source_operand, f32, require

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/generic-shader-skin-blend-20261025'
PRIOR=ROOT/'outputs/generic-ambient-shader-20261024'


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def blend_slice(shader):
    parsed=instructions(shader)
    # Extend the existing token decoder locally; do not change ambient tests.
    for op in parsed:
        if op['opcode']==2:
            op['sources']=[source_operand(t) for t in op['raw_operands'][1:]]
    outputs={}
    for i,op in enumerate(parsed):
        if op['opcode']==4:
            rows=[s['reg'][1]-58 for s in op.get('sources',[]) if s['reg'][0]==2 and 58<=s['reg'][1]<=60 and s['relative']]
            if rows:
                require(len(rows)==1 and op['mask']==15,'Matrix row shape')
                outputs[rows[0]]=(op['dest'],i)
    require(set(outputs)=={0,1,2},'Three blended matrix rows required')
    needed={(outputs[r][0],c) for r in range(3) for c in range(4)}
    chosen=[];external=set()
    for op in reversed(parsed[:max(v[1] for v in outputs.values())+1]):
        lanes=[c for c in range(4) if op['mask']&(1<<c) and (op['dest'],c) in needed]
        if not lanes:continue
        require(op['opcode'] in (2,4,5,46) and not op['dest_modifier'] and not op['token']&0x10000000,'Unsupported blend dependency')
        require(len(op.get('sources',[]))=={2:2,4:3,5:2,46:1}[op['opcode']],'Source count')
        for c in lanes:needed.remove((op['dest'],c))
        for s in op['sources']:
            require(s['modifier'] in (0,1),'Source modifier')
            for c in lanes:
                key=(s['reg'],s['swizzle'][c])
                if s['reg'][0]==1:external.add(key)
                elif s['reg'][0]!=2:needed.add(key)
            if s['relative']:
                a=s['relative'];require(a['reg']==(3,0) and not a['modifier'] and not a['relative'],'Address shape')
                needed.add((a['reg'],a['swizzle'][0]))
        chosen.append(dict(op,used_components=lanes))
    chosen.reverse()
    require(not needed and external=={((1,1),0),((1,1),1),((1,2),0),((1,2),1),((1,2),2)},'Unresolved or unexpected blend inputs')
    require(Counter(op['opcode'] for op in chosen)=={2:2,5:4,4:6,46:1},'Unexpected blend expression')
    definitions=[op for op in parsed if op['opcode']==81 and op['dest']==(2,1)]
    require(len(definitions)==1,'One c1 definition')
    c1=list(struct.unpack('<4f',struct.pack('<4I',*definitions[0]['raw_operands'][1:])))
    require(struct.pack('<f',c1[0])==struct.pack('<I',0x443f4060),'Installed index multiplier changed')
    return chosen,outputs,c1


def execute(sliced,palette,weights,indices):
    operations,outputs,c1=sliced
    # Explicit D3DCOLOR-style normalized INPUT values, not a claim about how
    # the active renderer constructs its actual vertex declaration/buffer.
    v2=[f32(indices[2]/255),f32(indices[1]/255),f32(indices[0]/255),0.0]
    registers={((1,1),c):weights[c] for c in range(2)}
    registers.update({((1,2),c):v2[c] for c in range(4)})
    def read(s,lane):
        kind,index=s['reg'];component=s['swizzle'][lane]
        if s['relative']:
            a=s['relative'];index+=int(registers[(a['reg'],a['swizzle'][0])])
        if kind==2:
            if index==0:
                require(component==1,'Only supplied c0.y available');value=1.0
            elif index==1:value=c1[component]
            else:
                require(58<=index<217,'cModel address outside 53 matrices')
                value=palette[(index-58)//3][(index-58)%3][component]
        else:value=registers[((kind,index),component)]
        return -value if s['modifier'] else value
    for op in operations:
        updates={}
        for lane in op['used_components']:
            args=[read(s,lane) for s in op['sources']]
            if op['opcode']==2:value=f32(args[0]+args[1])
            elif op['opcode']==5:value=f32(args[0]*args[1])
            elif op['opcode']==4:value=f32(f32(args[0]*args[1])+args[2])
            else:value=float(round(args[0]))
            updates[(op['dest'],lane)]=value
        registers.update(updates)
    return [registers[(outputs[row][0],lane)] for row in range(3) for lane in range(4)]


def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    extraction=json.loads((PRIOR/'extraction.json').read_text())
    source=Path(extraction['source']);before=sha(source)
    require(before==extraction['source_sha256']=='0e31c41484207212b069524a079270f5f820059e7d5d4dadc3869bc194d1bd97','Installed shader pin')
    program=next(p for p in extraction['programs'] if p['dynamic']==6)
    shader=(PRIOR/program['bytecode']).read_bytes()
    require(hashlib.sha256(shader).hexdigest()==program['bytecode_sha256']=='7c21e780fc6140c4ef23238307dd38a000eb5b7fe0316f6ca960773ee67d0b22','Shader bytecode pin')
    sliced=blend_slice(shader)
    # Every normalized byte ID in this shader's 53-matrix palette resolves
    # to exactly its intended three-register base under the CPU convention.
    for i in range(53):require(round(f32(f32(i/255)*sliced[2][0]))==3*i,'Normalized shader index mapping')
    rng=random.Random(2026102507);fixtures=[];residual_negative=0
    domains=list(itertools.product([0,1,26,52],repeat=3))
    weights=[(1,0),(0,1),(0,0),(0.25,0.5),(1,1),(-0.5,0.25),(f32(1/3),f32(1/3))]
    cases=[(ids,w) for ids,w in itertools.product(domains,weights)]
    cases.extend((tuple(rng.randrange(53) for _ in range(3)),tuple(f32(rng.uniform(-1,2)) for _ in range(2))) for _ in range(4096))
    for ids,w in cases:
        palette=[[[f32(rng.uniform(-5000,5000)) for _ in range(4)] for _ in range(3)] for _ in range(53)]
        selected=[v for i in ids for row in palette[i] for v in row]
        result=execute(sliced,palette,w,ids)
        fixtures.append(struct.pack('<50f',*selected,*w,*result))
        residual_negative+=f32(1-f32(w[1]+w[0]))<0
    query=run/'token-reference.bin';query.write_bytes(b''.join(fixtures))
    binary=OUT/'shader_skin_blend_x64.exe';raw=binary.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
    require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==0x8664 and struct.unpack_from('<H',raw,pe+24)[0]==0x20b,'PE32+ AMD64 required')
    build=(OUT/'build.log').read_text();require('warning ' not in build.lower() and 'error ' not in build.lower(),'Build diagnostics')
    p=subprocess.run([str(binary),str(query)],cwd=ROOT,capture_output=True,text=True,timeout=60)
    (run/'test.log').write_text(p.stdout+p.stderr)
    require(p.returncode==0 and f'cases={len(cases)}' in p.stdout and 'analytical=4 refusals=115' in p.stdout,'x64 test failed: '+p.stdout+p.stderr)
    require(sha(source)==before,'Original changed')
    tracked=['generic_shader_skin_blend_candidate.h','test_generic_shader_skin_blend.cpp','build_generic_shader_skin_blend.cmd','verify_generic_shader_skin_blend.py']
    receipt=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(cases),exact_matrix_bytes=len(cases)*48,
        normalized_index_checks=53,negative_residual_cases=residual_negative,analytical=4,guard_refusals=115,
        token_instructions=[dict(word_offset=op['offset_words'],opcode=op['opcode'],components=op['used_components']) for op in sliced[0]],
        executable=str(binary),executable_sha256=sha(binary),architecture='AMD64 PE32+',compiler_warnings=[],
        shader_sha256=program['bytecode_sha256'],source_shader_sha256=before,original_unchanged=True,
        sources={name:sha(ROOT/'work'/name) for name in tracked},sfm_process_used=False,computer_use=False,editor_changed=False,
        scope='Static0/dynamic6 token-derived matrix blending with CPU separate-float32 MAD. Already decoded weights/matrices; not native GPU parity, active shader selection, VTX mapping, weight packing, flex or full skinning.')
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status='PASS',receipt=str(run/'results.json'),cases=len(cases),exact_matrix_bytes=len(cases)*48,test=p.stdout.strip())))


if __name__=='__main__':main()
