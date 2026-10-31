"""Complete original Phong feature extraction vs owned x64; typed inputs only.

Historical runtime hashes are preserved in the publication backup, while the
authorized newly published runtime is authenticated separately. No original writes.
"""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,itertools,json,random,re,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS
from verify_ifm_draw_packet import check_ranges
from verify_shader_parameter_registry import decode

OUT=ROOT/'outputs/phong-material-features-20261031'
GAME=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
PARAMS=[('$basetexture',6,0),('$compress',14,4),('$stretch',15,8),('$bumpmap',26,0x3c),
        ('$bumpcompress',27,0x40),('$bumpstretch',28,0x44),('$envmap',20,0x24),
        ('$selfillumfresnel',34,0x70),('$phong',48,0xa8),('$phongwarptexture',44,0x90),
        ('$lightwarptexture',43,0x8c),('$detail',17,0x18),('$rimlight',58,0xc0)]


def need(ok,why):
    if not ok:raise RuntimeError(why)


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def expected(row):
    serial,flags,gate,only,*flat=row
    values=[flat[i:i+3] for i in range(0,39,3)]
    tex=lambda i:values[i][0]!=0 and values[i][1]%16==3
    integer=lambda i:values[i][0]!=0 and values[i][2]!=0
    base=tex(0);self=bool(flags&64);phong=integer(8)
    result=[base,base and tex(1) and tex(2),tex(3) and tex(4) and tex(5),tex(10),phong,
            tex(9),tex(11),self,self and integer(7),not only and tex(6),bool(gate) and phong and integer(12)]
    return struct.pack('<I11B',serial,*map(int,result))


