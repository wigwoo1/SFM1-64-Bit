"""Installed Phong VS2/VS3 stream-flex vertex contract, entirely offline.

The checked token interpreter is an independent CPU reference, NOT D3D GPU
execution. It refuses conditional dependencies and texture-morph profiles.
Private shader extracts/benchmark-derived fixtures must never be published.
"""
from datetime import datetime,timezone
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from vcs6_shader_cache_candidate import Vcs6ShaderCache
from verify_generic_ambient_irradiance import reg,source_operand,f32
from verify_generic_shader_vertex_skin import model_contexts

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/phong-vertex-contract-20261025'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
SOURCES={20:('phong_vs20.vcs','7998c95ed976687c6fe811ca0555e748fc9f1aa76e54a3a70412350f9942ce1c'),30:('phong_vs30.vcs','1df0572de0d6df583b40aaf192a027174392679a7780bd85d561355c1fa7dc0e')}
ARITY={1:1,2:2,4:3,5:2,7:1,8:2,9:2,12:2,35:1,36:1,46:1}

def require(value,message):
    if not value:raise RuntimeError(message)

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def parse(shader):
    require(len(shader)>=8 and len(shader)%4==0,'Shader framing')
    words=struct.unpack('<'+'I'*(len(shader)//4),shader)
    require(words[0] in (0xfffe0200,0xfffe0300),'VS2/VS3 required')
    ops=[];constants={};inputs={};outputs={};samplers=[];at=1;control=[]
    while at<len(words):
        start=at;token=words[at];code=token&65535
        if code==65535:
            require(at==len(words)-1 and not control,'Shader END/control framing')
            return dict(ops=ops,constants=constants,inputs=inputs,outputs=outputs,samplers=samplers,version=words[0])
        if code==65534:
            at+=1+((token>>16)&32767);require(at<=len(words),'Shader comment bounds');continue
        length=(token>>24)&15;require(at+length<len(words),'Shader instruction bounds');raw=words[at+1:at+1+length]
        if code in (29,39,43):
            require(control and control[-1]=={29:27,39:38,43:40}[code] and length==0,'Control terminator')
            control.pop()
        if code==42:require(control and control[-1]==40 and length==0,'ELSE framing')
        item=dict(offset_words=start,opcode=code,token=token,depth=len(control),dest=None,mask=0,sources=[])
        if code==31:
            require(length==2 and reg(raw[1])[0] in (1,6,10),'DCL shape')
            if reg(raw[1])[0]==10:
                require(reg(raw[1]) not in samplers,'Duplicate sampler');samplers.append(reg(raw[1]))
            else:
                semantic=(raw[0]&31,(raw[0]>>16)&15);table=inputs if reg(raw[1])[0]==1 else outputs
                require(semantic not in table,'Duplicate DCL');table[semantic]=reg(raw[1])
        elif code==81:
            require(length==5 and reg(raw[0])[0]==2,'DEF shape');index=reg(raw[0])[1]
            require(index not in constants,'Duplicate DEF');constants[index]=struct.unpack('<4f',struct.pack('<4I',*raw[1:]))
        elif code not in (0,25,26,27,28,29,30,38,39,40,41,42,43,44,45,47,48):
            require(length>0,'Missing destination')
            item.update(dest=reg(raw[0]),mask=(raw[0]>>16)&15,dest_modifier=(raw[0]>>20)&15,dest_shift=(raw[0]>>24)&15)
            if code in ARITY:
                cursor=1
                while cursor<len(raw):
                    operand=raw[cursor];cursor+=1;relative=None
                    if operand&0x2000:
                        require(cursor<len(raw),'Relative operand bounds');relative=source_operand(raw[cursor]);cursor+=1
                    item['sources'].append(source_operand(operand,relative))
                require(len(item['sources'])==ARITY[code],'Arithmetic operand count')
        ops.append(item)
        if code in (27,38,40,41):control.append(40 if code==41 else code)
        at+=1+length
    raise RuntimeError('Missing END')

def slice_points(parsed,points,end=None):
    needed=set(points);selected=[];external=set()
    ops=parsed['ops'][:end] if end is not None else parsed['ops']
    for op in reversed(ops):
        lanes=[c for c in range(4) if op['mask']&(1<<c) and (op['dest'],c) in needed]
        if not lanes:continue
        require(op['opcode'] in ARITY and not op['depth'] and not op['dest_modifier'] and not op['dest_shift'] and not op['token']&0x10000000,'Unsupported or conditional selected dependency')
        for c in lanes:needed.remove((op['dest'],c))
        dependencies=range(4) if op['opcode']==9 else range(3) if op['opcode'] in (8,36) else [0] if op['opcode']==7 else lanes
        for s in op['sources']:
            require(s['modifier'] in (0,1,11,12),'Unsupported source modifier')
            for lane in dependencies:
                key=(s['reg'],s['swizzle'][lane])
                if s['reg'][0]==1:external.add(key)
                elif s['reg'][0]!=2:needed.add(key)
            if s['relative']:
                a=s['relative'];require(a['reg']==(3,0) and not a['modifier'] and not a['relative'],'Address shape');needed.add((a['reg'],a['swizzle'][0]))
        selected.append(dict(op,used_components=lanes))
    require(not needed,'Unresolved selected dependency');selected.reverse()
    return dict(ops=selected,constants=parsed['constants'],points=points,external=external)

def profile(shader,version,compressed):
    parsed=parse(shader)
    require(parsed['version']==(0xfffe0200 if version==20 else 0xfffe0300),'Profile version')
    inputs=parsed['inputs'];require(all((u,i) in inputs for u,i in [(0,0),(1,0),(2,0),(3,0),(0,1),(3,1)]),'Stream-flex inputs required; texture morph is unsupported')
    require(((6,0) not in inputs)==compressed,'Compressed tangent input layout')
    output_regs=[(6,6),(6,4),(6,2),(6,3)] if version==20 else [parsed['outputs'][(5,i)] for i in [6,4,2,3]]
    full=slice_points(parsed,[(r,c) for r in output_regs for c in range(3)])
    expected_regs={inputs[s] for s in [(0,0),(1,0),(2,0),(3,0),(0,1),(3,1)]}
    if not compressed:expected_regs.add(inputs[6,0])
    require({r for r,c in full['external']}==expected_regs,'Vertex input dependency coverage')
    decoded=[]
    if compressed:
        for output in output_regs[1:3]:
            one=slice_points(parsed,[(output,c) for c in range(3)])
            flex=[op for op in one['ops'] if op['opcode']==4 and any(s['reg']==inputs[3,1] for s in op['sources']) and any(s['reg']==(2,225) for s in op['sources'])]
            require(len(flex)==1,'Single frame direction flex input')
            op=flex[0];base=op['sources'][2];require(base['modifier']==0 and not base['relative'],'Decoded frame source')
            end=next(i for i,p in enumerate(parsed['ops']) if p['offset_words']==op['offset_words'])
            decoded.append(slice_points(parsed,[(base['reg'],base['swizzle'][c]) for c in range(3)],end))
        require(all(s['external']<=set((inputs[3,0],c) for c in range(4)) for s in decoded),'Decoder input domain')
    return dict(parsed=parsed,full=full,decoded=decoded)

def execute(sliced,inputs,palette=None,scale=0):
    registers={(r,c):float(v) for r,values in inputs.items() for c,v in enumerate(values)}
    def read(s,lane):
        kind,index=s['reg'];component=s['swizzle'][lane]
        if s['relative']:
            a=s['relative'];index+=int(registers[(a['reg'],a['swizzle'][0])])
        if kind==2:
            if index==0:require(component<3,'Unavailable c0 component');value=float(component)
            elif index==225:require(component==0,'Unavailable flex component');value=scale
            elif index in sliced['constants']:value=sliced['constants'][index][component]
            else:require(palette is not None and 58<=index<217,'Palette bounds');value=palette[(index-58)//3][(index-58)%3][component]
        else:value=registers[((kind,index),component)]
        return {0:lambda:value,1:lambda:-value,11:lambda:abs(value),12:lambda:-abs(value)}[s['modifier']]()
    def dot(a,b):
        v=f32(a[0]*b[0])
        for x,y in zip(a[1:],b[1:]):v=f32(v+f32(x*y))
        return v
    for op in sliced['ops']:
        code=op['opcode'];special=None;updates={}
        if code in (8,9):special=dot(*[[read(s,c) for c in range(3 if code==8 else 4)] for s in op['sources']])
        elif code==7:
            v=abs(read(op['sources'][0],0));require(v>0 and math.isfinite(v),'Reference RSQ domain');special=f32(1/f32(math.sqrt(v)))
        elif code==36:
            n=[read(op['sources'][0],c) for c in range(3)];v=dot(n,n);require(v>0 and math.isfinite(v),'Reference NRM domain');inverse=f32(1/f32(math.sqrt(v)));special=[f32(x*inverse) for x in n]
        for c in op['used_components']:
            if code in (7,8,9):value=special
            elif code==36:value=special[c]
            else:
                args=[read(s,c) for s in op['sources']]
                if code==1:value=args[0]
                elif code==2:value=f32(args[0]+args[1])
                elif code==4:value=f32(f32(args[0]*args[1])+args[2])
                elif code==5:value=f32(args[0]*args[1])
                elif code==12:value=float(args[0]<args[1])
                elif code==35:value=abs(args[0])
                else:
                    require(math.isfinite(args[0]) and abs(args[0])<160,'MOVA index domain');value=float(round(args[0]))
            require(math.isfinite(value),'Nonfinite reference result');updates[(op['dest'],c)]=value
        registers.update(updates)
    return [registers[p] for p in sliced['points']]

def load_profiles(run,before):
    profiles={};inventory={}
    for version,(filename,pin) in SOURCES.items():
        path=GAME/'platform/shaders/fxc'/filename;before[str(path)]=sha(path);require(before[str(path)]==pin,'Original shader cache pin')
        cache=Vcs6ShaderCache(path.read_bytes());inventory[str(version)]=dict(static_count=cache.static_count,dynamic_count=cache.dynamic_combos,aliases=cache.aliases,primary_dynamic_ids={str(s):sorted(d) for s,d in cache.payloads.items()})
        for dynamic in [2,3]:
            shader=cache.shader(0,dynamic);target=run/f'phong_vs{version}-s0-d{dynamic}.bin';target.write_bytes(shader)
            profiles[version,dynamic]=dict(profile(shader,version,bool(dynamic&1)),sha256=sha(target),path=str(target))
        if version==30:
            for dynamic in [6,7]:
                shader=cache.shader(0,dynamic);parsed=parse(shader);require((0,2) in parsed['inputs'] and (0,1) not in parsed['inputs'] and any(op['opcode']==95 for op in parsed['ops']),'Texture-morph profile evidence')
                try:profile(shader,30,bool(dynamic&1))
                except RuntimeError as e:require('Stream-flex inputs' in str(e),'Wrong morph refusal')
                else:raise RuntimeError('Texture-morph profile wrongly admitted')
    return profiles,inventory

def parser_guards(profiles):
    shader=Path(profiles[20,2]['path']).read_bytes();checks=0
    for bad in [shader[:4],shader+b'\0\0\0\0',b'\0\0\0\0'+shader[4:]]:
        try:parse(bad)
        except RuntimeError:checks+=1
        else:raise RuntimeError('Malformed shader admitted')
    # A selected conditional write must fail, even if its arithmetic is simple.
    fake=dict(ops=[dict(dest=(0,0),mask=1,opcode=1,depth=1,dest_modifier=0,dest_shift=0,token=0,sources=[])],constants={})
    try:slice_points(fake,[((0,0),0)])
    except RuntimeError as e:require('conditional' in str(e),'Conditional dependency guard');checks+=1
    else:raise RuntimeError('Conditional dependency flattened')
    return checks

def benchmark_contexts(before):
    palette_dir=ROOT/'outputs/studio-skin-palette-20261025/verification-sn3w4epv';receipt=json.loads((palette_dir/'results.json').read_text());assets={str(Path(a['path'])):a['sha256'] for a in receipt['assets']}
    geometry=(palette_dir/'geometry.bin').read_bytes();require(hashlib.sha256(geometry).hexdigest()==receipt['geometry_sha256'],'Retained skin palette pin');palettes={};at=0
    while at<len(geometry):
        tick,model,bones,count=struct.unpack_from('<4I',geometry,at);at+=16+bones*48;values=struct.unpack_from('<'+'f'*(bones*12),geometry,at)
        palettes[tick,bones]=[[list(values[i*12+r*4:i*12+r*4+4]) for r in range(3)] for i in range(bones)];at+=bones*48+count*96
    require(at==len(geometry) and len(palettes)==4,'Palette framing')
    prior=ROOT/'outputs/studio-vertex-weights-20261025/verification-f_onyd_0';wr=json.loads((prior/'results.json').read_text())
    require(sha(prior/'queries.bin')==wr['query_sha256'] and sha(prior/'native_x86.bin')==wr['output_sha256'],'Retained native weight writer pins')
    q=(prior/'queries.bin').read_bytes();b=(prior/'native_x86.bin').read_bytes();writer={q[i*68:i*68+60]:b[i*16:i*16+16] for i in range(wr['cases'])};contexts=[]
    for stem in [GAME/'usermod/models/player/hwm/pyro',GAME/'usermod/models/weapons/w_models/w_flamethrower']:
        data=[]
        for ext in ['.mdl','.vvd','.dx90.vtx']:
            path=Path(str(stem)+ext);before[str(path)]=sha(path);require(before[str(path)]==assets[str(path)],'Benchmark asset pin');data.append(path.read_bytes())
        mdl,vvd,vtx=data;header=struct.unpack_from('<16I',vvd);lookup={}
        require(header[15]+header[4]*16<=len(vvd),'VVD tangent span')
        for i in range(header[4]):lookup.setdefault(vvd[header[14]+48*i:header[14]+48*i+48],set()).add(vvd[header[15]+16*i:header[15]+16*i+16])
        bones=struct.unpack_from('<I',mdl,156)[0]
        for vertex,opt,mapping in model_contexts(mdl,vvd,vtx):
            require(len(lookup[vertex])==1,'Ambiguous source tangent; do not choose arbitrarily');tangent=next(iter(lookup[vertex]));require(struct.unpack('<4f',tangent)[3] in (-1,1),'Tangent handedness')
            key=vertex+opt+bytes([0,2,0]);require(key in writer,'Retained weight coverage');row=writer[key];require(row[12]==1 and all(i in mapping for i in row[8:11]),'Unbound benchmark shader slot')
            contexts.append((bones,vertex,tangent,mapping,row))
    return contexts,palettes,dict(palette_receipt=str(palette_dir/'results.json'),writer_receipt=str(prior/'results.json'))

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));before={};profiles,inventory=load_profiles(run,before);guards=parser_guards(profiles)
    exe=OUT/'phong_vertex_skin_x64.exe';log=(OUT/'build.log').read_text();require('warning ' not in log.lower() and 'error ' not in log.lower(),'Build diagnostics')
    binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(binary[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',binary,pe+4)[0]==0x8664 and struct.unpack_from('<H',binary,pe+24)[0]==0x20b,'Native x64 PE')
    decoder=run/'decode-token-reference.bin';decode_count=0
    with decoder.open('wb') as f:
        for version in [20,30]:
            p=profiles[version,3];normal_reg=p['parsed']['inputs'][3,0]
            for code in range(65536):
                packed=struct.pack('<HH',code,(code*40503+17321)&65535)
                expected=[v for s in p['decoded'] for v in execute(s,{normal_reg:list(packed)})]
                f.write(packed+struct.pack('<6f',*expected));decode_count+=1
    print('Packed frame reference cases='+str(decode_count),flush=True)
    contexts,palettes,prior=benchmark_contexts(before);query=run/'vertex-token-reference.bin';cases=0;benchmark=0;benchmark_refusals=0;degenerate=0;rng=random.Random(2026102512)
    def record(f,version,dynamic,palette,indices,weights,normal,tangent,position,pdelta,ddelta,scale):
        nonlocal cases,degenerate
        p=profiles[version,dynamic];inputs=p['parsed']['inputs'];compressed=dynamic&1
        w=list(struct.unpack('<2h' if compressed else '<2f',weights[:4] if compressed else weights))
        n=list(normal[:4]) if compressed else list(struct.unpack('<3f',normal))
        values={inputs[0,0]:[*position,1],inputs[1,0]:[*w,0,0],inputs[2,0]:[f32(indices[2]/255),f32(indices[1]/255),f32(indices[0]/255),0],inputs[3,0]:n,inputs[0,1]:[*pdelta,0],inputs[3,1]:[*ddelta,0]}
        if not compressed:values[inputs[6,0]]=list(struct.unpack('<4f',tangent))
        profile_id=(0 if version==20 else 2)+(dynamic&1)
        try:expected=execute(p['full'],values,palette,scale)
        except RuntimeError as e:
            # Demand an independently identified undefined input, not an
            # arbitrary reference exception. Native tangent repair is unproven.
            require(str(e) in ('Reference NRM domain','Reference RSQ domain'),'Unexpected reference failure')
            if compressed:
                decoded=[execute(s,{inputs[3,0]:n}) for s in p['decoded']]
                require(decoded[0]==decoded[1],'Undefined packed frame must have identical decoded N/T')
            else:
                require(struct.unpack('<4f',tangent)[:3]==(0,0,0) and all(f32(x*scale)==0 for x in ddelta),'Undefined raw frame must have zero tangent and zero flex')
            expected=[0.0]*12;profile_id|=0x100;degenerate+=1
        else:cases+=1
        selected=[v for i in indices for row in palette[i] for v in row]
        f.write(struct.pack('<I36f',profile_id,*selected)+weights+normal+tangent+struct.pack('<10f',*position,*pdelta,*ddelta,scale)+struct.pack('<12f',*expected))
        return not profile_id&0x100
    with query.open('wb') as f:
        # Explicitly supplied packed input bytes, not a guessed native tangent
        # quantizer. Sign quadrants/boundary byte values occur in these fixtures.
        for i in range(1024):
            palette=[[[f32((1 if r==c else 0)+rng.uniform(-0.08,0.08)) if c<3 else f32(rng.uniform(-1000,1000)) for c in range(4)] for r in range(3)] for _ in range(53)]
            indices=[rng.randrange(53) for _ in range(3)];w=[f32(rng.uniform(0,0.5)) for _ in range(2)]
            position=[f32(rng.uniform(-100,100)) for _ in range(3)];pd=[f32(rng.uniform(-1,1)) for _ in range(3)];dd=[f32(rng.uniform(-0.08,0.08)) for _ in range(3)];scale=f32(rng.uniform(-2,2))
            for version in [20,30]:
                for dynamic in [2,3]:
                    if dynamic&1:
                        weights=struct.pack('<2H',*(int(x*32768)-1 for x in w))+b'\xa5'*4
                        packed=bytes([i&255,(i>>2)&255,(i*37)&255,(i*83)&255]);normal=packed+b'\xa5'*8;tangent=b'\xa5'*16
                    else:
                        weights=struct.pack('<2f',*w);normal=struct.pack('<3f',0.1,0.05,1);tangent=struct.pack('<4f',1,-0.02,0.07,-1 if i&1 else 1)
                    record(f,version,dynamic,palette,indices,weights,normal,tangent,position,pd,dd,scale)
        for tick in [50000,50417]:
            for bones,vertex,tangent,mapping,row in contexts:
                source=palettes[tick,bones];palette=[source[mapping[i]] if i in mapping else [[0.0]*4 for _ in range(3)] for i in range(53)]
                for version in [20,30]:
                    accepted=record(f,version,2,palette,list(row[8:11]),row[:8],vertex[28:40],tangent,struct.unpack_from('<3f',vertex,16),[0,0,0],[0,0,0],0)
                    benchmark+=accepted;benchmark_refusals+=not accepted
    print('Vertex frame reference cases='+str(cases),flush=True)
    result=subprocess.run([str(exe),str(decoder),str(query)],cwd=ROOT,capture_output=True,text=True,timeout=120);(run/'test.log').write_text(result.stdout+result.stderr)
    require(result.returncode==0,'Owned x64 test failure: '+result.stdout+result.stderr)
    require(f'packed_frame_cases={decode_count} vertex_frame_cases={cases} degenerate_frame_refusals={degenerate}' in result.stdout and 'checks=6 refusals=12' in result.stdout,'Test coverage')
    for path,digest in before.items():require(sha(path)==digest,'Original changed')
    tracked=['phong_shader_vertex_skin_candidate.h','test_phong_shader_vertex_skin.cpp','build_phong_shader_vertex_skin.cmd','verify_phong_shader_vertex_skin.py']
    receipt=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),packed_frame_cases=decode_count,exact_token_decoded_frame_bytes=decode_count*24,vertex_frame_cases=cases,benchmark_vertex_cases=benchmark,benchmark_strip_vertices=len(contexts),benchmark_zero_tangent_vertices=sum(struct.unpack('<4f',t)[:3]==(0,0,0) for _,v,t,m,r in contexts),benchmark_zero_tangent_refusals=benchmark_refusals,exact_token_world_frame_bytes=cases*48,analytical_checks=6,api_refusals=12,degenerate_frame_refusals=degenerate,parser_guard_refusals=guards,texture_morph_profile_refusals=2,
        executable=dict(path=str(exe),sha256=sha(exe),machine='AMD64',format='PE32+'),build_log=str(OUT/'build.log'),test_log=str(run/'test.log'),sources_sha256={name:sha(ROOT/'work'/name) for name in tracked},originals_sha256=before,originals_unchanged=True,
        decoder_fixture_sha256=sha(decoder),vertex_fixture_sha256=sha(query),cache_inventory=inventory,profiles={f'vs{v}-s0-d{d}':dict(path=p['path'],sha256=p['sha256'],selected_operations=[dict(offset_words=o['offset_words'],opcode=o['opcode'],components=o['used_components']) for o in p['full']['ops']],conditional_operations_in_full_shader=sum(o['depth']>0 for o in p['parsed']['ops']),selected_conditional_operations=0) for (v,d),p in profiles.items()},prior_evidence=prior,
        computer_use=False,sfm_process_used=False,gpu_used=False,editor_changed=False,
        limitations='Only explicit static0/dynamic2,3 stream-flex profiles from installed phong_vs20/vs30. Separate binary32 MAD, left-associated DP3/4, float sqrt/reciprocal NRM/RSQ CPU convention; GPU precision unverified. Decoding covers all 65536 patterns in each packed half per profile, paired by a bijection; not all 2^32 combinations. Synthetic compressed inputs supplied directly; native tangent packing is not implemented/proven. Benchmark uses actual uncompressed VVD tangent frames, retained verified skin matrices at frames120/121 and zero flex. Seven referenced vertices have zero stored tangents; these are refused in both profiles/frames, not repaired speculatively. Native handling/repair remains unverified. Actual effective material/active shader selection, native declaration/upload selection, texture-driven morphs, full lighting/material shading and viewport integration remain UNVERIFIED.')
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status='PASS',receipt=str(run/'results.json'),packed_frame_cases=decode_count,vertex_frame_cases=cases,benchmark_vertex_cases=benchmark)))

if __name__=='__main__':main()
