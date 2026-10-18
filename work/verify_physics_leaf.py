"""Complete original2A340 vs owned search/hit policy with reference-prepared supports."""
import collections,hashlib,json,math,random,shutil,struct,subprocess,tempfile
from pathlib import Path
from verify_physics_search import expected as searchExpected,make_row as searchRow,take,frame
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/physics-leaf-20261016'
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def f(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def read_input(raw,at):
    start=at;require(at+368<=len(raw),'Leaf prefix');prepared=struct.unpack_from('<I',raw,at+364)[0];require(prepared in [0,1],'Leaf prepared flag');at+=368
    if prepared:_,at=take(raw,at)
    return raw[start:at],at
def hit_expected(state,contents):
    s=bytearray(state);code=3;nearest=struct.unpack_from('<f',s,320)[0];distance=struct.unpack_from('<f',s,104)[0]
    if nearest>distance:
        code=4;s[132:144]=s[24:36];s[162:164]=s[54:56];s[320:324]=s[104:108];inverse=struct.unpack_from('<f',s,256)[0];tolerance=struct.unpack_from('<f',s,324)[0];struct.pack_into('<f',s,152,f(inverse*distance));gain=f(f(f(tolerance*2)+distance)*inverse)
        if 1>gain:
            code=5;maximum=struct.unpack_from('<f',s,252)[0];length=f(maximum*gain);struct.pack_into('<f',s,248,length);starts=struct.unpack_from('<3f',s,200);delta=struct.unpack_from('<3f',s,224);struct.pack_into('<3f',s,212,*[f(starts[j]+f(delta[j]*gain)) for j in range(3)]);struct.pack_into('<I',s,260,0);struct.pack_into('<f',s,300,f(length*f(.0254)));half=f(gain*.5);localDelta=struct.unpack_from('<3f',s,308);localStart=struct.unpack_from('<3f',s,276);struct.pack_into('<3f',s,264,*[f(f(localDelta[j]*half)+localStart[j]) for j in range(3)])
        struct.pack_into('<I',s,156,contents)
    return bytes(s),code
def expected(row):
    s=bytearray(row[:328]);contents,mask,prepared=struct.unpack_from('<3I',row,356);cache=b'';stats=(0,)*6
    if not contents&mask:return bytes(s)+struct.pack('<I',1),stats
    require(prepared==1,'Missing prepared support');wire,end=take(row,368);require(end==len(row),'Leaf input extent');search=row[200:264]+row[328:356]+frame(wire)+row[:108]+row[324:328];output,stats=searchExpected(search);s[:108]=output[:108];answer=struct.unpack_from('<I',output,108)[0];cache=output[112:];code=2
    if answer:s,code=hit_expected(s,contents)
    return bytes(s)+struct.pack('<I',code)+cache,stats
def controlled_row(mode,j,rng):
    search=searchRow(mode,j,rng);wire,stateAt=take(search,92);searchState=search[stateAt:stateAt+108];trace=searchState[:92];ray=search[:64];tree=struct.pack('<11f',*[rng.randint(-4,4)*.25 for _ in range(11)]);local=struct.pack('<3f',*[rng.randint(-8,8)*.25 for _ in range(3)]);nearest=rng.choice([-1.,0.,.25,1.,5.,100.,1e24]);tolerance=search[stateAt+108:stateAt+112];contents=rng.choice([1,2,4,7,0x80000000,0xffffffff]);mask=rng.choice([0,1,2,4,7,0x80000000,0xffffffff]);prepared=bool(contents&mask);return searchState+trace+ray+tree+local+struct.pack('<f',nearest)+tolerance+search[64:92]+struct.pack('<3I',contents,mask,prepared)+(frame(wire) if prepared else b'')
def main():
    require(sha(PRIOR)=='f4dc6b746f359e76671f6204b83af931dc9de75e888632391e9bd7545df8fcce','Prior receipt changed');prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Prior status');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libraries=run/'libraries';libraries.mkdir()
    for e in prior['libraries']:
        p=Path(e['source']);require(sha(p)==e['sha256'],'Original DLL changed');shutil.copyfile(p,libraries/p.name)
    executables=[]
    for name,machine in [('leaf_capture_x86',0x14c),('leaf_owned_x64',0x8664),('leaf_safety_x64',0x8664),('leaf_hit_native_x86',0x14c),('leaf_hit_owned_x64',0x8664)]:
        p=OUT/(name+'.exe');raw=p.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Architecture/manifest');executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    def execute(name,args,log):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=120);log.write_text(p.stdout+p.stderr);require(p.returncode==0,'Probe failed '+str(log)+' '+p.stdout+p.stderr);return p.stdout
    safety=execute('leaf_safety_x64',[],run/'safety.log');print(safety,flush=True);actual=[];observed=[];timings=[];models=[];modeCounts=collections.Counter()
    for index,m in enumerate(prior['models']):
        case=run/('%03d'%index);case.mkdir();queries=Path(m['path'])/'queries.bin';old=Path(m['path'])/'native-traces.bin';require(sha(Path(m['phy']))==m['phy_sha256'] and sha(queries)==m['query_sha256'] and sha(old)==m['trace_sha256'],'Prior asset/query/trace changed');trace=case/'traces.bin';capture=case/'captures.bin';execute('leaf_capture_x86',[libraries,m['phy'],queries,trace,capture],case/'capture.log');require(trace.read_bytes()==old.read_bytes(),'Observer changed original full trace');raw=capture.read_bytes();at=0;count=0
        while at<len(raw):
            require(at+8<=len(raw),'Leaf query prefix');query,gameData=struct.unpack_from('<2I',raw,at);require(query<m['queries'],'Leaf query outside input');row,at=read_input(raw,at+8);answer,at=take(raw,at);require(at+4<=len(raw),'Leaf timing missing');timing=struct.unpack_from('<I',raw,at)[0];at+=4;actual.append(row);observed.append(answer);timings.append(timing);count+=1
            if struct.unpack_from('<I',row,364)[0]:wire,_=take(row,368);modeCounts[struct.unpack_from('<I',wire)[0]]+=1
        models.append(dict(model=m['model'],path=str(case),leaf_calls=count,captures_sha256=sha(capture),unchanged_full_traces_sha256=sha(trace)));print('PASS original leaf',index+1,'calls='+str(count),flush=True)
    rng=random.Random(0x2a340);rows=[]
    for mode,amount in [(2,1536),(1,128),(0,128)]:
        for j in range(amount):rows.append(controlled_row(mode,j,rng))
    controlled=len(rows);rows+=actual;inputPath=run/'inputs.bin';inputPath.write_bytes(b''.join(rows));reference=[expected(row) for row in rows];expectedPath=run/'expected.bin';expectedPath.write_bytes(b''.join(frame(r[0]) for r in reference));outputPath=run/'owned.bin';ownedLog=execute('leaf_owned_x64',[inputPath,outputPath],run/'owned.log');raw=outputPath.read_bytes();at=0;answers=[]
    while at<len(raw):answer,at=take(raw,at);answers.append(answer)
    metrics=Path(str(outputPath)+'.stats').read_bytes();require(len(metrics)==len(rows)*24 and len(answers)==len(rows),'Owned output counts');stats=[struct.unpack_from('<6I',metrics,i*24) for i in range(len(rows))];differences=[]
    for i,(row,(want,metric)) in enumerate(zip(rows,reference)):
        if answers[i]!=want or stats[i]!=metric or (i>=controlled and (answers[i]!=observed[i-controlled] or metric[4]!=timings[i-controlled])):
            differences.append(dict(case=i,captured=i>=controlled,input=row.hex(),owned=answers[i].hex(),expected=want.hex(),observed=observed[i-controlled].hex() if i>=controlled else None,owned_stats=stats[i],expected_stats=metric))
            if len(differences)==5:break
    (run/'comparison.json').write_text(json.dumps(dict(differences=differences),indent=2)+'\n');require(not differences,'Leaf comparison mismatch '+str(run/'comparison.json'))
    # Execute the intact original post-search policy directly for both sides
    # of the gain1 boundary. These are helper calls, not whole leaf executions.
    hitRows=[];hitReference=[];hitOutcomes=collections.Counter();rng=random.Random(0x2a389)
    for j in range(8192):
        state=bytearray(rows[j%controlled][:328]);distance=rng.choice([-8.,-2.,-0.,0.,.03125,.5,1.,2.,8.,100.,1e24]);nearest=rng.choice([-8.,-2.,0.,.5,1.,2.,8.,100.,1e24]);inverse=rng.choice([0.,.125,.25,.5,1.,-.25]);tolerance=rng.choice([0.,.03125,.5,1.,-.5]);maximum=rng.choice([0.,1.,2.,8.,100.]);struct.pack_into('<3f',state,24,*[rng.randrange(-8,9)*.25 for _ in range(3)]);struct.pack_into('<f',state,104,distance);struct.pack_into('<f',state,320,nearest);struct.pack_into('<f',state,324,tolerance);struct.pack_into('<f',state,256,inverse);struct.pack_into('<f',state,252,maximum);contents=rng.getrandbits(32)
        if j<8:
            struct.pack_into('<f',state,104,2.);struct.pack_into('<f',state,320,10.);struct.pack_into('<f',state,324,0.);struct.pack_into('<I',state,256,[0x3effffff,0x3f000000,0x3f000001,0,0x80000000,0x3e800000,0x3f800000,0xbe800000][j])
        answer,code=hit_expected(state,contents);hitRows.append(bytes(state)+struct.pack('<I',contents));hitReference.append(answer);hitOutcomes[code]+=1
    hitInput=run/'hit-inputs.bin';hitInput.write_bytes(b''.join(hitRows));hitExpected=run/'hit-expected.bin';hitExpected.write_bytes(b''.join(hitReference))
    for name in ['leaf_hit_native_x86','leaf_hit_owned_x64']:
        path=run/(name+'.bin');execute(name,[libraries,hitInput,path],run/(name+'.log'));require(path.read_bytes()==hitExpected.read_bytes(),'Direct original/owned/independent hit mismatch '+name)
    invalid=run/'invalid';invalid.mkdir();bad=[('empty',b''),('short_state',rows[0][:327]),('short_support',next(x for x in rows if struct.unpack_from('<I',x,364)[0])[:-1])]
    for label,offset,value in [('bad_box_flag',352,2),('bad_prepared_flag',364,2),('missing_context',364,0)]:
        row=bytearray(next(x for x in rows if struct.unpack_from('<I',x,364)[0]));struct.pack_into('<I',row,offset,value);bad.append((label,bytes(row)))
    row=next(x for x in rows if struct.unpack_from('<I',x,364)[0])
    for label,offset in [('nan_direction',92),('nan_tolerance',324)]:
        b=bytearray(row);struct.pack_into('<I',b,offset,0x7fc12345);bad.append((label,bytes(b)))
    for label,data in bad:
        path=invalid/(label+'.bin');path.write_bytes(data);p=subprocess.run([str(OUT/'leaf_owned_x64.exe'),str(path),str(invalid/(label+'.out'))],capture_output=True,text=True,timeout=15);(invalid/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==1,'Invalid leaf accepted '+label)
    from verify_ifm_draw_packet import check_ranges
    physics=next(Path(e['source']) for e in prior['libraries'] if Path(e['source']).name.lower()=='vphysics.dll');image,evidence=check_ranges(physics,'vphysics-disassembly.txt',[('complete_original_leaf',0x1002a340,0x1002a4e6),('complete_original_binding',0x100294f0,0x10029559),('complete_default_contents',0x100273f0,0x100273f8)])
    constants={hex(a):hex(image.word(a)) for a in [0x100c54b8,0x100c53d0,0x100c53f0,0x100ee008]};require(list(constants.values())==['0x40000000','0x3f800000','0x3f000000','0x3cd013a9'],'Hit policy constants')
    for e in prior['libraries']:require(sha(Path(e['source']))==e['sha256'] and sha(libraries/Path(e['source']).name)==e['sha256'],'Original/copy DLL changed')
    for m in prior['models']:
        for field in ['phy','mdl']:require(sha(Path(m[field]))==m[field+'_sha256'],'Original asset changed')
    pins={'bin/engine.dll':'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f','bin/tools/ifm.dll':'8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4','tf_movies/maps/mtt_soldier.bsp':'60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx':'5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee'};game=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
    for name,h in pins.items():require(sha(game/name)==h,'Original engine/benchmark changed')
    sources=['source_physics_leaf_candidate.h','physics_leaf_test_fixture.h','probe_physics_leaf_capture.cpp','probe_physics_leaf.cpp','probe_physics_leaf_hit.cpp','test_physics_leaf.cpp','build_physics_leaf.cmd','verify_physics_leaf.py','source_physics_search_candidate.h','source_physics_tree_traversal_candidate.h','native_physics_search_fixture.h','native_physics_refinement_fixture.h','native_physics_support_fixture.h','native_vphysics_library_fixture.h','physics_search_test_fixture.h','physics_support_test_fixture.h','verify_physics_search.py']
    result=dict(status='PASS',cases=len(rows),controlled_owned_independent_cases=controlled,actual_complete_original_leaf_calls=len(actual),direct_native_owned_independent_hit_cases=len(hitRows),direct_hit_outcomes=dict(hitOutcomes),actual_prepared_support_modes=dict(modeCounts),differences=0,owned_independent_outcomes=dict(collections.Counter(struct.unpack_from('<I',r[0],328)[0] for r in reference)),actual_outcomes=dict(collections.Counter(struct.unpack_from('<I',r[0],328)[0] for r in reference[controlled:])),actual_native_search_timing_calls=sum(timings),safety=safety.strip(),invalid_owned_fixture_checks=len(bad),original_trace_cases=prior['totals']['traces'],all_original_trace_bytes_unchanged=True,executables=executables,models=models,prior_receipt=str(PRIOR),prior_receipt_sha256=sha(PRIOR),libraries=prior['libraries'],library_copies=str(libraries),static_evidence=evidence,constants=constants,source_sha256={n:sha(ROOT/'work'/n) for n in sources},pinned_original_files=pins,input_sha256=sha(inputPath),output_sha256=sha(outputPath),limits=['Actual whole2A340/294F0/296B0 run intact through original TraceBox; observers only bridge loaded-memory calls using workspace copied original DLLs','Actual point-ray leaf states compare entire328-byte pointer-free normalized search/external trace/ray/local state plus disposition and2050-byte edge-walk cache when used','Owned candidate composes existing owned search with new hit policy; input support contexts/cache are captured AFTER original294F0 binding. Owned file-to-leaf binding/cache lifetime and full tracer/editor integration still not proven','Direct hit helper calls execute original2A389..2A4E6 with matching test-only register/stack frame; all helper328byte outputs compare native/owned/independent including gain>=1 branch; these are not whole leaf calls','Controlled extra cases compare owned against independent Python composed reference; these are NOT direct complete-original leaf executions','Semantic external trace preserves64bit ids, opaque fields; native pointers only normalized in test adapter, never dereferenced as application ids','No original writes, editor build/integration, UI/Blender, dependencies or whole-project completion claim'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','controlled_owned_independent_cases','actual_complete_original_leaf_calls','actual_prepared_support_modes','direct_native_owned_independent_hit_cases','direct_hit_outcomes','differences','actual_outcomes','owned_independent_outcomes','safety','invalid_owned_fixture_checks']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
