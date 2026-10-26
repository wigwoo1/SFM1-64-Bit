"""Complete original collector vs independent x64 composition under explicit services.

Shader descriptors/flag inventory/device/editor settings are fixture inputs,
not discovered live shader defaults or a complete IMaterial/rendering proof.
"""
from pathlib import Path
from datetime import datetime,timezone
from collections import Counter
import hashlib,json,random,re,struct,subprocess,tempfile
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_ifm_draw_packet import check_ranges
from verify_material_keyvalues_parser import text,decode,DUMP
from verify_shader_parameter_registry import decode as decode_registry

OUT=ROOT/'outputs/material-parameter-collection-20261026'
def execute(exe,args,run,label):
    p=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=120)
    (run/(label+'.log')).write_text(p.stdout+p.stderr)
    require(p.returncode==0,'Execution '+label+' exit='+str(p.returncode)+': '+p.stdout+p.stderr)
    return p.stdout.strip()
def decode_collection(raw,count):
    at=0;results=[]
    for _ in range(count):
        n,w=struct.unpack_from('<2I',raw,at);at+=8;values=[]
        require(4<=n<=256,'Output parameter count')
        for _ in range(n):
            q=struct.unpack_from('<28I',raw,at);at+=112;s=raw[at:at+q[10]];at+=q[10];name=raw[at:at+q[11]];at+=q[11]
            values.append(dict(words=q,text_hex=s.hex(),name_hex=name.hex()))
        results.append(dict(warnings=w,values=values))
    require(at==len(raw),'Output exact span');return results
