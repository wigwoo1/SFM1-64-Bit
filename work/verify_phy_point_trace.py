"""Owned PHY-to-point trace composition against intact original TraceBox, no state replay."""
import collections,hashlib,json,math,random,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/phy-point-trace-20261016'
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def f(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def query(start,delta,origin=(0,0,0),angles=(0,0,0),mask=1):return struct.pack('<12fI',*start,*delta,*origin,*angles,mask)
def analytic(row,inside=False):
    values=struct.unpack('<12fI',row);start=[f(x+0.) for x in values[:3]];delta=values[3:6];fraction=0. if inside else 1.;b=bytearray(92);struct.pack_into('<6f',b,0,*start,*[f(f(delta[j]*fraction)+start[j]) for j in range(3)]);struct.pack_into('<f',b,44,fraction)
    if inside:struct.pack_into('<I',b,48,1);b[54:56]=bytes([1,1])
    return bytes(b)
def first_convex_centroid(mesh):
    require(struct.unpack_from('<H',mesh)[0]==1,'Analytic mesh solid');n=struct.unpack_from('<I',mesh,2)[0]
    if n!=1:return None
    triangles=struct.unpack_from('<I',mesh,10)[0];values=struct.unpack_from('<%df'%(triangles*9),mesh,14);points=set(tuple(values[i:i+3]) for i in range(0,len(values),3));require(len(points)>=4,'Convex point set');return tuple(f(sum(p[j] for p in points)/len(points)) for j in range(3))
def main():
    require(sha(PRIOR)=='f4dc6b746f359e76671f6204b83af931dc9de75e888632391e9bd7545df8fcce','Prior receipt changed');prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Prior status');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libraries=run/'libraries';libraries.mkdir()
    for e in prior['libraries']:
        p=Path(e['source']);require(sha(p)==e['sha256'],'Original DLL changed');shutil.copyfile(p,libraries/p.name)
    executables=[]
    for name,machine in [('point_trace_native_x86',0x14c),('point_trace_owned_x64',0x8664),('point_trace_safety_x64',0x8664)]:
        p=OUT/(name+'.exe');raw=p.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Architecture/manifest');executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    def execute(name,args,log):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=90);log.write_text(p.stdout+p.stderr);require(p.returncode==0,'Point trace probe failed '+str(log)+' '+p.stdout+p.stderr);return p.stdout
    safetyModel=next(m for m in prior['models'] if m['model']=='models/props_granary/grain_sack.mdl');safety=execute('point_trace_safety_x64',[safetyModel['phy']],run/'safety.log');print(safety,flush=True);rng=random.Random(2026101602);totals=collections.Counter();models=[];wrapProven=False;centroidQuirks=[]
    for index,m in enumerate(prior['models']):
        case=run/('%03d'%index);case.mkdir();originalQueries=Path(m['path'])/'queries.bin';oldTraces=Path(m['path'])/'native-traces.bin';mesh=Path(m['path'])/'native-mesh.bin';mdl=Path(m['mdl']);require(sha(Path(m['phy']))==m['phy_sha256'] and sha(mdl)==m['mdl_sha256'] and sha(originalQueries)==m['query_sha256'] and sha(oldTraces)==m['trace_sha256'] and sha(mesh)==m['mesh_sha256'],'Prior asset/query/trace changed');raw=mdl.read_bytes();lo=struct.unpack_from('<3f',raw,104);hi=struct.unpack_from('<3f',raw,116);center=[(a+b)*.5 for a,b in zip(lo,hi)];radius=max(64.,max(b-a for a,b in zip(lo,hi))*2);rows=[];independent={};centroidInside=[]
        for j in range(256):
            origin=[rng.uniform(-8192,8192) for _ in range(3)];angles=([0.,0.,0.] if j%4==0 else [0.,rng.choice([-180.,-90.,90.,180.,360.]),0.] if j%4==1 else [rng.uniform(-179,179) for _ in range(3)]);start=[origin[k]+center[k]+rng.uniform(-radius,radius) for k in range(3)];delta=[rng.uniform(-radius*2,radius*2) for _ in range(3)]
            if j%17==0:delta=[0.,-0.,0.]
            elif j%19==0:delta=[1e-7,-1e-7,0.]
            mask=[0,1,2,0xffffffff,0x4481][j%5];rows.append(query(start,delta,origin,angles,mask))
        for j in range(4):
            row=query([float(j),-0.,-float(j)],[float(j)*.25,0.,-0.],[-4,5,6],[15,37,-11],0);independent[len(rows)+m['queries']]=analytic(row);rows.append(row)
        row=query([1e6,1e6,1e6],[0,0,0],angles=[15,37,-11]);independent[len(rows)+m['queries']]=analytic(row);rows.append(row)
        centroid=first_convex_centroid(mesh.read_bytes())
        if centroid:
            for mask in [1,0xffffffff,2]:
                row=query(centroid,[0,0,0],mask=mask)
                if mask&1:centroidInside.append((len(rows)+m['queries'],row))
                else:independent[len(rows)+m['queries']]=analytic(row)
                rows.append(row)
        queries=case/'queries.bin';queries.write_bytes(originalQueries.read_bytes()+b''.join(rows));n=m['queries']+len(rows);outputs=[]
        for name,mode in [('point_trace_native_x86','fresh'),('point_trace_owned_x64','fresh'),('point_trace_owned_x64','reuse'),('point_trace_owned_x64','wrap')]:
            output=case/(name+'-'+mode+'.bin');execute(name,[libraries,m['phy'],queries,output,mode],case/(name+'-'+mode+'.log'));require(len(output.read_bytes())==n*92,'Point output size');outputs.append(output)
        native=outputs[0].read_bytes();nativeVisits=Path(str(outputs[0])+'.visits').read_bytes();require(len(nativeVisits)%8==0,'Native leaf sequence extent');require(Path(str(outputs[0])+'.trace60').read_bytes()[:m['queries']*60]==oldTraces.read_bytes(),'Original benchmark trace regression');require(all(struct.unpack_from('<II',nativeVisits,p)[0]<n for p in range(0,len(nativeVisits),8)),'Native visit query indices')
        for path in outputs[1:]:
            a=path.read_bytes();visits=Path(str(path)+'.visits').read_bytes()
            if a!=native or visits!=nativeVisits:
                diff=next((i for i,(x,y) in enumerate(zip(a,native)) if x!=y),None);detail=dict(path=str(path),first_byte=diff,query=diff//92 if diff is not None else None,field=diff%92 if diff is not None else None,visits_equal=visits==nativeVisits);(case/'difference.json').write_text(json.dumps(detail,indent=2));raise RuntimeError('Whole point trace mismatch '+str(case/'difference.json'))
        for j,want in independent.items():require(native[j*92:(j+1)*92]==want,'Independent mask0/outside analytic mismatch '+str(case)+' query='+str(j))
        for j,row in centroidInside:
            if native[j*92:(j+1)*92]!=analytic(row,inside=True):centroidQuirks.append(dict(model=m['model'],query=j,path=str(case),point=list(struct.unpack_from('<3f',row)),original_result=native[j*92:(j+1)*92].hex(),owned_matches_original=True))
        metrics=Path(str(outputs[2])+'.stats').read_bytes();wrapped=Path(str(outputs[3])+'.stats').read_bytes();require(len(metrics)==len(wrapped)==n*30,'Point stats extent');counts=[struct.unpack_from('<7IH',metrics,p) for p in range(0,len(metrics),30)];wrappedCounts=[struct.unpack_from('<7IH',wrapped,p) for p in range(0,len(wrapped),30)];info=collections.Counter(queries=n,benchmark_queries=m['queries'],additional_queries=len(rows),independent_analytic_queries=len(independent),leaf_visits=len(nativeVisits)//8)
        for x in counts:
            for j,name in enumerate(['filtered_leaves','search_misses','noncloser_hits','accepted_unshortened','accepted_shortened','packed_searches','walk_searches']):info[name]+=x[j]
        if info['walk_searches']:require(any(x[7]<65535 for x in wrappedCounts),'Wrapped raw cache never advanced');wrapProven=True
        for j in range(n):info['fraction_hits']+=struct.unpack_from('<f',native,j*92+44)[0]<1;info['start_solid']+=native[j*92+55]!=0
        totals.update(info);models.append(dict(model=m['model'],phy=m['phy'],phy_sha256=m['phy_sha256'],path=str(case),summary=dict(info),queries_sha256=sha(queries),native_semantic_trace_sha256=sha(outputs[0]),native_leaf_sequence_sha256=sha(Path(str(outputs[0])+'.visits'))));print('PASS complete native/owned',index+1,'queries='+str(n),'leaves='+str(info['leaf_visits']),flush=True)
    require(wrapProven,'No actual raw scratch wrap coverage');invalid=run/'invalid';invalid.mkdir();valid=Path(prior['models'][0]['phy']).read_bytes();badPhy=[('short_header',valid[:15]),('short_solid',valid[:48])];p=bytearray(valid);struct.pack_into('<I',p,8,0);badPhy.append(('zero_solids',bytes(p)));p=bytearray(valid);compact=struct.unpack_from('<I',p)[0]+32;node=compact+struct.unpack_from('<i',p,compact+32)[0];struct.pack_into('<I',p,node+20,0x7f800000);badPhy.append(('nonfinite_tree_radius',bytes(p)));queryPath=invalid/'valid-query.bin';queryPath.write_bytes(query([0,0,0],[1,0,0]));invalidChecks=0
    for label,data in badPhy:
        path=invalid/(label+'.phy');path.write_bytes(data);p=subprocess.run([str(OUT/'point_trace_owned_x64.exe'),str(libraries),str(path),str(queryPath),str(invalid/(label+'.bin')),'fresh'],capture_output=True,text=True,timeout=15);require(p.returncode==1,'Invalid owned PHY accepted '+label);invalidChecks+=1
    for label,data in [('empty',b''),('short',queryPath.read_bytes()[:-1]),('nan_start',struct.pack('<I',0x7fc12345)+queryPath.read_bytes()[4:]),('angle_domain',query([0,0,0],[1,0,0],angles=[36001,0,0]))]:
        path=invalid/(label+'-queries.bin');path.write_bytes(data);p=subprocess.run([str(OUT/'point_trace_owned_x64.exe'),str(libraries),prior['models'][0]['phy'],str(path),str(invalid/(label+'-query-out.bin')),'reuse'],capture_output=True,text=True,timeout=15);require(p.returncode==1,'Invalid owned point query accepted '+label);invalidChecks+=1
    from verify_ifm_draw_packet import check_ranges
    physics=next(Path(e['source']) for e in prior['libraries'] if Path(e['source']).name.lower()=='vphysics.dll');image,evidence=check_ranges(physics,'vphysics-disassembly.txt',[('complete_original_point_trace',0x1002b260,0x1002b561),('complete_ray_constructor',0x10027310,0x100273f0),('complete_search_constructor',0x100295f0,0x100296a6),('complete_support_bind',0x100294f0,0x10029559),('complete_cache_acquire',0x10029490,0x100294e3),('complete_default_contents',0x100273f0,0x100273f8)])
    for e in prior['libraries']:require(sha(Path(e['source']))==e['sha256'] and sha(libraries/Path(e['source']).name)==e['sha256'],'Original/copy library changed')
    for m in prior['models']:
        for field in ['phy','mdl']:require(sha(Path(m[field]))==m[field+'_sha256'],'Original asset changed')
    pins={'bin/engine.dll':'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f','bin/tools/ifm.dll':'8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4','tf_movies/maps/mtt_soldier.bsp':'60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx':'5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee'};game=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
    for name,h in pins.items():require(sha(game/name)==h,'Original engine/benchmark changed')
    sources=['source_phy_point_trace_candidate.h','probe_phy_point_trace.cpp','test_phy_point_trace.cpp','build_phy_point_trace.cmd','verify_phy_point_trace.py','source_phy_point_support_binding_candidate.h','source_physics_support_metadata_candidate.h','source_phy_compact_geometry_candidate.h','source_physics_ray_preparation_candidate.h','source_physics_leaf_candidate.h','source_physics_tree_traversal_candidate.h','source_physics_search_candidate.h','angle_basis_candidate.h','x64_fsin_cos.asm','native_vphysics_library_fixture.h','native_physics_search_fixture.h','physics_search_test_fixture.h']
    result=dict(status='PASS',totals=dict(totals),models=models,differences=0,full_semantic_trace_bytes=92,cache_modes=['fresh','reuse','wrap65535'],owned_mode_query_executions=totals['queries']*3,actual_raw_cache_wrap_verified=wrapProven,safety=safety.strip(),invalid_owned_fixture_checks=invalidChecks,original_convex_centroid_quirks=centroidQuirks,executables=executables,prior_receipt=str(PRIOR),prior_receipt_sha256=sha(PRIOR),original_benchmark_trace60_all_unchanged=True,libraries=prior['libraries'],library_copies=str(libraries),static_evidence=evidence,pinned_original_files=pins,source_sha256={n:sha(ROOT/'work'/n) for n in sources},limits=['Owned AMD64 input is only actual PHY bytes and52byte query; all metadata/ray/pose/support/search/tree/hit/finalization is owned; no captured contexts, leaf poststates, original DLL calls or pointer-bearing ABI','Complete normalized92byte result and entire leaf-ordinal sequence compared native/owned in three scratch lifetime modes','Original fullTraceBox/leaf routines execute intact from workspace copies; leaf observers only record visit ordinals, not results supplied to owned code','Available61 one-solid VPHY0100/type0/IVPS only, point rays and default contents1; no hull sweeps/custom contents/multisolid/other formats or18 missing loosePHY archive resolution proof','This is a standalone collision component, not BSP/staticprop caller/editor integration or completed native SFM workflow','Convex point centroids are retained as native/owned compatibility cases. Geometric inside expectation is recorded separately when original numerical search itself misses or differs; these do not weaken full original/owned bit comparisons','No performance claim from the four parallel safety calls; immutable component/separate scratch concurrency checked only','No original writes, editor build, rendering/animation/serialization changes, GUI/Blender/dependencies or whole-project completion percentage'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','totals','differences','cache_modes','owned_mode_query_executions','actual_raw_cache_wrap_verified','safety','invalid_owned_fixture_checks']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
