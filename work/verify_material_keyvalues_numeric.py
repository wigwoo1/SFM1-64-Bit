"""Raw material numeric conversion correction, not effective shader loading.

Native reference runs only workspace copies in isolated test processes. Owned
readers run without original libraries. No live SFM, editor, GPU or original writes.
"""
from collections import Counter
from datetime import datetime,timezone
from decimal import Decimal,localcontext
from pathlib import Path
import hashlib,json,random,re,shutil,struct,subprocess,tempfile
from verify_material_keyvalues_parser import text,decode,DUMP
from verify_ifm_draw_packet import check_ranges

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/material-keyvalues-numeric-20261026'
PRIOR=ROOT/'outputs/material-keyvalues-parser-20261017/verification-n2zi61gz'
FILES=ROOT/'outputs/material-keyvalues-files-20261017/verification-j6da5x2q'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(ok,message):
    if not ok:raise RuntimeError(message)
def invoke(exe,args,run,label):
    p=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=120)
    (run/(label+'.log')).write_text(p.stdout+p.stderr)
    need(p.returncode==0 and 'KeyValues Error:' not in p.stdout+p.stderr,'Execution '+label+'; see log')
    return p.stdout.strip()
def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    previous=json.loads((PRIOR/'results.json').read_text());files=json.loads((FILES/'results.json').read_text())
    need(previous['status']==files['status']=='PASS','Previous acceptance receipts')
    originals=files['original_hashes']
    for p,h in originals.items():need(sha(p)==h,'Original identity '+p)
    libraries=FILES/'copied-libraries'
    for name in ['tier0.dll','vstdlib.dll','materialsystem.dll','filesystem_stdio.dll']:
        original=next(p for p in originals if Path(p).name==name)
        need(sha(libraries/name)==originals[original],'Private library identity '+name)
    material=next(p for p in originals if Path(p).name=='materialsystem.dll')
    _,evidence=check_ranges(Path(material),'materialsystem-disassembly.txt',[
        ('raw_scalar_selection',0x100717c0,0x10071c9a),
        ('legacy_strtod_wrapper',0x1007e38d,0x1007e4c7)])
    original_input=PRIOR/'input.bin';need(sha(original_input)==previous['input_sha256'],'Previous raw input identity')
    prior_bytes=original_input.read_bytes();prior_count=struct.unpack_from('<I',prior_bytes)[0]
    cases=[];groups=[];tokens=[]
    def add(value,group,state=True):
        source='S{v "'+value+'"'+(' max 2147483647 min -2147483648' if state else '')+'}'
        cases.append(('numeric-'+str(len(cases)),source));groups.append(group);tokens.append(value)
    for sign in ['','+','-']:
        for number in ['0','1','12','.5','1.','1.25','2147483647','2147483648']:
            for suffix in ['e','e+','e-','E+','d','d+','D-','d3','D-3','e3','e-3','e9999','d9999','e-9999','d-9999','e3tail','d3tail']:
                add(sign+number+suffix,'exponent_grammar')
    for lead in ['',' ','\t','\v','\f','\r','\n',' \t']:
        for value in ['1d3','-1D-310','-0d9999','1d-9999','+1D309','1d','1e+','nan','inf','0x1p2']:
            for tail in ['',' ','suffix']:add(lead+value+tail,'whitespace_and_prefix')
    for power in sorted(set([-9999,-5201,-5200,-4933,-4932,-1000,-400,-325,-324,-323,-310,-309,-308,-307,-150,-50,0,1,38,39,99,307,308,309,400,1000,4932,4933,5200,5201,9999]+list(range(-5205,-5194))+list(range(-4940,-4920)))):
        for coefficient in ['0','-0','1','-1','2.2250738585072014','-4.9406564584124654']:
            for marker in ['e','E','d','D']:add(coefficient+marker+str(power),'range_and_sticky_state')
    rng=random.Random(2026102604)
    for _ in range(4096):
        size=rng.randrange(1,100);digits=''.join(str(rng.randrange(10)) for _ in range(size));point=rng.randrange(size+1)
        value=rng.choice(['','+','-'])+digits[:point]+'.'+digits[point:]+rng.choice(['e','E','d','D'])+str(rng.randrange(-600,601))
        add(value,'long_decimal_coefficients')
    # Adjacent binary32 midpoints and perturbations exercise legacy decimal
    # rounding, including normal/subnormal boundaries and both signs.
    with localcontext() as ctx:
        ctx.prec=240
        for _ in range(1024):
            bits=rng.randrange(1,0x7f7ffffe)
            lo=struct.unpack('<f',struct.pack('<I',bits))[0];hi=struct.unpack('<f',struct.pack('<I',bits+1))[0]
            middle=(Decimal.from_float(lo)+Decimal.from_float(hi))/2
            epsilon=(Decimal.from_float(hi)-Decimal.from_float(lo))*Decimal('1e-20')
            for value in [middle-epsilon,middle,middle+epsilon]:
                for sign in ['', '-']:
                    literal=format(value,'f')
                    if '.' not in literal:literal+='.0'
                    add(sign+literal,'float32_midpoint_neighborhood')
    for length in [24,25,26,64,512,4096,32767]:
        for value in ['9'*length,'0'*(length-1)+'1','0.'+'1'*(length-2)]:add(value,'token_numeric_extent')
    input=run/'input.bin';input.write_bytes(struct.pack('<I',prior_count+len(cases))+prior_bytes[4:]+b''.join(text(n)+text(s) for n,s in cases))
    outputs=[];executables=[];no_engine=run/'no-engine-present';need(not no_engine.exists(),'Owned library independence')
    logs=['native','files-native','prefix-native','parser','parser_safety','files_safety','numeric_safety','files','prefix']
    for label in logs:
        build=OUT/(label+'-build.log');need(build.is_file() and not re.search(r'\b(warning|error)\b',build.read_text(),re.I),'Clean build '+label)
    def identity(name,machine):
        exe=OUT/(name+'.exe');b=exe.read_bytes();offset=struct.unpack_from('<I',b,60)[0]
        need(struct.unpack_from('<H',b,offset+4)[0]==machine and struct.unpack_from('<H',b,offset+24)[0]==(0x20b if machine==0x8664 else 0x10b) and b'asInvoker' in b,'Architecture and manifest '+name)
        imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout
        (run/(name+'-imports.txt')).write_text(imports)
        modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I)
        need(set(x.lower() for x in modules)=={'kernel32.dll'},'Direct runtime imports '+name)
        executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules));return exe
    for name,machine in [('parser_native_x86',0x14c),('parser_owned_x64',0x8664)]:
        exe=identity(name,machine);output=run/(name+'.bin')
        invoke(exe,[libraries if machine==0x14c else no_engine,input,output],run,name);outputs.append(output.read_bytes())
    native,owned=map(decode,outputs)
    differences=[dict(case=i,group='previous_raw' if i<prior_count else groups[i-prior_count],token=None if i<prior_count else tokens[i-prior_count],native=a,owned=b) for i,(a,b) in enumerate(zip(native,owned)) if a!=b]
    raw_receipt=dict(cases=len(native),prior_cases=prior_count,groups=dict(Counter(groups)),input_sha256=sha(input),differences=differences,output_sha256=hashlib.sha256(outputs[0]).hexdigest())
    (run/'raw-comparison.json').write_text(json.dumps(raw_receipt,indent=2)+'\n')
    need(len(native)==len(owned)==prior_count+len(cases) and not differences,'Raw numeric comparison; see raw-comparison.json')
    need(outputs[0]==outputs[1],'Exact raw wire equality')
    # Preserve the original 423-document prefix outputs byte for byte.
    old_native=(PRIOR/'parser_native_x86.bin').read_bytes();old_decoded=decode(old_native)
    need(native[:prior_count]==old_decoded,'Previous raw acceptance output changed')
    # Show that this corpus catches the previously accepted implementation.
    old_exe=previous['executables'][1];need(sha(old_exe['path'])==old_exe['sha256'],'Previous owned executable identity')
    old_output=run/'previous-owned.bin';invoke(Path(old_exe['path']),[no_engine,input,old_output],run,'previous-owned')
    old_values=decode(old_output.read_bytes());old_differences=[i for i,(a,b) in enumerate(zip(native,old_values)) if a!=b]
    (run/'previous-owned-differences.json').write_text(json.dumps([dict(case=i,group='previous_raw' if i<prior_count else groups[i-prior_count],token=None if i<prior_count else tokens[i-prior_count],native=native[i],previous=old_values[i]) for i in old_differences],indent=2)+'\n')
    need(old_differences,'Correction regression must reproduce original defect')
    prefix_input=run/'prefix-input.bin';prefix_input.write_bytes(struct.pack('<I',len(tokens))+b''.join(text(s) for s in tokens))
    prefix_outputs=[]
    for name,machine in [('prefix_native_x86',0x14c),('prefix_owned_x64',0x8664)]:
        exe=identity(name,machine);output=run/(name+'.bin')
        invoke(exe,[libraries if machine==0x14c else no_engine,prefix_input,output],run,name);prefix_outputs.append(output.read_bytes())
    need(all(len(b)==16*len(tokens) for b in prefix_outputs),'Prefix output framing')
    prefix_diff=[dict(case=i,token=s,native=prefix_outputs[0][16*i:16*i+16].hex(),owned=prefix_outputs[1][16*i:16*i+16].hex()) for i,s in enumerate(tokens) if prefix_outputs[0][16*i:16*i+16]!=prefix_outputs[1][16*i:16*i+16]]
    (run/'prefix-comparison.json').write_text(json.dumps(dict(cases=len(tokens),differences=prefix_diff),indent=2)+'\n')
    need(not prefix_diff,'Prefix value/end/range comparison; see prefix-comparison.json')
    native_files=identity('files_native_x86',0x14c);owned_files=identity('files_owned_x64',0x8664);scenarios=[]
    for scenario in files['scenarios']:
        label=scenario['name'];ni=FILES/(label+'-native-input.bin');oi=FILES/(label+'-owned-input.bin')
        need(sha(ni)==scenario['native_input_sha256'] and sha(oi)==scenario['owned_input_sha256'],'Previous file input '+label)
        no=run/(label+'-native.bin');oo=run/(label+'-owned.bin')
        invoke(native_files,[libraries,ni,no],run,label+'-native');invoke(owned_files,[oi,oo],run,label+'-owned')
        need(no.read_bytes()==oo.read_bytes() and sha(oo)==scenario['output_sha256'],'File regression '+label)
        scenarios.append(dict(name=label,cases=scenario['cases'],output_sha256=sha(oo)))
    # Exercise fresh D-exponent range state across a real #include/#base load.
    folder=run/'material-files';folder.mkdir();catalog={};paths=[]
    for i,token in enumerate(['1d3','-1D3','1d-9999','-1D309','1d-310','-0d9999']):
        root=f'root-{i}.vmt';child=f'child-{i}.vmt';base=f'base-{i}.vmt'
        catalog[root]=f'#base {base} Root{{v "{token}"}} #include {child}'
        catalog[child]='Child{max 2147483647 min -2147483648}'
        catalog[base]='Base{base 2147483647}'
        paths.append(root)
    for name,source in catalog.items():(folder/name).write_text(source,encoding='ascii')
    ni=run/'new-files-native-input.bin';oi=run/'new-files-owned-input.bin'
    ni.write_bytes(struct.pack('<I',1)+text(str(folder))+text('GAME')+struct.pack('<II',1,len(paths))+b''.join(text(n)+text('GAME') for n in paths))
    oi.write_bytes(struct.pack('<I',len(catalog))+b''.join(text(n)+text(s) for n,s in catalog.items())+struct.pack('<I',len(paths))+b''.join(text(n) for n in paths))
    no=run/'new-files-native.bin';oo=run/'new-files-owned.bin'
    invoke(native_files,[libraries,ni,no],run,'new-files-native');invoke(owned_files,[oi,oo],run,'new-files-owned')
    need(no.read_bytes()==oo.read_bytes(),'New numeric file integration');scenarios.append(dict(name='new-numeric-file-integration',cases=len(paths),output_sha256=sha(oo)))
    safety={}
    for label in ['parser_safety','files_safety','numeric_safety']:
        exe=identity(label+'_owned_x64',0x8664);safety[label]=invoke(exe,[],run,label)
    for p,h in originals.items():need(sha(p)==h,'Original changed '+p)
    dependencies=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text())
    need(all(sha(ROOT/p)==h for p,h in dependencies['editor_source_dependencies'].items()),'Editor dependency changed')
    need(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==dependencies['executable_sha256'],'Editor build changed')
    sources={};pending=[ROOT/'work'/n for n in ['source_material_keyvalues_parser_candidate.h','build_material_keyvalues_numeric.cmd','verify_material_keyvalues_numeric.py','build_material_keyvalues_numeric_regression.cmd','verify_material_keyvalues_numeric_regression.py','test_material_keyvalues_numeric.cpp','probe_material_decimal_prefix_status.cpp','probe_material_keyvalues_parser.cpp','probe_material_keyvalues_files_native.cpp','probe_material_keyvalues_files_owned.cpp','test_material_keyvalues_parser.cpp','test_material_keyvalues_files.cpp','probe_material_value_construction.cpp','probe_material_vector_literal.cpp','probe_material_matrix_scan.cpp','probe_material_typed_dispatch.cpp']]
    while pending:
        p=pending.pop()
        if str(p) in sources:continue
        sources[str(p)]=sha(p)
        if p.suffix in ['.cpp','.h']:
            for n in re.findall(r'^\s*#include\s*"([^"]+)"',p.read_text(),re.M):
                if (p.parent/n).is_file():pending.append(p.parent/n)
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),raw=raw_receipt,prefix_status_cases=len(tokens),previous_owned_differences=len(old_differences),file_scenarios=scenarios,safety=safety,executables=executables,static_ranges=evidence,original_hashes=originals,sources=sources,build_logs={str(OUT/(n+'-build.log')):sha(OUT/(n+'-build.log')) for n in logs},editor_dependencies_unchanged=len(dependencies['editor_source_dependencies']),editor_sha256=dependencies['executable_sha256'],limits=['Raw parser and include/base resolver only; not effective material/shader parameter selection','Original x86 processes load workspace copies for verification only; owned x64 readers receive no engine libraries','Controlled ASCII C-locale and original errno reset per document; sticky within-document and resolved-file state covered','No live SFM/computer use, editor rebuild/run, original writes, GPU or Blender','Whole application remains incomplete'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    (OUT/'latest-verification.json').write_text(json.dumps(dict(path=str(run),receipt_sha256=sha(run/'results.json')),indent=2)+'\n')
    print(json.dumps(dict(status='PASS',raw_cases=len(native),old_defects=len(old_differences),file_cases=sum(s['cases'] for s in scenarios),safety=safety,receipt=str(run/'results.json')),indent=2),flush=True)
if __name__=='__main__':main()
