"""Complete native shared InitParams comparison under installed VLG mapping.

All original files read-only. Native x86 PE oracle uses controlled callbacks;
it is not an owned x64 runtime dependency or a full effective-material load.
"""
from collections import Counter
from datetime import datetime,timezone
from pathlib import Path
import hashlib,itertools,json,random,re,struct,subprocess,tempfile,zlib
from verify_ifm_draw_packet import check_ranges
from verify_shader_parameter_registry import decode
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/vertexlit-shared-init-20261025'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
STD=GAME/'bin/stdshader_dx9.dll'
PIN='fb09e2ddc8a99aff4ef31e3fddfdb1ca84d89379336abdab4e76e94adfa7e6f8'
META=ROOT/'outputs/material-effective-parameters-20261017/verification-l11z2le6'
PRIVATE=META/'copied-libraries/stdshader_dx9.dll'

def require(b,s):
    if not b:raise RuntimeError(s)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def fbits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def signed(v):return struct.unpack('<i',struct.pack('<I',v))[0]

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    print('Verification directory: '+str(run),flush=True)
    require(sha(STD)==sha(PRIVATE)==PIN,'Installed/private shader identity')
    old_phong=json.loads((ROOT/'outputs/phong-material-init-20261025/completion-check.json').read_text())
    for name,pin in old_phong['sources_sha256'].items():
        require(sha(ROOT/'work'/name)==pin,'Verified Phong dependency changed '+name)
    image,evidence=check_ranges(STD,'stdshader_dx9-disassembly.txt',[
        ('complete_shared_parameter_routine',0x10054490,0x10054ff3),
        ('complete_called_predicate',0x10054420,0x10054487),
        ('complete_called_Phong_stage',0x10037850,0x10037bcf),
        ('complete_tools_helper',0x10003120,0x1000315a),
        ('complete_VertexLit_info_mapping',0x10052f10,0x100532a2)])
    prior=json.loads((META/'results.json').read_text());raw=(META/'registry-0.bin').read_bytes()
    require(hashlib.sha256(raw).hexdigest()==prior['output_sha256'],'Registry provenance')
    metadata=decode(raw);require(metadata==json.loads((META/'shader-metadata.json').read_text()),'Registry JSON agreement')
    family=next(x for x in metadata if x['name']=='VertexLitGeneric')
    require(len(family['parameters'])==125,'Registered schema count')
    globals_to_indices={image.base+p['rva']+20:i for i,p in enumerate(family['parameters']) if i>=13}
    mapping=[0xffffffff]*110;registers={};links=[]
    helper=next(x for x in evidence if x['name']=='complete_VertexLit_info_mapping')
    for row in helper['instructions']:
        text=row['instruction'];load=re.fullmatch(r'mov\s+(ecx|edx),dword ptr ds:\[([0-9A-F]+)h\]',text)
        if load:registers[load[1]]=globals_to_indices[int(load[2],16)]
        store=re.fullmatch(r'mov\s+dword ptr \[eax(?:\+([0-9A-F]+)h)?\],(ecx|edx)',text)
        literal=re.fullmatch(r'mov\s+dword ptr \[eax(?:\+([0-9A-F]+)h)?\],([0-9A-F]+)(h)?',text)
        if store:offset=int(store[1] or '0',16);mapping[offset//4]=registers[store[2]]
        elif literal:offset=int(literal[1] or '0',16);mapping[offset//4]=int(literal[2],16 if literal[3] else 10)
    require(mapping[0]==6 and mapping[0x64//4]==9,'Base/flashlight literal mapping')
    for offset in [0x5c,0x14c,0xe8,0xec,0xfc,0x118,0xa4,0x13c]:
        require(mapping[offset//4]==0xffffffff,'Unimplemented other-family info field is declared')
    for i,index in enumerate(mapping):
        if index!=0xffffffff:links.append(dict(offset=hex(i*4),index=index,name=family['parameters'][index]['name']))
    info=run/'info.bin';info.write_bytes(struct.pack('<110I',*mapping))
    (run/'parameter-links.json').write_text(json.dumps(links,indent=2)+'\n')
    executables=[]
    for tag,machine in [('native',0x14c),('owned',0x8664)]:
        log=(OUT/(tag+'-build.log')).read_text();require(not re.search(r'\b(?:warning|error)\b',log,re.I),'Build diagnostics '+tag)
        exe=OUT/('vertexlit_shared_'+tag+('_x86.exe' if tag=='native' else '_x64.exe'))
        binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0]
        require(binary[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',binary,pe+4)[0]==machine
                and struct.unpack_from('<H',binary,pe+24)[0]==(0x10b if tag=='native' else 0x20b),'Executable architecture')
        executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine)))
    def execute(tag,args,label):
        exe=executables[0 if tag=='native' else 1]['path'];r=subprocess.run([exe,*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=120)
        (run/(label+'.log')).write_text(r.stdout+r.stderr);require(r.returncode==0,label+' failed '+str(r.returncode)+' '+r.stderr);return r.stdout
    require('analytical_checks=10 schema_refusals=1' in execute('owned',['--test'],'analytical'),'Analytical coverage')
    queries=[];labels=[]
    def initial():return [[6,0,0,0,0,0,1] for _ in range(125)]
    def defined(s,index,kind=1,integer=0):s[index][0]=kind;s[index][1]=integer
    def add(s,config=0,phong=0,fog=0,hardware=92,vertexlit=1,tools=0,label='control'):
        queries.append(struct.pack('<881I',config,phong&0xffffffff,fog&0xffffffff,hardware&0xffffffff,vertexlit,tools,
                                   *(word for state in s for word in state)));labels.append(label)
    add(initial())
    for global_phong,phong_state,warp,bump,mask,level,config,tools in itertools.product(
            [0,1],range(3),[1,3,6],[1,6],[0,1,2],[91,92],[0,0x100],range(2)):
        s=initial();defined(s,48,6 if phong_state==0 else 4,int(phong_state==2));defined(s,43,warp)
        defined(s,26,bump);defined(s,49,4,mask);defined(s,6);defined(s,13);s[13][-1]=2
        add(s,config,global_phong,hardware=level,tools=tools,label='predicate-hardware-grid')
    for type_byte in range(256):
        for target in [43,26]:
            s=initial();defined(s,48,4,1);defined(s,49,4,0);defined(s,43,1);defined(s,26,6);defined(s,target,type_byte)
            add(s,phong=1,label='full-type-byte-domain')
    for config,flags,bits in itertools.product([0,0x80,0x100,0x180],
            [0,0x100000,0x400000,0x500000,0x10000,0x80000000],range(32)):
        s=initial();s[0][1]=flags
        for bit,index in enumerate([26,20,22,6,43]):defined(s,index,1 if bits&(1<<bit) else 6)
        add(s,config,vertexlit=(bits>>1)&1,label='generic-conflicts-and-flags')
    for flags_defined,fog in itertools.product([0,0x80000000],[0,-1]):
        s=initial();s[1][1]=flags_defined;add(s,fog=fog,label='force-vertexfog')
    rng=random.Random(2026102520)
    for _ in range(512):
        s=[[rng.randrange(256),rng.getrandbits(32),*[rng.getrandbits(32) for _ in range(4)],rng.randrange(6)] for _ in range(125)]
        add(s,rng.getrandbits(32),rng.choice([-1,0,1]),rng.choice([-1,0,1]),
            rng.choice([-2147483648,-1,91,92,93,2147483647]),rng.randrange(2),rng.randrange(2),'varied-states')
    query=run/'queries.bin';query.write_bytes(b''.join(queries));(run/'case-labels.json').write_text(json.dumps(labels))
    outputs=[]
    for tag in ['native','owned']:
        target=run/(tag+'.bin');log=execute(tag,[PRIVATE,info,query,target],tag);print(log.strip(),flush=True);outputs.append(target.read_bytes())
    offset=0;routes=Counter();kinds=Counter();warning_counts=Counter();events=0
    for i,q in enumerate(queries):
        require(offset+3508<=len(outputs[0]),'Native output framing')
        route,n=struct.unpack_from('<2I',outputs[0],offset);size=3508+n*32
        require(n<=256 and offset+size<=len(outputs[0]),'Callback extent')
        if outputs[0][offset:offset+size]!=outputs[1][offset:offset+size]:
            a=outputs[0][offset:offset+size];b=outputs[1][offset:offset+size]
            diff=next((j for j in range(min(len(a),len(b))) if a[j]!=b[j]),min(len(a),len(b)))
            raise RuntimeError(f'Native/owned mismatch case={i} label={labels[i]} record_offset={offset} local_byte={diff} native={a[diff:diff+16].hex()} owned={b[diff:diff+16].hex()}')
        settings=struct.unpack_from('<6I',q);states=[struct.unpack_from('<7I',q,24+j*28) for j in range(125)]
        phong_value=states[48][1] if (states[48][0]&15)!=6 else 0
        eligible=bool(settings[1] and phong_value and ((states[43][0]&15)==3 or states[49][1]==1 or (states[26][0]&15)!=6))
        require(route==int(eligible and signed(settings[3])>=92),'Independent dispatch expectation')
        hardware_queries=0
        for j in range(n):
            event=struct.unpack_from('<8I',outputs[0],offset+3508+j*32);kind,param=event[:2]
            require(1<=kind<=10 and param<126,'Event domain');kinds[kind]+=1
            if kind==9:hardware_queries+=1;require(event[2]==settings[3],'Hardware callback value')
            if kind==10:warning_counts[event[2]]+=1
        require(hardware_queries==int(eligible),'Hardware query short circuit')
        final_flags=struct.unpack_from('<I',outputs[0],offset+8+4)[0]
        expected_fog=bool(states[0][1]&0x80000000 or (not states[1][1]&0x80000000 and settings[2]))
        require(bool(final_flags&0x80000000)==expected_fog,'Independent force-fog effect')
        routes[route]+=1;events+=n;offset+=size
    require(offset==len(outputs[0])==len(outputs[1]) and routes[0] and routes[1], 'Complete outputs/both branches')
    require(set(kinds)==set(range(1,11)) and set(warning_counts)=={1,2},'All callback/warning kinds must execute')
    # Compress duplicate evidence to avoid retaining two large identical files.
    capture=run/'native-records.bin.zlib';capture.write_bytes(zlib.compress(outputs[0],9))
    digest=hashlib.sha256(outputs[0]).hexdigest();require(zlib.decompress(capture.read_bytes())==outputs[0],'Compressed evidence roundtrip')
    for name in ['native.bin','owned.bin']:
        path=run/name;require(path.resolve().parent==run.resolve() and path.read_bytes()==outputs[0],'Disposable output identity')
        path.unlink()
    old=json.loads((ROOT/'outputs/material-var-defined-20261025/completion-check.json').read_text())
    for sources in [old['sources_sha256'],old['predicate_sources_sha256']]:
        for name,pin in sources.items():require(sha(ROOT/'work'/name)==pin,'Previous component changed '+name)
    require(sha(old['editor']['path'])==old['editor']['sha256'],'Editor changed')
    require(sha(STD)==sha(PRIVATE)==PIN,'Original/private image changed')
    files=['vertexlit_shared_init_candidate.h','probe_vertexlit_shared_init.cpp',
           'build_vertexlit_shared_init.cmd','verify_vertexlit_shared_init.py',
           'phong_material_init_candidate.h','shader_skin_material_predicate_candidate.h',
           'material_var_defined_candidate.h']
    receipt=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(queries),case_groups=dict(Counter(labels)),
        phong_cases=routes[1],generic_cases=routes[0],callback_events=events,callback_kinds=dict(kinds),warnings=dict(warning_counts),
        exact_state_route_trace_bytes=len(outputs[0]),output_sha256=digest,compressed_output_sha256=sha(capture),
        analytical_checks=10,schema_refusals=1,registered_parameters=125,mapped_info_fields=len(links),
        static_ranges=evidence,static_instruction_bytes=sum(x['bytes'] for x in evidence),executables=executables,
        sources_sha256={n:sha(ROOT/'work'/n) for n in files},
        build_logs_sha256={n:sha(OUT/n) for n in ['native-build.log','owned-build.log']},
        info_sha256=sha(info),query_sha256=sha(query),
        parameter_links_sha256=sha(run/'parameter-links.json'),original_stdshader_sha256=PIN,originals_unchanged=True,
        previous_components_unchanged=True,editor=old['editor'],computer_use=False,sfm_process_used=False,
        gpu_used=False,dll_loader_used=False,
        limitations='Complete shared routine executed unchanged under installed VertexLitGeneric125-parameter/verified info mapping. '
        'Other families/info mappings are not supported by this candidate; their undeclared fields are explicitly checked absent. '
        'Material variable setters/config/CVar/hardware/command line are controlled fixture services, not full native material lifecycle. '
        'Native Phong route observed through its sole cold tools-helper call, not inferred from owned dispatch. '
        'No actual effective benchmark construction, texture loading, proxies, active draw variant, GPU shading or editor integration.')
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status='PASS',receipt=str(run/'results.json'),cases=len(queries),phong=routes[1],generic=routes[0],exact_bytes=len(outputs[0]),warnings=dict(warning_counts))),flush=True)

if __name__=='__main__':main()
