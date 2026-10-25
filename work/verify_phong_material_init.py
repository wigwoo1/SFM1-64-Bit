"""Complete installed Phong InitParams stage against owned x64 behavior.

Native original routine runs in a private PE copy with controlled variable and
command-line callbacks. This does not construct a live effective SFM material.
"""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import itertools
import json
import random
import re
import struct
import subprocess
import tempfile
from verify_ifm_draw_packet import check_ranges
from verify_shader_parameter_registry import decode

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/phong-material-init-20261025'
GAME = Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
STD = GAME / 'bin/stdshader_dx9.dll'
STD_SHA = 'fb09e2ddc8a99aff4ef31e3fddfdb1ca84d89379336abdab4e76e94adfa7e6f8'
META = ROOT / 'outputs/material-effective-parameters-20261017/verification-l11z2le6'
PRIVATE = META / 'copied-libraries/stdshader_dx9.dll'
PARAMS = [('$flags',0,None),('$flags2',2,None),('$flashlighttexture',9,None),
          ('$basetexture',6,0),('$albedo',13,0x14),('$bumpmap',26,0x3c),
          ('$envmap',20,0x24),('$lightwarptexture',43,0x8c),('$phong',48,0xa8),
          ('$selfillumfresnelminmaxexp',35,0x74),('$envmapfresnel',51,0xb4),
          ('$ambientocclusion',100,0x158),('$displacementwrinkle',103,0x174),
          ('$selfillumfresnel',34,0x70),('$basemapalphaphongmask',49,0xac),
          ('$basemapluminancephongmask',53,0xb0),('$shadersrgbread360',99,0x154),
          ('$allowdiffusemodulation',106,0x164),('$phongdisablehalflambert',109,0x178)]


