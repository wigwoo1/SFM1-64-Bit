"""Private complete KeyValues creation dispatch, parsing and constructor proof.

Owners null; real original name table and matrix resources execute. Pool growth,
thread synchronization and live effective material loading are separate gates.
"""
from collections import Counter
from datetime import datetime,timezone
from pathlib import Path
import hashlib,json,math,random,re,struct,subprocess,tempfile
from verify_ifm_draw_packet import check_ranges
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha

OUT=ROOT/'outputs/material-typed-dispatch-20261026'
PYTHON='C:/Users/wigwo/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    require(sha(LIB)==sha(PRIVATE)==PIN,'Pinned original library')
    image,evidence=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('complete_string_vector_matrix_creation_dispatch',0x10002ec0,0x10003384),
        ('matrix_identity_test',0x10017430,0x1001753f),
        ('complete_name_interning',0x10017570,0x10017601),
        ('complete_matrix_constructor',0x100176a0,0x10017762),
        ('complete_scalar_vector_string_constructors',0x10017780,0x10017acb),
        ('complete_material_resource_cleanup',0x10017ad0,0x10017b46),
        ('original_variable_name_getter',0x10017b80,0x10017bd0),
        ('complete_pooled_creation_factories',0x10018c30,0x10018d58),
        ('complete_original_material_destructor',0x10018e60,0x10018e89),
        ('pool_hot_free',0x100702b0,0x100702c9),
        ('pool_hot_allocate_and_refusal',0x10070390,0x100703de),
        ('KeyValues_name_getter',0x100705f0,0x1007061d),
        ('KeyValues_null_key_lookup',0x10070abd,0x10070ace),
        ('KeyValues_null_key_return',0x10070bd6,0x10070be7),
        ('KeyValues_type_getter',0x10071260,0x10071280),
        ('KeyValues_integer_getter_executable_prefix',0x10070c30,0x10070c90),
        ('KeyValues_float_getter_executable_prefix',0x10070ce0,0x10070d86),
        ('KeyValues_string_getter_executable_prefix',0x10071470,0x1007169e),
        ('symbol_table_constructor',0x10073600,0x1007366b),
        ('symbol_table_insert_or_reuse',0x10073670,0x10073835),
        ('symbol_table_teardown',0x10073580,0x10073598)])
    rows=[];labels=[];rng=random.Random(2026102603)
    names=[b'$FIXTURE',b'PREFIX?$BaSeTeXtUrEtRaNsFoRm',b'A?B?C',b'',b'MiXeD_Case',b'nul\0suffix']
    def add(kind,text=b'',scalar=0,mode=3,pool=1,fill=0xa5,name=None,label='controlled'):
        if isinstance(text,str):text=text.encode()
        if name is None:name=names[len(rows)%len(names)]
        if isinstance(name,str):name=name.encode()
        require(len(name)<=1023 and len(text)<=4095,'Generated input bounds')
        rows.append((kind,mode,pool,fill,scalar,len(name),len(text),name,text));labels.append(label)
    identity='[1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1]'
    examples=['','hello','1e3','16777217','2147483648','4294967296','-9223372036854775808','1e999','-1e-999','nan','inf','0x1p2',
        '[]','[1]','[1 2 3]','[bad]','[1 2 bad]','[1\n2]','{255 0 127.5}','{1 2 bad}',
        identity,identity[:-1],identity+' trailing','\v'+identity,identity.replace('0','-0'),
        'center .5 .5 scale 1 1 rotate 0 translate 0 0',
        'center 0 0 scale 1 1 rotate 13885.6279296875 translate 0 0',
        'center 0 0 scale 1 1 rotate 1e38 translate 0 0',
        'center 0 0 scale 1 1 rotate 1e999 translate 0 0',
        'center 0 0 scale 1 1 rotate 90 translate 0',
        'Center 0 0 scale 1 1 rotate 90 translate 0 0',
        'center 0 0 scale 1 1 rotate 90 translate 0 0 garbage','\0ignored','[1 2]\0ignored']
    special=[0,0x80000000,1,0x80000001,0x7fffff,0x800000,0x3f800000,0x3fffffff,0x4effffff,0x4f000000,0xcf000001,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f800001]
    for text in examples:
        for mode in range(4):
            for pool in [0,1]:add(1,text,mode=mode,pool=pool,label='route_order_and_refusal')
    for kind in [2,3]:
        for value in special:
            for mode in range(4):
                for pool in [0,1]:add(kind,scalar=value,mode=mode,pool=pool,label='scalar_bits_and_quieting')
    for kind in range(256):add(kind,'[1 2 3]',scalar=7,label='all_KeyValues_type_bytes')
    for fill in range(256):
        for mode in range(4):
            for text in [identity,'[1 2 3]','hello']:
                add(1,text,fill=fill,mode=mode,label='all_initial_object_bytes')
    for byte in range(256):
        for text in [identity,'hello']:
            add(1,text,name=bytes([byte])+b'Name',label='all_name_leading_bytes')
    for index in range(768):
        add(1,identity if index%2 else 'hello',name=f'PREFIX?Name_{index}',label='actual_name_table_growth_and_reuse')
        add(1,identity,name=f'prefix?NAME_{index}',label='actual_name_table_growth_and_reuse')
    for name in ['X'*1023,'prefix?'+'Y'*1016,'?',b'A?\0ignored']:
        for text in [identity,'hello']:add(1,text,name=name,label='name_limits_and_question_suffix')
    scan_query=ROOT/'outputs/material-matrix-scan-20261025/verification-qtomolk1/queries.bin'
    scan_receipt=json.loads((scan_query.parent/'results.json').read_text());require(sha(scan_query)==scan_receipt['query_sha256'],'Pinned previous scanner queries')
    raw=scan_query.read_bytes();at=0;scan_count=0
    while at<len(raw):
        q=struct.unpack_from('<18I',raw,at);at+=72;text=raw[at:at+q[17]];at+=q[17];scan_count+=1
        for mode in [0,3]:add(1,text,mode=mode,label='previous_full_formatted_reader_inputs')
    require(at==len(raw) and scan_count==4201,'Previous scanner corpus framing')
    vector_query=ROOT/'outputs/material-vector-literal-20261025/verification-2xyvwagn/queries.bin'
    vector_receipt=json.loads((vector_query.parent/'results.json').read_text());require(sha(vector_query)==vector_receipt['query_sha256'],'Pinned previous vector queries')
    raw=vector_query.read_bytes();at=0;vector_count=0
    while at<len(raw):
        q=struct.unpack_from('<6I',raw,at);at+=24;text=raw[at:at+q[5]];at+=q[5]
        if q[0]:add(1,text,label='previous_vector_literal_inputs');vector_count+=1
    require(at==len(raw),'Previous vector corpus framing')
    def value():
        bits=rng.randrange(0x7f800000)|(rng.randrange(2)<<31)
        return format(struct.unpack('<f',struct.pack('<I',bits))[0],'.9g')
    for _ in range(2048):
        text='['+' '.join(value() for _ in range(16))+']'
        add(1,text,mode=rng.randrange(4),label='random_full_matrices')
    for _ in range(2048):
        p=[format(rng.uniform(-10,10),'.9g') for _ in range(7)];p[4]=value()
        text=f'center {p[0]} {p[1]} scale {p[2]} {p[3]} rotate {p[4]} translate {p[5]} {p[6]}'
        add(1,text,mode=rng.randrange(4),label='random_composed_texture_transforms')
    for _ in range(2048):
        text=rng.choice(['',' ','\t','\v'])+rng.choice(['','+','-'])+str(rng.getrandbits(70))+rng.choice(['e3','e-300','.5','suffix'])
        add(1,text,mode=rng.randrange(4),label='random_ordinary_string_construction')
    for _ in range(2048):
        add(rng.choice([2,3]),scalar=rng.getrandbits(32),mode=rng.randrange(4),label='random_scalar_construction')
    prior=json.loads((ROOT/'outputs/material-string-value-20261025/verification-zkqa42up/results.json').read_text())
    material_tokens=0
    for material in prior['authored_material_tokens']:
        require(sha(material['path'])==material['sha256'],'Pinned material input')
        for token in re.findall(rb'"([^"\r\n]*)"',Path(material['path']).read_bytes()):
            for mode in range(4):add(1,token,mode=mode,label='benchmark_literal_tokens_not_effective_materials');material_tokens+=1
    query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<7I',*row[:7])+row[7]+row[8] for row in rows))
    outputs=[];exes=[];logs=[];analytical_log=''
    for tag,suffix,machine,magic in [('native','x86',0x14c,0x10b),('owned','x64',0x8664,0x20b)]:
        exe=OUT/f'material_typed_dispatch_{tag}_{suffix}.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic and b'asInvoker' in raw,'Architecture/non-elevated manifest')
        build=OUT/(tag+'-build.log');require(not re.search(r'\b(warning|error)\b',build.read_text(),re.I),'Clean compiler '+tag)
        if tag=='owned':
            require(not re.search(r'\b(warning|error)\b',(OUT/'asm-build.log').read_text(),re.I),'Clean assembler')
            p=subprocess.run([str(exe),'--test'],capture_output=True,text=True,timeout=30);(run/'analytical.log').write_text(p.stdout+p.stderr);require(p.returncode==0 and 'refusals=3' in p.stdout,'Owned analytical checks')
            analytical_log=p.stdout.strip()
        library=PRIVATE if tag=='native' else run/'no-engine-present.dll'
        if tag=='owned':require(not library.exists(),'Owned engine independence argument')
        result=run/(tag+'.bin');p=subprocess.run([str(exe),str(library),str(query),str(result)],capture_output=True,text=True,timeout=60)
        (run/(tag+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,'Execution '+tag+': '+str(p.returncode)+' '+p.stdout+p.stderr)
        outputs.append(result.read_bytes());logs.append(p.stdout.strip());exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),build_log_sha256=sha(build),library_argument=str(library)))
    def decode(raw):
        records=[];at=0
        for _ in rows:
            require(at+112<=len(raw),'Output record header');h=struct.unpack_from('<28I',raw,at);end=at+112+h[10]+h[11]
            require(end<=len(raw),'Output record strings');records.append((h,raw[at+112:at+112+h[10]],raw[at+112+h[10]:end],raw[at:end]));at=end
        require(at==len(raw),'Trailing output');return records
    native,owned=map(decode,outputs)
    differences=[dict(case=i,label=labels[i],query=rows[i][:7],name_hex=rows[i][7].hex(),input_hex=rows[i][8].hex(),native=n[3].hex(),owned=o[3].hex()) for i,(n,o) in enumerate(zip(native,owned)) if n[3]!=o[3]]
    types=Counter(str(h[1]&15) if h[0] else 'none' for h,_,_,_ in native)
    services=re.search(r'native_allocations=(\d+) native_releases=(\d+) unique_native_symbols=(\d+) reused_native_symbols=(\d+)',logs[0])
    require(services is not None,'Native resource observations')
    service_counts=dict(zip(['allocation_records','released_records','unique_symbols','reused_symbols'],map(int,services.groups())))
    require(service_counts['allocation_records']==service_counts['released_records'],'Native resource balance')
    require(service_counts['unique_symbols']>=768 and service_counts['reused_symbols']>=768 and service_counts['unique_symbols']+service_counts['reused_symbols']==len(rows)-types['none'],'Actual symbol creation/reuse observations')
    protected=json.loads((ROOT/'outputs/material-trig-fallback-20261026/summary.json').read_text())
    require(all(sha(ROOT/name)==digest for name,digest in protected['source_closure'].items()),'Protected arithmetic/native-fixture sources changed')
    require(sha(ROOT/'work/material_string_value_candidate.h')=='4a9b16d46bd01e16f034922777e94cfb0d700ea614ab16198ec46c0bf8256007','Protected string setter changed')
    require(sha(LIB)==sha(PRIVATE)==PIN,'Original library unchanged')
    receipt=dict(status='FAIL' if differences else 'PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(rows),groups=dict(Counter(labels)),types=dict(types),differences=differences,executables=exes,logs=logs,exact_bytes=len(outputs[0]),query_sha256=sha(query),native_output_sha256=sha(run/'native.bin'),owned_output_sha256=sha(run/'owned.bin'),static_ranges=evidence,library_sha256=PIN,
        reused_formatted_reader_cases=scan_count,reused_vector_literal_cases=vector_count,benchmark_token_cases=material_tokens,
        native_services=service_counts,owned_analytical_log=analytical_log,
        scope='Actual original single-node KeyValues dispatcher, matrix/vector/scalar/string parsing, constructors, normalized active values/name/matrix identity, original symbol registration and cleanup; owned x64 value equivalent',
        limits=['Owners null; KeyValues node/name provider, allocator, null queue and single-thread lock services controlled',
                'Original symbol table constructor, intern/growth/reuse and teardown execute; symbol IDs and pointer identity normalized out',
                'Original40-byte pool hot pop/push and block-size refusal execute on supplied slot; pool growth/OOM/concurrency not verified',
                'Owned RAII allocation timing differs; native heap balances are checked, not claimed physically identical',
                'Allocation/release record counts include realloc replacement records; not physical malloc/free timing parity',
                'Warning counts compared, not warning message text; raw pointers and symbol IDs are excluded from owned value representation',
                'Benchmark literal assignments do not establish loaded effective VMTs, conditional evaluation or renderer selection',
                'No live SFM access, original writes, editor integration or complete editor build/run'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(receipt=str(run/'results.json'),cases=len(rows),differences=len(differences),groups=dict(Counter(d['label'] for d in differences)),first=differences[:2],types=dict(types))))
    require(not differences,'Complete typed dispatch parity')

if __name__=='__main__':main()
