"""Actual UnlitGeneric registry/mapping/collector/InitParams vs owned x64.

No live SFM, GPU or original writes. This verifies parameters before resources;
the owned material fallback and production editor are intentionally unchanged.
"""
from pathlib import Path
from datetime import datetime, timezone
from collections import Counter
import hashlib, itertools, json, random, re, shutil, struct, subprocess, tempfile, time, zlib
from verify_material_numeric_value import ROOT, LIB, PRIVATE, PIN, require, sha
from verify_material_keyvalues_parser import DUMP, text
from verify_vertexlit_concrete import decode
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges

OUT = ROOT/'outputs/unlit-generic-concrete-20261026'
NEW = ['unlit_generic_parameter_schema_candidate.h', 'unlit_generic_init_candidate.h',
       'unlit_generic_native_fixture.h', 'probe_unlit_generic_concrete.cpp',
       'build_unlit_generic_concrete.cmd', 'verify_unlit_generic_concrete.py']


def metadata(raw):
    at = 0
    def word():
        nonlocal at
        n = struct.unpack_from('<I', raw, at)[0]; at += 4
        return n
    n = word(); require(n == 68, 'Actual declaration count'); names = []; types = []
    for _ in range(n):
        size = word(); require(size <= 1023, 'Schema name bound')
        names.append(raw[at:at+size].decode('ascii')); at += size; types.append(word())
    prefix = raw[:at]; fields = word(); require(fields == 110, 'Actual mapping extent')
    mapping = [word() for _ in range(fields)]
    require(at == len(raw) and all(i == 0xffffffff or i < 68 for i in mapping), 'Full metadata framing')
    return names, types, mapping, prefix


