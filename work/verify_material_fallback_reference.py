"""Reference characterization only; owned x64 external fallback remains absent."""
from pathlib import Path
from datetime import datetime,timezone
import json,re,struct,subprocess,tempfile,shutil
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_material_keyvalues_parser import DUMP
from verify_material_parameter_orchestration import node,string,integer,packet
from verify_material_patch_files import text
from verify_ifm_draw_packet import check_ranges

OUT=ROOT/'outputs/material-fallback-20261026'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')

def decode(raw):
    at=0
    def word():
        nonlocal at
        n=struct.unpack_from('<I',raw,at)[0];at+=4;return n
    def string():
        nonlocal at
        n=word();s=raw[at:at+n].decode('ascii');at+=n;return s
    def tree():
        nonlocal at
        key=string();kind=word();bits=word();value=string();pointer=struct.unpack_from('<Q',raw,at)[0];at+=8;children=[tree() for _ in range(word())];return dict(key=key,kind=kind,bits=bits,text=value,pointer=pointer,children=children)
    results=[]
    for _ in range(word()):
        warnings=word();selected=word();shader=string();lookups=[string() for _ in range(word())];root=tree();aggregate=tree();values=[]
        for _ in range(word()):
            q=struct.unpack_from('<28I',raw,at);at+=112;value=raw[at:at+q[10]].decode('ascii');at+=q[10];name=raw[at:at+q[11]].decode('ascii');at+=q[11];values.append(dict(words=list(q),text=value,name=name))
        results.append(dict(warnings=warnings,selected=selected,shader=shader,lookups=lookups,root=root,aggregate=aggregate,values=values))
    require(at==len(raw),'Reference output extent');return results

