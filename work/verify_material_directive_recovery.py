"""Missing/empty #include/#base recovery and separate DevMsg accounting."""
from pathlib import Path
from datetime import datetime,timezone
import json,random,re,struct,subprocess,tempfile
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_material_keyvalues_parser import DUMP
from verify_material_parameter_orchestration import node,string,integer
from verify_material_partial_load import request
from verify_material_fallback_reference import decode
from verify_ifm_draw_packet import check_ranges
from pe_image import Image

OUT=ROOT/'outputs/material-directive-failures-20261026'
CHANGED=['material_fallback_native_fixture.h','probe_material_fallback_failures.cpp','source_material_keyvalues_parser_candidate.h','source_material_keyvalues_files_candidate.h','source_material_patch_files_candidate.h','material_parameter_orchestration_candidate.h','material_fallback_parameters_candidate.h','probe_material_fallback_owned.cpp','test_material_partial_load.cpp']

def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    previous=json.loads((ROOT/'outputs/material-partial-load-owned-20261026/summary.json').read_text());receipt=Path(previous['receipt']);prior=json.loads(receipt.read_text());require(sha(receipt)==previous['receipt_sha256'],'Prior receipt')
    allowed={ROOT/'work'/n for n in CHANGED};changes={}
    for p,h in prior['sources'].items():
        current=sha(Path(p))
        if Path(p) in allowed:require(current!=h,'Expected correction missing');changes[p]=dict(before=h,after=current)
        else:require(current==h,'Unrelated source changed '+p)
    require(len(changes)==len(CHANGED),'Exact modified source set')
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Protected original '+p)
    require(sha(LIB)==sha(PRIVATE)==PIN,'Private library identity')
    libs=ROOT/'outputs/material-fallback-owned-20261026/verification-0mhr77s9/copied-libraries'
    for name in ['tier0.dll','vstdlib.dll','materialsystem.dll','stdshader_dx9.dll','filesystem_stdio.dll']:require(sha(libs/name)==sha(LIB.parent/name),'Private dependency '+name)
    # Verify the exact untouched import metadata that caused the old fault.
    image=Image(LIB);opt=image.u32(60)+24;directory=image.u32(opt+104);at=image.offset(image.base+directory);imports=[]
    while any(image.b[at:at+20]):
        ilt,_,_,name,iat=struct.unpack_from('<5I',image.b,at);at+=20;i=0
        while True:
            target=image.word(image.base+ilt+4*i)
            if not target:break
            if image.base+iat+4*i==0x10095190:imports.append(dict(module=image.string(image.base+name),symbol=image.string(image.base+target+2),iat=hex(image.base+iat+4*i),unbound_target=hex(target)))
            i+=1
    require(imports==[dict(module='tier0.dll',symbol='?DevMsg@@YAXPBDZZ',iat='0x10095190',unbound_target='0xfade4')],'Exact named diagnostic import')
    _,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('directive_load_branch_and_cleanup',0x10071ffe,0x1007209f),('whole_file_loader',0x10006ba0,0x10006cb4)])
    require(any(x['address']==0x10072070 and x['instruction']=='call        dword ptr ds:[10095190h]' for x in ranges[0]['instructions']),'Verified DevMsg call site')
    names=['directive','partial','parser_safety','files_safety','numeric_safety','patch_safety','parser','files','fallback','parameters','pipeline'];executables=[];logs={}
    for tag in [*names,'reference']:
        p=OUT/(tag+'_owned_x64.exe' if tag!='reference' else 'reference_x86.exe');machine=0x14c if tag=='reference' else 0x8664;raw=p.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x20b if machine==0x8664 else 0x10b) and b'asInvoker' in raw,'Architecture/manifest '+tag)
        result=subprocess.run([str(DUMP),'/imports',str(p)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',result,re.M|re.I);require({x.lower() for x in modules}=={'kernel32.dll'},'Direct imports '+tag);executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine),direct_imports=modules))
    for p in OUT.glob('*-build.log'):require(not re.search(r'\b(warning|error)\b',p.read_text(),re.I),'Compiler diagnostics '+p.name);logs[str(p)]=sha(p)
    require(len(logs)==13,'Fresh complete build logs')
    allocations=0
    def execute(exe,args,label):
        nonlocal allocations
        p=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,errors='replace',timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,'Execution '+label+' exit='+str(p.returncode)+'; see log')
        count=re.search(r'allocations=(\d+) releases=(\d+) null_frees=(\d+)',p.stdout)
        if count:require(count[1]==count[2],'Native allocation cleanup '+label);allocations+=int(count[1])
        return p.stdout+p.stderr
    comparisons=[]
    def compare(mount,inp,label,count,expected=None,expected_dev=0):
        a=run/(label+'-native.bin');b=run/(label+'-owned.bin');nlog=execute(OUT/'reference_x86.exe',['--caller',PRIVATE,libs,mount,inp,a],label+'-native');olog=execute(OUT/'fallback_owned_x64.exe',[mount,inp,b,run/'no-engine.dll'],label+'-owned')
        nd=re.search(r'dev_messages=(\d+)',nlog);od=re.search(r'dev_messages=(\d+)',olog);require(nd and od and int(nd[1])==int(od[1])==expected_dev,'Separate DevMsg counts '+label)
        if expected:require(sha(a)==expected,'Frozen original record '+label)
        n=decode(a.read_bytes());o=decode(b.read_bytes());require(len(n)==len(o)==count,'Record count '+label)
        differences=[dict(case=i,native=x,owned=y) for i,(x,y) in enumerate(zip(n,o)) if x!=y];(run/(label+'-comparison.json')).write_text(json.dumps(dict(differences=len(differences),first=differences[:2]),indent=2)+'\n');require(not differences and a.read_bytes()==b.read_bytes(),'Directive comparison '+label)
        comparisons.append(dict(label=label,cases=count,bytes=a.stat().st_size,query_sha256=sha(inp),output_sha256=sha(a),developer_messages=expected_dev));return n
    # Re-run all504 previous whole-caller requests against BOTH fresh binaries.
    failure=Path(json.loads((ROOT/'outputs/material-fallback-failures-20261026/summary.json').read_text())['receipt']);fv=json.loads(failure.read_text());acc=Path(fv['prior_receipt']);av=json.loads(acc.read_text());old=Path(av['prior_regression_receipt'])
    malformed=[x for x in fv['records'] if x['mode']=='caller']
    for batch in prior['comparisons']:
        label=batch['label']
        if label=='legacy160':folder=old.parent;inp=folder/'queries.bin';mount=folder/'mount'
        elif label=='legacy128':folder=acc.parent;inp=folder/'queries.bin';mount=folder/'mount'
        elif label=='expanded':inp=receipt.parent/'expanded.in';mount=receipt.parent/'mount'
        else:row=malformed[int(label.removeprefix('failure'))];inp=failure.parent/(row['label']+'.in');mount=failure.parent/'mount'
        require(sha(inp)==batch['query_sha256'],'Frozen504 request');compare(mount,inp,'prior-'+label,batch['cases'],batch['output_sha256'])
    # Reproduce the exact earlier faulting request, without altering its bytes.
    discovery=ROOT/'outputs/material-fallback-failures-20261026/discovery-ywoz0zl_';inp=discovery/'missing_directive-caller.in';compare(discovery/'mount',inp,'repaired-original-fault',1,expected_dev=1)
    mount=run/'mount';(mount/'materials').mkdir(parents=True);fixture_hashes={}
    def write(name,source):
        p=mount/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(source,encoding='ascii');fixture_hashes[str(p)]=sha(p)
    rng=random.Random(2026102616);independent=0
    for i in range(128):
        prefix='materials/directives/'+str(i)+'/';name='directives/'+str(i)+'/root';declared=i%5!=0;prelude=i%3==0
        write(prefix+'empty.vmt','');write(prefix+'template.vmt','Template{$color "[.2 .4 .6]"}');write(prefix+'truncated.vmt','Template{$color "[.2 .4 .6]"');write(prefix+'transitive.vmt','#base absent-child.vmt Template{$color "[.2 .4 .6]"}')
        missing_count=1+i%4;directives=[]
        for j in range(missing_count):directives.append(rng.choice(['#include','#base','#INCLUDE','#BASE'])+' '+('empty.vmt' if j%2 else 'absent'+str(j)+'.vmt'))
        kind=['none','template','truncated','transitive'][i%4];dev=missing_count+int(kind=='transitive');warnings=3 if kind=='truncated' else 0
        if kind!='none':directives.append('#base '+kind+'.vmt')
        body='VertexLitGeneric{$alpha .375}' if declared else '';content=' '.join([*directives,body] if i%2 else [body,*directives]);write(prefix+'root.vmt',content)
        target=name
        if prelude:
            target='directives/'+str(i)+'/pre';write(prefix+'pre.vmt','patch{include '+prefix+'prebase.vmt insert{oldMarker 7}}');write(prefix+'prebase.vmt','Wireframe{$fallbackmaterial '+name+'}')
        source_name=rng.choice(['Wireframe','wireframe','WIREFRAME']);q=[('fixture/root',0,node(source_name,[integer('$model',rng.randrange(2)),string('$fallbackmaterial',target)]),node('Wireframe_DX9',[string('errorMarker','yes')]))]
        inp=run/('new'+str(i)+'.in');inp.write_bytes(request(q));rows=compare(mount,inp,'directive'+str(i),1,expected_dev=dev);row=rows[0]
        expected_root='VertexLitGeneric' if declared else ('Wireframe' if prelude else source_name)
        require(row['root']['key']==expected_root and row['shader']==('VertexLitGeneric' if declared else 'Wireframe_DX9'),'Local declaration/retained name');independent+=1
        require(row['warnings']==warnings and not row['aggregate']['children'],'Warning channel/cleared accumulator');independent+=1
        require(row['values'][5]['words'][3]==(0x3ec00000 if declared else 0x3f800000),'Actual alpha/default');independent+=1
        require(not any(n['key']=='errorMarker' for n in row['root']['children']),'No false error-source recovery');independent+=1
    # Fresh owned strict regressions use frozen independently obtained native
    # outputs; native strict components themselves are unaffected and not rerun.
    regressions=[]
    loop=Path(json.loads((ROOT/'outputs/material-parameter-orchestration-20261026/summary.json').read_text())['receipt']);lv=json.loads(loop.read_text());dest=run/'strict-loop.bin';require(sha(loop.parent/'queries.bin')==lv['query_sha256'],'Strict loop request');execute(OUT/'parameters_owned_x64.exe',[run/'absent.dll',run/'absent-dir',loop.parent/'queries.bin',dest],'strict-loop');require(sha(dest)==lv['output_sha256'],'Strict loop records');regressions.append(dict(kind='parameter_loop',cases=lv['cases']))
    files=Path(json.loads((ROOT/'outputs/material-file-parameters-20261026/summary.json').read_text())['receipt']);filev=json.loads(files.read_text())
    for group in filev['groups']:
        label=group['name'];folder=files.parent/label;require(sha(folder/'file-queries.bin')==group['query_sha256'],'File request');raw=run/(label+'-raw.bin');params=run/(label+'-params.bin');projection=run/(label+'-projection.bin');meta=run/(label+'-meta.bin');execute(OUT/'pipeline_owned_x64.exe',[folder/'file-queries.bin',raw,params,projection,meta,run/'no-engine.dll'],'strict-file-'+label);require(sha(raw)==group['raw_output_sha256'] and sha(params)==group['parameter_output_sha256'] and sha(projection)==group['projection_sha256'],'Strict file bytes')
    regressions.append(dict(kind='file_parameters',cases=filev['cases']))
    folder=ROOT/'outputs/material-patch-files-20261017/verification-zsdt1ra8';rawv=json.loads((folder/'results.json').read_text())
    for group in rawv['scenarios']:
        inp=folder/(group['name']+'-input.bin');require(sha(inp)==group['input_sha256'],'Raw patch request');dest=run/(group['name']+'-raw.bin');execute(OUT/'files_owned_x64.exe',[inp,dest],'strict-raw-'+group['name']);require(sha(dest)==group['output_sha256'],'Raw patch bytes')
    regressions.append(dict(kind='raw_patch_files',cases=sum(x['cases'] for x in rawv['scenarios'])))
    rp=Path(json.loads((ROOT/'outputs/material-keyvalues-numeric-20261026/summary.json').read_text())['raw_receipt']);rv=json.loads(rp.read_text());inp=rp.parent/'input.bin';require(sha(inp)==rv['raw']['input_sha256'],'Numeric request');dest=run/'numeric.bin';execute(OUT/'parser_owned_x64.exe',[run/'no-engine.dll',inp,dest],'strict-numeric');require(sha(dest)==rv['raw']['output_sha256'],'Numeric parser bytes');regressions.append(dict(kind='numeric_parser',cases=rv['raw']['cases']))
    safety={}
    for tag in ['directive','partial','parser_safety','files_safety','numeric_safety','patch_safety','fallback','parameters','pipeline']:safety[tag]=execute(OUT/(tag+'_owned_x64.exe'),['--test'] if tag in ['fallback','parameters','pipeline'] else [],'safety-'+tag).strip()
    require(safety['directive']=='PASS directive_recovery_checks=11','Directive safety count')
    for p,h in fixture_hashes.items():require(sha(Path(p))==h,'Disposable source changed '+p)
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Original changed '+p)
    for p,h in prior['sources'].items():require(sha(Path(p))==(changes[p]['after'] if p in changes else h),'Source changed during acceptance')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());require(all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()) and sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor unchanged')
    sources={p:sha(Path(p)) for p in prior['sources']}
    for n in ['test_material_directive_recovery.cpp','build_material_directive_failures.cmd','verify_material_directive_recovery.py']:sources[str(ROOT/'work'/n)]=sha(ROOT/'work'/n)
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),cases=sum(x['cases'] for x in comparisons),exact_bytes=sum(x['bytes'] for x in comparisons),differences=0,prior_fallback_cases=504,new_cases=129,independent_checks=independent,comparisons=comparisons,regressions=regressions,safety=safety,executables=executables,build_logs=logs,sources=sources,documented_source_changes=changes,original_hashes=prior['original_hashes'],allocations=allocations,releases=allocations,static_ranges=ranges,devmsg_import=imports[0],fixture_hashes=fixture_hashes,prior_receipt=str(receipt),prior_receipt_sha256=sha(receipt),editor_sha256=build['executable_sha256'],editor_dependencies_unchanged=len(build['editor_source_dependencies']),limits=['Reference repair binds a proven tier0 DevMsg import to a separate counted cdecl-varargs diagnostic service. Original executable bytes remain unchanged; Warning and DevMsg are not combined','Owned missing/empty directive recovery is opt-in. Strict default source-file APIs preserve their refusals. Retained root-name results preserve the cleared caller name and loaded base children','Controlled default environment and supplied error source; real builtin error material/normal public IMaterial/resource/texture/proxy/render lifecycle remain unverified','Retained-name INCLUDE peers, retained-name patch include targets, multiple-root fallback, unsafe/excessive/cyclic inputs and broader unsupported grammar remain explicit refusals, not fabricated file outcomes','No renderer/editor integration or full editor build/run, live SFM access/control, original writes,GPU orBlender'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(path=str(run),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','exact_bytes','independent_checks','regressions','safety','allocations']},indent=2))

if __name__=='__main__':main()
