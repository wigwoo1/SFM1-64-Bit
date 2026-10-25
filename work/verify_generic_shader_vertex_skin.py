"""Offline native normal pack + token-derived assembled world skin reference.

DP3/DP4 reduce left-to-right with separate binary32 operations; NRM uses a
rounded float sqrt/reciprocal CPU convention. Native GPU rounding is unverified.
"""
from datetime import datetime,timezone
import hashlib
import itertools
import json
import math
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from verify_generic_ambient_irradiance import instructions,source_operand,f32
from verify_ifm_draw_packet import check_ranges
from verify_studio_skin_palette import STUDIO,STUDIO_SHA
from verify_studio_skin_declaration import SOURCE,SHADERS
from verify_studio_vertex_weights import Reader

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/generic-vertex-skin-20261025'
SHADER_OUT=ROOT/'outputs/generic-compressed-input-20261025'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def require(value,message):
    if not value:raise RuntimeError(message)


def parse(shader):
    ops=instructions(shader)
    for op in ops:
        if op['opcode'] in (2,8,9,35):
            cursor=1;sources=[]
            while cursor<len(op['raw_operands']):
                t=op['raw_operands'][cursor];cursor+=1;relative=None
                if t&0x2000:relative=source_operand(op['raw_operands'][cursor]);cursor+=1
                sources.append(source_operand(t,relative))
            op['sources']=sources
    constants={op['dest'][1]:struct.unpack('<4f',struct.pack('<4I',*op['raw_operands'][1:])) for op in ops if op['opcode']==81}
    return ops,constants


def slice_ops(parsed,outputs,end=None):
    ops,constants=parsed;needed={(reg,c) for reg in outputs for c in range(3)};selected=[];external=set()
    for op in reversed(ops[:end] if end is not None else ops):
        lanes=[c for c in range(4) if op['mask']&(1<<c) and (op['dest'],c) in needed]
        if not lanes:continue
        require(op['opcode'] in (1,2,4,5,8,9,12,35,36,46) and not op['dest_modifier'] and not op['token']&0x10000000,'Unsupported skin dependency')
        require(len(op.get('sources',[]))=={1:1,2:2,4:3,5:2,8:2,9:2,12:2,35:1,36:1,46:1}[op['opcode']],'Source count')
        for c in lanes:needed.remove((op['dest'],c))
        dependencies=range(4) if op['opcode']==9 else range(3) if op['opcode'] in (8,36) else lanes
        for source in op['sources']:
            require(source['modifier'] in (0,1),'Source modifier')
            for lane in dependencies:
                key=(source['reg'],source['swizzle'][lane])
                if source['reg'][0]==1:external.add(key)
                elif source['reg'][0]!=2:needed.add(key)
            if source['relative']:
                a=source['relative'];require(a['reg']==(3,0) and not a['modifier'] and not a['relative'],'Address shape');needed.add((a['reg'],a['swizzle'][0]))
        selected.append(dict(op,used_components=lanes))
    require(not needed,'Unresolved skin dependency');selected.reverse()
    return selected,constants,outputs,external


def normal_slice(parsed):
    ops,_=parsed
    start=next(i for i,op in enumerate(ops) if op['opcode']==36)
    flex=next(i for i,op in enumerate(ops) if any(s['reg']==(1,7) for s in op.get('sources',[])))
    matches=[(i,op) for i,op in enumerate(ops[start+1:flex],start+1) if op['opcode']==5 and op['mask']==4]
    require(len(matches)==1,'Single decoded normal Z completion');end,op=matches[0]
    result=slice_ops(parsed,[op['dest']],end+1)
    require(result[3]=={((1,3),0),((1,3),1)},'Packed normal dependency domain')
    return result


def full_slice(parsed):
    result=slice_ops(parsed,[(6,7),(6,4)])
    require({reg for reg,_ in result[3]}=={(1,i) for i in [0,1,2,3,6,7]},'Full skin external register domain')
    return result


