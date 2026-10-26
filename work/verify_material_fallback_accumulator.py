"""Repeated successful fallback loads reset patch state; failures retain it."""
from pathlib import Path
from datetime import datetime,timezone
import json,random,re,struct,subprocess,tempfile,shutil
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_material_keyvalues_parser import DUMP
from verify_material_parameter_orchestration import node,string,integer,packet
from verify_material_patch_files import text
from verify_material_fallback_reference import decode
from verify_material_fallback_owned import queries,execute
from verify_ifm_draw_packet import check_ranges

OUT=ROOT/'outputs/material-fallback-accumulator-20261026'
OWNED=ROOT/'outputs/material-fallback-owned-20261026'
REF=ROOT/'outputs/material-fallback-20261026'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')

def find(n,path):
    current=n
    for key in path.split('/'):
        current=next((x for x in current['children'] if x['key'].lower()==key.lower()),None)
        if current is None:return None
    return current

def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    previous=json.loads((OWNED/'summary.json').read_text());prior_path=Path(previous['receipt']);prior=json.loads(prior_path.read_text());require(sha(prior_path)==previous['receipt_sha256'],'Prior accepted receipt')
    allowed={ROOT/'work/material_fallback_parameters_candidate.h',ROOT/'work/probe_material_fallback_owned.cpp',ROOT/'work/material_fallback_native_fixture.h'};changes={}
    for p,h in prior['sources'].items():
        current=sha(Path(p))
        if Path(p) in allowed:changes[p]=dict(before=h,after=current)
        else:require(current==h,'Unrelated accepted source changed '+p)
    require(len(changes)==3 and all(x['before']!=x['after'] for x in changes.values()),'Only three documented source changes')
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Protected original '+p)
    require(sha(PRIVATE)==sha(LIB)==PIN,'Private library identity')
    # Prior unaffected regression probes and all source closures remain pinned;
    # only the new fallback module and its direct owned probe require rebuilding.
    for e in prior['executables']:
        if Path(e['path']) not in [OWNED/'fallback_owned_x64.exe',REF/'fallback_reference_x86.exe']:require(sha(Path(e['path']))==e['sha256'],'Unaffected tested executable '+e['path'])
    binaries=[]
    for p,machine in [(REF/'fallback_reference_x86.exe',0x14c),(OWNED/'fallback_owned_x64.exe',0x8664)]:
        raw=p.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x20b if machine==0x8664 else 0x10b) and b'asInvoker' in raw,'Architecture/manifest')
        imports=subprocess.run([str(DUMP),'/imports',str(p)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require({x.lower() for x in modules}=={'kernel32.dll'},'Direct imports');binaries.append(dict(path=str(p),sha256=sha(p),machine=hex(machine),direct_imports=modules))
    build_logs={}
    for p in [OWNED/'owned-build.log',OWNED/'asm-build.log',REF/'native-build.log']:
        require(not re.search(r'\b(warning|error)\b',p.read_text(),re.I),'Fresh compiler/assembler diagnostics');build_logs[str(p)]=sha(p)
    _,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('whole_file_loader',0x10006ba0,0x10006cb4),('whole_patch_chain',0x10006970,0x10006b96),('whole_patch_accumulation',0x10005100,0x100051c1),('whole_KV_clear',0x10071230,0x1007125f),('escaped_quote_initializer',0x10094370,0x100943c9),('literal_quote_initializer',0x100943d0,0x100943f3),('conversion_constructor',0x1006ebe0,0x1006ec64),('quote_dispatch',0x10070620,0x100706b7)])
    clear=next(x for x in ranges if x['name']=='whole_patch_chain');require(any(x['address']==0x100069ab and x['instruction']=='call        10071230' for x in clear['instructions']),'Original accumulator clear precedes patch root check')
    libs=prior_path.parent/'copied-libraries'
    for name in ['tier0.dll','vstdlib.dll','materialsystem.dll','stdshader_dx9.dll','filesystem_stdio.dll']:require(sha(libs/name)==sha(GAME/'bin'/name),'Pinned private dependency '+name)
    # Freshly exercise the exact prior160 native requests against both probes.
    old=prior_path.parent;require(sha(old/'queries.bin')==prior['query_sha256'] and sha(old/'native.bin')==prior['output_sha256'],'Frozen160 requests/results')
    legacy_native=run/'legacy-native.bin';legacy_owned=run/'legacy-owned.bin';execute(REF/'fallback_reference_x86.exe',[PRIVATE,libs,old/'mount',old/'queries.bin',legacy_native],run,'legacy-native');execute(OWNED/'fallback_owned_x64.exe',[old/'mount',old/'queries.bin',legacy_owned,run/'no-engine.dll'],run,'legacy-owned');require(sha(legacy_native)==sha(legacy_owned)==prior['output_sha256'],'All160 fallback records unchanged')
    mount=run/'mount';mount.mkdir();cases=[];expected=[];patterns=['PP','PN','PPN','PNP','PPP','PNPN','PPM','PNM','PPS','PNS','PPPM','PPPS'];rng=random.Random(2026102613)
    def write(name,source):p=mount/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(source,encoding='ascii')
    for i in range(128):
        pattern=patterns[i%len(patterns)];model=rng.randrange(2);error=node(rng.choice(['VertexLitGeneric','Wireframe_DX9']),[string('$errorfixture','yes')]);owner='fixture/root';last_patch=None
        for stage,kind in enumerate(pattern):
            if kind in 'MS':break
            last_patch=stage if kind=='P' else None
            terminal=stage+1==len(pattern);next_kind=pattern[stage+1] if not terminal else ''
            target=owner if next_kind=='S' else 'matrix/'+str(i)+'/m'+str(stage+1)
            shader='VertexLitGeneric' if terminal else 'Wireframe'
            body=shader+'{$alpha 1'+(' $fallbackmaterial '+target if not terminal else '')+'}'
            name='materials/matrix/'+str(i)+'/'
            if kind=='P':
                write(name+'base'+str(stage)+'.vmt',body)
                write(name+'m'+str(stage)+'.vmt','patch{include '+name+'base'+str(stage)+'.vmt insert{marker'+str(stage)+' '+str(stage+1)+' $alpha '+rng.choice(['.125','.25','.5','.75'])+' "nested/value" '+str(stage+10)+'}}')
            else:write(name+'m'+str(stage)+'.vmt',body)
        source=node('Wireframe',[integer('$model',model),string('$fallbackmaterial','matrix/'+str(i)+'/m0')])
        if i%3==0:source['children'].append(node('GPU<1',[integer('$model',model)]))
        cases.append((owner,0,source,error));expected.append(dict(pattern=pattern,last_patch=last_patch,recovery=pattern[-1] in 'MS',error_shader=error['key']))
    inp=run/'queries.bin';inp.write_bytes(struct.pack('<I',len(cases))+b''.join(text(owner)+struct.pack('<I',prefix)+packet(source)+packet(error) for owner,prefix,source,error in cases));native=run/'native.bin';owned=run/'owned.bin'
    require(sha(inp)=='f42a59c0464758d2ed479078f19e5dd82bf784a8029c02c000bff21e3418c265','Retain exact expanded query corpus that exposed quoted-token fixture gap')
    native_log=execute(REF/'fallback_reference_x86.exe',[PRIVATE,libs,mount,inp,native],run,'native');owned_log=execute(OWNED/'fallback_owned_x64.exe',[mount,inp,owned,run/'absent-engine.dll'],run,'owned');n=decode(native.read_bytes());o=decode(owned.read_bytes());require(len(n)==len(o)==len(cases),'Case extent')
    differences=[dict(case=i,expected=expected[i],native=a,owned=b) for i,(a,b) in enumerate(zip(n,o)) if a!=b];(run/'comparison.json').write_text(json.dumps(dict(differences=len(differences),first_differences=differences[:6]),indent=2)+'\n');require(not differences and native.read_bytes()==owned.read_bytes(),'Repeated accumulator comparison; comparison.json')
    independent=0
    for record,e in zip(n,expected):
        aggregate=record['aggregate'];insert=find(aggregate,'insert');markers=[] if insert is None else [x['key'] for x in insert['children'] if x['key'].startswith('marker')]
        require(markers==([] if e['last_patch'] is None else ['marker'+str(e['last_patch'])]),'Only last successful file accumulator survives');independent+=1
        require(record['warnings']==int(e['recovery']),'Missing/self warning count');independent+=1
        require(record['root']['key']==(e['error_shader'] if e['recovery'] else 'VertexLitGeneric'),'Final recovery root');independent+=1
        if e['last_patch'] is None:require(not aggregate['children'],'Successful plain load clears entire accumulator');independent+=1
        else:require(find(aggregate,'insert/marker'+str(e['last_patch']))['bits']==e['last_patch']+1,'Correct last patch marker');independent+=1
    safety=execute(OWNED/'fallback_owned_x64.exe',['--test'],run,'safety');require('analytical_checks=20 refusals=10' in safety,'Updated bounded safety checks')
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Original changed '+p)
    for p,h in prior['sources'].items():require(sha(Path(p))==(changes[p]['after'] if p in changes else h),'Source changed during acceptance '+p)
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());require(all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()) and sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Production editor unchanged')
    count=re.search(r'allocations=(\d+) releases=(\d+) null_frees=(\d+)',native_log);require(count and count[1]==count[2],'Original tracked allocations balanced')
    sources={p:sha(Path(p)) for p in prior['sources']};sources[str(ROOT/'work/verify_material_fallback_accumulator.py')]=sha(ROOT/'work/verify_material_fallback_accumulator.py')
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(cases),exact_bytes=native.stat().st_size,differences=0,independent_checks=independent,patterns=patterns,expected=expected,query_sha256=sha(inp),output_sha256=sha(native),legacy_fallback_cases=prior['cases'],legacy_output_sha256=sha(legacy_native),safety=safety.strip(),executables=binaries,build_logs=build_logs,native_log=native_log,owned_log=owned_log,allocations=int(count[1]),releases=int(count[2]),null_frees=int(count[3]),static_ranges=ranges,sources=sources,documented_source_changes=changes,original_hashes=prior['original_hashes'],prior_regression_receipt=str(prior_path),prior_regression_receipt_sha256=sha(prior_path),unaffected_prior_regressions=prior['regressions'],editor_dependencies_unchanged=len(build['editor_source_dependencies']),editor_sha256=build['executable_sha256'],limits=['Original whole caller/KeyValues/file/patch/real filesystem run together; controlled material/name/symbol/heap/hardware/config/error-source services remain','Every successful file parse clears the accumulator before root-patch testing. Missing-root return and owner self-reference preserve the last successful accumulator; only its raw state is retained through recovery','128 fixed-default-environment string-name cases cover repeated patches/plain/missing/self sequences with512 independent checks; not normal public IMaterial/resource lifecycle','Malformed/partial included-file recovery, non-string names, multiple roots, cycles, unsafe/excessive paths and real builtin error-source content remain unverified/refused','Fallback module, its direct owned probe and reference quote-table initialization changed.160 prior fallback cases rerun freshly;886/159/660 earlier regressions were not rerun because their source closure and executable identities remain unchanged','No production integration/full-editor build/run, live SFM/computer use, original writes,GPU orBlender. Full application incomplete'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(path=str(run),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','exact_bytes','independent_checks','legacy_fallback_cases','safety','allocations','releases']},indent=2))

if __name__=='__main__':main()
