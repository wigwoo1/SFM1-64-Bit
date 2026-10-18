"""Actual original completed-load VCollide policy and current asset evidence.

No native async/load scheduler or live SFM filesystem claim. No original writes.
"""
import collections,hashlib,io,json,random,struct,subprocess,tempfile,zipfile
from pathlib import Path
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/static-prop-physics-resolution-20261016'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
DATACACHE=GAME/'bin/datacache.dll'
DATACACHE_SHA='2534c6e8cc5da07f33170f220983f8fd562d2b6fb3316d5d678c2c984d1f6e06'
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def main():
    require(sha(DATACACHE)==DATACACHE_SHA,'Datacache original changed');prior=json.loads(PRIOR.read_text());require(sha(GAME/'tf_movies/maps/mtt_soldier.bsp')==prior['map_sha256'] and sha(Path(prior['gameinfo']))==prior['gameinfo_sha256'],'Benchmark/mounts changed');OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    ranges=[('public_GetVCollide',0x1000ceb0,0x1000cec7),('GetVCollideEx',0x1000df90,0x1000e06f),('completion_prologue_and_dispatch',0x1000eb80,0x1000ec2d),('failed_completion_type2',0x1000eefb,0x1000ef0d),('failed_completion_return',0x1000f050,0x1000f084),('GetVirtualModel',0x10006450,0x1000649d),('cached_GetVirtualModelByHeader',0x1000e3d0,0x1000e40f),('cached_virtual_return',0x1000e4b1,0x1000e4bb),('cached_GetStudioHdr',0x1000f350,0x1000f38c),('cached_studio_return',0x1000f586,0x1000f58f),('begin_load_flags_and_group_fallback_static_only',0x1000dbe0,0x1000dcea)]
    image,evidence=check_ranges(DATACACHE,'datacache-disassembly.txt',ranges);(run/'original-code-evidence.json').write_text(json.dumps(evidence,indent=2));require([image.word(0x1002acbc+4*i) for i in [13,15,17,28,33]]==[0x1000f350,0x1000ceb0,0x10006450,0x1000e3d0,0x1000df90] and image.word(0x1000f084+8)==0x1000eefb,'Datacache vtable/dispatch identity')
    relative=Path('models/props_facemovie/farm_gate/farm_gate.mdl');mdl=next((Path(m)/relative for m in prior['declared_loose_mounts'] if (Path(m)/relative).is_file()),None);require(mdl is not None,'Missing benchmark farm gate MDL');md=mdl.read_bytes();require(md[:4]==b'IDST' and struct.unpack_from('<I',md,4)[0] in [48,49] and struct.unpack_from('<I',md,336)[0]==0,'Farm gate included-model state changed')
    missing=[];archives=[]
    for mount in prior['declared_loose_mounts']:
        p=Path(mount)/relative.with_suffix('.phy');require(not p.is_file(),'Farm gate loose physics now available: '+str(p));missing.append(str(p))
        root=Path(mount)
        if root.is_dir():archives.extend(str(p) for p in root.iterdir() if p.is_file() and p.suffix.lower()=='.vpk')
    require(not archives,'Mounted directory archives need new resolution work');bsp=GAME/'tf_movies/maps/mtt_soldier.bsp';b=bsp.read_bytes();offset,length,version,compression=struct.unpack_from('<4I',b,8+40*16);require(version==compression==0 and offset+length<=len(b),'Benchmark embedded archive extent');pak=b[offset:offset+length];z=zipfile.ZipFile(io.BytesIO(pak));inventory=[dict(name=e.filename,uncompressed_bytes=e.file_size,compressed_bytes=e.compress_size,compression=e.compress_type,crc32=hex(e.CRC)) for e in z.infolist()];require(not any(e['name'].lower().endswith('.phy') for e in inventory),'Embedded PHY needs resolution');(run/'asset-evidence.json').write_text(json.dumps(dict(mdl=str(mdl),mdl_sha256=sha(mdl),included_models=0,model_contents=struct.unpack_from('<I',md,332)[0],loose_phy_absent_paths=missing,root_vpk_files=archives,embedded_pak_sha256=hashlib.sha256(pak).hexdigest(),embedded_pak_entries=inventory,limitation='Declared loose paths and this BSP pak only. Live engine mounts/Steam caches/dynamic overrides UNVERIFIED; do not label farm_gate runtime cache absent.'),indent=2))
    executables=[]
    for name,machine in [('vcollide_ready_native_x86',0x14c),('vcollide_ready_owned_x64',0x8664),('vcollide_ready_safety_x64',0x8664)]:
        p=OUT/(name+'.exe');raw=p.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Executable identity/manifest');executables.append(dict(path=str(p),machine=hex(machine),sha256=sha(p)))
    rows=[];expected=bytearray();summary=collections.Counter()
    def add(handle,flags,disabled,completion,ids,groups,refs,category):
        require(flags&2 or completion or disabled or handle==0xffff,'Native fixture must not schedule IO');row=struct.pack('<HHIII9Q8H9I',handle,flags,disabled,completion,len(groups),*ids,*(groups+[0]*(8-len(groups))),*refs);rows.append(row);summary[category]+=1
        state=flags|(2 if completion else 0);obj=ids[0];counts=refs[:];answers=[]
        for _ in range(2):
            answer=0
            if not disabled and handle!=0xffff:
                if not(state&0x80):
                    state|=0x80
                    if not obj:
                        for h in groups[1:]:
                            if ids[h]:obj=ids[h];counts[h]=(counts[h]+1)&0xffffffff;summary['retains']+=1;break
                answer=obj
            answers.append(answer)
        if not disabled and handle!=0xffff:
            summary['returned_present' if answers[0] else 'returned_absent']+=1
        if completion:summary['failed_completions']+=1
        expected.extend(struct.pack('<IHHQ2Q9I',0 if completion else 2,state,0,obj,*answers,*counts))
    for flags in [0,1,2,0x80,0x82,0x4002,0xfffd,0xffff]:
        for completion in [0,1]:
            for disabled in [0,1]:
                for handle in [0,0xffff]:
                    if not(flags&2 or completion or disabled or handle==0xffff):continue
                    for present in [0,1]:
                        for groups in [[],[1],[1,2],[2,1],[1,2,3,4,5,6,7,8],[1,2,2,2]]:
                            ids=[0]*9;ids[0]=0xaabbccdd12345678 if present else 0;ids[2]=0x1122334412345678;ids[7]=0x9876543212345678;add(handle,flags,disabled,completion,ids,groups,[0xffffffff]*9,'directed')
    rng=random.Random(2026101625)
    for _ in range(8192):
        completion=rng.randrange(2);disabled=int(rng.randrange(4)==0);handle=0xffff if rng.randrange(8)==0 else 0;flags=rng.randrange(65536)
        if not(completion or disabled or handle==0xffff):flags|=2
        ids=[(rng.getrandbits(64)|1) if rng.randrange(3)==0 else 0 for _ in range(9)];require(len([x for x in ids if x])==len(set(x for x in ids if x)),'Duplicate semantic fixture ID');groups=[rng.randrange(1,9) for _ in range(rng.randrange(9))];refs=[rng.choice([0,1,0x7fffffff,0xffffffff,rng.randrange(65536)]) for _ in range(9)];add(handle,flags,disabled,completion,ids,groups,refs,'random')
    # Exact disk-header no-included-model failure scenario; explicit failed IO
    # event, NOT derived from the independent file inventory above.
    add(0,0x80,0,1,[0]*9,[],[0]*9,'no_include_failed_io')
    (run/'queries.bin').write_bytes(b''.join(rows));(run/'independent-expected.bin').write_bytes(expected)
    def execute(name,args,log):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=60);log.write_text(p.stdout+p.stderr);require(p.returncode==0,'Cache probe failed '+str(log)+' '+p.stdout+p.stderr);return p.stdout
    safety=execute('vcollide_ready_safety_x64',[],run/'safety.log');print(safety,flush=True)
    for name in ['vcollide_ready_native_x86','vcollide_ready_owned_x64']:
        output=run/(name+'.bin');message=execute(name,[DATACACHE,mdl,run/'queries.bin',output],run/(name+'.log'));actual=output.read_bytes()
        if actual!=expected:
            first=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)));(run/'difference.json').write_text(json.dumps(dict(program=name,byte=first,case=first//68,field=first%68,actual_length=len(actual),expected_length=len(expected)),indent=2));raise RuntimeError('Cache policy difference '+str(run/'difference.json'))
        print(message,flush=True)
    require(summary['retains']>100 and summary['returned_absent']>100 and summary['returned_present']>100 and summary['failed_completions']>100,'Coverage counts');execute('vcollide_ready_native_x86',[DATACACHE,mdl,run/'queries.bin',run/'native-repeat.bin'],run/'native-repeat.log');require((run/'native-repeat.bin').read_bytes()==expected,'Repeated native run changed')
    pins={str(DATACACHE):DATACACHE_SHA,str(mdl):hashlib.sha256(md).hexdigest(),str(bsp):prior['map_sha256'],prior['gameinfo']:prior['gameinfo_sha256']}
    for p,s in pins.items():require(sha(Path(p))==s,'Original source changed')
    sources={str(p.relative_to(ROOT)):sha(p) for p in [ROOT/'work'/name for name in ['source_studio_vcollide_ready_query_candidate.h','native_private_mdlcache_fixture.h','probe_studio_vcollide_ready.cpp','test_studio_vcollide_ready.cpp','build_studio_vcollide_ready.cmd','verify_studio_vcollide_ready.py','datacache-disassembly.txt']]}
    result=dict(status='PASS',asset_resolution_status='PARTIALLY VERIFIED',path=str(run),queries=len(rows),query_calls=2*len(rows),semantic_bytes_per_case=68,summary=dict(summary),safety=safety.strip(),query_sha256=sha(run/'queries.bin'),results_sha256=sha(run/'vcollide_ready_owned_x64.bin'),asset_evidence_sha256=sha(run/'asset-evidence.json'),executables=executables,original_hashes=pins,sources=sources,repeated_native_identical=True,limitations=['READY records only: uncompleted loads are rejected; native async/begin-load scheduling NOT implemented.','Original datacache public query, completed failure dispatch, cached studio/virtual-model paths execute intact in a private PE. Data/cvars/model/VM/object storage are controlled fixtures.','Controlled VM group lists use a private runtime MDL header with include count varied and pre-cached VM; this is not proof of a newly valid on-disk model. Original farm_gate MDL remains unchanged.','No actual vphysics shape load or live SFM filesystem mount initialization here.','Asset inventory cannot by itself establish live runtime absence; farm_gate cache resolution still pending.','Owned provider must own/synchronize records and retained collision objects; this policy alone is NOT a model registry or thread-safe resource manager.'])
    (run/'results.json').write_text(json.dumps(result,indent=2));print('PASS receipt '+str(run/'results.json'),flush=True)
if __name__=='__main__':main()
