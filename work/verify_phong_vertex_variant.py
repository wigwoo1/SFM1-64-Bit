"""Offline Phong constructor/selector and compiled vertex-profile contracts.

Constructor results are actual native execution. Inline selector expressions
are independently replayed from checked installed x86 bytes, not claimed as
whole-native-draw execution. Shader arithmetic remains a CPU token reference.
"""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,itertools,json,random,struct,subprocess,tempfile
from verify_ifm_draw_packet import check_ranges
from vcs6_shader_cache_candidate import Vcs6ShaderCache
import verify_phong_shader_vertex_skin as phong
from verify_generic_ambient_irradiance import f32

ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/phong-vertex-variants-20261025'
STD=phong.GAME/'bin/stdshader_dx9.dll';STD_SHA='fb09e2ddc8a99aff4ef31e3fddfdb1ca84d89379336abdab4e76e94adfa7e6f8'
RANGES=[('VS20_static_defaults',0x10037690,0x100376ee),('VS30_static_defaults',0x10037750,0x100377a8),('VS30_dynamic_defaults',0x10054370,0x100543bc),
 ('VS20_static_inline',0x10038738,0x10038752),('VS30_static_inline',0x10038b85,0x10038ba5),('VS20_dynamic_inline',0x1003a8a7,0x1003a8c6),('VS30_dynamic_inline',0x1003ac0a,0x1003ac2c),
 ('VS20_static_call',0x10038752,0x1003875c),('VS30_static_call',0x10038ba5,0x10038baf),('VS20_dynamic_bones_and_fog',0x1003a7df,0x1003a80a),('VS20_light_suppression',0x1003a86c,0x1003a88b),
 ('VS30_skinning_gate',0x1003ab33,0x1003ab5d),('VS30_compression_gate',0x1003ab8d,0x1003abb3)]
def require(b,s):
    if not b:raise RuntimeError(s)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def signed(x):return struct.unpack('<i',struct.pack('<I',x&0xffffffff))[0]

def replay(code,registers,memory,output):
    """Small checked decoder for this MOV/LEA/ADD/SHL integer-only domain."""
    regs=list(registers);at=0;count=0
    def byte():
        nonlocal at
        require(at<len(code),'Microcode byte bounds');v=code[at];at+=1;return v
    def dword():return sum(byte()<<(8*i) for i in range(4))
    def operand(mod,rm):
        if mod==3:return True,rm
        value=0
        if rm==4:
            sib=byte();base=sib&7;index=(sib>>3)&7;scale=1<<(sib>>6)
            if index!=4:value=regs[index]*scale
            value+=dword() if mod==0 and base==5 else regs[base]
        elif mod==0 and rm==5:value=dword()
        else:value=regs[rm]
        if mod==1:
            n=byte();value+=n-256 if n&128 else n
        elif mod==2:value+=signed(dword())
        return False,value&0xffffffff
    while at<len(code):
        count+=1;require(count<=64,'Microcode step bound');opcode=byte();require(opcode in (0x8b,0x8d,0x03,0xc1),'Unsupported selector opcode')
        modrm=byte();mod=modrm>>6;dest=(modrm>>3)&7;is_reg,address=operand(mod,modrm&7)
        if opcode==0x8d:require(not is_reg,'LEA register operand');regs[dest]=address
        elif opcode==0xc1:
            require(is_reg and dest==4,'Only SHL register is admitted');regs[address]=(regs[address]<<(byte()&31))&0xffffffff
        else:
            require(is_reg or address in memory,'Unprovided selector memory');value=regs[address] if is_reg else memory[address]
            regs[dest]=(value if opcode==0x8b else regs[dest]+value)&0xffffffff
    return regs[output]

