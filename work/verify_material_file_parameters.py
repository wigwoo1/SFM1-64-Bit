"""Original normal file/patch loader -> original complete parameter-loop comparison.

The reference boundary is serialized between two private processes. It does not
claim one normally constructed IMaterial, texture Init, or external fallback VMTs.
"""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,random,re,shutil,struct,subprocess,tempfile
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_material_keyvalues_parser import DUMP
from verify_material_patch_files import decode as decode_files,request,text
from verify_material_source_provider import bsp_bytes,zip_bytes
from verify_material_parameter_orchestration import packet,node,decode as decode_parameters
from verify_ifm_draw_packet import check_ranges

OUT=ROOT/'outputs/material-file-parameters-20261026'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
OLD=ROOT/'outputs/material-patch-files-20261017/verification-zsdt1ra8'
META=ROOT/'outputs/material-effective-parameters-20261017/verification-l11z2le6'

def execute(exe,args,run,label):
    p=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,errors='replace',timeout=120)
    (run/(label+'.log')).write_text(p.stdout+p.stderr)
    require(p.returncode==0,'Execution '+label+' exit='+str(p.returncode)+' see '+str(run/(label+'.log')))
    return p.stdout+p.stderr

def project(n):
    return node(n['key'],[project(c) for c in n['children']],n['kind'],n['value'] if n['kind'] in [2,3] else 0,n['value'] if n['kind']==1 else '')

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    accepted=json.loads((ROOT/'outputs/material-parameter-orchestration-20261026/summary.json').read_text());prior=json.loads(Path(accepted['receipt']).read_text())
    old=json.loads((OLD/'results.json').read_text());originals=dict(old['original_hashes'])
    protected=json.loads(Path(json.loads((ROOT/'outputs/material-keyvalues-numeric-20261026/summary.json').read_text())['raw_receipt']).read_text())['original_hashes'];originals.update(protected)
    sources=dict(prior['sources']);sources.update({str(ROOT/'work'/n):sha(ROOT/'work'/n) for n in ['source_material_patch_files_candidate.h','native_material_keyvalues_fixture.h','probe_material_patch_files_native.cpp','probe_material_patch_files_owned.cpp','source_material_source_provider_candidate.h']})
    for p,h in originals.items():require(sha(Path(p))==h,'Original identity '+p)
    for p,h in sources.items():require(sha(Path(p))==h,'Prior source identity '+p)
    require(sha(LIB)==sha(PRIVATE)==PIN,'Pinned private parameter-loop library')
    for name in ['tier0.dll','stdshader_dx9.dll']:
        require(sha(META/'copied-libraries'/name)==sha(GAME/'bin'/name),'Pinned private shader library '+name)
    _,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('whole_original_file_loader',0x10006ba0,0x10006cb4),('whole_original_patch_chain',0x10006970,0x10006b96),('whole_parameter_loop',0x10006f10,0x10007355)])
    libs=run/'copied-libraries';libs.mkdir()
    for name in ['tier0.dll','vstdlib.dll','materialsystem.dll','filesystem_stdio.dll']:
        shutil.copyfile(GAME/'bin'/name,libs/name);require(sha(libs/name)==originals[str(GAME/'bin'/name)],'Private library copy')
    executables=[]
    for name,machine in [('files_native_x86',0x14c),('parameters_native_x86',0x14c),('files_owned_x64',0x8664),('file_parameters_owned_x64',0x8664)]:
        p=OUT/(name+'.exe');raw=p.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if machine==0x14c else 0x20b) and b'asInvoker' in raw,'Architecture and manifest '+name)
        imports=subprocess.run([str(DUMP),'/imports',str(p)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require({m.lower() for m in modules}=={'kernel32.dll'},'Direct dependency '+name)
        executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine),direct_imports=modules))
    build_logs={}
    for tag in ['files-native','parameters-native','files-owned','pipeline-owned','asm']:
        p=OUT/(tag+'-build.log');require(not re.search(r'\b(warning|error)\b',p.read_text(),re.I),'Build diagnostics '+tag);build_logs[str(p)]=sha(p)
    # Revalidate all historical positive file cases against freshly compiled
    # original and owned probes; do not freeze pre-correction parser behavior.
    regression=[]
    for scenario in old['scenarios']:
        label=scenario['name'];inp=OLD/(label+'-input.bin');require(sha(inp)==scenario['input_sha256'],'Historical request changed')
        a=run/(label+'-native.bin');b=run/(label+'-owned.bin');execute(OUT/'files_native_x86.exe',[libs,inp,a],run,label+'-native');execute(OUT/'files_owned_x64.exe',[inp,b],run,label+'-owned')
        require(a.read_bytes()==b.read_bytes(),'Historical file regression '+label)
        regression.append(dict(name=label,cases=scenario['cases'],bytes=a.stat().st_size,input_sha256=sha(inp),output_sha256=sha(a)))
    loose=run/'loose';loose.mkdir();cases=[];state=[0,0,0,0,0,0,0,0,1,0,92,0]
    def write(name,source):
        p=loose/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(source,encoding='ascii')
    def add(name,prefix=0,q=None):cases.append((name,prefix,list(state if q is None else q)))
    write('materials/base.vmt','Wireframe{$alpha 1 $model 0 $color "[1 2 3]"}')
    write('materials/inner.vmt','patch{include materials/base.vmt insert{$alpha .5} replace{$model 1}}')
    write('materials/outer.vmt','patch{include materials/inner.vmt insert{$alpha .25} replace{$model 0}}')
    write('materials/paths.vmt','patch{include materials/base.vmt insert{"group/item" 1 "group/other" 2}}')
    write('materials/hash.vmt','#base template.vmt VertexLitGeneric{$alpha .125}')
    write('materials/template.vmt','Template{$model 1 $basetexture fixture/base $phong 1}')
    write('materials/peers.vmt','patch{include materials/hash.vmt insert{$x 7}} Extra{retained 1}')
    for name in ['base','inner','outer','paths','hash','peers']:add(name)
    add('materials/outer',1)
    for depth in [1,2,5,10]:
        for i in range(depth):write('materials/chain'+str(depth)+'-'+str(i)+'.vmt','patch{include materials/'+('base.vmt' if i+1==depth else 'chain'+str(depth)+'-'+str(i+1)+'.vmt')+' insert{$alpha '+str((i+1)/16)+'}}')
        add('chain'+str(depth)+'-0')
    rng=random.Random(2026102611)
    for i in range(128):
        shader=rng.choice(['VertexLitGeneric','Wireframe','Wireframe_DX9']);prefix='materials/generated/'+str(i)+'/'
        write(prefix+'base.vmt',shader+'{$model 0 $alpha 1 $basetexture fixture/base $phong 1 $color "[.1 .2 .3]" GPU<1{$x 4} dx9{$x 8}}')
        depth=rng.randrange(1,5)
        for d in range(depth):write(prefix+'p'+str(d)+'.vmt','patch{include '+prefix+('base.vmt' if d+1==depth else 'p'+str(d+1)+'.vmt')+' insert{$alpha '+rng.choice(['.25','.5','.75','1'])+' $x '+str(d)+'} replace{$model '+str(d%2)+'}}')
        q=state[:];q[0]=rng.randrange(4);q[1]=rng.randrange(2);q[2]=rng.choice([0,1,256]);q[8]=rng.randrange(2);q[9]=rng.randrange(2);q[10]=rng.choice([89,91,92,95]);q[11]=rng.randrange(2);add('generated/'+str(i)+'/p0',q=q)
    write('materials/loose-patch.vmt','patch{include materials/packed-base.vmt insert{$alpha .375}}')
    packed=run/'controlled.bsp';packed.write_bytes(bsp_bytes(zip_bytes([('materials/packed-base.vmt',b'Wireframe{$alpha 1 $model 1}'),('materials/packed-patch.vmt',b'patch{include materials/base.vmt replace{$alpha .625}}')])))
    add('loose-patch');add('packed-patch')
    scenarios=[('controlled',[(packed,'GAME',0),(loose,'GAME',1)],cases)]
    # Isolate each original mount: duplicate material logical paths across mods
    # must not silently select a different file from the protected source.
    model_sources=prior['file_sources']
    for i,m in enumerate(model_sources):
        p=Path(m['path']);require(sha(p)==m['sha256'],'Model identity');relative=p.relative_to(GAME);mount=GAME/relative.parts[0];name=Path(*relative.parts[2:]).with_suffix('').as_posix()
        require(relative.parts[1]=='materials','Model source layout');alias='fixture-model-'+str(i)
        write('materials/'+alias+'.vmt','patch{include materials/'+name+'.vmt replace{$alpha .875}}')
        write('materials/'+alias+'-insert.vmt','patch{include materials/'+name+'.vmt insert{$alpha .875}}')
        mounts=[(loose,'GAME',1),(mount,'GAME',1)];logical=Path(*relative.parts[1:])
        candidates=[base/logical for base,path_id,position in mounts if (base/logical).is_file()]
        require(candidates and candidates[0]==p and sha(candidates[0])==m['sha256'],'Mounted model source must match protected benchmark identity')
        model_cases=[]
        for request_name in [alias,alias+'-insert']:
            for phong,level in [(0,92),(1,91),(1,92)]:q=state[:];q[8]=phong;q[10]=level;model_cases.append((request_name,0,q))
        scenarios.append(('model-'+str(i),mounts,model_cases))
    groups=[]
    for label,mounts,queries in scenarios:
        group=run/label;group.mkdir();inp=group/'file-queries.bin';native_input=group/'native-file-queries.bin'
        native_input.write_bytes(request(mounts,[(name,prefix) for name,prefix,q in queries]))
        mount_bytes=request(mounts,[])[:-4];inp.write_bytes(mount_bytes+struct.pack('<I',len(queries))+b''.join(text(name)+struct.pack('<I',prefix)+struct.pack('<12I',*q) for name,prefix,q in queries))
        raw_native=group/'native-files.bin';raw_owned=group/'owned-files.bin';params_owned=group/'owned-parameters.bin';projection=group/'owned-projection.bin';metadata=group/'metadata.bin'
        native_file_log=execute(OUT/'files_native_x86.exe',[libs,native_input,raw_native],group,'files-native')
        owned_log=execute(OUT/'file_parameters_owned_x64.exe',[inp,raw_owned,params_owned,projection,metadata,group/'no-engine.dll'],group,'pipeline-owned')
        require(raw_native.read_bytes()==raw_owned.read_bytes(),'Resolved raw file/patch documents differ '+label)
        documents=decode_files(raw_native.read_bytes());require(len(documents)==len(queries) and all(d['ok'] and d['roots'] for d in documents),'Resolved originals')
        native_projection=group/'native-projection.bin';native_projection.write_bytes(b''.join(struct.pack('<12I',*q)+packet(project(d['roots'][0])) for (name,prefix,q),d in zip(queries,documents)))
        require(native_projection.read_bytes()==projection.read_bytes(),'Actual original-loaded trees equal owned projection '+label)
        params_native=group/'native-parameters.bin';native_parameter_log=execute(OUT/'parameters_native_x86.exe',[PRIVATE,META/'copied-libraries',native_projection,params_native],group,'parameters-native')
        require(params_native.read_bytes()==params_owned.read_bytes(),'Original file -> whole parameter loop comparison '+label)
        records=decode_parameters(params_native.read_bytes(),len(queries))
        if label=='controlled':require(records[2]['values'][5]['words'][3]==0x3f000000,'Inner patch alpha wins')
        else:
            require(all(not any(n['key'].lower()=='$alpha' for n in d['roots'][0]['children']) for d in documents[:3]),'Replace does not introduce a missing model alpha')
            require(all(d['values'][5]['words'][3]==0x3f800000 for d in records[:3]),'Missing alpha receives real shader default')
            require(all(d['values'][5]['words'][3]==0x3f600000 for d in records[3:]),'Insert introduces model alpha')
        b=metadata.read_bytes();at=0;meta=[]
        def vector():
            nonlocal at
            n=struct.unpack_from('<I',b,at)[0];at+=4;v=[]
            for _ in range(n):size=struct.unpack_from('<I',b,at)[0];at+=4;v.append(b[at:at+size].decode('ascii'));at+=size
            return v
        for _ in queries:meta.append(dict(dependencies=vector(),patch_files=vector()))
        require(at==len(b),'Metadata extent')
        if label=='controlled':require(meta[2]==dict(dependencies=['materials/outer.vmt','materials/inner.vmt','materials/base.vmt'],patch_files=['materials/outer.vmt','materials/inner.vmt']),'Layer/dependency order')
        groups.append(dict(name=label,cases=len(queries),raw_exact_bytes=raw_native.stat().st_size,parameter_exact_bytes=params_native.stat().st_size,native_file_log=native_file_log,native_parameter_log=native_parameter_log,owned_log=owned_log,metadata=meta,case_names=[name for name,prefix,q in queries],query_sha256=sha(inp),native_file_request_sha256=sha(native_input),raw_output_sha256=sha(raw_native),projection_sha256=sha(native_projection),parameter_output_sha256=sha(params_native)))
    safety=execute(OUT/'file_parameters_owned_x64.exe',['--test'],run,'safety');require('analytical_checks=18 refusals=12' in safety,'Safety case count')
    for p,h in originals.items():require(sha(Path(p))==h,'Original changed '+p)
    require(sha(PRIVATE)==PIN,'Private parameter library changed')
    for p,h in sources.items():require(sha(Path(p))==h,'Accepted source changed '+p)
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());graph=set()
    def visit(p):
        k=p.relative_to(ROOT).as_posix()
        if k in graph:return
        graph.add(k)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            if (p.parent/inc).is_file():visit(p.parent/inc)
    visit(ROOT/'work/probe_vulkan_win32_surface_x64.cpp');require(graph==set(build['editor_source_dependencies']) and all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()),'Editor dependencies changed');require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor changed')
    pending=[ROOT/'work'/n for n in ['material_file_parameters_candidate.h','probe_material_file_parameters.cpp','build_material_file_parameters.cmd','verify_material_file_parameters.py','probe_material_patch_files_native.cpp','probe_material_patch_files_owned.cpp','probe_material_parameter_orchestration.cpp']];visited=set()
    while pending:
        p=pending.pop()
        if p in visited:continue
        visited.add(p)
        sources[str(p)]=sha(p)
        if p.suffix in ['.cpp','.h']:
            for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(),re.M):
                if (p.parent/inc).is_file():pending.append(p.parent/inc)
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),cases=sum(g['cases'] for g in groups),raw_exact_bytes=sum(g['raw_exact_bytes'] for g in groups),parameter_exact_bytes=sum(g['parameter_exact_bytes'] for g in groups),groups=groups,differences=0,regression_cases=sum(s['cases'] for s in regression),regressions=regression,safety=safety.strip(),executables=executables,build_logs=build_logs,sources=sources,static_ranges=ranges,original_hashes=originals,protected_original_files=len(originals),model_sources=model_sources,editor_dependencies_unchanged=len(graph),editor_sha256=build['executable_sha256'],limits=['Normal original whole VMT/patch loader and real filesystem are run unchanged in one isolated process; serialized complete original-loaded first roots feed unchanged original CMaterial parameter loop in a second private process','Owned pipeline composes existing accepted source/patch loader and three-schema parameter loop with transactionally published value-owned output. Not a normal public IMaterial factory or one-process original full lifecycle','Filesystem provider is explicit loose/stored-BSP GAME mounts, not full VPK/compressed/implicit archive search. Existing extent/type/unsafe-path gates remain','Nonempty external fallback material and other concrete shader schemas remain refused. No original file failure/error-shader recovery or external fallback restart claim','No texture/proxy/resource initialization, editor integration, live SFM/computer use, original writes, GPU or Blender. Application incomplete'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(path=str(run),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','raw_exact_bytes','parameter_exact_bytes','regression_cases','safety']},indent=2))

if __name__=='__main__':main()