def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    previous=json.loads((ROOT/'outputs/material-parameter-condition-20261026/summary.json').read_text())
    numeric=json.loads((ROOT/'outputs/material-keyvalues-numeric-20261026/summary.json').read_text());protected=json.loads(Path(numeric['raw_receipt']).read_text())['original_hashes']
    for p,h in protected.items():require(sha(Path(p))==h,'Protected original identity '+p)
    for checkpoint in [previous,numeric]:
        for p,h in checkpoint['source_sha256'].items():require(sha(ROOT/p)==h,'Previous source identity '+p)
    require(sha(LIB)==sha(PRIVATE)==PIN,'Pinned original library')
    registry_path=ROOT/'outputs/material-effective-parameters-20261017/verification-l11z2le6'
    registry_receipt=json.loads((registry_path/'results.json').read_text());registry_exe=registry_receipt['executable'];require(registry_receipt['status']=='PASS' and sha(Path(registry_exe['path']))==registry_exe['sha256'],'Accepted original shader registry reader')
    registry_libs=registry_path/'copied-libraries'
    for name in ['stdshader_dx9.dll','tier0.dll']:
        original=next(p for p in registry_receipt['original_hashes'] if Path(p).name==name)
        require(sha(Path(original))==sha(registry_libs/name)==registry_receipt['original_hashes'][original],'Registry library identity '+name)
    registry_output=run/'original-shader-registry.bin';execute(Path(registry_exe['path']),[registry_libs,registry_output],run,'original-shader-registry')
    require(sha(registry_output)==registry_receipt['output_sha256'],'Fresh registry differs from accepted original metadata')
    shader_records=decode_registry(registry_output.read_bytes());shader_registry={r['name'].lower():r for r in shader_records};require(len(shader_registry)==157,'Actual registry inventory')
    image,evidence=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('complete_collection',0x10005940,0x10005d08),('complete_parameter_lookup',0x10002e30,0x10002eb6),
        ('complete_flag_lookup',0x10003390,0x1000343e),('complete_flag_update',0x10003440,0x1000352e),
        ('complete_name_condition',0x100055d0,0x10005935),('replacement_cleanup_dispatch',0x10017550,0x10017568),
        ('complete_name_getter_split_symbol',0x100705f0,0x1007061d),('pool_hot_allocate',0x10070390,0x100703de),('pool_hot_release',0x100702b0,0x100702c9),
        ('complete_KeyValues_integer_getter',0x10070c30,0x10070c90),('complete_immediate_SetInt',0x10019460,0x10019560),
        ('original_builtin_GetFlagName',0x10036a40,0x10036a51)])
    require([image.word(0x10070c90+4*i) for i in range(7)]==[0x10070c53,0x10070c82,0x10070c73,0x10070c82,0x10070c63,0x10070c82,0x10070c7c],'Integer getter type table')
    require(image.word(0x101017f8)==0x101018b8 and image.word(0x101018b8)==0x1009c9dc and image.word(0x1009c9dc+40)==0x10036a40,'Actual built-in flag provider identity')
    actual_flags=[image.string(image.word(0x10101818+4*i)) for i in range(32)]
    require(image.string(image.word(0x10101818+128))=='','Original flag terminator')
    flag_source=(ROOT/'work/material_parameter_flags_candidate.h').read_text();require(re.findall(r'"([^"\r\n]*)"',flag_source)==actual_flags,'Owned functional flag identifiers and bit order')
    params=['$flags','$flags_defined','$flags2','$flags_defined2','$x','$y','$matrix']
    flags=['$flag','$flag2','$flag3'];cases=[];labels=[];rng=random.Random(2026102606)
    def node(name,value=1,kind=2):
        return (kind,value if kind!=1 else 0,name,str(value) if kind==1 else '')
    def add(base=(),over=None,parameters=None,flag_names=None,mode=6,state=(0,0,0,0,0,0,0),label='controlled'):
        p=params if parameters is None else parameters;f=flags if flag_names is None else flag_names
        b=list(base);o=[] if over is None else list(over)
        require(len(p)+len(b)+len(o)<=256 and len(f)<=32,'Safe original table bounds')
        cases.append(dict(mode=mode|(8 if over is not None else 0),state=state,parameters=p,flags=f,base=b,over=o));labels.append(label)
    identity='[1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1]'
    examples=[[],[node('$x',7)],[node('$x',7),node('$x',8)],
        [node('$new',7),node('$NEW',8)],[node('!360?$x',7),node('$x',8)],
        [node('$x',7),node('!360?$x',8)],[node('!360?$new',7),node('!360?$new',8),node('$new',9)],
        [node('?$x',7),node('$x',8)],[node('360?$x',7),node('$x',8)],
        [node('$flag',1),node('$flag',0)],[node('$flag',0),node('$flag2',1)],
        [node('!360?$flags',identity,1),node('$flag',1)],
        [node('!360?$flags','123',1),node('$flag',1)],
        [node('!360?$flags_defined','[1 2 3]',1),node('$flag',1)],
        [node('$x','[1 2 bad]',1),node('$y','[bad]',1)],[node('$x','',1)],
        [node('%tool',7),node('$x',8)],[node('HDR?$x',7),node('ldr?$x',8)],
        [node('$matrix',identity,1),node('!360?$matrix','[1 2 3]',1)],
        [node('  $x\t',7),node('$new ',8),node('$new',9)]]
    for mode in range(8):
        for rows in examples:
            add(rows,mode=mode,label='core_routes')
            for over in [[],[node('$x',99)],[node('!360?$x',99)],[node('$flag',0)]]:add(rows,over,mode=mode,label='override_and_empty_root')
    for state in [(0,0,0,0,0,0,0),(1,1,1,3,1,0,0),(0,1,0x100,2,1,0x100,0),(0,0,1,-1,1,1,0),(0,-1,0,1,1,0,1),(0,0,0,0,0x100,0,0)]:
        add([node(n,7) for n in ['hdr?$x','!hdr?$x','GPU>=2?$y','GPU<2?$y','%tool','!unknown?$new']],state=state,label='conditions_and_percent_gate')
    for kind in range(256):
        if kind==5:continue
        for bits in [0,1,0x80000000,0x7f800001]:
            add([node('$flag',bits,kind),node('$x',bits,kind)],label='all_admitted_type_bytes')
    for value in ['','0','-0','1e3','1d3','0d3','-1','2147483648','-2147483649','99999999999999999999999','\t+7suffix','nan','[1 2]']:
        add([node('$flag',value,1),node('$x',value,1)],label='flag_integer_string_conversion')
    for f in [[],['$flag','$flag'],['','$flag'],['$x'],['$flags'],['$flag2','$flag'],['$f'+str(i) for i in range(32)]]:
        add([node('$flag',1),node('$f31',1),node('$x',7),node('$flag2',0)],flag_names=f,label='explicit_flag_inventory')
    for p in [params+['$X'],params+['$x '],params+['$x\t'],params+['  $x'],params[:4],params+['$unknown?name']]:
        add([node('$x',1),node(' $X ',2),node('!360?$x',3),node('!360?$unknown?name',4)],parameters=p,label='descriptor_backward_and_space_matching')
    for _ in range(2048):
        def random_rows():
            rows=[]
            for _ in range(rng.randrange(0,15)):
                name=rng.choice(['$x','$X','$y','$new','$new2','$matrix','$flag','$flag2','%editor','!360?$x','hdr?$x','!unknown?$new','!360?$flags','?$new','  $x\t'])
                kind=rng.choice([0,1,2,3,7]);value=rng.choice(['','hello','1e3','-0','[1 2 3]','{255 128 0}',identity,'[1 bad]','[bad]','center .5 .5 scale 2 3 rotate 90 translate 1 -1']) if kind==1 else rng.getrandbits(32)
                rows.append(node(name,value,kind))
            return rows
        state=(rng.randrange(2),rng.randrange(-1,3),rng.choice([0,1,0x100]),rng.randrange(-1,5),rng.randrange(2),rng.randrange(2),rng.randrange(2))
        add(random_rows(),random_rows() if rng.randrange(2) else None,mode=rng.randrange(8),state=state,label='random_composed_collections')
    # Exercise the actual256-slot table boundary with initialized known slots.
    add(parameters=params[:4]+['$p'+str(i) for i in range(252)],label='maximum_known_slots')
    add([node('extra'+str(i),i) for i in range(252)],parameters=params[:4],label='maximum_appended_slots')
    for i,name in enumerate(actual_flags):
        for value in [0,1]:
            add([node(name,value)],parameters=params[:4],flag_names=[],mode=22,label='original_builtin_flag_bits')
            cases[-1]['expected_flag_bits']=[(1<<i) if value else 0,1<<i]
        add([node('  '+name.upper()+'\t',1),node('!360?'+name,0)],flag_names=[],mode=22,label='original_builtin_flag_spelling_and_duplicate')
    # Raw VMT declarations from protected model sources enter the collector,
    # with an explicit test descriptor/inventory (not live VertexLitGeneric defaults).
    numeric_receipt=json.loads(Path(numeric['raw_receipt']).read_text());parser=next(e for e in numeric_receipt['executables'] if Path(e['path']).name=='parser_owned_x64.exe')
    require(sha(Path(parser['path']))==parser['sha256'],'Verified raw parser binary')
    material_tokens=json.loads((ROOT/'outputs/material-string-value-20261025/verification-zkqa42up/results.json').read_text());materials=material_tokens['authored_material_tokens']
    inp=run/'materials-input.bin';rawout=run/'materials-owned-raw.bin';sources=[]
    for m in materials:
        p=Path(m['path']);require(sha(p)==m['sha256'],'Protected model material');sources.append((str(p),p.read_text(encoding='ascii')))
    inp.write_bytes(struct.pack('<I',len(sources))+b''.join(text(n)+text(s) for n,s in sources));absent=run/'no-engine-present.dll';require(not absent.exists(),'Owned engine absence')
    execute(Path(parser['path']),[absent,inp,rawout],run,'owned-model-material-parser')
    for doc in decode(rawout.read_bytes()):
        for root in doc['roots']:
            rows=[(n['kind'],n['value'] if n['kind'] in [2,3] else 0,n['key'],n['value'] if n['kind']==1 else '') for n in root['children']]
            descriptor=params[:4]+['$basetexture','$bumpmap','$color2','$surfaceprop','$basetexturetransform']
            for state in [(0,0,0,0,0,0,0),(1,1,1,3,1,0,0)]:add(rows,parameters=descriptor,flag_names=[],mode=22,state=state,label='parsed_model_VMT_collection')
            actual_descriptor=[p['name'] for p in shader_registry[root['key'].lower()]['parameters']]
            require(actual_descriptor[:4]==params[:4] and len(actual_descriptor)==125,'Benchmark actual VertexLitGeneric descriptor names')
            for state in [(0,0,0,0,0,0,0),(1,1,1,3,1,0,0)]:
                for mode in [22,23]:add(rows,parameters=actual_descriptor,flag_names=[],mode=mode,state=state,label='registry_backed_model_VMT_collection')
    def packet(case):
        h=[case['mode'],*[v&0xffffffff for v in case['state']],len(case['parameters']),len(case['flags']),len(case['base']),len(case['over'])]
        return struct.pack('<12I',*h)+b''.join(text(s) for s in case['parameters']+case['flags'])+b''.join(struct.pack('<2I',t,b&0xffffffff)+text(n)+text(s) for t,b,n,s in case['base']+case['over'])
    query=run/'queries.bin';query.write_bytes(b''.join(packet(c) for c in cases));outputs=[];executables=[];native_log=''
    for label,suffix,machine,magic in [('native','x86',0x14c,0x10b),('owned','x64',0x8664,0x20b)]:
        exe=OUT/f'collection_{label}_{suffix}.exe';b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0]
        require(struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==magic and b'asInvoker' in b,'Architecture/non-elevated manifest')
        build=OUT/(label+'-build.log');require(not re.search(r'\b(warning|error)\b',build.read_text(),re.I),'Clean compiler '+label)
        imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(label+'-imports.txt')).write_text(imports)
        modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(set(n.lower() for n in modules)=={'kernel32.dll'},'Direct dependencies')
        output=run/(label+'.bin');log=execute(exe,[PRIVATE if label=='native' else absent,query,output],run,label)
        if label=='native':native_log=log
        outputs.append(output.read_bytes());executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules,build_log_sha256=sha(build)))
    native,owned=[decode_collection(b,len(cases)) for b in outputs]
    differences=[dict(case=i,label=labels[i],input=cases[i],native=a,owned=b) for i,(a,b) in enumerate(zip(native,owned)) if a!=b]
    (run/'comparison.json').write_text(json.dumps(dict(cases=len(cases),differences=differences),indent=2)+'\n');require(not differences,'Collection comparison; see comparison.json')
    require(outputs[0]==outputs[1],'Exact ordered collection wire equality')
    for c,doc in zip(cases,native):
        if 'expected_flag_bits' in c:require([doc['values'][i]['words'][2] for i in range(2)]==c['expected_flag_bits'],'Analytical built-in flag bit order')
    require(not re.search(r'\b(warning|error)\b',(OUT/'asm-build.log').read_text(),re.I),'Clean assembler')
    analytical=execute(Path(executables[1]['path']),['--test'],run,'analytical');require('refusals=6' in analytical,'Analytical refusal coverage')
    counts=Counter(v['words'][1]&15 for doc in native for v in doc['values']);require({1,2,4,6,7}<=set(counts),'Constructed string/vector/integer/undefined/matrix coverage')
    for checkpoint in [previous,numeric]:
        for p,h in checkpoint['source_sha256'].items():require(sha(ROOT/p)==h,'Prior source changed')
    for p,h in protected.items():require(sha(Path(p))==h,'Protected original changed')
    for m in materials:require(sha(Path(m['path']))==m['sha256'],'Model source changed')
    for name in ['stdshader_dx9.dll','tier0.dll']:
        original=next(p for p in registry_receipt['original_hashes'] if Path(p).name==name);require(sha(Path(original))==sha(registry_libs/name)==registry_receipt['original_hashes'][original],'Registry original/copy changed')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());require(all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()),'Editor dependency changed');require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor build changed')
    sources={};pending=[ROOT/'work'/n for n in ['material_parameter_collection_candidate.h','material_parameter_collection_native_fixture.h','probe_material_parameter_collection.cpp','build_material_parameter_collection.cmd','verify_material_parameter_collection.py']]
    while pending:
        p=pending.pop()
        if str(p) in sources:continue
        sources[str(p)]=sha(p)
        if p.suffix in ['.h','.cpp']:
            for name in re.findall(r'^\s*#include\s*"([^"]+)"',p.read_text(),re.M):
                if (p.parent/name).is_file():pending.append(p.parent/name)
    sources[str(ROOT/'work/material_trig_x87_candidate.inc')]=sha(ROOT/'work/material_trig_x87_candidate.inc')
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(cases),groups=dict(Counter(labels)),exact_bytes=len(outputs[0]),differences=0,value_types=dict(counts),query_sha256=sha(query),executables=executables,analytical=analytical,native_log=native_log,static_ranges=evidence,sources=sources,benchmark_sources=materials,original_builtin_flags=actual_flags,original_flag_provider_cases=sum(bool(c['mode']&16) for c in cases),previous_verified_sources_unchanged=True,editor_dependencies_unchanged=len(build['editor_source_dependencies']),editor_sha256=build['executable_sha256'],protected_original_files=len(protected),limits=['Complete collector executes unchanged with original name interning, pooled factories, setters and resource destruction; shader descriptors, node names, owner callbacks, immediate queue and hardware/editor state are controlled fixture services','96 flag cases and6 model cases use the actual built-in original flag provider object/vtable/function; other cases use supplied flag inventories to exercise structural edge cases. Owned actual-mode flags are compiled identifiers, not table bytes supplied in queries','No live shader descriptor inventory, shader default/fallback evaluation, texture resources, proxy execution or actual IMaterial creation is claimed','At least4 and at most256 descriptor slots; at most32 explicit flags, ASCII fixture names <=1023/prefix255, values<=4095; wide-string KeyValues type5 refused; native queries conservatively bound known count plus input nodes to256','Owned output is value-owned and transactional on refusal; no allocator timing, stale pointers, numeric symbol IDs, exact diagnostic text, live concurrency or OOM equivalence claim','Model VMT declarations parse with the verified owned reader and enter this collector under supplied descriptors and actual built-in flag inventory; no whole native VMT loader or rendered-image equivalence','No editor integration/build/run, live SFM/computer use, original writes, GPU or Blender; application remains incomplete'])
    result['limits'].append('Percent-prefixed-name decisions reproduce the checked low-byte/int branch using explicitly supplied words. The semantic identities of the two external methods and their live values remain UNVERIFIED; API fields deliberately use neutral percentGate/percentKeep names.')
    result['registry']=dict(reference_executable=registry_exe,fresh_output_sha256=sha(registry_output),accepted_receipt=str(registry_path/'results.json'),shaders=len(shader_registry),benchmark_shader='VertexLitGeneric',benchmark_parameter_names=125,registry_backed_cases=12)
    result['limits'][1]='96 flag cases and18 model cases use the actual built-in original flag provider object/vtable/function; other cases use supplied flag inventories to exercise structural edge cases. Owned actual-mode flags are compiled identifiers, not table bytes supplied in queries.'
    result['limits'][2]='Original shader registry metadata is separately revalidated; shader default/fallback initialization, texture resources, proxy execution and actual IMaterial creation remain unverified.'
    result['limits'].append('Twelve additional model cases use all125 actual VertexLitGeneric parameter names from a fresh original157-shader registry read that exactly matches its prior accepted wire. Collector descriptor callbacks hold those actual names; this is staged composition, not the concrete shader object in the collector process, InitParams/default execution, or an owned shader registry implementation.')
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(path=str(run),receipt_sha256=sha(run/'results.json')),indent=2)+'\n')
    print(json.dumps(dict(status='PASS',cases=len(cases),exact_bytes=len(outputs[0]),types=dict(counts),analytical=analytical,receipt=str(run/'results.json')),indent=2))
if __name__=='__main__':main()
