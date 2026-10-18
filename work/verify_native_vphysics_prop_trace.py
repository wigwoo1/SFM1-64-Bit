"""Owned x64 collision geometry vs original DLL; original-only trace reference corpus."""
import hashlib,json,math,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/native-vphysics-prop-trace-20261015'
GAME=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
MAP=GAME/'tf_movies/maps/mtt_soldier.bsp'
MAP_SHA='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(value,message):
    if not value:raise RuntimeError(message)
def mesh_summary(b):
    n=struct.unpack_from('<H',b)[0];p=2;convexes=triangles=0
    for _ in range(n):
        count=struct.unpack_from('<I',b,p)[0];p+=4;convexes+=count
        for _ in range(count):
            game,num=struct.unpack_from('<2I',b,p);p+=8;triangles+=num
            values=struct.unpack_from('<%df'%(num*9),b,p);require(all(math.isfinite(v) for v in values),'Nonfinite mesh');p+=num*36
    require(p==len(b),'Mesh extent');return dict(solids=n,convexes=convexes,triangles=triangles)
def main():
    require(sha(MAP)==MAP_SHA,'Pinned benchmark changed');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libraries=run/'libraries';libraries.mkdir();libraryEvidence=[]
    for name in ['tier0.dll','vphysics.dll']:
        source=GAME/'bin'/name;dest=libraries/name;before=sha(source);shutil.copyfile(source,dest);require(sha(dest)==before,'Library copy mismatch');libraryEvidence.append(dict(source=str(source),copy=str(dest),sha256=before))
    # Declared loose-file search order only, with no archive/dynamic-mount claim.
    gameinfo=GAME/'usermod/gameinfo.txt';text=gameinfo.read_text();paths=[]
    search=re.search(r'SearchPaths\s*\{([^}]*)\}',text,re.I|re.S);require(search is not None,'SearchPaths block')
    for line in search[1].splitlines():
        m=re.match(r'\s*Game\s+([^\s/]+)',line,re.I)
        if not m:continue
        value=m[1].strip('"')
        if value=='|gameinfo_path|.':mount=GAME/'usermod'
        elif '|' in value or '*' in value or ':' in value:raise RuntimeError('Unsupported declared mount')
        else:mount=GAME/value
        paths.extend([Path(str(mount)+'_english'),mount])
    require(paths and paths[0]==GAME/'usermod_english','Declared game mounts')
    b=MAP.read_bytes();go,gl,gv,gc=struct.unpack_from('<4I',b,8+35*16);require(gv==gc==0 and go+gl<=len(b),'Game lump');names=None
    for i in range(struct.unpack_from('<I',b,go)[0]):
        id,flags,version,off,length=struct.unpack_from('<IHHII',b,go+4+16*i)
        if id==int.from_bytes(b'prps','little'):
            require(flags==0 and version==5 and off+length<=len(b),'Static prop lump');n=struct.unpack_from('<I',b,off)[0];names=[b[off+4+j*128:off+4+(j+1)*128].split(b'\0')[0].decode('ascii') for j in range(n)]
    require(names is not None and len(names)==79,'Pinned static model inventory')
    def resolve(name):return next((p/name for p in paths if (p/name).is_file()),None)
    executables=[]
    for name,machine in [('vphysics_prop_native_x86',0x14c),('phy_geometry_owned_x64',0x8664)]:
        exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Executable architecture/manifest');executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine)))
    records=[];missing=[];rng=random.Random(2026101517)
    for model in names:
        phy=resolve(str(Path(model).with_suffix('.phy')));mdl=resolve(model)
        if phy is None:missing.append(dict(model=model,reason='No loose PHY in declared paths; archives/actual runtime absence not verified'));continue
        require(mdl is not None,'PHY model header missing');originalHash=sha(phy);mdlHash=sha(mdl);data=mdl.read_bytes();require(data[:4]==b'IDST' and struct.unpack_from('<I',data,4)[0] in [48,49],'MDL header type');lo=struct.unpack_from('<3f',data,104);hi=struct.unpack_from('<3f',data,116);require(all(math.isfinite(x) for x in lo+hi) and all(a<=z for a,z in zip(lo,hi)),'MDL model bounds');center=[(a+z)*.5 for a,z in zip(lo,hi)];radius=max(64.,max(z-a for a,z in zip(lo,hi))*2)
        case=run/('%03d'%len(records));case.mkdir();rows=[]
        def add(start,delta,origin=(0,0,0),angles=(0,0,0),mask=0x4481):rows.append(struct.pack('<12fI',*start,*delta,*origin,*angles,mask))
        for axis in range(3):
            for sign in [-1,1]:
                for offset in [-radius,0,radius]:
                    start=center[:];delta=[0.,0.,0.];start[axis]-=sign*radius;delta[axis]=sign*radius*2;start[(axis+1)%3]+=offset;add(start,delta)
        for _ in range(96):
            start=[rng.uniform(a-radius*.25,z+radius*.25) for a,z in zip(lo,hi)];delta=[rng.uniform(-radius,radius) for _ in range(3)];origin=[rng.uniform(-2048,2048) for _ in range(3)];add([a+o for a,o in zip(start,origin)],delta,origin,rng.choice([(0,0,0),(0,90,0),(15,37,-11)]))
        for point in [center,list(lo),list(hi),[v+radius*2 for v in center]]:add(point,[0,0,0])
        for mask in [0,0xffffffff,0x4481]:add([center[0]-radius,center[1],center[2]],[radius*2,0,0],mask=mask)
        query=case/'queries.bin';query.write_bytes(b''.join(rows));nativeMesh=case/'native-mesh.bin';ownedMesh=case/'owned-mesh.bin';trace=case/'native-traces.bin';metadata=case/'native-metadata.json'
        def execute(exe,args,log):
            result=subprocess.run([str(OUT/(exe+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=30,cwd=case);(case/log).write_text(result.stdout+result.stderr);require(result.returncode==0,'Probe '+exe+' failed '+str(case)+' '+result.stdout+result.stderr)
        execute('vphysics_prop_native_x86',[libraries,phy,query,trace,metadata,nativeMesh],'native.log');execute('phy_geometry_owned_x64',[phy,ownedMesh],'owned.log');a,c=nativeMesh.read_bytes(),ownedMesh.read_bytes();require(a==c,'Owned PHY mesh mismatch: '+str(case));summary=mesh_summary(a);meta=json.loads(metadata.read_text());require(meta['status']=='PASS' and meta['cases']==len(rows) and len(trace.read_bytes())==len(rows)*60,'Physics result extent');require(sha(phy)==originalHash and sha(mdl)==mdlHash,'Original model changed')
        records.append(dict(model=model,phy=str(phy),phy_sha256=originalHash,mdl=str(mdl),mdl_sha256=mdlHash,summary=summary,queries=len(rows),query_sha256=sha(query),mesh_sha256=sha(nativeMesh),trace_sha256=sha(trace),native_metadata=meta,path=str(case)))
        print('PASS',len(records),model,summary,flush=True)
    require(len(records)==61 and len(missing)==18,'Benchmark loose-PHY availability changed: review coverage')
    # Repeat exact native load/extract/trace/unload for first shape. No shared
    # process or live state retained between the executions.
    first=Path(records[0]['path']);repeat=run/'repeat';repeat.mkdir();result=subprocess.run([str(OUT/'vphysics_prop_native_x86.exe'),str(libraries),records[0]['phy'],str(first/'queries.bin'),str(repeat/'traces.bin'),str(repeat/'metadata.json'),str(repeat/'mesh.bin')],capture_output=True,text=True,timeout=30,cwd=repeat);require(result.returncode==0 and sha(repeat/'traces.bin')==records[0]['trace_sha256'] and sha(repeat/'mesh.bin')==records[0]['mesh_sha256'],'Repeat native execution differs')
    # Invalid data is given ONLY to the owned parser, never to original DLLs.
    valid=Path(records[0]['phy']).read_bytes();start=struct.unpack_from('<I',valid)[0];compact=start+32;root=compact+struct.unpack_from('<i',valid,compact+32)[0];leaf=root
    while struct.unpack_from('<i',valid,leaf)[0]:leaf+=28
    ledge=leaf+struct.unpack_from('<i',valid,leaf+4)[0]
    bad=[]
    def mutate(label,at,fmt,value):
        raw=bytearray(valid);struct.pack_into(fmt,raw,at,value);bad.append((label,bytes(raw)))
    bad.extend([('short_header',valid[:15]),('short_solid',valid[:start+40]),('no_script',valid[:start+4+struct.unpack_from('<I',valid,start)[0]])])
    for label,at,value in [('bad_id',4,1),('bad_header_size',0,0xffffffff),('no_solids',8,0),('bad_vphy',start+4,0),('bad_size',start,1),('root_outside',compact+32,0x7fffffff),('bad_ivps',compact+44,0),('bad_axis_map',start+28,1),('leaf_no_ledge',leaf+4,0),('points_outside',ledge,0x7fffffff),('bad_ledge_flags',ledge+8,0)]:mutate(label,at,'<I',value)
    mutate('version',start+8,'<H',1);mutate('model_type',start+10,'<H',1);mutate('triangle_count',ledge+12,'<H',0x7fff)
    point=ledge+struct.unpack_from('<i',valid,ledge)[0];mutate('nonfinite_vertex',point,'<I',0x7f800000)
    invalid=run/'invalid';invalid.mkdir()
    for label,raw in bad:
        path=invalid/(label+'.phy');path.write_bytes(raw);result=subprocess.run([str(OUT/'phy_geometry_owned_x64.exe'),str(path),str(invalid/(label+'.bin'))],capture_output=True,text=True,timeout=15);require(result.returncode==1,'Malformed PHY did not reject: '+label)
    from verify_ifm_draw_packet import check_ranges
    physics,evidence=check_ranges(GAME/'bin/vphysics.dll','vphysics-disassembly.txt',[('interface_registration',0x100c39d0,0x100c39e5),('interface_factory',0x10005b40,0x10005b46),('trace_wrapper',0x10005e00,0x10005e0c),('query_model_create',0x10009270,0x100092b0),('query_convex_count',0x1001de40,0x1001de44),('query_triangle_count',0x10007040,0x1000705e),('query_game_data',0x10007060,0x1000707d),('query_triangle_vertices',0x10007080,0x10007189)])
    require(physics.string(0x100c5bb0)=='VPhysicsCollision007' and physics.word(0x100ee0c0)==0x100c5a78 and physics.word(0x100c5a78+31*4)==0x10005e00 and physics.word(0x100c5a78+36*4)==0x100090a0 and physics.word(0x100c5a78+37*4)==0x10005eb0,'Physics interface map');require(physics.word(0x100ee00c)==0x421d7af6,'Original physics unit scale')
    for e in libraryEvidence:require(sha(Path(e['source']))==e['sha256'] and sha(Path(e['copy']))==e['sha256'],'Library modified')
    require(sha(MAP)==MAP_SHA,'Benchmark modified');totals={key:sum(r['summary'][key] for r in records) for key in ['solids','convexes','triangles']};totals.update(models=len(records),traces=sum(r['queries'] for r in records),fraction_hits=sum(r['native_metadata']['fraction_hits'] for r in records),start_solid=sum(r['native_metadata']['start_solid'] for r in records))
    receipt=dict(status='PASS',scope='Owned x64 PHY triangle geometry verified; physics trace outputs are original-only calibration, not owned trace parity',totals=totals,mesh_differences=0,invalid_owned_checks=len(bad),repeat_native_exact=True,executables=executables,libraries=libraryEvidence,gameinfo=str(gameinfo),gameinfo_sha256=sha(gameinfo),declared_loose_mounts=list(map(str,paths)),map_sha256=MAP_SHA,models=records,missing_loose_phy=missing,static_evidence=evidence,source_sha256={n:sha(ROOT/'work'/n) for n in ['source_phy_compact_geometry_candidate.h','probe_owned_phy_compact_geometry.cpp','probe_native_vphysics_prop_trace.cpp','build_native_vphysics_prop_trace.cmd','verify_native_vphysics_prop_trace.py']},limits=['Owned geometry extraction only, not owned physics tracing or engine solid6 dispatch integration','Original DLLs copied to workspace and loaded normally only by short-lived test x86 process; not private PE/no-DllMain oracle in this milestone','Native trace corpus tests first solid per model; geometry compares every loaded solid/convex/triangle','Little-endian VPHY version0100/type0/IVPS only; axis-map variants/legacy/MOPP/other versions rejected or unsupported','61 loose PHY files in declared GameInfo English mount order; archives/dynamic mounts/live runtime source selection unverified; 18 model names have no loose PHY','Native traces use null convex-contents callback; exact engine-specific callback contents still unverified','No original writes, editor changes/build/integration, application UI launches or dependency installs; test DLL helper processes exit after use'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status='PASS',totals=totals,invalid_owned_checks=len(bad))));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
