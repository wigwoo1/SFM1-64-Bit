"""Actual original static-prop getters + engine/physics vs owned retained registry.
Per-prop clipping only: no invented spatial order or full-scene trace claim.
"""
import collections,hashlib,json,math,random,shutil,struct,subprocess,tempfile
from pathlib import Path
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/static-prop-point-registry-20261016'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
PRIOR=ROOT/'outputs/studio-vcollide-read-20261016/verification-gr1jjsch/results.json'
DIRECT=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
ENGINE_SHA='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
f32=lambda v:struct.unpack('<f',struct.pack('<f',v))[0]
def main():
    prior=json.loads(PRIOR.read_text());direct=json.loads(DIRECT.read_text());require(prior['status']==direct['status']=='PASS' and sha(GAME/'bin/engine.dll')==ENGINE_SHA,'Original/prior identity');OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libs=run/'libraries';libs.mkdir()
    for e in prior['libraries']:
        require(sha(Path(e['source']))==e['sha256'],'Original DLL changed')
        if Path(e['source']).name in ['tier0.dll','vphysics.dll']:shutil.copyfile(e['source'],libs/Path(e['source']).name)
    for p,s in prior['original_hashes'].items():require(sha(Path(p))==s,'Original asset changed')
    image,evidence=check_ranges(GAME/'bin/engine.dll','engine-disassembly.txt',[('static_prop_ref_handle',0x101c4300,0x101c4304),('static_prop_getters',0x101c4470,0x101c44c5),('zero_flags',0x101c4bb0,0x101c4bb3),('collideable_handle_entity',0x101c4bd0,0x101c4bd4),('whole_clip',0x1016f300,0x1016f6a0)]);(run/'original-code-evidence.json').write_text(json.dumps(evidence,indent=2));require([image.word(0x1035f90c+i*4) for i in [0,7,8,9,11,12]]==[0x101c4bd0,0x101c4470,0x101c4480,0x101c4490,0x101c44c0,0x101c4bb0],'Static prop getter table changed')
    # Original full-load receipt justifies known absent caches ONLY for this
    # explicit mount scope; current files are pinned again above/below.
    mountFile=run/'mounts.txt';mountFile.write_text((Path(prior['path'])/'mounts.txt').read_text());bsp=GAME/'tf_movies/maps/mtt_soldier.bsp';b=bsp.read_bytes();g=struct.unpack_from('<I',b,568)[0]
    for i in range(struct.unpack_from('<I',b,g)[0]):
        t,_,_,off,length=struct.unpack_from('<IHHII',b,g+4+i*16)
        if t==0x73707270:break
    n=struct.unpack_from('<I',b,off)[0];names=[b[off+4+i*128:off+4+(i+1)*128].split(b'\0')[0].decode('ascii') for i in range(n)];p=off+4+n*128;p+=4+2*struct.unpack_from('<I',b,p)[0];count=struct.unpack_from('<I',b,p)[0];p+=4;props=[b[p+i*60:p+(i+1)*60] for i in range(count)];require((n,count)==(79,485),'Benchmark inventory');loaded={r['model']:r for r in prior['records']}
    executables=[]
    for name,machine in [('prop_registry_clip_native_x86',0x14c),('prop_registry_owned_x64',0x8664),('prop_registry_safety_x64',0x8664)]:
        exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Executable identity');executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine)))
    def execute(exe,args,log):
        r=subprocess.run([str(OUT/(exe+'.exe')),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=60);log.write_text(r.stdout+r.stderr);require(r.returncode==0,'Registry probe failed '+str(log)+' '+r.stdout+r.stderr);return r.stdout
    safety=execute('prop_registry_safety_x64',[bsp,mountFile],run/'safety.log');print(safety,flush=True)
    groups=[[] for _ in names];ownedRows=[];bindings=bytearray();rng=random.Random(2026101629);world=0x12345678
    def entity(i):return ((i+1)<<32)|0xdecafbad
    def model(i):return ((i+1)<<32)|0x12345678
    def surface(i):return (i*1009)&0xffff
    # World-space sampling only. No reference tracing or traversal state enters
    # the queries; Euler math below chooses samples, not expected trace values.
    def transform(point,origin,angles):
        pitch,yaw,roll=[math.radians(v) for v in angles];sp,cp=math.sin(pitch),math.cos(pitch);sy,cy=math.sin(yaw),math.cos(yaw);sr,cr=math.sin(roll),math.cos(roll);cols=[(cp*cy,cp*sy,-sp),(sr*sp*cy-cr*sy,sr*sp*sy+cr*cy,sr*cp),(cr*sp*cy+sr*sy,cr*sp*sy-sr*cy,cr*cp)];return [f32(origin[j]+sum(point[k]*cols[k][j] for k in range(3))) for j in range(3)]
    def add(i,m,r,start,end,mask):
        start=list(map(f32,start));delta=[f32(end[j]-start[j]) for j in range(3)];index=len(ownedRows);ownedRows.append(struct.pack('<Q6fIQ',entity(i),*start,*delta,mask,world));origin=struct.unpack_from('<3f',r);angles=struct.unpack_from('<3f',r,12);cache=0 if loaded[names[m]]['phy'] else 1;native=struct.pack('<12f5IQ',*start,*delta,*origin,*angles,mask,r[30]<<16,cache,0x40000000|i,surface(m),world);require(len(native)==76,'Native wire extent');groups[m].append((index,native))
    for i,r in enumerate(props):
        m=struct.unpack_from('<H',r,24)[0];md=Path(loaded[names[m]]['mdl']).read_bytes();lo=struct.unpack_from('<3f',md,104);hi=struct.unpack_from('<3f',md,116);center=[(a+b)*.5 for a,b in zip(lo,hi)];radius=max(64.,max(b-a for a,b in zip(lo,hi))*2);origin=struct.unpack_from('<3f',r);angles=struct.unpack_from('<3f',r,12)
        bindings.extend(struct.pack('<QQIII',entity(i),model(m),0x40000000|i,r[30],0)+r[:12]+(r[12:24] if r[30]==6 else bytes(12))+struct.pack('<III',int(bool(loaded[names[m]]['phy'])),int(not loaded[names[m]]['phy']),surface(m)))
        k=0
        for axis in range(3):
            for sign in [-1,1]:
                for offset in [-.6*radius,0,.6*radius]:
                    a=center[:];z=center[:];a[axis]-=sign*radius;z[axis]+=sign*radius;a[(axis+1)%3]+=offset;z[(axis+1)%3]+=offset;add(i,m,r,transform(a,origin,angles),transform(z,origin,angles),[0,1,0x4481,0xbfffffff,2,0x80000000][k%6]);k+=1
        for point in [center,[0,0,0],lo,hi]:a=transform(point,origin,angles);add(i,m,r,a,a,1)
        for _ in range(16):
            a=[rng.uniform(center[j]-radius,center[j]+radius) for j in range(3)];z=[rng.uniform(center[j]-radius,center[j]+radius) for j in range(3)];add(i,m,r,transform(a,origin,angles),transform(z,origin,angles),1)
    queries=run/'queries.bin';queries.write_bytes(b''.join(ownedRows));expected=bytearray(len(ownedRows)*92);expectedEvents=bytearray(len(ownedRows)*8);records=[];dummy=direct['models'][0]['phy']
    for m,rows in enumerate(groups):
        require(rows,'Unused benchmark dictionary model');case=run/('%03d'%m);case.mkdir();nativeq=case/'queries.bin';nativeq.write_bytes(b''.join(row for _,row in rows));output=case/'native.bin';asset=loaded[names[m]];execute('prop_registry_clip_native_x86',[GAME/'bin/engine.dll',libs,asset['phy'] or dummy,asset['mdl'],nativeq,output],case/'native.log');raw=output.read_bytes();events=Path(str(output)+'.events').read_bytes();require(len(raw)==len(rows)*92 and len(events)==len(rows)*8,'Native trace extent')
        for j,(index,_) in enumerate(rows):expected[index*92:(index+1)*92]=raw[j*92:(j+1)*92];expectedEvents[index*8:(index+1)*8]=events[j*8:(j+1)*8]
        records.append(dict(model=names[m],queries=len(rows),path=str(case),trace_sha256=sha(output),known_absent_cache=asset['phy'] is None))
        if (m+1)%10==0:print('PASS original static-prop getters + engine/physics',m+1,flush=True)
    (run/'native-ordered.bin').write_bytes(expected);(run/'native-ordered.events').write_bytes(expectedEvents);owned=run/'owned.bin';print(execute('prop_registry_owned_x64',[bsp,mountFile,queries,owned],run/'owned.log'),flush=True)
    for path,reference,label in [(owned,expected,'trace'),(Path(str(owned)+'.events'),expectedEvents,'events'),(Path(str(owned)+'.bindings'),bindings,'bindings')]:
        actual=path.read_bytes()
        if actual!=reference:
            first=next((i for i,(a,b) in enumerate(zip(actual,reference)) if a!=b),min(len(actual),len(reference)));(run/'difference.json').write_text(json.dumps(dict(kind=label,byte=first,case=first//(92 if label=='trace' else 8 if label=='events' else 64),field=first%(92 if label=='trace' else 8 if label=='events' else 64)),indent=2));raise RuntimeError('Registry '+label+' mismatch '+str(run/'difference.json'))
    totals=collections.Counter();hitIndex=None
    for i in range(len(ownedRows)):
        dispatch,hit=struct.unpack_from('<2I',expectedEvents,i*8);totals['physics_dispatches']+=dispatch;totals['hits']+=hit
        prop=i//38;solid=props[prop][30];totals['solid'+str(solid)+'_queries']+=1
        if solid==0:require(dispatch==hit==0,'Non-solid prop dispatched or hit')
        if hit:hitIndex=i
    require(totals['hits']>100,'Insufficient real collision hits');require(hitIndex is not None,'Need hit for wide world identity check');wide=0x88776655decafbad;row=bytearray(ownedRows[hitIndex]);struct.pack_into('<Q',row,36,wide);wideQuery=run/'wide-world-query.bin';wideQuery.write_bytes(row);wideOut=run/'wide-world.bin';execute('prop_registry_owned_x64',[bsp,mountFile,wideQuery,wideOut],run/'wide-world.log');wideExpected=bytearray(expected[hitIndex*92:(hitIndex+1)*92]);struct.pack_into('<Q',wideExpected,80,wide);require(wideOut.read_bytes()==wideExpected,'World identity truncation or trace changed')
    for p,s in prior['original_hashes'].items():require(sha(Path(p))==s,'Original asset changed after queries')
    require(sha(GAME/'bin/engine.dll')==ENGINE_SHA,'Original engine changed');sources={str(Path('work')/p):sha(ROOT/'work'/p) for p in ['source_static_prop_point_registry_candidate.h','static_prop_registry_test_fixture.h','probe_static_prop_point_registry.cpp','probe_static_prop_registry_native.cpp','test_static_prop_point_registry.cpp','build_static_prop_point_registry.cmd','verify_static_prop_point_registry.py']}
    result=dict(status='PASS',path=str(run),props=485,models=79,queries=len(ownedRows),totals=dict(totals),trace_differences=0,binding_differences=0,query_sha256=sha(queries),trace_sha256=sha(owned),binding_sha256=sha(Path(str(owned)+'.bindings')),safety=safety.strip(),wide_world_identity=hex(wide),wide_world_source_case=hitIndex,executables=executables,records=records,sources=sources,asset_scope_receipt=str(PRIOR),asset_scope_receipt_sha256=sha(PRIOR),original_engine_sha256=ENGINE_SHA,limitations=['Per-prop clipping only; no spatial enumeration order, world merge, actual scene adapter or editor integration.','Original static-prop getter vtables, full engine ClipRayToCollideable, engine contents callback/client annotation and actual physics execute. Model/header/cache/property/world services remain explicit fixtures.','Non-solid props use actual original solid0 getters; solid6 uses loaded physics. Absent cache supplied only after prior actual full-load proof for these configured providers.','A valid ignored dummy PHY is loaded by the native harness for known-absent models; it is never returned to engine clipping and is not an asset fallback.','All fixture surface properties are deliberately supplied runtime values, not a material-property registry. Unknown properties throw for solid6.','Owned registry retains shared immutable assets and explicit caller64bit identities. IDs need scene/generation ownership; general archive/include/async/multisolid/solid2 tracing remains unsupported.'])
    (run/'results.json').write_text(json.dumps(result,indent=2));print('PASS receipt '+str(run/'results.json'),flush=True)
if __name__=='__main__':main()