def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));(OUT/'active-run.txt').write_text(str(run),encoding='utf-8')
    idx=json.loads((ROOT/'outputs/phong-pixel-variant-20261030/latest-verification.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'selector receipt identity')
    prior=json.loads(Path(idx['receipt']).read_text());sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256'])
    need(all(sha(p)==h for p,h in sources.items()),'prior source identities')
    changed=[p for p,h in protected.items() if sha(p)!=h]
    canonical=ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe'
    publication=ROOT/'publication/sync-20261030-1301-verified.local.json';pub=json.loads(publication.read_text())
    build_path=ROOT/'publication/stage/docs/build-status.json';build=json.loads(build_path.read_text())
    state=json.loads((ROOT/'publication/state.json').read_text())
    need(pub['commit']==state['commit']=='69839ff9a195287ec8247875aa19e0814d98b00a','accepted publication commit')
    need(changed==[str(canonical)],'only explicitly published runtime may differ')
    backup=ROOT/'publication/runtime-before-20261030-1301/sfm64_session_ui.exe'
    need(sha(backup)==protected[str(canonical)],'historical runtime backup')
    need(sha(canonical)==build['executable_sha256']=='831094c2b3b4f30b8e7507aca4012205602b3132f6635be6446586f334096d59','published runtime identity')
    protected[str(backup)]=sha(backup);protected[str(canonical)]=sha(canonical)
    protected[str(publication)]=sha(publication);protected[str(build_path)]=sha(build_path)
    protected[idx['receipt']]=idx['receipt_sha256']
    for name in ['phong_material_features_candidate.h','probe_phong_material_features.cpp',Path(__file__).name,'verify_shader_parameter_registry.py']:
        p=ROOT/'work'/name;sources[str(p)]=sha(p)
    std=GAME/'bin/stdshader_dx9.dll'
    image,evidence=check_ranges(std,'stdshader_dx9-disassembly.txt',[
        ('complete_phong_feature_helper',0x10037f40,0x100380eb),('draw_feature_handoff',0x10038226,0x1003823a),
        ('normal_path_envmap_recheck',0x1003827e,0x100382b2),('material_info_index_handoff',0x10052f10,0x100532a2),
        ('rim_control_registration',0x10080890,0x100808a9),('convar_ctor_prefix',0x10066b00,0x10066b60),
        ('convar_parent_self_link',0x10066980,0x1006698c)])
    need(image.string(0x100966c0)=='r_rimlight','rim control name')
    meta=ROOT/'outputs/material-effective-parameters-20261017/verification-l11z2le6'
    metadata_receipt=json.loads((meta/'results.json').read_text());registry=meta/'registry-0.bin'
    need(sha(registry)==metadata_receipt['output_sha256'],'accepted registry evidence')
    metadata=json.loads((meta/'shader-metadata.json').read_text());need(metadata==decode(registry.read_bytes()),'registry decode')
    material=next(x for x in metadata if x['name']=='VertexLitGeneric')
    handoff=next(x for x in evidence if x['name']=='material_info_index_handoff');loaded={};links={}
    for row in handoff['instructions']:
        ins=row['instruction'];m=re.fullmatch(r'mov\s+(ecx|edx),dword ptr ds:\[([0-9A-F]+)h\]',ins)
        if m:loaded[m[1]]=int(m[2],16)
        m=re.fullmatch(r'mov\s+dword ptr \[eax(?:\+([0-9A-F]+)h?)?\],(ecx|edx)',ins)
        if m:links[int(m[1] or '0',16)]=loaded[m[2]]
    for name,index,offset in PARAMS:
        p=material['parameters'][index];need(p['name'].lower()==name,'schema name/index '+name)
        if index>=13:need(links[offset]==image.base+p['rva']+20,'schema offset/name '+name)
    need(any(x['bytes']=='c70006000000' for x in handoff['instructions']),'base index6 assignment')
    for p in [registry,meta/'results.json',meta/'shader-metadata.json']:protected[str(p)]=sha(p)
    batch=run/'build.cmd';lines=['@echo off',f'cd /d "{ROOT}"']
    for arch,vcvars in [('x86',VS.parent/'vcvars32.bat'),('x64',VS)]:
        lines += [f'call "{vcvars}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_phong_material_features.cpp /Fo:"{run/(arch+".obj")}" /Fe:"{run/(arch+".exe")}" >"{run/(arch+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1']
    batch.write_text('\n'.join(lines+['exit /b 0','']),encoding='utf-8')
    def execute(args,label,success=True):
        p=subprocess.run([str(a) for a in args],cwd=ROOT,capture_output=True,text=True,timeout=120)
        (run/(label+'.log')).write_text(p.stdout+p.stderr,encoding='utf-8')
        if success:need(p.returncode==0,f'{label} exit{p.returncode}: {(p.stdout+p.stderr)[-1200:]}')
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver')
    rows=[];coverage={};rng=random.Random(2026103101)
    def add(values,flags=64,gate=1,only=0,label='controlled'):
        if not only and not values[6][0]:return
        row=[len(rows),flags,gate,only,*[x for v in values for x in v]];rows.append(row);coverage[label]=coverage.get(label,0)+1
    def full():return [[1,3,1] for _ in range(13)]
    add(full());add(full(),flags=0,gate=0)
    for present in range(1<<13):
        values=full()
        for i in range(13):values[i][0]=(present>>i)&1
        add(values,only=0 if present&64 else 255,label='all_declaration_patterns')
    for i,typ in itertools.product(range(13),range(256)):
        values=full();values[i][1]=typ;add(values,label='all_type_bytes_each_parameter')
    for i,value,declared in itertools.product([7,8,12],[0,1,2,0xffffffff,0x80000000],range(2)):
        values=full();values[i]=[declared,0x7f,value];add(values,label='raw_integer_truth')
    for flags,gate,only in itertools.product([0,1,63,64,65,0xffffffff,0x80000000],[0,1,2,0xffffffff,0x80000000],[0,1,2,255]):
        add(full(),flags,gate,only,'flags_global_and_flashlight')
    for _ in range(4096):
        values=[[rng.randrange(2),rng.randrange(256),rng.getrandbits(32)] for _ in range(13)]
        # Frequently exercise texture type3 and integer0 instead of only false/random branches.
        for v in values:
            if rng.randrange(2):v[1]=(v[1]&240)|3
            if rng.randrange(3)==0:v[2]=0
        only=rng.choice([0,1,255])
        if not only:values[6][0]=1
        add(values,rng.getrandbits(32),rng.choice([0,1,rng.getrandbits(32)]),only,'seeded_combinations')
    query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<43I',*r) for r in rows))
    wanted=b''.join(expected(r) for r in rows);binaries={};warnings={}
    for arch,machine,magic in [('x86',0x14c,0x10b),('x64',0x8664,0x20b)]:
        exe=run/(arch+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        need(struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic,'probe architecture')
        warnings[arch]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(arch+'-build.log')).read_text(errors='replace'))
        need(not warnings[arch],'probe compiler warnings')
        binaries[arch]=dict(path=str(exe),sha256=sha(exe))
        for repeat in range(2):
            target=run/(arch+'-'+str(repeat)+'.bin');execute([exe,std if arch=='x86' else run/'absent-original.dll',query,target],arch+'-'+str(repeat))
            need(target.read_bytes()==wanted,arch+' complete feature trace')
    # Bind independently verified feature bytes to the existing original selector
    # fixture. External draw settings stay explicitly controlled, not inferred.
    selector_rows=[];selector_want=bytearray()
    for i in sorted(set([0,1,len(rows)-1]+list(range(2,len(rows),max(1,len(rows)//128))))):
        features=list(wanted[i*15+4:i*15+15]);only=rows[i][3]
        for kind in [0,1]:
            x=[features[9],features[7],features[8],int(bool(only)),int(bool(only)),features[3],features[4],features[5],features[1],features[6],i%7,features[10],i%3,i%2,(i//2)%2,(i//3)%2]
            serial=len(selector_rows);selector_rows.append(struct.pack('<18I',serial,kind,*x))
            # Import only the established independent selector reference after
            # its source was authenticated in the previous source closure.
            from verify_phong_pixel_variant import expected as selector_expected
            selector_want.extend(selector_expected(serial,kind,x))
    sq=run/'selector-queries.bin';sq.write_bytes(b''.join(selector_rows))
    for arch in ['x86','x64']:
        e=prior['executables'][arch];need(sha(e['path'])==e['sha256'],'selector executable identity')
        protected[e['path']]=e['sha256'];target=run/(arch+'-selector.bin')
        execute([e['path'],std if arch=='x86' else run/'absent-original.dll',sq,target],arch+'-selector')
        need(target.read_bytes()==selector_want,'feature-to-selector complete trace '+arch)
    refusals=[]
    for label,mutate in [('missing-env',lambda r:r.__setitem__(4+6*3,0)),('type-range',lambda r:r.__setitem__(5,256)),('flash-range',lambda r:r.__setitem__(3,256))]:
        row=rows[0].copy();mutate(row);bad=run/(label+'.bin');bad.write_bytes(struct.pack('<43I',*row))
        for arch in ['x86','x64']:
            p=execute([run/(arch+'.exe'),std if arch=='x86' else run/'absent-original.dll',bad,run/(label+'-'+arch+'.bin')],label+'-'+arch,False)
            need(p.returncode!=0,'required-index/domain refusal '+label);refusals.append(label+'-'+arch)
    need(all(sha(p)==h for p,h in sources.items()),'current source identities preserved')
    need(all(sha(p)==h for p,h in protected.items()),'current protected identities preserved')
    result=dict(status='VERIFIED',time_utc=datetime.now(timezone.utc).isoformat(),cases=len(rows),coverage=coverage,repetitions=2,
                feature_trace_bytes=len(wanted),feature_trace_sha256=hashlib.sha256(wanted).hexdigest(),selector_composition_cases=len(selector_rows),
                selector_trace_sha256=hashlib.sha256(selector_want).hexdigest(),refusals=refusals,executables=binaries,compiler_warnings=warnings,
                authenticated_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in evidence],parameters=PARAMS,
                source_sha256=sources,protected_sha256=protected,
                publication_checkpoint=dict(commit=pub['commit'],old_runtime_backup=str(backup),new_runtime_sha256=sha(canonical)),
                limits=['Complete original feature helper only; material initialization, texture loading and proxy state not executed.',
                        'Explicit effective typed records, flags, raw r_rimlight parent value and flashlight-only byte; actual benchmark state UNVERIFIED.',
                        'Feature-to-selector composition uses controlled external detail/filter/half-Lambert/world/tool inputs; not a whole native draw.',
                        'No editor integration/rebuild, GPU/live SFM/computer use, original writes or publication.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt)),indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:result[k] for k in ['status','cases','coverage','feature_trace_bytes','selector_composition_cases','refusals','executables','compiler_warnings']},indent=2),flush=True)


if __name__=='__main__':main()
