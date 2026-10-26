"""Reference-only characterization of partial material loads; no owned changes.

Each new case runs in a separate private process. Previous exact records are
replayed, not regenerated from these new expectations. Originals stay read-only.
"""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,struct,subprocess,tempfile
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_material_keyvalues_parser import DUMP
from verify_material_parameter_orchestration import node,string,integer,packet
from verify_material_patch_files import text
from verify_material_fallback_reference import decode
from verify_ifm_draw_packet import check_ranges

OUT=ROOT/'outputs/material-fallback-failures-20261026'

def decode_files(raw):
    at=0
    def word():
        nonlocal at
        n=struct.unpack_from('<I',raw,at)[0];at+=4;return n
    def string_value():
        nonlocal at
        n=word();s=raw[at:at+n].decode('ascii');at+=n;return s
    def tree():
        nonlocal at
        key=string_value();kind=word();bits=word();value=string_value();pointer=struct.unpack_from('<Q',raw,at)[0];at+=8
        return dict(key=key,kind=kind,bits=bits,text=value,pointer=pointer,children=[tree() for _ in range(word())])
    records=[]
    for _ in range(word()):
        loaded=word();warnings=word();roots=[tree() for _ in range(word())];aggregate=tree()
        records.append(dict(loaded=loaded,warnings=warnings,roots=roots,aggregate=aggregate))
    require(at==len(raw),'Direct file record framing');return records

def find(n,key):return next((x for x in n['children'] if x['key'].lower()==key.lower()),None)