def main():
    OUT.mkdir(exist_ok=True); run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    print('Run '+str(run), flush=True)
    ptr = json.loads((ROOT/'outputs/builtin-error-material-seed-20261026/latest-verification.json').read_text())
    receipt = Path(ptr['receipt']); require(sha(receipt) == ptr['receipt_sha256'], 'Prior accepted receipt')
    prior = json.loads(receipt.read_text())
    for p, h in prior['sources'].items(): require(sha(Path(p)) == h, 'Accepted source '+p)
    for p, h in prior['original_hashes'].items(): require(sha(Path(p)) == h, 'Protected original '+p)
    for p, h in prior['private_library_hashes'].items(): require(sha(Path(p)) == h, 'Private library '+p)
    require(sha(LIB) == sha(PRIVATE) == PIN, 'Original material identity')
    sources = source_closure([Path(p) for p in prior['sources']] + [ROOT/'work'/p for p in NEW])
    libs = ROOT/'outputs/material-fallback-owned-20261026/verification-0mhr77s9/copied-libraries'
    _, ranges = check_ranges(LIB.parent/'stdshader_dx9.dll', 'stdshader_dx9-disassembly.txt', [
        ('actual_unlit_mapping',0x1004c7c0,0x1004ca56), ('actual_unlit_concrete_InitParams',0x1004ca60,0x1004cab4),
        ('actual_InitParams_wrapper',0x10064a30,0x10064a57), ('actual_shared_skin_predicate',0x10054420,0x10054487),
        ('actual_whole_shared_InitParams',0x10054490,0x10054ff3)])
    started = time.time()
    p = subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_unlit_generic_concrete.cmd')], cwd=ROOT, capture_output=True, text=True, timeout=120)
    (run/'build-driver.log').write_text(p.stdout+p.stderr); require(p.returncode == 0, 'Fresh bounded full probe build')
    executables = []; logs = {}
    for name, machine in [('concrete_native_x86.exe',0x14c),('concrete_owned_x64.exe',0x8664)]:
        p = OUT/name; raw = p.read_bytes(); pe = struct.unpack_from('<I',raw,60)[0]
        require(p.stat().st_mtime >= started-2 and struct.unpack_from('<H',raw,pe+4)[0] == machine and struct.unpack_from('<H',raw,pe+24)[0] == (0x10b if machine == 0x14c else 0x20b) and b'asInvoker' in raw, 'Fresh architecture/manifest '+name)
        imports = subprocess.run([str(DUMP),'/imports',str(p)], capture_output=True, text=True, check=True).stdout
        modules = re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.I|re.M)
        require({m.lower() for m in modules} == {'kernel32.dll'}, 'Direct imports '+name)
        executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine),direct_imports=modules))
    for name in ['native-build.log','owned-build.log','asm-build.log']:
        p = OUT/name
        require(p.stat().st_mtime >= started-2 and not re.search(r'\b(?:fatal\s+)?(?:warning|error)\s+[A-Z]+\d+\s*:|^\s*(?:warning|error)\s*:',p.read_text(),re.I|re.M), 'Clean fresh build '+name)
        logs[str(p)] = sha(p)
    def execute(exe,args,label):
        p = subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=120)
        (run/(label+'.log')).write_text(p.stdout+p.stderr)
        require(p.returncode == 0, label+' exit='+str(p.returncode)+'; see log')
        return p.stdout.strip()
    native = OUT/'concrete_native_x86.exe'; owned = OUT/'concrete_owned_x64.exe'
    execute(native,['--metadata',PRIVATE,libs,run/'native-schema.bin'],'native-schema')
    names, types, mapping, prefix = metadata((run/'native-schema.bin').read_bytes())
    discovery = Path(json.loads((OUT/'discovery-pointer.json').read_text())['path'])
    require(sha(run/'native-schema.bin') == sha(discovery/'schema.bin'), 'Initial independent metadata capture retained')
    execute(owned,['--metadata',run/'owned-schema.bin'],'owned-schema')
    require((run/'owned-schema.bin').read_bytes() == prefix, 'All68 owned names/types equal actual descriptors')
    expected = {0:6,0x14:13,0x20:16,0x24:17,0x28:18,0x2c:19,0x30:20,0x38:22,0x50:23,0x54:24,0x58:25,0x5c:26,0x64:9,0xa4:27,0xbc:38,0xe8:44,0xec:45,0xfc:48,0x118:55,0x138:63,0x13c:62,0x14c:64,0x154:67}
    require(all(mapping[o//4] == i for o,i in expected.items()), 'Actual initialized-field mapping')
    require(all(mapping[o//4] == 0xffffffff for o in [0x3c,0xa8,0xac,0x8c,0x140,0x148,0x164,0x168,0x16c,0x17c]), 'Absent bump/Phong/warp/effect fields')
    cases = []; labels = []
    def add(nodes=(),settings=(0,0,0,92,0),mode=22,repeats=2,over=(),label='controlled'):
        q = [mode,0,0,0,0,0,0,0,68,0,len(nodes),len(over)]
        require(68+len(nodes)+len(over) <= 256, 'Original fixed output capacity')
        cases.append(dict(q=q,nodes=list(nodes)+list(over),settings=settings,repeats=repeats)); labels.append(label)
    def integer(i,n): return (2,n&0xffffffff,names[i],'')
    def string(i,s): return (1,0,names[i],s)
    add(label='undefined_defaults')
    error_index = len(cases)
    error_nodes = [(2,1,'$model',''),(3,0x3d4ccccd,'$decalscale',''),(1,0,'$basetexture','error'),(2,1,'$gammacolorread',''),(2,1,'$linearwrite','')]
    add(error_nodes,label='actual_builtin_error')
    grid = []
    for flags,config,fog,defined,base,model in itertools.product([0,0x40,0x100000,0x400000,0x10000,0x510040],[0,0x80,0x100,0x180],[0,1,-1],[0,0x80000000],[0,1],[0,1]):
        nodes = [integer(0,flags),integer(1,defined),string(17,'fixture/env'),string(19,'fixture/envmask'),string(13,'fixture/albedo'),integer(36,1),string(31,'fixture/warp'),(1,0,'$bumpmap','fixture/extra_normal')]
        if base: nodes.append(string(6,'fixture/base'))
        index = len(cases); add(nodes,(config,1,fog,91 if model else 95,model),mode=22|model,repeats=3,label='flags_config_model_grid')
        grid.append((index,flags,config,fog,defined,base,model))
    for i in range(68):
        for kind,bits,value in [(2,0,''),(2,0xffffffff,''),(3,0x80000000,''),(3,0x7fc12345,''),(1,0,'[0 1 2 3]'),(1,0,'fixture/texture')]:
            add([(kind,bits,names[i],value)],settings=(0x80,1,1,92,1),repeats=2,label='each_declared_parameter_typed')
    rng = random.Random(2026102619)
    for i in range(512):
        nodes=[]
        for j in rng.sample(range(4,68),rng.randrange(1,26)):
            kind=rng.randrange(4)
            if kind==0: nodes.append(integer(j,rng.choice([0,1,-1,rng.getrandbits(32)])))
            elif kind==1: nodes.append((3,rng.choice([0,0x80000000,0x7f800001,0x7fc12345,0x7f800000,0xff800000,rng.getrandbits(32)]),names[j],''))
            else: nodes.append(string(j,rng.choice(['fixture/texture','','[0 1 2]','[0 1 2 3]','[1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1]'])))
        nodes += [(2,i,'extra_'+str(i),''),(1,0,'$bumpmap','fixture/undeclared')]
        over=[string(rng.randrange(4,68),'fixture/override')] if i%2 else []
        add(nodes,(rng.choice([0,0x80,0x100,0x180]),rng.choice([0,1,-1]),rng.choice([0,1,-1]),rng.choice([80,90,91,92,95]),rng.randrange(2)),mode=rng.randrange(8)|16|(8 if over else 0),repeats=1+i%3,over=over,label='varied_types_extras_overrides')
    capacity=[]
    for extras in [1,187,188]:
        capacity.append((len(cases),extras));add([(3,0x3d4ccccd,'extra_'+str(i),'') for i in range(extras)],repeats=1,label='extra_variable_capacity')
    # The original collector constructs the four internal flag variables before
    # reading nodes: direct $flags/$flags_defined writes are ignored (and warn).
    # Preserve that complete initial1501-case corpus, then add real flag-name
    # declarations to vary the actual initializer bits independently.
    named_grid=[]
    for flags,config,fog,defined,base,model in itertools.product([0,0x40,0x100000,0x400000,0x10000,0x510040],[0,0x80,0x100,0x180],[0,1,-1],[0,1],[0,1],[0,1]):
        nodes=[(2,int(bool(flags&bit)),name,'') for bit,name in [(0x40,'$selfillum'),(0x100000,'$basealphaenvmapmask'),(0x400000,'$normalmapalphaenvmapmask'),(0x10000,'$decal')]]
        if defined:nodes.append((2,0,'$vertexfog',''))
        nodes += [string(17,'fixture/env'),string(19,'fixture/envmask'),string(13,'fixture/albedo'),integer(36,1),string(31,'fixture/warp'),(1,0,'$bumpmap','fixture/extra_normal')]
        if base:nodes.append(string(6,'fixture/base'))
        index=len(cases);add(nodes,(config,1,fog,91 if model else 95,model),mode=22|model,repeats=3,label='actual_named_flags_config_model_grid')
        named_grid.append((index,flags,config,fog,defined,base,model))
    def packet(c):
        return struct.pack('<12I',*c['q'])+b''.join(text(n) for n in names)+b''.join(struct.pack('<2I',t,b)+text(n)+text(s) for t,b,n,s in c['nodes'])+struct.pack('<6I',c['repeats'],*[n&0xffffffff for n in c['settings']])
    query=run/'queries.bin';query.write_bytes(b''.join(packet(c) for c in cases));query_hash=sha(query)
    first=OUT/'verification-cdlh8ezs'
    require(query.read_bytes().startswith((first/'queries.bin').read_bytes()),'All1501 initial queries retained without edits/removal')
    (run/'case-settings.json').write_text(json.dumps(dict(groups=dict(Counter(labels)),labels=labels,settings=cases),indent=2)+'\n')
    outputs=[];runtime_logs={}
    for tag,exe,args in [('native',native,[PRIVATE,libs]),('owned',owned,[run/'absent-engine.dll',run/'absent-libraries'])]:
        dest=run/(tag+'.bin');runtime_logs[tag]=execute(exe,[*args,query,dest],tag);outputs.append(dest.read_bytes())
    records=[decode(b,len(cases)) for b in outputs]
    differences=[dict(case=i,group=labels[i],input=cases[i],native=a,owned=b) for i,(a,b) in enumerate(zip(*records)) if a!=b]
    (run/'comparison.json').write_text(json.dumps(dict(cases=len(cases),differences=len(differences),first=differences[:3]),indent=2)+'\n')
    require(not differences and outputs[0]==outputs[1], 'Exact complete collection/initialization comparison; see comparison.json')
    require(all(b.startswith((first/'native.bin').read_bytes()) for b in outputs),'All1501 prior native results unchanged')
    reference=records[0];baseline=reference[0]['values'];checks=0
    def check(ok,message):
        nonlocal checks
        require(ok,message);checks+=1
    check(baseline[16]['words'][3]==0x40800000 and baseline[44]['words'][3]==baseline[45]['words'][3]==0x3f000000,'Actual Unlit detail/edge defaults')
    check(baseline[48]['words'][3]==baseline[55]['words'][3]==baseline[27]['words'][3]==0x3f800000 and baseline[9]['text']=='effects/flashlight001','Actual glow/outline/HDR/flashlight defaults')
    check(reference[error_index]['values']==prior['caller'][0]['values'],'Exact prior real builtin caller69-value record')
    for index,flags,config,fog,defined,base,model in grid:
        values=reference[index]['values'];f=values[0]['words'][2]
        check((f&0x400040)==0,'Unlit always clears selfillum and base-alpha environment flags')
        check(values[6]['text']==('fixture/base' if base else '') and values[13]['text']=='fixture/albedo','No albedo copy from undeclared extra normal')
        check(((values[17]['words'][1]&15)==6)==bool(config&0x80 and base),'Config environment removal only with defined base')
        check(values[1]['words'][2]==(0x800 if model else 0) and bool(f&0x80000000)==bool(fog),'Direct internal flags_defined input is ignored by collector')
        check(values[36]['words'][2]==1 and reference[index]['route']==0,'Unlit does not enter Phong despite declared scalar and extra normal')
    for index,flags,config,fog,defined,base,model in named_grid:
        values=reference[index]['values'];f=values[0]['words'][2]
        check((f&0x400040)==0,'Named selfillum/basealpha flags cleared')
        check(bool(f&0x100000)==bool(base and flags&0x100000),'Normal-map alpha mask bit retained only with base, no declared bump')
        check(bool(f&2)==bool(flags&0x10000),'Named decal enables vertex color flag')
        check(bool(f&0x80000000)==bool(fog and not defined) and bool(values[1]['words'][2]&0x80000000)==bool(defined),'Named vertexfog0 suppresses forced fog')
        check(bool(f&0x800)==bool(model),'Original model input preserved')
        check(values[6]['text']==('fixture/base' if base else '') and values[13]['text']=='fixture/albedo','No albedo copy from extra normal')
        check(((values[17]['words'][1]&15)==6)==bool(config&0x80 and base),'Named-grid environment removal')
    for index,n in capacity:
        check(len(reference[index]['values'])==68+n and all(v['words'][3:7]==[0x3d4ccccd]*4 for v in reference[index]['values'][68:]),'Extra variables preserved through declared-only defaults')
    check(all(row['route']==0 for row in reference),'No unintended Phong route')
    analytical=execute(owned,['--test'],'analytical');require(analytical=='PASS analytical_checks=10 refusals=4','Owned transactional checks')
    count=re.search(r'stage_calls=(\d+) native_allocations=(\d+) native_releases=(\d+)',runtime_logs['native'])
    require(count and int(count[1])==sum(c['repeats'] for c in cases) and count[2]==count[3], 'Original stage count/cleanup')
    require(sha(query)==query_hash and not (run/'absent-engine.dll').exists() and not (run/'absent-libraries').exists(),'Input unchanged/owned absent-runtime paths')
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Original changed '+p)
    for p,h in prior['private_library_hashes'].items():require(sha(Path(p))==h,'Private library changed '+p)
    for p,h in sources.items():require(sha(Path(p))==h,'Source changed during verification '+p)
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());graph=set()
    def visit(p):
        k=p.relative_to(ROOT).as_posix()
        if k in graph:return
        graph.add(k)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            if (p.parent/inc).is_file():visit((p.parent/inc).resolve())
    visit((ROOT/'work/probe_vulkan_win32_surface_x64.cpp').resolve())
    require(graph==set(build['editor_source_dependencies']) and all(sha(ROOT/k)==h for k,h in build['editor_source_dependencies'].items()),'Production editor graph unchanged')
    require(not set('work/'+n for n in NEW)&graph and sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==prior['editor_sha256']==build['executable_sha256'],'Editor unchanged')
    capture=run/'exact-records.bin.zlib';capture.write_bytes(zlib.compress(outputs[0],9));require(zlib.decompress(capture.read_bytes())==outputs[0],'Exact evidence compression')
    limits=['Owned UnlitGeneric68 schema/mapped concrete/shared InitParams + existing collector/material-system completion, not a Source shader renderer or texture/resource Init implementation','Actual original registry/descriptors/mapping/collector/InitParams/material-variable methods use controlled owner/allocator/hardware/config/command-line/queue/logging services; original executable integrity checked, no original instruction replacement','Existing shared125-field VertexLitGeneric API and material fallback/editor code unchanged; builtin error fallback is not yet enabled in the owned whole caller','Original-only complete builtin caller69-value record matches new isolated collected/completed values exactly, but normal IMaterial factory/resource lifetime/proxies/render fidelity and application integration remain incomplete','All prior83 source hashes and242 protected originals retained; prior633 fallback/14429 strict/VertexLit comparisons not rerun because their sources are unchanged. No live SFM, GPU, Blender or original writes']
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),scope='VERIFIED UnlitGeneric parameter initialization; application incomplete',previous_goal_turn='progress',cases=len(cases),stage_invocations=sum(c['repeats'] for c in cases),groups=dict(Counter(labels)),independent_checks=checks,declared_parameters=68,mapping=mapping,parameter_names=names,parameter_types=types,exact_bytes=len(outputs[0]),differences=0,output_sha256=hashlib.sha256(outputs[0]).hexdigest(),query_sha256=query_hash,compressed_output_sha256=sha(capture),metadata_sha256=sha(run/'native-schema.bin'),compiled_metadata_sha256=sha(run/'owned-schema.bin'),analytical=analytical,native_log=runtime_logs['native'],allocations=int(count[2]),releases=int(count[3]),executables=executables,build_logs=logs,sources=sources,original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],static_ranges=ranges,prior_receipt=str(receipt),prior_receipt_sha256=sha(receipt),editor_dependencies_unchanged=len(graph),editor_sha256=build['executable_sha256'],limits=limits)
    dest=run/'results.json';dest.write_text(json.dumps(result,indent=2)+'\n');replay=OUT/('replay-'+run.name.removeprefix('verification-'));replay.mkdir()
    files=[Path(p) for p in sources]+[Path(e['path']) for e in executables]+[Path(p) for p in logs]+[dest,query,capture,run/'native-schema.bin',run/'owned-schema.bin',run/'case-settings.json',run/'comparison.json',run/'native.log',run/'owned.log',run/'analytical.log']
    for p in files:
        to=replay/p.relative_to(ROOT);to.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,to)
    manifest={p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()};(replay/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    pointer=dict(receipt=str(dest),receipt_sha256=sha(dest),replay=str(replay),replay_manifest_sha256=sha(replay/'manifest.json'));(OUT/'latest-verification.json').write_text(json.dumps(pointer,indent=2)+'\n')
    print(json.dumps({**pointer,**{k:result[k] for k in ['status','cases','stage_invocations','groups','independent_checks','exact_bytes','analytical','allocations','releases']}},indent=2))


if __name__=='__main__':main()
