"""Exact owned x64 fallback comparison and frozen accepted regression requests."""
from pathlib import Path
from datetime import datetime,timezone
import json,random,re,struct,subprocess,tempfile,shutil
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_material_keyvalues_parser import DUMP
from verify_material_parameter_orchestration import node,string,integer,packet
from verify_material_patch_files import text
from verify_material_fallback_reference import decode

OUT=ROOT/'outputs/material-fallback-owned-20261026'
REF=ROOT/'outputs/material-fallback-20261026'
LOOP=ROOT/'outputs/material-parameter-orchestration-20261026'
FILES=ROOT/'outputs/material-file-parameters-20261026'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
META=ROOT/'outputs/material-effective-parameters-20261017/verification-l11z2le6'

def execute(exe,args,run,label):
    p=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,errors='replace',timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,'Execution '+label+' see '+str(run/(label+'.log')));return p.stdout+p.stderr

def queries(raw):
    at=0
    def word():
        nonlocal at
        v=struct.unpack_from('<I',raw,at)[0];at+=4;return v
    def text():
        nonlocal at
        n=word();s=raw[at:at+n].decode('ascii');at+=n;return s
    def tree():
        kind=word();bits=word();key=text();value=text();children=[tree() for _ in range(word())];return node(key,children,kind,bits,value)
    result=[]
    for _ in range(word()):owner=text();prefix=word();root=tree();error=tree();result.append((owner,prefix,root,error))
    require(at==len(raw),'Frozen request framing');return result