def request(cases):return struct.pack('<I',len(cases))+b''.join(text(name)+struct.pack('<I',prefix)+packet(source)+packet(second) for name,prefix,source,second in cases)

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    previous=json.loads((ROOT/'outputs/material-fallback-accumulator-20261026/summary.json').read_text())
    receipt_path=Path(previous['receipt']);prior=json.loads(receipt_path.read_text())
    require(sha(receipt_path)==previous['receipt_sha256'],'Prior accepted receipt')
    for p,h in prior['sources'].items():require(sha(Path(p))==h,'Accepted source '+p)
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Protected original '+p)
    for e in prior['executables']:require(sha(Path(e['path']))==e['sha256'],'Prior accepted executable')
    require(sha(PRIVATE)==sha(LIB)==PIN,'Pinned private material library')
    older=Path(prior['prior_regression_receipt']);old=json.loads(older.read_text())
    require(sha(older)==prior['prior_regression_receipt_sha256'],'Earlier fallback receipt')
    libs=older.parent/'copied-libraries'
    for name in ['tier0.dll','vstdlib.dll','materialsystem.dll','stdshader_dx9.dll','filesystem_stdio.dll']:
        require(sha(libs/name)==sha(LIB.parent/name),'Private library identity '+name)
    exe=OUT/'fallback_failures_reference_x86.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
    require(struct.unpack_from('<H',raw,pe+4)[0]==0x14c and struct.unpack_from('<H',raw,pe+24)[0]==0x10b and b'asInvoker' in raw,'PE32 x86/asInvoker')
    imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout
    modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require({x.lower() for x in modules}=={'kernel32.dll'},'Reference direct imports')
    build_log=OUT/'reference-build.log';require(not re.search(r'\b(warning|error)\b',build_log.read_text(),re.I),'Fresh build diagnostics')
    _,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('whole_file_loader',0x10006ba0,0x10006cb4),('whole_patch_chain',0x10006970,0x10006b96),('whole_material_loop',0x10006f10,0x10007355)])
    allocations=releases=null_frees=0
    def execute(mode,mount,inp,dest,label):
        nonlocal allocations,releases,null_frees
        try:p=subprocess.run([str(exe),'--'+mode,str(PRIVATE),str(libs),str(mount),str(inp),str(dest)],cwd=run,capture_output=True,timeout=15)
        except subprocess.TimeoutExpired as e:
            (run/(label+'.log')).write_bytes((e.stdout or b'')+(e.stderr or b''));raise RuntimeError('Bounded private process timed out: '+label)
        (run/(label+'.log')).write_bytes(p.stdout+p.stderr)
        require(p.returncode==0,'Private process '+label+' exit='+str(p.returncode)+'; see log')
        count=re.search(rb'allocations=(\d+) releases=(\d+) null_frees=(\d+)',p.stdout)
        require(count and count[1]==count[2],'All tracked allocations released '+label)
        allocations+=int(count[1]);releases+=int(count[2]);null_frees+=int(count[3])
        return dest.read_bytes()
    regressions=[]
    for label,folder,expected,cases in [('prior160',older.parent,old['output_sha256'],old['cases']),('prior128',receipt_path.parent,prior['output_sha256'],prior['cases'])]:
        inp=folder/'queries.bin';expected_query=old['query_sha256'] if label=='prior160' else prior['query_sha256'];require(sha(inp)==expected_query,'Frozen query '+label)
        dest=run/(label+'.bin');execute('caller',folder/'mount',inp,dest,label);require(sha(dest)==expected,'Frozen original result '+label)
        regressions.append(dict(name=label,cases=cases,query_sha256=sha(inp),output_sha256=sha(dest),bytes=dest.stat().st_size))
    mount=run/'mount';mount.mkdir();catalog={
        'valid':'VertexLitGeneric{$alpha .375}',
        'missing':None,'empty':'',
        'patch_missing':'patch{include materials/absent.vmt insert{outerMarker 1 $alpha .5}}',
        'patch_no_include':'patch{insert{outerMarker 1 $alpha .5}}',
        'truncated':'VertexLitGeneric{$alpha .375',
        'key_without_value':'VertexLitGeneric{$alpha}',
        'patch_nested_missing':'patch{include materials/inner.vmt insert{outerMarker 1 $alpha .5}}',
        'patch_truncated':'patch{include materials/truncated.vmt insert{outerMarker 1 $alpha .5}}',
        'inner':'patch{include materials/absent.vmt insert{innerMarker 2 $alpha .75}}'}
    for name,source in catalog.items():
        if source is not None:
            p=mount/('materials/'+name+'.vmt');p.parent.mkdir(exist_ok=True);p.write_text(source,encoding='ascii')
    fixture_hashes={str(p):sha(p) for p in mount.rglob('*') if p.is_file()}
    def normalized(n):
        return dict(key=n['key'],kind=n['kind'],bits=n['bits'],text=n['text'],pointer=0,children=[normalized(c) for c in n['children']])
    families=[name for name in catalog if name!='inner'];records=[];independent=0
    for name in families:
        patch_failure=name in ['patch_missing','patch_no_include','patch_nested_missing']
        root_failure=name in ['missing','empty']
        expected_warning=1 if patch_failure else 3 if name in ['truncated','key_without_value','patch_truncated'] else 0
        for seed in [0,1]:
            source=node('priorRoot',[integer('prior',7)] if seed else [])
            aggregate=node('patchAccumulator',[node('insert',[integer('prior',8)])] if seed else [])
            label='file-'+name+'-'+str(seed);inp=run/(label+'.in');dest=run/(label+'.bin');inp.write_bytes(request([(name,0,source,aggregate)]))
            row=decode_files(execute('file',mount,inp,dest,label))[0]
            require(row['loaded']==int(not root_failure),'Outer file success '+label);independent+=1
            require(row['warnings']==expected_warning,'File warning calls '+label);independent+=1
            if root_failure:
                # Exact unchanged pre-existing root and accumulator, including types.
                require(row['roots']==[normalized(source)],'Exact complete root retained '+label);independent+=1
                require(row['aggregate']==normalized(aggregate),'Exact complete accumulator retained '+label);independent+=1
            else:
                require(row['roots'][0]['key']==('patch' if patch_failure else 'VertexLitGeneric'),'Final file root '+label);independent+=1
                require(find(row['aggregate'],'insert') is None or find(find(row['aggregate'],'insert'),'prior') is None,'Successful parse clears prior patch state '+label);independent+=1
            records.append(dict(label=label,mode='file',family=name,seed=seed,query_sha256=sha(inp),output_sha256=sha(dest),bytes=dest.stat().st_size,record=row))
        for error_shader in ['Wireframe_DX9','VertexLitGeneric']:
            for model in [0,1]:
                for preceding_patch in [0,1]:
                    target=name
                    if preceding_patch:
                        target='preceding-'+name;p=mount/('materials/'+target+'.vmt');p.write_text('patch{include materials/preceding-base-'+name+'.vmt insert{precedingMarker 9}}',encoding='ascii');fixture_hashes.setdefault(str(p),sha(p))
                        p=mount/('materials/preceding-base-'+name+'.vmt');p.write_text('Wireframe{$fallbackmaterial '+name+'}',encoding='ascii');fixture_hashes.setdefault(str(p),sha(p))
                    source=node('Wireframe',[integer('$model',model),string('$fallbackmaterial',target)])
                    error=node(error_shader,[string('errorMarker','yes')]);label='caller-'+name+'-'+error_shader+'-'+str(model)+'-'+str(preceding_patch)
                    inp=run/(label+'.in');dest=run/(label+'.bin');inp.write_bytes(request([('fixture/root',0,source,error)]))
                    row=decode(execute('caller',mount,inp,dest,label))[0]
                    require(row['root']['key']==(error_shader if root_failure else 'patch' if patch_failure else 'VertexLitGeneric'),'Caller root '+label);independent+=1
                    require(row['shader']==(error_shader if root_failure else 'Wireframe_DX9' if patch_failure else 'VertexLitGeneric'),'Caller shader '+label);independent+=1
                    require(row['warnings']==(1 if root_failure else expected_warning),'Caller warning calls '+label);independent+=1
                    require((find(row['root'],'errorMarker') is not None)==root_failure,'Only failed root load selects supplied error source '+label);independent+=1
                    expected_alpha=0x3f800000 if root_failure or patch_failure or name=='key_without_value' else 0x3f000000 if name=='patch_truncated' else 0x3ec00000
                    require(row['values'][5]['words'][3]==expected_alpha,'Actual completed alpha '+label);independent+=1
                    insert=find(row['aggregate'],'insert')
                    require(bool(insert and find(insert,'precedingMarker'))==bool(root_failure and preceding_patch),'Accumulator retained only across root failure '+label);independent+=1
                    if patch_failure or name=='patch_truncated':
                        require(insert and find(insert,'outerMarker')['bits']==1,'Current outer patch accumulated '+label);independent+=1
                    if name=='patch_nested_missing':require(find(insert,'innerMarker')['bits']==2,'Nested failed include accumulator '+label);independent+=1
                    records.append(dict(label=label,mode='caller',family=name,preceding_patch=preceding_patch,model=model,error_shader=error_shader,query_sha256=sha(inp),output_sha256=sha(dest),bytes=dest.stat().st_size,record=row))
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Original changed '+p)
    for p,h in prior['sources'].items():require(sha(Path(p))==h,'Accepted source changed '+p)
    for p,h in fixture_hashes.items():require(sha(Path(p))==h,'Disposable source changed '+p)
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());require(all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()) and sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Production editor unchanged')
    sources=dict(prior['sources'])
    for n in ['probe_material_fallback_failures.cpp','build_material_fallback_failures.cmd','verify_material_fallback_failures.py']:sources[str(ROOT/'work'/n)]=sha(ROOT/'work'/n)
    result=dict(status='PASS_REFERENCE_ONLY',utc=datetime.now(timezone.utc).isoformat(),cases=len(records),file_cases=sum(x['mode']=='file' for x in records),caller_cases=sum(x['mode']=='caller' for x in records),independent_checks=independent,reference_bytes=sum(x['bytes'] for x in records),records=records,regressions=regressions,allocations=allocations,releases=releases,null_frees=null_frees,executable=dict(path=str(exe),sha256=sha(exe),machine='0x14c',direct_imports=modules),build_log=dict(path=str(build_log),sha256=sha(build_log)),sources=sources,fixture_hashes=fixture_hashes,original_hashes=prior['original_hashes'],static_ranges=ranges,prior_receipt=str(receipt_path),prior_receipt_sha256=sha(receipt_path),editor_sha256=build['executable_sha256'],editor_dependencies_unchanged=len(build['editor_source_dependencies']),limits=['Reference only: owned loader retains its existing refusals for malformed/partial loads; empty root currently also refuses rather than matching original recovery','Nine families sampled; exact private original caller/file/KeyValues/filesystem/shader behavior with controlled owner/config/error source/allocator services, not normal public IMaterial/resource lifecycle','Warnings count imported diagnostic calls, not user-visible error lines; error material content is supplied, not original builtin material','Missing hash-include directive reached an unsupported private fixture path in preliminary discovery (access violation); no engine behavior inferred from that fault. Kept outside accepted corpus, not silently treated as passing','No parser broadening, production integration/editor build or run, live SFM/computer use, original writes,GPU orBlender'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(path=str(run),receipt_sha256=sha(run/'results.json')),indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ['status','cases','file_cases','caller_cases','independent_checks','reference_bytes','allocations','releases']},indent=2))

if __name__=='__main__':main()
