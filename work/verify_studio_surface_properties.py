"""Original surface table + actual datacache model/bone cache fill vs owned x64.
Scope: studio collision surface name/index resolution; physical values omitted.
"""
import hashlib,json,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/studio-surface-properties-20261016'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
PRIOR=ROOT/'outputs/static-prop-point-spatial-20261016/verification-lt9fea6j/results.json'
SCOPE=ROOT/'outputs/studio-vcollide-read-20261016/verification-gr1jjsch/results.json'
DUMP=Path('C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def wireString(s):
    b=s.encode('ascii');return struct.pack('<I',len(b))+b
def declarations(text):
    # Independent checked flat fixture scanner for lookups, not native outputs.
    tokens=re.findall(r'//[^\n]*|"[^"\n]*"|[{}]|[^\s{}"]+',text);tokens=[t[1:-1] if t.startswith('"') else t for t in tokens if not t.startswith('//')];i=0;names=[]
    while i<len(tokens):
        name=tokens[i];require(tokens[i+1]=='{','Fixture declaration grammar');i+=2
        while tokens[i]!='}':require(tokens[i] not in ['{','}'] and tokens[i+1] not in ['{','}'],'Fixture flat grammar');i+=2
        i+=1;names.append(name)
    return names
def main():
    prior=json.loads(PRIOR.read_text());scope=json.loads(SCOPE.read_text());require(prior['status']==scope['status']=='PASS','Prior proof');mounts=[Path(s) for s in (Path(prior['path'])/'mounts.txt').read_text().splitlines()]
    def resolve(name):
        require(not Path(name).is_absolute() and '..' not in Path(name).parts,'Fixture relative source path')
        for mount in mounts:
            p=mount/name
            if p.is_file():return p
        raise RuntimeError('Missing surface script '+name)
    manifest=resolve('scripts/surfaceproperties_manifest.txt');manifestText=manifest.read_text();paths=[resolve(n) for n in re.findall(r'"file"\s*"([^"]+)"',manifestText)];require(len(paths)==3,'Benchmark manifest scope changed');scripts=[(n,p.read_text()) for n,p in zip(re.findall(r'"file"\s*"([^"]+)"',manifestText),paths)]
    originals={**prior['original_hashes'],**{str(p):sha(p) for p in [manifest,*paths]}}
    for p,s in originals.items():require(sha(Path(p))==s,'Original changed before query '+p)
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libs=run/'libraries';libs.mkdir()
    for name in ['tier0.dll','vphysics.dll']:shutil.copyfile(GAME/'bin'/name,libs/name)
    _,cacheCode=check_ranges(GAME/'bin/datacache.dll','datacache-disassembly.txt',[('resolve_studio_model_bone_surface_caches',0x10005870,0x100058dd)])
    im,physicsCode=check_ranges(GAME/'bin/vphysics.dll','vphysics-disassembly.txt',[('surface_count',0x10016220,0x10016224),('surface_lookup',0x100163b0,0x10016439),('surface_parse',0x10016a50,0x100174d6)])
    require([im.word(0x100c69d0+i*4) for i in [1,2,3]]==[0x10016a50,0x10016220,0x100163b0],'Original surface vtable evidence');(run/'original-code-evidence.json').write_text(json.dumps(dict(datacache=cacheCode,physics=physicsCode),indent=2))
    executables=[]
    for name in ['surface_native_x86','surface_owned_x64','surface_safety_x64']:
        exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];machine=0x14c if name.endswith('x86') else 0x8664;require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Executable architecture/manifest')
        imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(name+'-imports.txt')).write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(modules and set(x.lower() for x in modules)=={'kernel32.dll'},'Runtime DLL dependency')
        executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
    def execute(name,args,log):
        r=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=60);log.write_text(r.stdout+r.stderr);require(r.returncode==0,'Probe failed '+str(log)+' '+r.stdout+r.stderr);return r.stdout.strip()
    models=[Path(r['mdl']) for r in scope['records']]
    # Disposable model buffers expose cache poisoning and multi-bone/name paths
    # absent from the benchmark's one-bone static models. No originals written.
    synthetic=bytearray(2048);synthetic[:4]=b'IDST'
    for at,value in [(4,49),(76,len(synthetic)),(156,5),(160,400),(308,1600),(396,0x12345678)]:struct.pack_into('<I',synthetic,at,value)
    synthetic[1600:1606]=b'metal\0'
    for i,name in enumerate(['wood','mteal','Grass','default','$MATERIAL_INDEX_SHADOW']):
        base=400+i*216;at=1640+i*48;value=name.encode('ascii')+b'\0';synthetic[at:at+len(value)]=value;struct.pack_into('<III',synthetic,base+176,at-base,1,0x87654321)
    multi=run/'five-bone-model.mdl';multi.write_bytes(synthetic);poison=bytearray(models[0].read_bytes());struct.pack_into('<I',poison,396,0x7fffffff);struct.pack_into('<I',poison,struct.unpack_from('<I',poison,160)[0]+184,0xabcdef01);poisonPath=run/'poisoned-cache-model.mdl';poisonPath.write_bytes(poison);models+=[multi,poisonPath]
    modelNames=[];surfaceNames={}
    for p in models:
        b=p.read_bytes();at=struct.unpack_from('<I',b,308)[0];surfaceNames[str(p)]=b[at:].split(b'\0')[0].decode('ascii');modelNames.append(surfaceNames[str(p)]);count,offset=struct.unpack_from('<II',b,156)
        for i in range(count):at=offset+i*216;name=at+struct.unpack_from('<I',b,at+176)[0];modelNames.append(b[name:].split(b'\0')[0].decode('ascii'))
    scenarios=[('configured-manifest',scripts),('duplicate-file',scripts+[(scripts[0][0].upper(),'SHOULD_NOT_REGISTER {}')]),('different-order',[scripts[0],scripts[2],scripts[1]])]
    rng=random.Random(2026101679)
    for k in range(4):
        tail=['surface_%02d_%03d'%(k,i) for i in range(300)];rng.shuffle(tail);names=['default']+tail
        a='// controlled valid declarations\n'+'\n'.join('"'+n+'" { "density" "1" }' for n in names[:150]);b='\n'.join(n+' { base default }' for n in names[150:]);c='\n'.join(n.upper()+' { friction 0.7 }' for n in names[::3]);scenarios.append(('controlled-%d'%k,[('first',a),('second',b),('override',c),('FIRST','ignored {}')]))
    records=[];totalLookups=totalModels=totalBones=0;benchmarkResolution=None
    for label,inputs in scenarios:
        case=run/label;case.mkdir();names=[]
        for _,text in inputs:names+=declarations(text)
        queries=names+[n.upper() for n in names]+[n.lower() for n in names]+modelNames+['mteal','unknown','','$MATERIAL_INDEX_SHADOW','$material_index_shadow','Wood','metal','SHOULD_NOT_REGISTER','ignored']
        payload=struct.pack('<I',len(inputs))+b''.join(wireString(n)+wireString(t) for n,t in inputs)+struct.pack('<I',len(queries))+b''.join(wireString(n) for n in queries)+struct.pack('<I',len(models))+b''.join(wireString(str(p)) for p in models)
        fixture=case/'fixture.bin';fixture.write_bytes(payload);native=case/'native.bin';owned=case/'owned.bin';execute('surface_native_x86',[GAME/'bin/datacache.dll',libs,fixture,native],case/'native.log');print(execute('surface_owned_x64',['unused',libs,fixture,owned],case/'owned.log'),flush=True)
        a,b=native.read_bytes(),owned.read_bytes()
        if a!=b:
            first=next((i for i,(x,y) in enumerate(zip(a,b)) if x!=y),min(len(a),len(b)));(case/'difference.json').write_text(json.dumps(dict(byte=first,native_size=len(a),owned_size=len(b),native_hex=a[max(0,first-16):first+20].hex(),owned_hex=b[max(0,first-16):first+20].hex()),indent=2));raise RuntimeError('Surface provider mismatch '+str(case/'difference.json'))
        n=struct.unpack_from('<I',a)[0];at=4+4*n;tableCount=struct.unpack_from('<I',a,at)[0];at+=4;q=struct.unpack_from('<I',a,at)[0];at+=4;lookups=struct.unpack_from('<'+'i'*q,a,at);at+=4*q;require(q==len(queries),'Lookup result extent');n=struct.unpack_from('<I',a,at)[0];at+=4;resolved=[];bones=0
        for model in models:
            index,count=struct.unpack_from('<iI',a,at);at+=8;boneIndices=struct.unpack_from('<'+'i'*count,a,at);at+=count*4;bones+=count;resolved.append(dict(model=str(model),surface_name=surfaceNames[str(model)],index=index,trace_surface_u16=index&65535,bones=list(boneIndices)))
        require(at==len(a) and n==len(models),'Model surface result extent');totalLookups+=q;totalModels+=n;totalBones+=bones
        if label=='configured-manifest':
            benchmarkResolution=resolved[:79];require(any(r['index']==-1 for r in benchmarkResolution),'Missing unknown model surface coverage');require(resolved[-2]['bones']==[14,-1,12,0,0xf000] and resolved[-2]['index']==3,'Multi-bone configured IDs');require(resolved[-1]['index']==resolved[0]['index'] and resolved[-1]['bones']==resolved[0]['bones'],'Disk cache words were trusted');require(lookups[queries.index('$MATERIAL_INDEX_SHADOW')]==0xf000,'Reserved shadow ID');(case/'resolved-models.json').write_text(json.dumps(resolved,indent=2))
        require(lookups[queries.index('mteal')]==-1,'Unknown lookup was substituted');records.append(dict(name=label,table_count=tableCount,lookups=q,models=n,bones=bones,path=str(case),input_sha256=sha(fixture),result_sha256=sha(owned)));print('PASS exact original provider/cache '+label+' lookups='+str(q)+' count='+str(tableCount),flush=True)
    safety=execute('surface_safety_x64',[],run/'safety.log');print(safety,flush=True)
    for p,s in originals.items():require(sha(Path(p))==s,'Original changed after tests '+p)
    names=['source_surface_name_table_candidate.h','source_studio_surface_properties_candidate.h','probe_studio_surface_properties.cpp','test_studio_surface_properties.cpp','build_studio_surface_properties.cmd','verify_studio_surface_properties.py'];changed={str(ROOT/'work'/p):sha(ROOT/'work'/p) for p in names};sources=dict(changed);pending=[ROOT/'work'/p for p in names];visited=set()
    while pending:
        p=pending.pop()
        if p in visited:continue
        visited.add(p);sources[str(p)]=sha(p)
        if p.suffix in ['.cpp','.h']:
            for local in re.findall(r'^\s*#include\s*"([^"]+)"',p.read_text(),re.M):
                child=p.parent/local
                if child.is_file():pending.append(child)
    logs=[]
    for p in OUT.glob('*-build.log'):
        text=p.read_text();require(not re.search(r'\b(?:warning|error) [A-Z]+\d+',text),'Compiler diagnostic '+str(p));logs.append(dict(path=str(p),sha256=sha(p),warnings=0,errors=0))
    limitations=['Name/index lookup only: physical/audio/friction/inheritance numeric values are not represented. Do not use as a complete physics/material database.', 'Configured ordered scripts are resolved by explicit existing loose mount list and manifest; actual live engine startup/manifest loader/Steam/archive providers not executed here.', 'Original normal physics parser/lookup/count plus unchanged private datacache cache-fill function execute with actual surface object; native runtime model buffers are workspace memory only.', 'Checked flat ASCII script grammar only; unsupported nested/includes/conditions/escapes/malformed inputs reject. Model/bone name readers preserve input bytes.', 'No editor integration/build, full world+prop scene trace, save/animation/rendering changes or dependencies installed.']
    result=dict(status='PASS',path=str(run),lookup_queries=totalLookups,model_resolutions=totalModels,bone_resolutions=totalBones,differences=0,records=records,safety=safety,benchmark=benchmarkResolution,executables=executables,changed_sources=changed,sources=sources,build_logs=logs,original_hashes=originals,reference_receipts={str(p):sha(p) for p in [PRIOR,SCOPE]},manifest=str(manifest),scripts=[str(p) for p in paths],limitations=limitations)
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2));(OUT/'source-integrity.json').write_text(json.dumps(dict(status='PASS',authoritative_receipt=str(receipt),receipt_sha256=sha(receipt),sources=sources,build_logs=logs,executables=executables,original_hashes=originals),indent=2))
    lines=['VERIFIED studio surface name/index provider and model/bone resolution.','Whole application PARTIALLY VERIFIED.',str(receipt),'Lookups='+str(totalLookups)+' model resolutions='+str(totalModels)+' bone resolutions='+str(totalBones)+'; differences=0',safety,'Original unchanged file hashes='+str(len(originals)),'','Exact freshly built/tested executables:']+[e['path']+' SHA256='+e['sha256'] for e in executables]+['','Changed files:']+list(changed)+['','Limitations:']+limitations+['','Reproduce:','cmd /c work\\build_studio_surface_properties.cmd','C:\\Users\\wigwo\\AppData\\Local\\Python\\bin\\python3.14.exe work\\verify_studio_surface_properties.py'];(OUT/'verification.txt').write_text('\n'.join(lines)+'\n');print('PASS '+str(receipt),flush=True)
if __name__=='__main__':main()
