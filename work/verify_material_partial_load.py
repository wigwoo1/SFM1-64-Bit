"""Owned partial-load comparison; explicitly audit changes to accepted modules."""
from pathlib import Path
from datetime import datetime,timezone
import json,random,re,struct,subprocess,tempfile
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_material_keyvalues_parser import DUMP
from verify_material_parameter_orchestration import node,string,integer,packet
from verify_material_patch_files import text
from verify_material_fallback_reference import decode
from verify_ifm_draw_packet import check_ranges

OUT=ROOT/'outputs/material-partial-load-owned-20261026'
CHANGED=['source_material_keyvalues_parser_candidate.h','source_material_keyvalues_files_candidate.h','source_material_patch_files_candidate.h','material_parameter_orchestration_candidate.h','material_fallback_parameters_candidate.h','probe_material_fallback_owned.cpp']

def request(cases):return struct.pack('<I',len(cases))+b''.join(text(name)+struct.pack('<I',prefix)+packet(source)+packet(error) for name,prefix,source,error in cases)

def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    summary=json.loads((ROOT/'outputs/material-fallback-failures-20261026/summary.json').read_text());receipt=Path(summary['receipt']);prior=json.loads(receipt.read_text());require(sha(receipt)==summary['receipt_sha256'],'Prior receipt')
    allowed={ROOT/'work'/p for p in CHANGED};changes={}
    for p,h in prior['sources'].items():
        current=sha(Path(p))
        if Path(p) in allowed:require(current!=h,'Intentional edit missing');changes[p]=dict(before=h,after=current)
        else:require(current==h,'Unrelated accepted source '+p)
    require(len(changes)==len(CHANGED),'Exact modified source set')
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Protected original '+p)
    require(sha(PRIVATE)==sha(LIB)==PIN,'Private original library')
    accumulator=json.loads(Path(prior['prior_receipt']).read_text());old_path=Path(accumulator['prior_regression_receipt']);older=json.loads(old_path.read_text());libs=old_path.parent/'copied-libraries'
    for name in ['tier0.dll','vstdlib.dll','materialsystem.dll','stdshader_dx9.dll','filesystem_stdio.dll']:require(sha(libs/name)==sha(LIB.parent/name),'Private dependency '+name)
    binaries=[];logs={}
    for p in OUT.glob('*_x64.exe'):
        if p.name.removesuffix('_owned_x64.exe') not in ['partial','parser_safety','files_safety','numeric_safety','patch_safety','parser','files','fallback','parameters','pipeline']:continue
        binaries.append(p)
    binaries.append(OUT/'reference_x86.exe');require(len(binaries)==11,'Fresh binary extent');executables=[]
    for p in binaries:
        machine=0x14c if p.name=='reference_x86.exe' else 0x8664;raw=p.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x20b if machine==0x8664 else 0x10b) and b'asInvoker' in raw,'Architecture/manifest '+p.name)
        imports=subprocess.run([str(DUMP),'/imports',str(p)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require({x.lower() for x in modules}=={'kernel32.dll'},'Direct imports '+p.name)
        executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine),direct_imports=modules))
    for p in OUT.glob('*-build.log'):require(not re.search(r'\b(warning|error)\b',p.read_text(),re.I),'Compiler diagnostics '+p.name);logs[str(p)]=sha(p)
    require(len(logs)==12,'Complete fresh build logs')
    _,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('whole_diagnostic_reporter',0x10070460,0x100704f1),('whole_recursive_parser',0x100717c0,0x10071c9a),('whole_patch_chain',0x10006970,0x10006b96),('whole_file_loader',0x10006ba0,0x10006cb4)])
    allocations=0
    def execute(exe,args,label):
        nonlocal allocations
        p=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,errors='replace',timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        require(p.returncode==0,'Execution '+label+' exit='+str(p.returncode)+'; see log')
        count=re.search(r'allocations=(\d+) releases=(\d+) null_frees=(\d+)',p.stdout)
        if count:require(count[1]==count[2],'Tracked allocation cleanup '+label);allocations+=int(count[1])
        return p.stdout+p.stderr
    ref=OUT/'reference_x86.exe';owned=OUT/'fallback_owned_x64.exe';comparisons=[]
    def compare(mount,inp,label,count,expected=None):
        a=run/(label+'-native.bin');b=run/(label+'-owned.bin');execute(ref,['--caller',PRIVATE,libs,mount,inp,a],label+'-native');execute(owned,[mount,inp,b,run/'no-engine.dll'],label+'-owned')
        if expected:require(sha(a)==expected,'Frozen original '+label)
        n=decode(a.read_bytes());o=decode(b.read_bytes());require(len(n)==len(o)==count,'Comparison count '+label)
        differences=[dict(case=i,native=x,owned=y) for i,(x,y) in enumerate(zip(n,o)) if x!=y];(run/(label+'-comparison.json')).write_text(json.dumps(dict(differences=len(differences),first=differences[:3]),indent=2)+'\n')
        require(not differences and a.read_bytes()==b.read_bytes(),'Partial load comparison '+label)
        comparisons.append(dict(label=label,cases=count,bytes=a.stat().st_size,query_sha256=sha(inp),output_sha256=sha(a)));return n
    for label,folder,v in [('legacy160',old_path.parent,older),('legacy128',Path(prior['prior_receipt']).parent,accumulator)]:
        require(sha(folder/'queries.bin')==v['query_sha256'],'Legacy request identity');compare(folder/'mount',folder/'queries.bin',label,v['cases'],v['output_sha256'])
    # Reuse exact prior malformed-case inputs and frozen original outputs.
    frozen=receipt.parent;malformed=[x for x in prior['records'] if x['mode']=='caller']
    for i,row in enumerate(malformed):
        inp=frozen/(row['label']+'.in');require(sha(inp)==row['query_sha256'],'Frozen failure request');compare(frozen/'mount',inp,'failure'+str(i),1,row['output_sha256'])
    mount=run/'mount';(mount/'materials').mkdir(parents=True);fixture_hashes={}
    def write(name,source):
        p=mount/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(source,encoding='ascii');fixture_hashes[str(p)]=sha(p)
    discovery=OUT/'parser-discovery-3qtxhvof/records.json';samples=json.loads(discovery.read_text());cases=[]
    # Previously observed retained-context diagnostics, complete partial trees,
    # EOF and missing-value paths at multiple nested depths.
    for i,s in enumerate(samples):
        if len(s['record']['roots'])!=1:continue
        write('materials/parser'+str(i)+'.vmt',s['source']);cases.append(('fixture/root',0,node('Wireframe',[string('$fallbackmaterial','parser'+str(i))]),node('Wireframe_DX9')))
    nested_count=len(cases);rng=random.Random(2026102615);expected=[]
    for i in range(128):
        depth=i%6;kind=['missing','empty','truncated','missing_value','nested_eof','valid'][i%6];prefix='materials/varied/'+str(i)+'/'
        # Permute the terminal kind independently from chain length.
        kind=rng.choice(['missing','empty','truncated','missing_value','nested_eof','valid']);alpha=rng.choice(['.125','.25','.375','.5'])
        terminal='VertexLitGeneric{'+('$alpha '+alpha if kind in ['truncated','valid'] else '$alpha' if kind=='missing_value' else 'group{child 7')+('}' if kind in ['valid','missing_value'] else '')
        end_name=prefix+'end.vmt'
        if kind!='missing':write(end_name,'' if kind=='empty' else terminal)
        for d in range(depth):
            target=prefix+('end.vmt' if d+1==depth else 'p'+str(d+1)+'.vmt')
            write(prefix+'p'+str(d)+'.vmt','patch{include '+target+' insert{marker'+str(d)+' '+str(d+1)+' $alpha '+rng.choice(['.25','.5','.75'])+' "nested/value" '+str(d)+'}}')
        logical='varied/'+str(i)+('/p0' if depth else '/end');err=rng.choice(['Wireframe_DX9','VertexLitGeneric'])
        cases.append(('fixture/root',0,node('Wireframe',[integer('$model',rng.randrange(2)),string('$fallbackmaterial',logical)]),node(err,[string('errorMarker','yes')])))
        expected.append(dict(kind=kind,depth=depth,error=err))
    inp=run/'expanded.in';inp.write_bytes(request(cases));require(sha(inp)==sha(OUT/'verification-vwxdle8i/expanded.in'),'Keep exact initially refused expanded corpus');records=compare(mount,inp,'expanded',len(cases));independent=0
    for row,sample in zip(records[:nested_count],samples):
        require(row['warnings']==sample['record']['warnings'] and row['root']==sample['record']['roots'][0],'Nested full tree/context diagnostic reference');independent+=1
    for row,e in zip(records[nested_count:],expected):
        failed=e['kind'] in ['missing','empty'];require(row['root']['key']==(e['error'] if failed and not e['depth'] else 'patch' if failed else 'VertexLitGeneric'),'Expanded final root');independent+=1
        require(row['warnings']==(1 if failed else 8 if e['kind']=='nested_eof' else 3 if e['kind'] in ['truncated','missing_value'] else 0),'Expanded diagnostic count');independent+=1
    for p,h in fixture_hashes.items():require(sha(Path(p))==h,'Disposable provider input changed '+p)
    # Affected strict components must preserve the prior independently obtained
    # native bytes after fresh compilation, not only old executable identities.
    regressions=[]
    loop=Path(json.loads((ROOT/'outputs/material-parameter-orchestration-20261026/summary.json').read_text())['receipt']);lv=json.loads(loop.read_text());dest=run/'strict-loop.bin';require(sha(loop.parent/'queries.bin')==lv['query_sha256'],'Strict loop input');execute(OUT/'parameters_owned_x64.exe',[run/'absent.dll',run/'absent-dir',loop.parent/'queries.bin',dest],'strict-loop');require(sha(dest)==lv['output_sha256'],'886 strict parameter records');regressions.append(dict(kind='parameter_loop',cases=lv['cases'],output_sha256=sha(dest)))
    files=Path(json.loads((ROOT/'outputs/material-file-parameters-20261026/summary.json').read_text())['receipt']);fv=json.loads(files.read_text())
    for group in fv['groups']:
        label=group['name'];folder=files.parent/label;require(sha(folder/'file-queries.bin')==group['query_sha256'],'Frozen file input')
        raw=run/(label+'-raw.bin');params=run/(label+'-params.bin');projection=run/(label+'-projection.bin');meta=run/(label+'-meta.bin');execute(OUT/'pipeline_owned_x64.exe',[folder/'file-queries.bin',raw,params,projection,meta,run/'no-engine.dll'],'strict-file-'+label)
        require(sha(raw)==group['raw_output_sha256'] and sha(params)==group['parameter_output_sha256'] and sha(projection)==group['projection_sha256'],'Strict file/parameter bytes '+label)
    regressions.append(dict(kind='file_parameters',cases=fv['cases']))
    old=ROOT/'outputs/material-patch-files-20261017/verification-zsdt1ra8';ov=json.loads((old/'results.json').read_text())
    for group in ov['scenarios']:
        q=old/(group['name']+'-input.bin');require(sha(q)==group['input_sha256'],'Strict raw file input');dest=run/(group['name']+'-strict-files.bin');execute(OUT/'files_owned_x64.exe',[q,dest],'strict-raw-'+group['name']);require(sha(dest)==group['output_sha256'],'Strict raw file bytes')
    regressions.append(dict(kind='raw_patch_files',cases=sum(x['cases'] for x in ov['scenarios'])))
    raw_receipt=Path(json.loads((ROOT/'outputs/material-keyvalues-numeric-20261026/summary.json').read_text())['raw_receipt']);rv=json.loads(raw_receipt.read_text());raw_input=raw_receipt.parent/'input.bin'
    require(sha(raw_input)==rv['raw']['input_sha256'],'Frozen numeric raw parser request');dest=run/'strict-parser.bin';execute(OUT/'parser_owned_x64.exe',[run/'no-engine.dll',raw_input,dest],'strict-parser');require(sha(dest)==rv['raw']['output_sha256'],'12724 strict numeric parser bytes');regressions.append(dict(kind='raw_numeric_parser',cases=rv['raw']['cases'],output_sha256=sha(dest)))
    safety={}
    for tag,args in [('partial',[]),('parser_safety',[]),('files_safety',[]),('numeric_safety',[]),('patch_safety',[]),('fallback',['--test']),('parameters',['--test']),('pipeline',['--test'])]:safety[tag]=execute(OUT/(tag+'_owned_x64.exe'),args,'safety-'+tag).strip()
    require(safety['partial']=='PASS partial_load_checks=15' and safety['fallback']=='PASS analytical_checks=20 refusals=10','Expected safety counts')
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Original changed '+p)
    for p,h in prior['sources'].items():require(sha(Path(p))==(changes[p]['after'] if p in changes else h),'Source changed during run '+p)
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());require(all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()) and sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Production editor unchanged')
    sources={p:sha(Path(p)) for p in prior['sources']}
    for n in ['test_material_partial_load.cpp','build_material_partial_load.cmd','verify_material_partial_load.py']:sources[str(ROOT/'work'/n)]=sha(ROOT/'work'/n)
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),cases=sum(x['cases'] for x in comparisons),exact_bytes=sum(x['bytes'] for x in comparisons),differences=0,prior_fallback_cases=288,new_failure_cases=72,nested_parser_cases=nested_count,varied_cases=128,independent_checks=independent,comparisons=comparisons,regressions=regressions,safety=safety,executables=executables,build_logs=logs,sources=sources,documented_source_changes=changes,original_hashes=prior['original_hashes'],allocations=allocations,releases=allocations,static_ranges=ranges,fixture_hashes=fixture_hashes,editor_sha256=build['executable_sha256'],editor_dependencies_unchanged=len(build['editor_source_dependencies']),limits=['Independent owned x64 optional partial parsing and structured load/diagnostic outcomes; default parser/file APIs retain strict refusals','Original full material/file/KeyValues/filesystem/shader caller executes with controlled material/name/symbol/heap/hardware/config/error-source services; normal IMaterial/resource lifecycle remains unverified','EOF and missing-value partial tree/context diagnostics plus missing/empty root/nested patch IO covered; malformed quotes, assignment/conditions, unsupported directives/cycles/multiple roots/excessive unsafe inputs still refuse','Direct seeded reuse of original file-loader nodes is not an owned public API claim. The fallback caller clears its working root before loading','Missing hash-include private-fixture fault remains unverified; no speculative patch. Real builtin error source/texture/proxy/render fidelity and editor integration incomplete','No full-editor rebuild/run, original writes, live SFM access/control,GPU orBlender'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(path=str(run),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','exact_bytes','independent_checks','regressions','safety']},indent=2))

if __name__=='__main__':main()