def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    ref_summary=json.loads((REF/'summary.json').read_text());ref_receipt=Path(ref_summary['receipt']);reference=json.loads(ref_receipt.read_text());require(sha(ref_receipt)==ref_summary['receipt_sha256'],'Reference receipt identity')
    originals=reference['original_hashes'];changed={};allowed=ROOT/'work/material_parameter_orchestration_candidate.h'
    for p,h in reference['sources'].items():
        current=sha(Path(p))
        if Path(p)==allowed:changed[p]=dict(before=h,after=current)
        else:require(current==h,'Unrelated accepted source changed '+p)
    require(len(changed)==1 and changed[str(allowed)]['before']!=changed[str(allowed)]['after'],'Exactly the documented shared-loop extension changed')
    for p,h in originals.items():require(sha(Path(p))==h,'Protected original '+p)
    require(sha(PRIVATE)==PIN,'Private original material image')
    libs=run/'copied-libraries';libs.mkdir()
    for n in ['tier0.dll','vstdlib.dll','materialsystem.dll','stdshader_dx9.dll','filesystem_stdio.dll']:shutil.copyfile(GAME/'bin'/n,libs/n);require(sha(libs/n)==sha(GAME/'bin'/n),'Copied library identity')
    bins=[REF/'fallback_reference_x86.exe',OUT/'fallback_owned_x64.exe',LOOP/'orchestration_native_x86.exe',LOOP/'orchestration_owned_x64.exe',FILES/'files_native_x86.exe',FILES/'parameters_native_x86.exe',FILES/'files_owned_x64.exe',FILES/'file_parameters_owned_x64.exe'];executables=[]
    for p in bins:
        raw=p.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];machine=0x8664 if p.name.endswith('x64.exe') else 0x14c;require(struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x20b if machine==0x8664 else 0x10b) and b'asInvoker' in raw,'Architecture/manifest '+p.name)
        imports=subprocess.run([str(DUMP),'/imports',str(p)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require({x.lower() for x in modules}=={'kernel32.dll'},'Direct imports '+p.name);executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine),direct_imports=modules))
    logs={}
    for folder in [REF,OUT,LOOP,FILES]:
        for p in folder.glob('*-build.log'):require(not re.search(r'\b(warning|error)\b',p.read_text(),re.I),'Build diagnostics '+str(p));logs[str(p)]=sha(p)
    original_run=ref_receipt.parent;require(sha(original_run/'queries.bin')==reference['query_sha256'] and sha(original_run/'reference.bin')==reference['output_sha256'],'Original reference query/output preserved');cases=queries((original_run/'queries.bin').read_bytes());labels=reference['labels'][:];mount=run/'mount';shutil.copytree(original_run/'mount',mount)
    rng=random.Random(2026102612)
    for i in range(128):
        shader=rng.choice(['Wireframe','Wireframe','VertexLitGeneric','Wireframe_DX9']);target=rng.choice(['target','other','patch','chain','missing','unknown','fixture/root']);children=[integer('$model',rng.randrange(2)),string('$fallbackmaterial',target),string('$alpha',rng.choice(['.25','.5','1']))];mode=rng.randrange(4)
        if mode==1:children.append(node('GPU<1',[string('$fallbackmaterial',rng.choice(['target','other','missing','']))]))
        if mode==2:children.append(node('Wireframe_DX9',[string('$fallbackmaterial',rng.choice(['target','other','missing','']))]))
        if mode==3:children.append(node('Wireframe_DX9',[integer('$extra',7)]))
        if i%8==0:children.append(string('$fallbackmaterial','target'))
        error=node(rng.choice(['Wireframe_DX9','VertexLitGeneric']),[string('$errorfixture','yes'),string('$color','[.2 .4 .6]')]);cases.append(('fixture/root',0,node(shader,children),error));labels.append('varied_'+str(i))
    for shader in ['VertexLitGeneric','Wireframe_DX9']:
        for name in ['C:/ignored','../../ignored', 'x'*300]:cases.append(('fixture/root',0,node(shader,[string('$fallbackmaterial',name)]),node('Wireframe_DX9')));labels.append('ignored_unsafe_or_long_'+shader)
    for i in range(5):
        p=mount/('materials/deep'+str(i)+'.vmt');p.write_text('patch{include materials/'+('target.vmt' if i==4 else 'deep'+str(i+1)+'.vmt')+' insert{$alpha '+str((i+1)/8)+'}}',encoding='ascii')
    for model in [0,1]:cases.append(('fixture/root',0,node('Wireframe',[integer('$model',model),string('$fallbackmaterial','deep0')]),node('VertexLitGeneric',[string('$errorfixture','yes')])));labels.append('five_layer_patch_model_'+str(model))
    inp=run/'queries.bin';inp.write_bytes(struct.pack('<I',len(cases))+b''.join(text(owner)+struct.pack('<I',prefix)+packet(root)+packet(error) for owner,prefix,root,error in cases));native=run/'native.bin';owned=run/'owned.bin'
    native_log=execute(bins[0],[PRIVATE,libs,mount,inp,native],run,'native');owned_log=execute(bins[1],[mount,inp,owned,run/'no-engine.dll'],run,'owned')
    n=decode(native.read_bytes());o=decode(owned.read_bytes());require(len(n)==len(o)==len(cases),'Compared case extent');differences=[dict(case=i,label=labels[i],native=a,owned=b) for i,(a,b) in enumerate(zip(n,o)) if a!=b];(run/'comparison.json').write_text(json.dumps(dict(cases=len(cases),differences=len(differences),first_differences=differences[:8]),indent=2)+'\n');require(not differences and native.read_bytes()==owned.read_bytes(),'Fallback comparison; comparison.json')
    require(n[:reference['cases']]==reference['records'],'All107 original independent expectations preserved')
    # Frozen legacy requests retain their original native output hashes. The
    # changed header is explicitly recorded above, not silently re-baselined.
    regression=[];loop_summary=json.loads((LOOP/'summary.json').read_text());loop_run=Path(loop_summary['receipt']).parent;lr=json.loads((loop_run/'results.json').read_text());require(sha(loop_run/'queries.bin')==lr['query_sha256'],'Whole-loop frozen query')
    for tag,exe in [('native',bins[2]),('owned',bins[3])]:
        dest=run/('legacy-loop-'+tag+'.bin');execute(exe,[PRIVATE if tag=='native' else run/'absent.dll',META/'copied-libraries' if tag=='native' else run/'absent-libraries',loop_run/'queries.bin',dest],run,'legacy-loop-'+tag);require(sha(dest)==lr['output_sha256'],'Legacy886 loop '+tag)
    legacy_safety=execute(bins[3],['--test'],run,'legacy-loop-safety');require('analytical_checks=11 refusals=9' in legacy_safety,'Legacy safety')
    regression.append(dict(kind='whole_loop',cases=lr['cases'],output_sha256=lr['output_sha256']))
    fr=Path(json.loads((FILES/'summary.json').read_text())['receipt']);file_receipt=json.loads(fr.read_text())
    for group in file_receipt['groups']:
        label=group['name'];old=fr.parent/label;require(sha(old/'file-queries.bin')==group['query_sha256'] and sha(old/'native-file-queries.bin')==group['native_file_request_sha256'],'Legacy file query')
        raw=run/(label+'-legacy-owned-raw.bin');params=run/(label+'-legacy-owned-params.bin');projection=run/(label+'-legacy-projection.bin');metadata=run/(label+'-legacy-metadata.bin');execute(bins[7],[old/'file-queries.bin',raw,params,projection,metadata,run/'absent.dll'],run,label+'-legacy-owned');require(sha(raw)==group['raw_output_sha256'] and sha(params)==group['parameter_output_sha256'] and sha(projection)==group['projection_sha256'],'Legacy file/parameter bytes '+label)
        raw_native=run/(label+'-legacy-native-raw.bin');params_native=run/(label+'-legacy-native-params.bin');execute(bins[4],[libs,old/'native-file-queries.bin',raw_native],run,label+'-legacy-native-files');execute(bins[5],[PRIVATE,META/'copied-libraries',old/'native-projection.bin',params_native],run,label+'-legacy-native-parameters');require(sha(raw_native)==group['raw_output_sha256'] and sha(params_native)==group['parameter_output_sha256'],'Fresh native legacy file/parameter '+label)
    regression.append(dict(kind='file_parameters',cases=file_receipt['cases']))
    file_safety=execute(bins[7],['--test'],run,'legacy-file-safety');require('analytical_checks=18 refusals=12' in file_safety,'Legacy file safety')
    old=ROOT/'outputs/material-patch-files-20261017/verification-zsdt1ra8';legacy=json.loads((old/'results.json').read_text())
    for group in legacy['scenarios']:
        label=group['name'];q=old/(label+'-input.bin');require(sha(q)==group['input_sha256'],'Raw legacy query');a=run/(label+'-raw-native.bin');b=run/(label+'-raw-owned.bin');execute(bins[4],[libs,q,a],run,label+'-raw-native');execute(bins[6],[q,b],run,label+'-raw-owned');require(a.read_bytes()==b.read_bytes() and sha(a)==group['output_sha256'],'Raw660 legacy '+label)
    regression.append(dict(kind='raw_file',cases=sum(g['cases'] for g in legacy['scenarios'])))
    safety=execute(bins[1],['--test'],run,'safety');require('analytical_checks=19 refusals=10' in safety,'Owned safety checks')
    for p,h in originals.items():require(sha(Path(p))==h,'Original changed '+p)
    for p,h in reference['sources'].items():require(sha(Path(p))==(changed[p]['after'] if p in changed else h),'Accepted source identity changed during run')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());graph=set()
    def visit(p):
        k=p.relative_to(ROOT).as_posix()
        if k in graph:return
        graph.add(k)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            if (p.parent/inc).is_file():visit(p.parent/inc)
    visit(ROOT/'work/probe_vulkan_win32_surface_x64.cpp');require(graph==set(build['editor_source_dependencies']) and all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()),'Editor graph/source changed');require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor binary changed')
    sources={p:sha(Path(p)) for p in reference['sources']};pending=[ROOT/'work'/x for x in ['material_fallback_parameters_candidate.h','probe_material_fallback_owned.cpp','build_material_fallback_owned.cmd','verify_material_fallback_owned.py']];seen=set()
    while pending:
        p=pending.pop()
        if p in seen:continue
        seen.add(p);sources[str(p)]=sha(p)
        if p.suffix in ['.h','.cpp']:
            for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(),re.M):
                if (p.parent/inc).is_file():pending.append(p.parent/inc)
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(cases),exact_bytes=native.stat().st_size,differences=0,query_sha256=sha(inp),output_sha256=sha(native),original_reference_cases=reference['cases'],original_independent_expectations=reference['independent_expectations'],regressions=regression,safety=safety.strip(),legacy_safety=legacy_safety.strip(),legacy_file_safety=file_safety.strip(),labels=labels,executables=executables,build_logs=logs,native_log=native_log,owned_log=owned_log,sources=sources,documented_shared_source_change=changed,original_hashes=originals,editor_dependencies_unchanged=len(graph),editor_sha256=build['executable_sha256'],limits=['Owned path uses the same parameter initialization/override/fallback loop as legacy callers; optional services enable reload while old default refusal behavior is retained','Original whole caller+KeyValues+file/patch loader+real filesystem reference is compared in one process. Material storage/owner/name/symbol/heap/hardware/config/lock/logging/error-source data remain explicit fixtures','Fallback comparison uses controlled default hardware/configuration and string-valued filenames. Error source is supplied and normalized, not discovered builtin material content','Missing root file and self-reference recover to supplied error source. Unsafe/excessive paths, unsupported trees/shader schemas, non-string fallback fields, malformed/partial loads, file cycles, multiple roots and repeated nonempty patch accumulators refuse without publishing partial output','No normal public IMaterial factory, texture/proxy/resource Init, renderer/editor integration, live SFM/computer use, original writes, GPU or Blender; whole application incomplete'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(path=str(run),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','exact_bytes','regressions','safety']},indent=2))

if __name__=='__main__':main()