def selector_expected(kind,args,programs):
    # Register numbering follows IA32: EAX ECX EDX EBX ESP EBP ESI EDI.
    frame=0x200000;api=0x300000;vtable=0x400000;regs=[0]*8;regs[5]=frame
    mem={api:vtable,vtable+0x44:0x500000,vtable+0x2c:0x500000};a,b,c,d,e=args
    if kind==0:
        mem[frame-0x50]=a;mem[frame-0x4c]=b;regs[6]=c;regs[7]=api;out=1
    elif kind==1:
        mem[frame-0x50]=a;mem[frame-0x4c]=0;mem[frame-0x490]=c;regs[6]=b;regs[7]=api;out=1
    elif kind==2:
        mem[frame-0x498]=0 if d else c;mem[frame-0x474]=int(signed(b)>0);mem[frame+0x1c]=a;regs[7]=int(signed(e)==2);regs[6]=api;out=1
    else:
        mem[frame-0x34]=int(signed(e)==2);mem[frame-0x464]=d;mem[frame-0x3c]=c;mem[frame-0x40]=int(signed(b)>0 and d==0);regs[7]=int(a!=0 and d==0);regs[6]=api;out=0
    return replay(programs[kind],regs,mem,out)

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));before={str(STD):sha(STD)};require(before[str(STD)]==STD_SHA,'Installed stdshader pin')
    image,evidence=check_ranges(STD,'stdshader_dx9-disassembly.txt',RANGES)
    programs=[image.b[image.offset(start):image.offset(end)] for name,start,end in RANGES[3:7]]
    names={hex(a):image.string(a) for a in [0x100966b4,0x100964dc,0x1008d0e8,0x100964e8,0x1008b268,0x1008b21c,0x1008f4fc,0x1008c1f8,0x1008f41c,0x1008f430,0x10096488]}
    require(names['0x100966b4']=='phong_vs20' and names['0x100964dc']=='phong_vs30' and names['0x1008f4fc']=='TESSELLATION','Selector label pins')
    binaries=[]
    for tag,name,machine in [('native','phong_defaults_native_x86',0x14c),('owned','phong_defaults_owned_x64',0x8664),('selectors','phong_selectors_x64',0x8664),('skin','phong_variant_skin_x64',0x8664)]:
        log=(OUT/(tag+'-build.log')).read_text();require('warning ' not in log.lower() and 'error ' not in log.lower(),'Build diagnostics '+tag)
        path=OUT/(name+'.exe');b=path.read_bytes();pe=struct.unpack_from('<I',b,60)[0];require(b[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==(0x10b if machine==0x14c else 0x20b),'Executable architecture');binaries.append(dict(path=str(path),sha256=sha(path),machine=hex(machine)))
    def invoke(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,name+' failed '+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout
    rng=random.Random(2026102514);queries=[];levels=[0,79,91,92,93,100,-1,-2147483648,2147483647]
    for kind,first,second,flags in itertools.product(range(2),levels,levels,[0,1,0x80000000,0xffffffff]):queries.append((kind,first&0xffffffff,second&0xffffffff,flags,0,0))
    for morph,fog in itertools.product(range(256),[-1,0,1,2,3,255,2147483647]):queries.append((2,0,0,0,morph,fog&0xffffffff))
    for _ in range(2048):queries.append((rng.randrange(3),*[rng.getrandbits(32) for _ in range(5)]))
    query=run/'defaults-queries.bin';query.write_bytes(b''.join(struct.pack('<6I',*q) for q in queries));results=[]
    for tag in ['native_x86','owned_x64']:
        target=run/(tag+'.bin');invoke('phong_defaults_'+tag,[STD,query,target],tag);results.append(target.read_bytes())
    require(len(results[0])==len(queries)*32 and results[0]==results[1],'Native constructor defaults/query count parity')
    print('Native constructor cases='+str(len(queries)),flush=True)
    selector_query=run/'selector-byte-reference.bin';selector_cases=0
    with selector_query.open('wb') as f:
        rows=[]
        rows.extend((0,a,b,c,0,0) for a,b,c in itertools.product(range(2),repeat=3))
        rows.extend((1,a,b,c,0,0) for a,b,c in itertools.product(range(2),repeat=3))
        for comp,bones,lights,flow,fog in itertools.product(range(2),[-1,0,1,3,65,2147483647],range(3),range(2),[-1,0,1,2,3]):rows.append((2,comp,bones&0xffffffff,lights,flow,fog&0xffffffff))
        for comp,bones,morph,tess,fog in itertools.product(range(2),[-1,0,1,3,65,2147483647],range(2),range(3),[-1,0,1,2,3]):rows.append((3,comp,bones&0xffffffff,morph,tess,fog&0xffffffff))
        for kind,a,b,c,d,e in rows:
            expected=selector_expected(kind,(a,b,c,d,e),programs);f.write(struct.pack('<8I',kind,a,b,c,d,e,expected,0));selector_cases+=1
    selector_log=invoke('phong_selectors_x64',[selector_query],'selectors');require(f'checks=6 refusals=7 selector_cases={selector_cases}' in selector_log,'Selector safety/coverage')
    guards=0
    for code in [bytes([0xff]),bytes([0x8b]),bytes([0x8b,0x45,0]),bytes([0xc1,0xe8,1])]:
        try:replay(code,[0]*8,{},0)
        except RuntimeError:guards+=1
        else:raise RuntimeError('Unsupported byte replay admitted')
    profiles={};catalog=[];missing=0;morph_refusals=0;unskinned=0
    for version,(filename,pin) in phong.SOURCES.items():
        source=phong.GAME/'platform/shaders/fxc'/filename;before[str(source)]=sha(source);require(before[str(source)]==pin,'Shader cache pin');cache=Vcs6ShaderCache(source.read_bytes())
        for static,dynamic in itertools.product(range(cache.static_count),range(cache.dynamic_combos)):
            try:shader=cache.shader(static,dynamic)
            except KeyError:missing+=1;catalog.append(dict(version=version,static=static,dynamic=dynamic,status='MISSING_REFUSED'));continue
            digest=hashlib.sha256(shader).hexdigest()
            if not dynamic&2:unskinned+=1;catalog.append(dict(version=version,static=static,dynamic=dynamic,status='UNSKINNED_OUTSIDE_SCOPE',sha256=digest));continue
            try:p=phong.profile(shader,version,bool(dynamic&1))
            except RuntimeError as e:
                require(version==30 and 'Stream-flex inputs' in str(e),'Unexpected profile refusal');morph_refusals+=1;catalog.append(dict(version=version,static=static,dynamic=dynamic,status='TEXTURE_MORPH_REFUSED',sha256=digest));continue
            path=run/f'phong-vs{version}-s{static}-d{dynamic}.bin';path.write_bytes(shader);profiles[version,static,dynamic]=p
            catalog.append(dict(version=version,static=static,dynamic=dynamic,status='STREAM_SKIN_ADMITTED',sha256=digest,selected_operations=[dict(offset_words=o['offset_words'],opcode=o['opcode'],components=o['used_components']) for o in p['full']['ops']]))
    require(len(profiles)==88 and morph_refusals==32,'Compiled variant coverage')
    (run/'catalog.json').write_text(json.dumps(catalog,indent=2)+'\n')
    print(f'Compiled stream skin slots={len(profiles)} morph_refusals={morph_refusals} missing={missing}',flush=True)
    prior_dir=ROOT/'outputs/studio-vertex-tangent-20261025/verification-iciphryt';prior=json.loads((prior_dir/'results.json').read_text());native_query=prior_dir/'queries.bin';native_output=prior_dir/'native_x86.bin'
    require(sha(native_query)==prior['query_sha256'] and sha(native_output)==prior['native_output_sha256'],'Native tangent handoff evidence')
    qdata=native_query.read_bytes();ndata=native_output.read_bytes();lookup={};synthetic={0:[],1:[]}
    for i in range(prior['cases']):
        q=qdata[i*84:i*84+84];row=ndata[i*48:i*48+48];lookup[q]=row
        if i<prior['synthetic_cases'] and q[58]==2 and q[59] and q[51] and row[12] and len(synthetic[q[57]])<64:synthetic[q[57]].append((q,row))
    require(all(len(rows)==64 for rows in synthetic.values()),'Native synthetic input coverage')
    contexts,palettes,old_evidence=phong.benchmark_contexts(before);opts=[]
    for stem in [phong.GAME/'usermod/models/player/hwm/pyro',phong.GAME/'usermod/models/weapons/w_models/w_flamethrower']:
        data=[Path(str(stem)+ext).read_bytes() for ext in ['.mdl','.vvd','.dx90.vtx']];opts.extend((v,o,m) for v,o,m in phong.model_contexts(*data))
    require(len(opts)==len(contexts),'Context order count')
    sample=sorted({int(i*(len(contexts)-1)/31) for i in range(32)}|{0,4249,4250,len(contexts)-1}|{i for i,(_,v,t,m,w) in enumerate(contexts) if t[:12]==bytes(12)})
    vertex_query=run/'variant-vertex-token-reference.bin';world_cases=0;benchmark_cases=0;zero_cases=0
    def write_vertex(f,key,palette,row,position,pdelta,ddelta,scale):
        nonlocal world_cases
        version,static,dynamic=key;p=profiles[key];registers=p['parsed']['inputs'];compressed=dynamic&1;indices=list(row[8:11]);weights=row[:8];normal=row[16:28];tangent=row[28:44]
        w=struct.unpack('<2h' if compressed else '<2f',weights[:4] if compressed else weights);n=list(normal[:4]) if compressed else list(struct.unpack('<3f',normal))
        inputs={registers[0,0]:[*position,1],registers[1,0]:[*w,0,0],registers[2,0]:[f32(indices[2]/255),f32(indices[1]/255),f32(indices[0]/255),0],registers[3,0]:n,registers[0,1]:[*pdelta,0],registers[3,1]:[*ddelta,0]}
        if not compressed:inputs[registers[6,0]]=struct.unpack('<4f',tangent)
        expected=phong.execute(p['full'],inputs,palette,scale);selected=[v for i in indices for matrow in palette[i] for v in matrow];profile_id=(0 if version==20 else 2)+compressed
        f.write(struct.pack('<I36f',profile_id,*selected)+weights+normal+tangent+struct.pack('<10f',*position,*pdelta,*ddelta,scale)+struct.pack('<12f',*expected));world_cases+=1
    with vertex_query.open('wb') as f:
        for key in profiles:
            for q,row in synthetic[key[2]&1]:
                palette=[[[f32((1 if r==c else 0)+rng.uniform(-0.03,0.03)) if c<3 else f32(rng.uniform(-100,100)) for c in range(4)] for r in range(3)] for _ in range(53)]
                write_vertex(f,key,palette,row,struct.unpack_from('<3f',q,16),[0.1,-0.2,0.3],[0.01,0.02,-0.01],0.5)
            for tick in [50000,50417]:
                for i in sample:
                    bones,vertex,tangent,mapping,old_weight=contexts[i];v,opt,m=opts[i];require(v==vertex and m==mapping,'Ordered VTX identity');compressed=key[2]&1
                    query=vertex+opt+bytes([compressed,2,1])+bytes.fromhex('1234567812345678')+tangent;require(query in lookup,'Native writer coverage');row=lookup[query];indices=list(row[8:11]);require(all(j in mapping for j in indices),'Native slot mapping')
                    source=palettes[tick,bones];palette=[source[mapping[j]] if j in mapping else [[0.0]*4 for _ in range(3)] for j in range(53)]
                    write_vertex(f,key,palette,row,struct.unpack_from('<3f',vertex,16),[0,0,0],[0,0,0],0);benchmark_cases+=1;zero_cases+=tangent[:12]==bytes(12)
    old_phong=ROOT/'outputs/phong-vertex-contract-20261025/verification-m67x4ebf/results.json';old=json.loads(old_phong.read_text());decoder=old_phong.parent/'decode-token-reference.bin';require(sha(decoder)==old['decoder_fixture_sha256'],'Packed decoder evidence')
    skin_log=invoke('phong_variant_skin_x64',[decoder,vertex_query],'skin');require(f'packed_frame_cases=131072 vertex_frame_cases={world_cases} degenerate_frame_refusals=0' in skin_log and 'checks=6 refusals=12' in skin_log,'Expanded skin coverage')
    for path,pin in before.items():require(sha(path)==pin,'Original changed')
    tracked=['phong_vertex_variant_candidate.h','probe_phong_vertex_defaults.cpp','test_phong_vertex_variant.cpp','build_phong_vertex_variant.cmd','verify_phong_vertex_variant.py']
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),native_constructor_cases=len(queries),exact_constructor_bytes=len(results[0]),selector_byte_replay_cases=selector_cases,replay_guard_refusals=guards,selector_safety_checks=6,selector_safety_refusals=7,
        compiled_stream_skin_slots=len(profiles),compiled_stream_vs20=sum(v==20 for v,s,d in profiles),compiled_stream_vs30=sum(v==30 for v,s,d in profiles),texture_morph_slots_refused=morph_refusals,missing_slots_refused=missing,unskinned_slots_outside_scope=unskinned,
        expanded_vertex_cases=world_cases,benchmark_sample_vertices=len(sample),benchmark_vertex_cases=benchmark_cases,zero_source_tangent_vertex_cases=zero_cases,exact_token_world_frame_bytes=world_cases*48,packed_decoder_regression_cases=131072,
        executables=binaries,sources_sha256={name:sha(ROOT/'work'/name) for name in tracked},originals_sha256=before,originals_unchanged=True,static_ranges=evidence,native_label_strings=names,
        defaults_query_sha256=sha(run/'defaults-queries.bin'),native_constructor_output_sha256=sha(run/'native_x86.bin'),selector_fixture_sha256=sha(selector_query),catalog_sha256=sha(run/'catalog.json'),vertex_fixture_sha256=sha(vertex_query),prior_tangent_receipt=str(prior_dir/'results.json'),prior_phong_receipt=str(old_phong),
        computer_use=False,sfm_process_used=False,gpu_used=False,editor_changed=False,
        limitations='Three complete original constructors execute unpatched with explicitly supplied hardware/API values; this is not native material-system initialization. Inline integer selector arithmetic is independently decoded/replayed from checked bytes, not a whole-native-draw execution claim. All88 available skinned stream-input slots tested under the documented CPU shader arithmetic convention; samples include all7 zero tangent contexts and64 controlled native writer inputs per compression type. Previous full9610-vertex four-profile coverage is retained separately. Actual live material/hardware/global state and active variant, native strip uploads, GPU precision, texture morph/tessellation execution, lighting/material shading and editor integration remain UNVERIFIED. Missing shader slots fail lookup; no static0 or generic fallback.')
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(status='PASS',receipt=str(run/'results.json'),native_constructor_cases=len(queries),selector_cases=selector_cases,stream_slots=len(profiles),expanded_vertex_cases=world_cases,benchmark_cases=benchmark_cases)))

if __name__=='__main__':main()