def execute(sliced,inputs,palette=None,scale=0,staticLight=1):
    operations,constants,outputs,_=sliced
    registers={(reg,c):float(values[c]) for reg,values in inputs.items() for c in range(len(values))}
    def read(s,lane):
        kind,index=s['reg'];component=s['swizzle'][lane]
        if s['relative']:
            a=s['relative'];index+=int(registers[(a['reg'],a['swizzle'][0])])
        if kind==2:
            if index==0:
                require(component in (0,1),'Unprovided c0 component');value=float(component)
            elif index==225:require(component==0,'Flex constant component');value=scale
            elif index==50:require(component==0,'Static light constant component');value=staticLight
            elif index in constants:value=constants[index][component]
            else:require(palette is not None and 58<=index<217,'Palette constant bounds');value=palette[(index-58)//3][(index-58)%3][component]
        else:value=registers[((kind,index),component)]
        return -value if s['modifier'] else value
    def dot(a,b):
        value=f32(a[0]*b[0])
        for x,y in zip(a[1:],b[1:]):value=f32(value+f32(x*y))
        return value
    for op in operations:
        updates={};code=op['opcode'];special=None
        if code in (8,9):
            count=3 if code==8 else 4;special=dot(*[[read(s,c) for c in range(count)] for s in op['sources']])
        elif code==36:
            n=[read(op['sources'][0],c) for c in range(3)];length=dot(n,n);require(length>0,'Reference normal degeneracy');reciprocal=f32(1/f32(math.sqrt(length)));special=[f32(v*reciprocal) for v in n]
        for c in op['used_components']:
            if code in (8,9):value=special
            elif code==36:value=special[c]
            else:
                args=[read(s,c) for s in op['sources']]
                if code==1:value=args[0]
                elif code==2:value=f32(args[0]+args[1])
                elif code==4:value=f32(f32(args[0]*args[1])+args[2])
                elif code==5:value=f32(args[0]*args[1])
                elif code==12:value=float(args[0]<args[1])
                elif code==35:value=abs(args[0])
                else:value=float(round(args[0]))
            updates[(op['dest'],c)]=value
        registers.update(updates)
    return [registers[(reg,c)] for reg in outputs for c in range(3)]


def model_contexts(mdl,vvd,vtx):
    m,v,t=map(Reader,[mdl,vvd,vtx]);require(m.u(8)==v.u(8)==t.u(16),'Benchmark checksums')
    ids=[]
    if v.u(48):
        for i in range(v.u(48)):
            at=v.u(52)+12*i;source,n=v.u(at+4),v.u(at+8);require(source+n<=v.u(16),'VVD fixup span');ids.extend(range(source,source+n))
    else:ids=list(range(v.u(16)))
    require(len(ids)==v.u(16),'Benchmark fixup count');vertices=[v.span(v.u(56)+48*i,48) for i in ids];metadata={}
    for bi,body in enumerate(m.table(0,232,236,16)):
        for mi,model in enumerate(m.table(body,4,12,148)):
            require(m.u(model+84)%48==0,'Model vertex alignment')
            for mei,mesh in enumerate(m.table(model,72,76,116)):metadata[bi,mi,mei]=(m.u(model+84)//48+m.u(mesh+12),m.u(mesh+8))
    result=[]
    for bi,body in enumerate(t.table(0,28,32,8)):
        for mi,model in enumerate(t.table(body,0,4,8)):
            lod=t.table(model,0,4,12)[0]
            for mei,mesh in enumerate(t.table(lod,0,4,9)):
                first,count=metadata.pop((bi,mi,mei))
                for group in t.table(mesh,0,4,33):
                    optimized=t.table(group,0,4,9);indices=t.table(group,8,12,2);mapping={}
                    for strip in t.table(group,16,20,35):
                        for at in t.table(strip,19,23,8):
                            slot,bone=struct.unpack('<2i',t.span(at,8));require(0<=slot<53 and 0<=bone<m.u(156),'Strip mapping bounds');mapping[slot]=bone
                        begin,length=t.u(strip+4),t.u(strip);require(begin+length<=len(indices),'Strip index span')
                        referenced=sorted({t.h(at) for at in indices[begin:begin+length]})
                        for vi in referenced:
                            require(vi<len(optimized),'Optimized vertex bounds');opt=t.span(optimized[vi],9);original=struct.unpack_from('<H',opt,4)[0];require(original<count,'Original mesh vertex');vertex=vertices[first+original]
                            for i in range(opt[3]):require(opt[i]<vertex[15] and mapping.get(opt[6+i])==vertex[12+opt[i]],'Model/strip palette agreement')
                            result.append((vertex,opt,mapping.copy()))
    require(not metadata,'Benchmark mesh coverage');return result


def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));before={str(STUDIO):sha(STUDIO),str(SOURCE):sha(SOURCE)}
    require(before[str(STUDIO)]==STUDIO_SHA and before[str(SOURCE)]=='0e31c41484207212b069524a079270f5f820059e7d5d4dadc3869bc194d1bd97','Original pins')
    image,evidence=check_ranges(STUDIO,'studiorender-disassembly.txt',[('complete_normal_packer',0x1000a100,0x1000a210)])
    require([image.word(a) for a in [0x100703d0,0x100702d8,0x10070270,0x10070290]]==[0x427c0000,0x3f000000,0x42800000,0x43000000],'Pack constants')
    slices={};normal_slices={}
    for dynamic,pin in SHADERS.items():
        shader=(SHADER_OUT/('generic-vs20-s0-d'+str(dynamic)+'.bin')).read_bytes();require(hashlib.sha256(shader).hexdigest()==pin,'Shader pin');parsed=parse(shader);slices[dynamic]=full_slice(parsed)
        if dynamic in (5,7):normal_slices[dynamic]=normal_slice(parsed)
    binaries=[]
    for tag,name,machine in [('native','pack_normal_native_x86',0x14c),('owned','pack_normal_owned_x64',0x8664),('skin','shader_vertex_skin_x64',0x8664)]:
        log=(OUT/(tag+'-build.log')).read_text();require('warning ' not in log.lower() and 'error ' not in log.lower(),'Build diagnostics '+tag);path=OUT/(name+'.exe');b=path.read_bytes();pe=struct.unpack_from('<I',b,60)[0]
        require(b[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==(0x10b if machine==0x14c else 0x20b),'Architecture');binaries.append(dict(path=str(path),sha256=sha(path),machine=hex(machine)))
    def invoke(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,name+' '+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout
    prior_dir=ROOT/'outputs/studio-vertex-weights-20261025/verification-f_onyd_0';prior=json.loads((prior_dir/'results.json').read_text());require(sha(prior_dir/'queries.bin')==prior['query_sha256'] and sha(prior_dir/'native_x86.bin')==prior['output_sha256'],'Prior native writer evidence')
    q=(prior_dir/'queries.bin').read_bytes();writer=(prior_dir/'native_x86.bin').read_bytes();writer_map={q[i*68:i*68+60]:writer[i*16:i*16+16] for i in range(prior['cases'])}
    palette_dir=ROOT/'outputs/studio-skin-palette-20261025/verification-sn3w4epv';pal_receipt=json.loads((palette_dir/'results.json').read_text());geometry=(palette_dir/'geometry.bin').read_bytes();require(hashlib.sha256(geometry).hexdigest()==pal_receipt['geometry_sha256'],'Prior skin palette geometry evidence');palettes={};at=0
    while at<len(geometry):
        tick,model,bones,count=struct.unpack_from('<4I',geometry,at);at+=16+bones*48;values=struct.unpack_from('<'+'f'*(bones*12),geometry,at);palettes[tick,bones]=[[list(values[i*12+r*4:i*12+r*4+4]) for r in range(3)] for i in range(bones)];at+=bones*48+count*96
    require(at==len(geometry) and len(palettes)==4,'Benchmark palette coverage')
    assets={str(Path(a['path'])):a['sha256'] for a in pal_receipt['assets']};contexts=[]
    for base in [Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/usermod/models/player/hwm/pyro'),Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/usermod/models/weapons/w_models/w_flamethrower')]:
        data=[]
        for ext in ['.mdl','.vvd','.dx90.vtx']:
            path=Path(str(base)+ext);before[str(path)]=sha(path);require(before[str(path)]==assets[str(path)],'Benchmark asset pin');data.append(path.read_bytes())
        bones=struct.unpack_from('<I',data[0],156)[0];contexts.extend((bones,*row) for row in model_contexts(*data))
    normals=[];rng=random.Random(2026102510)
    for axis in range(3):
        for sign in [-1.0,1.0]:n=[0.0]*3;n[axis]=sign;normals.append(struct.pack('<3f',*n))
    for _ in range(2048):
        n=[rng.uniform(-1,1) for _ in range(3)];length=math.sqrt(sum(v*v for v in n));normals.append(struct.pack('<3f',*[v/length for v in n]))
    normals.extend(vertex[28:40] for _,vertex,_,_ in contexts);pack_query=run/'pack-queries.bin';pack_query.write_bytes(b''.join(normals));packed_outputs=[]
    for tag in ['native_x86','owned_x64']:
        path=run/(tag+'.bin');invoke('pack_normal_'+tag,[STUDIO,pack_query,path],tag);packed_outputs.append(path.read_bytes())
    require(packed_outputs[0]==packed_outputs[1] and len(packed_outputs[0])==len(normals)*4,'Native pack differs');packed_map={n:packed_outputs[0][i*4:i*4+4] for i,n in enumerate(normals)}
    decode_query=run/'normal-token-reference.bin'
    with decode_query.open('wb') as f:
        for dynamic in [5,7]:
            for code in range(65536):
                packed=struct.pack('<I',code);expected=execute(normal_slices[dynamic],{(1,3):list(packed)})
                f.write(packed+struct.pack('<3f',*expected))
    skin_query=run/'skin-token-reference.bin';skin_count=0;benchmark_count=0
    def record(f,dynamic,palette,indices,wdata,ndata,p,pd,nd,scale,staticLight):
        nonlocal skin_count
        compressed=dynamic&1
        w=list(struct.unpack('<2h' if compressed else '<2f',wdata[:4] if compressed else wdata))
        n=list(ndata[:4]) if compressed else list(struct.unpack('<3f',ndata))
        inputs={(1,0):[*p,1.0],(1,1):[*w,0,0],(1,2):[f32(indices[2]/255),f32(indices[1]/255),f32(indices[0]/255),0],(1,3):n,(1,6):[*pd,0],(1,7):[*nd,0]}
        expected=execute(slices[dynamic],inputs,palette,scale,staticLight);selected=[v for i in indices for row in palette[i] for v in row]
        f.write(struct.pack('<I36f',dynamic,*selected)+wdata+ndata+struct.pack('<11f',*p,*pd,*nd,scale,staticLight)+struct.pack('<6f',*expected));skin_count+=1
    with skin_query.open('wb') as f:
        for _ in range(256):
            palette=[]
            for _ in range(53):
                # Invertible near-identity matrices avoid undefined zero NRM.
                palette.append([[f32((1.0 if r==c else 0)+rng.uniform(-0.1,0.1)) if c<3 else f32(rng.uniform(-5000,5000)) for c in range(4)] for r in range(3)])
            indices=[rng.randrange(53) for _ in range(3)];weights=[f32(rng.uniform(0,0.5)) for _ in range(2)];p=[f32(rng.uniform(-100,100)) for _ in range(3)];pd=[f32(rng.uniform(-1,1)) for _ in range(3)];nd=[f32(rng.uniform(-0.1,0.1)) for _ in range(3)];scale=f32(rng.uniform(-1,1));normal=normals[rng.randrange(2054)]
            for dynamic in [4,5,6,7]:
                if dynamic&1:
                    a=int(weights[0]*32768)-1;b=int(weights[1]*32768)-1;w=struct.pack('<2H',a&65535,b&65535)+b'\xa5'*4;n=packed_map[normal]+b'\xa5'*8
                else:w=struct.pack('<2f',*weights);n=normal
                record(f,dynamic,palette,indices,w,n,p,pd,nd,scale,rng.choice([-1.0,0.0,0.5,1.0,f32(1e-30)]))
        for tick in [50000,50417]:
            for bones,vertex,opt,mapping in contexts:
                source_palette=palettes[tick,bones];palette=[source_palette[mapping[i]] if i in mapping else [[0.0]*4 for _ in range(3)] for i in range(53)];p=struct.unpack_from('<3f',vertex,16)
                for dynamic in [4,5,6,7]:
                    key=vertex+opt+bytes([dynamic&1,2,0]);require(key in writer_map,'Prior native writer query coverage');row=writer_map[key];require(row[12]==1,'Invalid benchmark weights');indices=list(row[8:11]);require(all(i in mapping for i in indices),'Unbound shader slot')
                    n=packed_map[vertex[28:40]]+b'\xa5'*8 if dynamic&1 else vertex[28:40]
                    record(f,dynamic,palette,indices,row[:8],n,p,[0,0,0],[0,0,0],0.0,1.0);benchmark_count+=1
    log=invoke('shader_vertex_skin_x64',[decode_query,skin_query],'skin');require(f'normal_decode_cases=131072 skin_vertex_cases={skin_count}' in log and 'checks=8 refusals=11' in log,'Skin test coverage')
    for p,digest in before.items():require(sha(p)==digest,'Original changed')
    tracked=['generic_packed_normal_candidate.h','generic_shader_vertex_skin_candidate.h','probe_generic_pack_normal.cpp','test_generic_shader_vertex_skin.cpp','build_generic_shader_vertex_skin.cmd','verify_generic_shader_vertex_skin.py']
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),pack_normal_cases=len(normals),exact_native_pack_bytes=len(packed_outputs[0]),normal_decode_cases=131072,exact_decoded_normal_bytes=131072*12,
        skin_vertex_cases=skin_count,benchmark_skin_cases=benchmark_count,benchmark_strip_vertices=len(contexts),exact_world_position_normal_bytes=skin_count*24,analytical=8,guard_refusals=11,
        executables=binaries,sources_sha256={name:sha(ROOT/'work'/name) for name in tracked},originals_sha256=before,originals_unchanged=True,
        shader_token_slices={str(d):[dict(offset_words=o['offset_words'],opcode=o['opcode'],components=o['used_components']) for o in slices[d][0]] for d in slices},static_ranges=evidence,
        prior_writer_receipt=str(prior_dir/'results.json'),prior_palette_receipt=str(palette_dir/'results.json'),computer_use=False,sfm_process_used=False,gpu_used=False,editor_changed=False,
        limitations='Native normal pack exact. Token skin/normal reference uses separate binary32 MAD, left-associated DP3/4, float sqrt/reciprocal NRM. Four explicit static0/dynamic4..7 variants, selected matrices/declared conversions; active runtime variant, GPU rounding, native strip uploads and full renderer parity unverified. Benchmark uses retained verified skin palettes at frames120/121, zero flex deltas and explicit staticLightEnabled=1; synthetic cases include nonzero flex and static-light zero/negative/underflow predicates. Tangents/material shading not implemented here.')
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(status='PASS',receipt=str(run/'results.json'),pack_normal_cases=len(normals),normal_decode_cases=131072,skin_vertex_cases=skin_count,benchmark_skin_cases=benchmark_count)))


if __name__=='__main__':main()
