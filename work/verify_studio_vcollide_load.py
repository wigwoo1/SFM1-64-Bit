"""Whole original synchronous no-include cache/FS/physics pipeline vs owned assets.
All write targets are dedicated workspace outputs. Original assets stay read-only.
"""
import hashlib,json,shutil,struct,subprocess,tempfile,zipfile,io
from pathlib import Path
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/studio-vcollide-read-20261016'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def main():
    prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS' and sha(GAME/'tf_movies/maps/mtt_soldier.bsp')==prior['map_sha256'] and sha(Path(prior['gameinfo']))==prior['gameinfo_sha256'],'Prior inputs changed');OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libraries=json.loads((OUT/'library-copies.json').read_text());require(len(libraries)==5,'Library copy inventory')
    for r in libraries:require(sha(Path(r['source']))==sha(Path(r['copy']))==r['sha256'],'Library identity')
    _,evidence=check_ranges(GAME/'bin/datacache.dll','datacache-disassembly.txt',[('whole_begin_load',0x1000dbe0,0x1000df8a),('read_request',0x100063b0,0x10006435),('poll_complete',0x1000d6b0,0x1000d812),('detach_job',0x1000d010,0x1000d171),('publish_job',0x1000ccd0,0x1000cd71),('complete_dispatch',0x1000eb80,0x1000f084),('collision_destructor',0x10006ef0,0x10006f26)]);(run/'original-code-evidence.json').write_text(json.dumps(evidence,indent=2))
    # ABI correction is checked by actual loaded slot RVA assertions too.
    _,fsEvidence=check_ranges(GAME/'bin/filesystem_stdio.dll','filesystem-disassembly.txt',[('AddSearchPath',0x1000ea40,0x1000eae9),('RemoveAllSearchPaths',0x1000b190,0x1000b1e1),('not_AddSearchPath_RemoveSearchPath',0x10009823,0x10009836),('not_GetSearchPath_RemoveSearchPaths',0x1000b3e0,0x1000b410),('AsyncReadMultiple',0x100112e0,0x10011301),('AsyncRelease',0x100114b0,0x100114c5)]);(run/'filesystem-code-evidence.json').write_text(json.dumps(fsEvidence,indent=2))
    b=(GAME/'tf_movies/maps/mtt_soldier.bsp').read_bytes();g=struct.unpack_from('<I',b,568)[0]
    for i in range(struct.unpack_from('<I',b,g)[0]):
        t,f,v,offset,length=struct.unpack_from('<IHHII',b,g+4+i*16)
        if t==0x73707270:break
    require((f,v)==(0,5),'Static prop encoding');names=[b[offset+4+i*128:offset+4+(i+1)*128].split(b'\0')[0].decode('ascii') for i in range(struct.unpack_from('<I',b,offset)[0])];require(len(names)==79,'Model inventory');po,pl,pv,pc=struct.unpack_from('<4I',b,8+40*16);require(pv==pc==0,'BSP archive encoding');pak=zipfile.ZipFile(io.BytesIO(b[po:po+pl]));require(not any(x.filename.lower().endswith('.phy') for x in pak.infolist()),'Embedded collision requires provider support')
    mounts=[Path(p) for p in prior['declared_loose_mounts']];mountFile=run/'mounts.txt';mountFile.write_text('\n'.join(map(str,mounts))+'\n');old={r['model']:r for r in prior['models']};originals={str(GAME/'tf_movies/maps/mtt_soldier.bsp'):prior['map_sha256'],prior['gameinfo']:prior['gameinfo_sha256']}
    exeNames=['filesystem_slots_native_x86','studio_file_read_native_x86','studio_vcollide_load_native_x86','studio_collision_asset_owned_x64','studio_collision_asset_safety_x64'];executables=[]
    for name in exeNames:
        p=OUT/(name+'.exe');raw=p.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];machine=0x8664 if name.endswith('x64') else 0x14c;require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Executable identity/manifest');executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    def execute(name,args,log):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=45);log.write_text(p.stdout+p.stderr);require(p.returncode==0,'Probe failed '+str(log)+' '+p.stdout+p.stderr);return p.stdout
    first=prior['models'][0];safety=execute('studio_collision_asset_safety_x64',[first['phy'],first['mdl']],run/'safety.log');print(safety,flush=True)
    # Real original read service, in separate cwd from its mounted file root.
    readRoot=run/'read-fixtures';readRoot.mkdir();readEvidence=[]
    for i,size in enumerate([0,1,256,4097]):
        source=readRoot/('present-'+str(i)+'.bin');source.write_bytes(bytes((n*17+3)&255 for n in range(size)));output=run/('read-'+str(i)+'.bin');metadata=json.loads(execute('studio_file_read_native_x86',[GAME/'bin/datacache.dll',OUT/'libraries',readRoot,source.name,output],run/('read-'+str(i)+'.log')));require(metadata['result']==0 and metadata['bytes']==size and output.read_bytes()==source.read_bytes() and metadata['released'],'Actual original read mismatch');readEvidence.append(metadata)
    metadata=json.loads(execute('studio_file_read_native_x86',[GAME/'bin/datacache.dll',OUT/'libraries',readRoot,'never-created.bin',run/'read-missing.bin'],run/'read-missing.log'));require(metadata['start']==0 and metadata['result']==-1 and metadata['bytes']==0 and metadata['had_control']==1 and metadata['released'],'Missing read contract');readEvidence.append(metadata)
    records=[];loaded=missing=0
    for i,name in enumerate(names):
        mdl=next((p/name for p in mounts if (p/name).is_file()),None);require(mdl is not None,'Model missing '+name);md=mdl.read_bytes();require(md[:4]==b'IDST' and len(md)>=400 and struct.unpack_from('<I',md,336)[0]==0,'No-included-model scope');originals[str(mdl)]=sha(mdl);phy=next((p/Path(name).with_suffix('.phy') for p in mounts if (p/Path(name).with_suffix('.phy')).is_file()),None)
        if phy:originals[str(phy)]=sha(phy)
        case=run/('%03d'%i);case.mkdir();native=case/'native.mesh';owned=case/'owned.mesh';a=json.loads(execute('studio_vcollide_load_native_x86',[OUT/'libraries',mountFile,name,mdl,native],case/'native.log'));c=json.loads(execute('studio_collision_asset_owned_x64',[mountFile,name,mdl,owned],case/'owned.log'));require(native.read_bytes()==owned.read_bytes(),'Original whole-load/owned geometry mismatch '+name);require(a['read_requests']==1 and a['read_start']==0 and a['requested_name'].lower()==str(Path(name).with_suffix('.phy')).replace('\\','/').lower() and a['requested_path']=='GAME' and a['flags']==0x82 and a['jobs_drained'] and a['released'],'Whole load lifecycle contract');require(all(a[k]==c[k] for k in ['loaded_solids','convexes','triangles']),'Mesh count mismatch')
        if phy:
            loaded+=1;require(a['read_result']==0 and a['loaded_solids']==1 and c['known_missing_in_configured_loose_mounts']==0,'Known present load result');require(name in old and sha(native)==old[name]['mesh_sha256'],'Prior direct-native mesh changed '+name)
        else:
            missing+=1;require(a['read_result']==-1 and a['loaded_solids']==0 and c['known_missing_in_configured_loose_mounts']==1 and native.read_bytes()==b'\0\0','Completed missing result')
        records.append(dict(model=name,mdl=str(mdl),phy=str(phy) if phy else None,metadata=a,mesh_sha256=sha(native),path=str(case)))
        if (i+1)%10==0:print('PASS original full cache/FS/physics vs owned',i+1,flush=True)
    require((loaded,missing)==(61,18),'Availability changed; review coverage')
    # Valid workspace PHY copies prove first mounted source wins, in both orders.
    shadow=[];aRoot=run/'shadow-a';bRoot=run/'shadow-b'
    for root in [aRoot,bRoot]:(root/'models').mkdir(parents=True)
    different=next(x for x in prior['models'] if x['mesh_sha256']!=first['mesh_sha256']);shutil.copyfile(first['phy'],aRoot/'models/shadow.phy');shutil.copyfile(different['phy'],bRoot/'models/shadow.phy')
    for label,ordered,reference in [('ab',[aRoot,bRoot],first),('ba',[bRoot,aRoot],different)]:
        mf=run/('shadow-'+label+'.txt');mf.write_text('\n'.join(map(str,ordered))+'\n');paths=[]
        for exe,args in [('studio_vcollide_load_native_x86',[OUT/'libraries',mf,'models/shadow.mdl',first['mdl']]),('studio_collision_asset_owned_x64',[mf,'models/shadow.mdl',first['mdl']])]:
            output=run/('shadow-'+label+'-'+exe+'.mesh');execute(exe,[*args,output],run/('shadow-'+label+'-'+exe+'.log'));paths.append(output)
        require(paths[0].read_bytes()==paths[1].read_bytes() and sha(paths[0])==reference['mesh_sha256'],'Explicit mount precedence differs');shadow.append(dict(order=label,mesh_sha256=sha(paths[0])))
    gate=next(r for r in records if 'farm_gate/farm_gate.mdl' in r['model']);execute('studio_vcollide_load_native_x86',[OUT/'libraries',mountFile,gate['model'],gate['mdl'],run/'gate-repeat.mesh'],run/'gate-repeat.log');require(sha(run/'gate-repeat.mesh')==gate['mesh_sha256'],'Repeated missing gate result differs')
    for p,s in originals.items():require(sha(Path(p))==s,'Original asset changed')
    for e in libraries:require(sha(Path(e['source']))==sha(Path(e['copy']))==e['sha256'],'Library changed after probes')
    files=['source_studio_loose_collision_asset_candidate.h','probe_studio_loose_collision_asset.cpp','test_studio_loose_collision_asset.cpp','build_studio_loose_collision_asset.cmd','probe_native_studio_vcollide_load.cpp','build_native_studio_vcollide_load.cmd','probe_native_studio_file_read.cpp','build_native_studio_file_read.cmd','probe_filesystem_slots.cpp','build_filesystem_slots.cmd','verify_studio_vcollide_load.py','filesystem-disassembly.txt','datacache-disassembly.txt'];sources={str(Path('work')/p):sha(ROOT/'work'/p) for p in files}
    result=dict(status='PASS',path=str(run),models=79,loaded_shapes=loaded,completed_missing=missing,geometry_differences=0,matched_prior_native_shapes=61,safety=safety.strip(),real_filesystem_read_cases=readEvidence,mount_precedence=shadow,farm_gate=gate,records=records,libraries=libraries,original_hashes=originals,executables=executables,sources=sources,limitations=['Explicit declared GAME directory mounts; BSP pak checked to contain no PHY. Steam/dynamic/other live SFM mounts not initialized.','No-included-model synchronous scope only; model cache dictionary and already-loaded real MDL header supplied explicitly.','Full original publicGetVCollide/begin/read/jobmap/poll/detach/completion/physics load actually execute; FS observer only forwards/counts without synthetic results or code patches.','Owned asset reader supports configured loose directories, no included-model discovery/VPK/embedded archive/async/multisolid support; unsupported data and IO errors throw.','Owned assets/lifetimes are prepared but not integrated into editor/model registry/spatial partition; static-prop lighting guard remains.'])
    (run/'results.json').write_text(json.dumps(result,indent=2));print('PASS receipt '+str(run/'results.json'),flush=True)
if __name__=='__main__':main()