def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    previous=json.loads((ROOT/'outputs/material-file-parameters-20261026/summary.json').read_text());prior=json.loads(Path(previous['receipt']).read_text());originals=prior['original_hashes'];accepted=prior['sources']
    for p,h in originals.items():require(sha(Path(p))==h,'Original identity '+p)
    for p,h in accepted.items():require(sha(Path(p))==h,'Accepted source identity '+p)
    require(sha(LIB)==sha(PRIVATE)==PIN,'Private original identity')
    libs=run/'copied-libraries';libs.mkdir()
    for n in ['tier0.dll','vstdlib.dll','materialsystem.dll','stdshader_dx9.dll','filesystem_stdio.dll']:
        shutil.copyfile(GAME/'bin'/n,libs/n);require(sha(libs/n)==sha(GAME/'bin'/n),'Private library copy')
    _,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('complete_material_loop',0x10006f10,0x10007355),('complete_file_loader',0x10006ba0,0x10006cb4),('complete_KV_constructor',0x10070a10,0x10070a79),('complete_KV_clear',0x10071230,0x1007125f),('complete_KV_assignment',0x10070e50,0x10071044),('complete_KV_destructor',0x10070540,0x100705de)])
    exe=OUT/'fallback_reference_x86.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(struct.unpack_from('<H',raw,pe+4)[0]==0x14c and b'asInvoker' in raw,'Reference architecture/manifest')
    imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;require({x.lower() for x in re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I)}=={'kernel32.dll'},'Direct reference imports')
    require(not re.search(r'\b(warning|error)\b',(OUT/'native-build.log').read_text(),re.I),'Compiler diagnostics')
    mount=run/'mount';mount.mkdir()
    catalog={'materials/target.vmt':'VertexLitGeneric{$alpha .375 $basetexture fixture/base}',
             'target.vmt':'VertexLitGeneric{$alpha .625}',
             'materials/materials/target.vmt':'VertexLitGeneric{$alpha .25}',
             'materials/other.vmt':'Wireframe_DX9{$alpha .75}',
             'materials/chain.vmt':'Wireframe{$fallbackmaterial target}',
             'materials/patch.vmt':'patch{include materials/target.vmt insert{$alpha .5}}',
             'materials/unknown.vmt':'Missing_Fixture_Shader{$alpha .125}'}
    for name,source in catalog.items():p=mount/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(source,encoding='ascii')
    error=node('Wireframe_DX9',[string('$errorfixture','yes')]);cases=[];labels=[];checks=[]
    def add(label,root,prefix=0,owner='fixture/root',expect=None):labels.append(label);cases.append((owner,prefix,root));checks.append(expect)
    for shader in ['VertexLitGeneric','Wireframe_DX9']:
        for fallback in ['missing','fixture/root','target']:
            add('ignored_'+shader+'_'+fallback,node(shader,[string('$fallbackmaterial',fallback)]),expect=dict(root=shader,shader=shader,lookups=[shader],warnings=0))
    add('no_fallback',node('Wireframe'),expect=dict(root='Wireframe',shader='Wireframe_DX9',lookups=['Wireframe','Wireframe_DX9'],warnings=0))
    add('empty_fallback',node('Wireframe',[string('$fallbackmaterial','')]),expect=dict(root='Wireframe',shader='Wireframe_DX9',lookups=['Wireframe','Wireframe_DX9'],warnings=0))
    for owner in ['fixture/root','FIXTURE/ROOT','Fixture/Root']:
        add('self_'+owner,node('Wireframe',[string('$fallbackmaterial','fixture/root')]),owner=owner,expect=dict(root='Wireframe_DX9',shader='Wireframe_DX9',lookups=['Wireframe','Wireframe_DX9'],warnings=1))
    add('missing_file',node('Wireframe',[string('$fallbackmaterial','missing')]),expect=dict(root='Wireframe_DX9',shader='Wireframe_DX9',lookups=['Wireframe','Wireframe_DX9'],warnings=1))
    for label,name,prefix,bits in [('normal','target',0,0x3ec00000),('prefix_root','target',1,0x3f200000),('prefix_materials','materials/target',1,0x3ec00000),('doubled_materials','materials/target',0,0x3e800000)]:
        add(label,node('Wireframe',[string('$fallbackmaterial',name)]),prefix,expect=dict(root='VertexLitGeneric',shader='VertexLitGeneric',lookups=['Wireframe','VertexLitGeneric'],warnings=0,alpha=bits))
    add('repeated_extension',node('Wireframe',[string('$fallbackmaterial','target.vmt')]),expect=dict(root='Wireframe_DX9',shader='Wireframe_DX9',lookups=['Wireframe','Wireframe_DX9'],warnings=1))
    add('patch_target',node('Wireframe',[string('$fallbackmaterial','patch')]),expect=dict(root='VertexLitGeneric',shader='VertexLitGeneric',lookups=['Wireframe','VertexLitGeneric'],warnings=0,alpha=0x3f000000))
    add('chain',node('Wireframe',[integer('$model',1),string('$fallbackmaterial','chain')]),expect=dict(root='VertexLitGeneric',shader='VertexLitGeneric',lookups=['Wireframe','Wireframe','VertexLitGeneric'],warnings=0,alpha=0x3ec00000,model=True))
    add('unknown_shader_file',node('Wireframe',[string('$fallbackmaterial','unknown')]),expect=dict(root='Missing_Fixture_Shader',shader='Wireframe_DX9',lookups=['Wireframe','Missing_Fixture_Shader','Wireframe_DX9'],warnings=0,alpha=0x3e000000))
    add('shader_block_override',node('Wireframe',[string('$fallbackmaterial','target'),node('Wireframe_DX9',[string('$fallbackmaterial','other')])]),expect=dict(root='Wireframe_DX9',shader='Wireframe_DX9',lookups=['Wireframe','Wireframe_DX9'],warnings=0,alpha=0x3f400000))
    add('hardware_block_override',node('Wireframe',[string('$fallbackmaterial','other'),node('GPU<1',[string('$fallbackmaterial','target')])]),expect=dict(root='VertexLitGeneric',shader='VertexLitGeneric',lookups=['Wireframe','VertexLitGeneric'],warnings=0,alpha=0x3ec00000))
    add('inherited_base_fallback',node('Wireframe',[string('$fallbackmaterial','target'),node('Wireframe_DX9',[integer('$extra',1)])]),expect=dict(root='VertexLitGeneric',shader='VertexLitGeneric',lookups=['Wireframe','VertexLitGeneric'],warnings=0,alpha=0x3ec00000))
    add('empty_override_suppresses',node('Wireframe',[string('$fallbackmaterial','target'),node('Wireframe_DX9',[string('$fallbackmaterial','')])]),expect=dict(root='Wireframe',shader='Wireframe_DX9',lookups=['Wireframe','Wireframe_DX9'],warnings=0))
    query=run/'queries.bin';query.write_bytes(struct.pack('<I',len(cases))+b''.join(text(owner)+struct.pack('<I',prefix)+packet(root)+packet(error) for owner,prefix,root in cases));out=run/'reference.bin'
    p=subprocess.run([str(exe),str(PRIVATE),str(libs),str(mount),str(query),str(out)],cwd=run,capture_output=True,text=True,timeout=60);(run/'reference.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Native caller failure; reference.log')
    records=decode(out.read_bytes());require(len(records)==len(cases),'Case count');differences=[]
    for i,(record,expected) in enumerate(zip(records,checks)):
        for k,value in expected.items():
            actual=record['root']['key'] if k=='root' else record['values'][5]['words'][3] if k=='alpha' else bool(record['values'][0]['words'][2]&0x800) if k=='model' else record[k]
            if actual!=value:differences.append(dict(case=i,label=labels[i],field=k,expected=value,actual=actual))
    (run/'expectations.json').write_text(json.dumps(dict(differences=differences,labels=labels,records=records),indent=2)+'\n');require(not differences,'Independent reference expectations; expectations.json')
    count=re.search(r'allocations=(\d+) releases=(\d+) null_frees=(\d+)',p.stdout);require(count and count[1]==count[2],'Tracked allocations released')
    for path,h in originals.items():require(sha(Path(path))==h,'Original changed '+path)
    for path,h in accepted.items():require(sha(Path(path))==h,'Accepted source changed '+path)
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());require(all(sha(ROOT/path)==h for path,h in build['editor_source_dependencies'].items()),'Editor sources changed');require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor changed')
    sources=dict(accepted)
    for name in ['material_fallback_native_fixture.h','probe_material_fallback_reference.cpp','build_material_fallback_reference.cmd','verify_material_fallback_reference.py']:sources[str(ROOT/'work'/name)]=sha(ROOT/'work'/name)
    result=dict(status='PASS_REFERENCE_ONLY',utc=datetime.now(timezone.utc).isoformat(),cases=len(cases),independent_expectations=sum(len(c) for c in checks),exact_reference_bytes=out.stat().st_size,query_sha256=sha(query),output_sha256=sha(out),labels=labels,records=records,executable=dict(path=str(exe),sha256=sha(exe),machine='0x14c',direct_imports=['KERNEL32.dll']),allocations=int(count[1]),releases=int(count[2]),null_frees=int(count[3]),sources=sources,original_hashes=originals,static_ranges=ranges,editor_sha256=build['executable_sha256'],editor_dependencies_unchanged=len(build['editor_source_dependencies']),limits=['Original whole caller, KeyValues allocation/construction/clear/assignment/destruction, fallback file/patch loading and real original filesystem now execute together in one private reference process','Symbol table/heap, material owner/name, shader lookup dispatch, hardware/config/locking/logging and a supplied error-material source tree remain controlled. This does not establish normal public IMaterial creation or actual builtin error-material content','Fixed controlled environment: level92, no HDR/sRGB/GPU tier/Phong overrides. Acyclic bounded string-valued fallback filenames; numeric conversion, arbitrary cycles, truncation/unsafe paths and partial malformed fallback loads not characterized','No owned x64 fallback implementation or integration is added by this reference milestone. Existing owned APIs still refuse nonempty external fallback fields, including fields this reference shows may be ignored','No texture/proxy/resource Init, production editor build/run, live SFM/computer use, original writes, GPU or Blender. Full project incomplete'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-reference.json').write_text(json.dumps(dict(path=str(run),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','independent_expectations','exact_reference_bytes','allocations','releases']},indent=2))

if __name__=='__main__':main()