def require(b, message):
    if not b:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    print('Verification directory: ' + str(run), flush=True)
    originals = {str(STD): sha(STD)}
    require(originals[str(STD)] == sha(PRIVATE) == STD_SHA, 'Installed/private shader identity')
    image, ranges = check_ranges(STD, 'stdshader_dx9-disassembly.txt', [
        ('complete_phong_parameter_stage',0x10037850,0x10037bcf),
        ('complete_tools_switch_cache_helper',0x10003120,0x1000315a),
        ('material_info_index_handoff',0x10052f10,0x100532a2),
        ('public_parameter_stage_wrapper',0x10064a30,0x10064a57),
        ('VertexLit_parameter_stage_prefix',0x10053420,0x1005346c),
        ('shared_init_eligibility_hardware_gate',0x10054770,0x100547a5),
        ('shared_init_phong_dispatch',0x10054856,0x1005486d),
        ('texture_init_phong_gate',0x10055000,0x10055052)])
    require(image.string(0x1008b358) == '-tools' and
            image.string(0x1008c0e8) == 'effects/flashlight001', 'Native literal identity')
    metadata_receipt = json.loads((META / 'results.json').read_text())
    require(sha(META / 'registry-0.bin') == metadata_receipt['output_sha256'], 'Registry provenance')
    metadata = json.loads((META / 'shader-metadata.json').read_text())
    require(metadata == decode((META / 'registry-0.bin').read_bytes()), 'Decoded registry/metadata agreement')
    vertexlit = next(x for x in metadata if x['name'] == 'VertexLitGeneric')
    table = image.base + vertexlit['vtable_rva']
    require(image.word(table+4*4) == 0x10064a30 and image.word(table+12*4) == 0x10053420,
            'Installed shader public/protected parameter-stage slots')
    # Derive name links from the checked index-copy helper, not SDK structures.
    helper = next(x for x in ranges if x['name'] == 'material_info_index_handoff')
    loaded = {}
    links = {}
    for row in helper['instructions']:
        text = row['instruction']
        load = re.fullmatch(r'mov\s+(ecx|edx),dword ptr ds:\[([0-9A-F]+)h\]', text)
        if load:
            loaded[load[1]] = int(load[2],16)
        store = re.fullmatch(r'mov\s+dword ptr \[eax(?:\+([0-9A-F]+)h)?\],(ecx|edx)', text)
        if store:
            offset = int(store[1] or '0',16)
            links[offset] = loaded[store[2]]
    for name,index,offset in PARAMS:
        p = vertexlit['parameters'][index]
        require(p['name'].lower() == name, 'Installed parameter name/index')
        if index >= 13:
            require(links[offset] == image.base+p['rva']+20, 'Info field/name link: ' + name)
    # BaseTexture is a constant base-parameter index in the same helper.
    require(any(x['bytes'] == 'c70006000000' for x in helper['instructions']), 'BaseTexture index6 assignment')
    binaries = []
    for tag,machine in [('native',0x14c),('owned',0x8664)]:
        log = (OUT/(tag+'-build.log')).read_text()
        require(not re.search(r'\b(?:warning|error)\b',log,re.I), 'Build diagnostics: ' + tag)
        exe = OUT/('phong_init_'+tag+('_x86.exe' if tag=='native' else '_x64.exe'))
        raw=exe.read_bytes(); pe=struct.unpack_from('<I',raw,60)[0]
        require(raw[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0] == machine
                and struct.unpack_from('<H',raw,pe+24)[0] == (0x10b if tag=='native' else 0x20b), 'Architecture')
        binaries.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine)))
    def execute(tag,args,label):
        exe = Path(binaries[0 if tag=='native' else 1]['path'])
        result = subprocess.run([str(exe),*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=120)
        (run/(label+'.log')).write_text(result.stdout+result.stderr)
        require(result.returncode == 0, label+' failed '+str(result.returncode)+' '+result.stderr)
        return result.stdout
    analytical=execute('owned',['--test'],'analytical')
    require('analytical_checks=15 required_variable_refusals=4' in analytical, 'Analytical checks')
    queries=[];labels=[]
    def initial():
        return [[1,0,0x501000,0x3e800000,1,2,3,1] for _ in PARAMS]
    def add(states,config=0,tools=0,cache=0,label='controlled'):
        require(len(states)==19 and all(len(s)==8 for s in states), 'Fixture state shape')
        queries.append(struct.pack('<155I',config,tools,cache,*(word for s in states for word in s)))
        labels.append(label)
    # Every presence/defined state of the five branch sources, both base-defined
    # values, both config bit8 values, flag mutations and cold/warm tools states.
    for sources,base,config,flags,tools,cache in itertools.product(
            itertools.product(range(3),repeat=5),range(2),[0,0x100],
            [0,0x10000,0x500000,0xffffffff],range(2),range(2)):
        states=initial(); states[0][2]=flags; states[3][1]=base
        for p,state in zip([4,5,6,7,8],sources):
            states[p][0]=state!=0; states[p][1]=state==2
        states[4][7]=2;states[8][2]=0  # Defined zero must remain distinct.
        add(states,config,tools,cache,'branch-grid')
    rng=random.Random(2026102517)
    for _ in range(2048):
        states=[[1 if i<4 else rng.randrange(2),rng.randrange(2),rng.getrandbits(32),
                 rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),
                 rng.randrange(6)] for i in range(19)]
        add(states,rng.getrandbits(32),rng.randrange(2),rng.randrange(2),'random-state')
    # Conditional cases anchored to prior authored requests. These do not claim
    # to be effective material state: both undefined/defined unknowns are varied.
    predicate_receipt = ROOT/'outputs/shader-skin-material-predicate-20261025/verification-mk5rusk1/results.json'
    authored = json.loads(predicate_receipt.read_text())['authored_materials']
    benchmark_conditions=[]
    for m in authored:
        path=Path(m['path']);originals[str(path)]=sha(path)
        require(originals[str(path)]==m['sha256'], 'Authored VMT changed')
        for tools,cache,config,unknown in itertools.product(range(2),range(2),[0,0x100],range(16)):
            states=initial();states[3][1]=1;states[7][1]=1;states[7][7]=5
            states[8][1]=1;states[8][2]=1
            for bit,p in enumerate([4,5,6,11]):states[p][1]=(unknown>>bit)&1
            states[4][7]=2
            if m['authored']['$bumpmap']:states[5][1]=1;states[5][7]=5
            if m['authored']['$basemapalphaphongmask']:states[14][1]=1;states[14][2]=1
            if path.name=='pyro_head.vmt':states[11][1]=1;states[11][3]=0x3f800000
            add(states,config,tools,cache,'benchmark-conditioned')
        benchmark_conditions.append(dict(path=str(path),sha256=m['sha256'],cases=128,
            status='CONDITIONAL_STAGE_INPUTS_ONLY',authored=m['authored'],
            unknowns='Albedo, optional bump, environment and AO stage-entry definitions varied; supplied -tools/config states. '
                     'Known VMT requests are assumptions at this stage entry, not verified native effective construction.'))
    query=run/'queries.bin';query.write_bytes(b''.join(queries));(run/'case-labels.json').write_text(json.dumps(labels))
    outputs=[];logs=[]
    for tag in ['native','owned']:
        target=run/(tag+'.bin');logs.append(execute(tag,[PRIVATE,query,target],tag));outputs.append(target.read_bytes())
    if outputs[0]!=outputs[1]:
        offset=0
        for i in range(len(queries)):
            n=struct.unpack_from('<I',outputs[0],offset)[0];size=4+608+n*32
            if outputs[0][offset:offset+size]!=outputs[1][offset:offset+size]:
                raise RuntimeError('Native/owned mismatch case='+str(i)+' label='+labels[i]+' output_offset='+str(offset))
            offset+=size
        raise RuntimeError('Output framing mismatch')
    offset=0;events=0;callback_kinds=set()
    for i in range(len(queries)):
        count=struct.unpack_from('<I',outputs[0],offset)[0];require(count<=128,'Trace count');offset+=612
        for j in range(count):
            event=struct.unpack_from('<8I',outputs[0],offset+j*32);require(event[0] in range(1,7) and event[1]<19,'Trace domain');callback_kinds.add(event[0])
        offset+=count*32;events+=count
    require(offset==len(outputs[0]) and callback_kinds==set(range(1,7)), 'Complete result/callback coverage')
    cold=sum(struct.unpack_from('<I',q,8)[0]==0 for q in queries)
    require('tools_lookup_calls='+str(cold) in logs[0], 'Cold/warm command line helper coverage')
    for path,pin in originals.items():require(sha(path)==pin,'Original changed: '+path)
    require(sha(PRIVATE)==STD_SHA,'Private original changed')
    files=['phong_material_init_candidate.h','probe_phong_material_init.cpp',
           'build_phong_material_init.cmd','verify_phong_material_init.py']
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(queries),
                exact_state_and_trace_bytes=len(outputs[0]),callback_events=events,
                analytical_checks=15,required_variable_refusals=4,tools_cold_lookups=cold,
                tools_warm_cache_cases=len(queries)-cold,benchmark_conditioned_cases=384,
                benchmark_conditions=benchmark_conditions,parameter_links=[dict(name=n,index=i,info_offset=None if o is None else hex(o)) for n,i,o in PARAMS],
                static_ranges=ranges,static_instruction_bytes=sum(x['bytes'] for x in ranges),
                executables=binaries,sources_sha256={n:sha(ROOT/'work'/n) for n in files},
                query_sha256=sha(query),native_output_sha256=sha(run/'native.bin'),owned_output_sha256=sha(run/'owned.bin'),
                originals_sha256=originals,originals_unchanged=True,private_original_sha256=STD_SHA,
                computer_use=False,sfm_process_used=False,dll_loader_used=False,gpu_used=False,editor_changed=False,
                limitations='Complete Phong parameter-stage behavior under controlled variable callbacks and supplied config/-tools states. '
                'Material variable setters are controlled fixture callbacks, not original IMaterialVar implementation. '
                'Does not execute full shared generic initialization, native VMT conversion, texture loading, active draw/variant selection, '
                'proxies, GPU shading or editor integration. Three benchmark-conditioned groups are not actual loaded effective materials.')
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(status='PASS',receipt=str(run/'results.json'),cases=len(queries),exact_bytes=len(outputs[0]),events=events,tools_cold=cold)),flush=True)


if __name__=='__main__':
    main()
