"""Original parameter-name condition dispatch vs independent x64 policy.

Explicit hardware/convar state only. This is not the complete material loader,
not KeyValues bracket-expression evaluation, and not live SFM device discovery.
"""
from pathlib import Path
from datetime import datetime,timezone
from collections import Counter
import hashlib,itertools,json,random,re,struct,subprocess,tempfile
from verify_ifm_draw_packet import check_ranges
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_material_keyvalues_parser import text,decode,DUMP

OUT=ROOT/'outputs/material-parameter-condition-20261026'
PREVIOUS=ROOT/'outputs/material-keyvalues-numeric-20261026'
def execute(exe,args,run,label):
    p=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=60)
    (run/(label+'.log')).write_text(p.stdout+p.stderr)
    require(p.returncode==0,'Execution '+label+': '+p.stdout+p.stderr)
    return p.stdout.strip()
def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    previous=json.loads((PREVIOUS/'summary.json').read_text());receipt=json.loads(Path(previous['raw_receipt']).read_text())
    require(previous['status'].startswith('VERIFIED') and receipt['status']=='PASS','Previous numeric checkpoint')
    for p,h in receipt['original_hashes'].items():require(sha(Path(p))==h,'Protected original identity '+p)
    for p,h in previous['source_sha256'].items():require(sha(ROOT/p)==h,'Previous numeric source identity '+p)
    require(sha(LIB)==sha(PRIVATE)==PIN,'Pinned reference material library')
    image,evidence=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('complete_parameter_name_condition',0x100055d0,0x10005935),
        ('caller_skips_before_flag_value_processing',0x10005a52,0x10005a6d),
        ('complete_ascii_case_insensitive_comparison',0x1006e300,0x1006e37e),
        ('complete_length_limited_prefix_copy',0x1006e560,0x1006e586),
        ('KeyValues_name_getter',0x100705f0,0x1007061d)])
    addresses=[0x10095c68,0x10095840,0x10095834,0x1009583c,0x10095c60,0x1009588c,0x10095894,0x1009589c,0x10095c58,0x100958a4,0x10095c54]
    names=['lowfill','hdr','srgb','ldr','GPU>=3','GPU>=2','GPU>=1','GPU<2','GPU<3','GPU<1','360']
    require([image.string(a) for a in addresses]==names,'Original exact condition names')
    rows=[];labels=[];rng=random.Random(2026102605)
    def add(name,state=(0,0,0,0),seed=0xa5,label='controlled'):
        if isinstance(name,str):name=name.encode('ascii')
        prefix=name.split(b'\0',1)[0].find(b'?')
        require(len(name)<=1023 and prefix<=255,'Safe native name domain')
        rows.append((*[v&0xffffffff for v in state],seed,len(name),name));labels.append(label)
    # All combinations around signed and zero/nonzero/low-byte branch boundaries.
    states=itertools.product([-2147483648,-1,0,1,2147483647],[-1,0,1,2,256],[0,1,0x80,0xff,0x100,0x80000000,0x1000001],[-2147483648,-1,0,1,2,3,4,2147483647])
    for state in states:
        for token in names:
            for negate in ['', '!']:
                add(negate+token+'?$fixture',state,label='condition_state_cross_product')
    for seed in range(256):
        for name in ['$x','?hdr?$x','hdr?$x','HDR?$x','!hdr?$x','360?$color2','!360?$color2','unknown?$x','!unknown?$x','!!hdr?$x','!?$x','hdr ?$x',' hdr?$x','hdr?$x?tail']:
            add(name,(1,1,1,2),seed,'all_output_flag_seeds')
    # Every case spelling, exact comparison (no trimming), and first-question rule.
    for token in names:
        letters=[i for i,c in enumerate(token) if c.isalpha()]
        for mask in range(1<<len(letters)):
            chars=list(token)
            for bit,index in enumerate(letters):chars[index]=chars[index].upper() if mask&(1<<bit) else chars[index].lower()
            variant=''.join(chars)
            for suffix in ['', '$x', '$x?more']:
                for prefix in ['', '!','!!',' ','\t']:
                    add(prefix+variant+'?'+suffix,(0,-1,0x100,3),label='case_and_exact_prefix')
    for byte in range(256):
        for tail in [b'hdr?$x',b'?$x',b'!360?$x']:
            add(bytes([byte])+tail,(1,1,1,2),label='all_leading_bytes')
    for length in [1,2,3,127,252,253,254,255]:
        for negate in ['', '!']:
            prefix=negate+'x'*(length-len(negate))
            add(prefix+'?$x',label='native_stack_prefix_boundary')
    for name in [b'',b'?',b'??',b'hdr\0?ignored',b'hdr?$x\0ignored',b'!unknown?\0ignored',b'x'*1023,b'?'+b'x'*1022,b'hdr?'+b'x'*1019]:add(name,label='name_extent_and_nul')
    for _ in range(4096):
        token=rng.choice(names+['unknown','!!hdr','!360','!','GPU > 2'])
        state=tuple(rng.getrandbits(32) for _ in range(4))
        add(rng.choice(['','!'])+token+'?'+rng.choice(['$x','','$x?other']),state,label='random_full_state_words')
    # Parse the three actual model VMT sources with the separately verified
    # owned raw reader. Feed only direct parameter names, retaining source hashes.
    prior_tokens=json.loads((ROOT/'outputs/material-string-value-20261025/verification-zkqa42up/results.json').read_text())
    materials=prior_tokens['authored_material_tokens'];raw_cases=[];source_provenance=[]
    for m in materials:
        p=Path(m['path']);require(sha(p)==m['sha256'],'Benchmark material identity')
        raw_cases.append((str(p),p.read_text(encoding='ascii')));source_provenance.append(dict(path=str(p),sha256=m['sha256']))
    parser=next(e for e in receipt['executables'] if Path(e['path']).name=='parser_owned_x64.exe');require(sha(Path(parser['path']))==parser['sha256'],'Owned raw reader executable identity')
    parser_input=run/'benchmark-input.bin';parser_output=run/'benchmark-owned-raw.bin'
    parser_input.write_bytes(struct.pack('<I',len(raw_cases))+b''.join(text(n)+text(s) for n,s in raw_cases))
    absent=run/'no-engine-present.dll';require(not absent.exists(),'Owned engine absence')
    execute(Path(parser['path']),[absent,parser_input,parser_output],run,'benchmark-owned-reader')
    documents=decode(parser_output.read_bytes());benchmark_keys=[]
    for doc in documents:
        for root in doc['roots']:
            for child in root['children']:
                benchmark_keys.append(child['key']);add(child['key'],(0,0,0,0),label='parsed_benchmark_direct_keys')
    require('360?$color2' in benchmark_keys,'Real benchmark conditional key')
    query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<6I',*row[:6])+row[6] for row in rows))
    outputs=[];executables=[];native_log=''
    for label,suffix,machine,magic in [('native','x86',0x14c,0x10b),('owned','x64',0x8664,0x20b)]:
        exe=OUT/f'condition_{label}_{suffix}.exe';b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0]
        require(struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==magic and b'asInvoker' in b,'Architecture and manifest')
        build=OUT/(label+'-build.log');require(not re.search(r'\b(warning|error)\b',build.read_text(),re.I),'Clean build '+label)
        imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(label+'-imports.txt')).write_text(imports)
        modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(set(m.lower() for m in modules)=={'kernel32.dll'},'Direct dependencies')
        result=run/(label+'.bin');log=execute(exe,[PRIVATE if label=='native' else absent,query,result],run,label)
        if label=='native':native_log=log
        outputs.append(result.read_bytes());require(len(outputs[-1])==12*len(rows),'Output framing')
        executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules,build_log_sha256=sha(build)))
    differences=[dict(case=i,label=labels[i],name_hex=rows[i][6].hex(),query=rows[i][:6],native=struct.unpack_from('<3I',outputs[0],12*i),owned=struct.unpack_from('<3I',outputs[1],12*i)) for i in range(len(rows)) if outputs[0][12*i:12*i+12]!=outputs[1][12*i:12*i+12]]
    (run/'comparison.json').write_text(json.dumps(dict(cases=len(rows),differences=differences),indent=2)+'\n')
    require(not differences,'Condition comparison; see comparison.json')
    analytical=execute(Path(executables[1]['path']),['--test'],run,'analytical');require('refusals=2' in analytical,'Owned analytical coverage')
    hdr=re.search(r'native_hdr_calls=(\d+) native_srgb_calls=(\d+) native_owner_warning_calls=(\d+)',native_log)
    require(hdr and all(int(n)>0 for n in hdr.groups()),'Original virtual service/warning branches executed')
    benchmark_results=[]
    for i,label in enumerate(labels):
        if label=='parsed_benchmark_direct_keys':
            record=struct.unpack_from('<3I',outputs[1],12*i);benchmark_results.append(dict(name=rows[i][6].decode('ascii'),skip=bool(record[0]),conditional=bool(record[1]),unrecognized=bool(record[2])))
    require(any(r['name']=='360?$color2' and r['skip'] and r['conditional'] and not r['unrecognized'] for r in benchmark_results),'Actual 360 benchmark selection')
    for p,h in receipt['original_hashes'].items():require(sha(Path(p))==h,'Protected original changed '+p)
    for m in materials:require(sha(Path(m['path']))==m['sha256'],'Benchmark source changed')
    for p,h in previous['source_sha256'].items():require(sha(ROOT/p)==h,'Previous numeric source changed')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());require(all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()),'Editor source dependency changed')
    require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor build changed')
    source_names=['material_parameter_condition_candidate.h','material_parameter_condition_native_fixture.h','probe_material_parameter_condition.cpp','build_material_parameter_condition.cmd','verify_material_parameter_condition.py']
    sources={};pending=[ROOT/'work'/n for n in source_names]
    while pending:
        p=pending.pop()
        if str(p) in sources:continue
        sources[str(p)]=sha(p)
        if p.suffix in ['.cpp','.h']:
            for inc in re.findall(r'^\s*#include\s*"([^"]+)"',p.read_text(),re.M):
                if (p.parent/inc).is_file():pending.append(p.parent/inc)
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(rows),groups=dict(Counter(labels)),exact_bytes=len(outputs[0]),differences=0,query_sha256=sha(query),executables=executables,analytical=analytical,native_log=native_log,static_ranges=evidence,condition_names=dict(zip(map(hex,addresses),names)),benchmark_sources=source_provenance,benchmark_results=benchmark_results,sources=sources,protected_original_files=len(receipt['original_hashes']),previous_numeric_sources_unchanged=True,editor_dependencies_unchanged=len(build['editor_source_dependencies']),editor_sha256=build['executable_sha256'],limits=['Parameter-name prefix filtering only; bracket-expression conditions, flags, duplicate/default/override rules and complete effective material loading remain unverified','Native function/name getter/copy/ASCII comparator execute unmodified in an isolated private image; node-name, warning, convar storage and hardware virtual services are controlled verification scaffolding','Hardware inputs are explicit fixture states, not discovered live SFM settings; srgb tests only low eight return bits while HDR tests all 32 bits','Names bounded to 1023 bytes and conditional prefix to 255; original unchecked longer-prefix stack overwrite is deliberately refused; output preserved on refusal','Benchmark direct-key path uses verified owned raw reader plus isolated condition comparison; no whole native material loader, proxy execution or visual/render equivalence claim','No production integration, editor rebuild/run, original writes, live SFM control, GPU or Blender; whole application remains incomplete'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(path=str(run),receipt_sha256=sha(run/'results.json')),indent=2)+'\n')
    print(json.dumps(dict(status='PASS',cases=len(rows),benchmark_keys=len(benchmark_results),analytical=analytical,receipt=str(run/'results.json')),indent=2))
if __name__=='__main__':main()
