"""Original live tree visitation vs file-owned AMD64 traversal, leaf-poststate replay only."""
import collections,hashlib,json,math,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/physics-tree-20261015'
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def f(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def parse_tree(raw):
    n=struct.unpack_from('<I',raw)[0];require(len(raw)==4+n*40,'Tree extent');return [struct.unpack_from('<10I',raw,4+i*40) for i in range(n)]
def parse_walk(raw):
    at=0
    while at<len(raw):
        require(at+52<=len(raw),'Traversal prefix');q=struct.unpack_from('<I',raw,at)[0];initial=raw[at+4:at+48];n=struct.unpack_from('<I',raw,at+48)[0];at+=52;require(at+n*92+44<=len(raw),'Traversal events');events=[]
        for i in range(n):events.append((struct.unpack_from('<I',raw,at)[0],raw[at+4:at+48],raw[at+48:at+92]));at+=92
        final=raw[at:at+44];at+=44;yield q,initial,events,final
def replay(tree,initial,events,final,counters):
    centers=[struct.unpack('<4f',struct.pack('<4I',*n[2:6])) for n in tree];state=initial;at=0;pending=[]
    def eligible(i):
        values=struct.unpack('<11f',state);mid=values[:3];u=values[6:9];length,box=values[9:];c=centers[i];d=[f(c[j]-mid[j]) for j in range(3)];r=f(c[3]+box)
        if length>0:d=[f(f(u[1]*d[2])-f(u[2]*d[1])),f(f(u[2]*d[0])-f(u[0]*d[2])),f(f(u[0]*d[1])-f(u[1]*d[0]))]
        return f(r*r)>f(f(f(d[0]*d[0])+f(d[1]*d[1]))+f(d[2]*d[2]))
    def distance(i):
        values=struct.unpack('<11f',state);d=[f(values[j+3]-centers[i][j]) for j in range(3)];return f(f(f(d[0]*d[0])+f(d[1]*d[1]))+f(d[2]*d[2]))
    if not eligible(0):require(not events and initial==final,'Root rejection mismatch');counters['root_rejected']+=1;return
    current=0
    while True:
        n=tree[current]
        if n[0]==0:
            require(at<len(events),'Missing leaf event');index,before,after=events[at];require(index==current and before==state,'Independent order/prestate mismatch');require(before[12:36]==after[12:36] and before[40:44]==after[40:44],'Fixed leaf fields changed');counters['leaf_visits']+=1;counters['leaf_updates']+=before!=after;state=after;at+=1
        else:
            left,right=n[7:9];a,b=eligible(left),eligible(right);counters['child_tests']+=2
            if a and b:
                dl,dr=distance(left),distance(right);counters['distance_ties']+=dl==dr;near,far=(right,left) if dr<=dl else (left,right);pending.append(far);counters['max_pending']=max(counters['max_pending'],len(pending));current=near;continue
            if a or b:current=left if a else right;continue
            counters['both_children_rejected']+=1
        if not pending:break
        current=pending.pop();counters['pending_pops']+=1;counters['pending_own_bounds_now_reject']+=not eligible(current)
    require(at==len(events) and state==final,'Independent final/count mismatch')
def main():
    prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Prior status');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libraries=run/'libraries';libraries.mkdir()
    for e in prior['libraries']:
        p=Path(e['source']);require(sha(p)==e['sha256'],'Original DLL changed');shutil.copyfile(p,libraries/p.name)
    programs=[('tree_capture_x86',0x14c),('tree_owned_x64',0x8664),('tree_safety_x64',0x8664),('regression/raw_native_x86',0x14c),('regression/raw_owned_x64',0x8664),('regression/geometry_legacy_owned_x64',0x8664)];executables=[]
    for name,machine in programs:
        p=OUT/(name+'.exe');raw=p.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Architecture/manifest');executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    def execute(name,args,log):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=45);log.write_text(p.stdout+p.stderr);require(p.returncode==0,'Probe failed '+str(log)+' '+p.stdout+p.stderr);return p.stdout
    safety=execute('tree_safety_x64',[],run/'safety.log');print(safety,flush=True)
    models=[];totals=collections.Counter(nodes=0,leaves=0,internal_nodes=0,traversals=0,queries=0);counters=collections.Counter()
    for index,m in enumerate(prior['models']):
        case=run/('%03d'%index);case.mkdir();queries=Path(m['path'])/'queries.bin';old=Path(m['path'])/'native-traces.bin';require(sha(Path(m['phy']))==m['phy_sha256'] and sha(queries)==m['query_sha256'] and sha(old)==m['trace_sha256'],'Prior input changed');trace=case/'traces.bin';capture=case/'captures.bin';nativeTree=case/'native-tree.bin';ownedTree=case/'owned-tree.bin';ownedWalk=case/'owned-walk.bin'
        execute('tree_capture_x86',[libraries,m['phy'],queries,trace,capture,nativeTree],case/'native.log');require(trace.read_bytes()==old.read_bytes(),'Tree observation changed original full trace')
        execute('tree_owned_x64',[m['phy'],capture,ownedTree,ownedWalk],case/'owned.log');require(nativeTree.read_bytes()==ownedTree.read_bytes(),'Native/file tree bytes or links differ');require(capture.read_bytes()==ownedWalk.read_bytes(),'Owned traversal replay differs');tree=parse_tree(nativeTree.read_bytes());seen=[];count=0;leafCount=sum(n[0]==0 for n in tree);require(sorted(n[9] for n in tree if n[0]==0)==list(range(leafCount)),'Ledge ordinals')
        for query,initial,events,final in parse_walk(capture.read_bytes()):require(query<m['queries'],'Query outside input');seen.append(query);replay(tree,initial,events,final,counters);count+=1
        require(seen==list(range(m['queries'])),'Expected one actual tree traversal per query');totals.update(nodes=len(tree),leaves=leafCount,internal_nodes=len(tree)-leafCount,traversals=count,queries=m['queries']);models.append(dict(model=m['model'],path=str(case),nodes=len(tree),leaves=leafCount,traversals=count,tree_sha256=sha(nativeTree),captures_sha256=sha(capture),unchanged_full_traces_sha256=sha(trace)));print('PASS original/file/owned',index+1,'nodes='+str(len(tree)),'traversals='+str(count),flush=True)
    # Invalid/nonfinite new tree fields: owned-only disposable PHY mutations.
    valid=Path(prior['models'][0]['phy']).read_bytes();compact=struct.unpack_from('<I',valid)[0]+32;root=compact+struct.unpack_from('<i',valid,compact+32)[0];invalid=run/'invalid';invalid.mkdir();invalidChecks=0
    for label,at,bits in [('nan_center',root+8,0x7fc12345),('infinite_radius',root+20,0x7f800000),('shared_child',root,28),('backwards_cycle',root,0xffffffe4)]:
        b=bytearray(valid);struct.pack_into('<I',b,at,bits);path=invalid/(label+'.phy');path.write_bytes(b);empty=invalid/'empty.bin';empty.write_bytes(b'');p=subprocess.run([str(OUT/'tree_owned_x64.exe'),str(path),str(empty),str(invalid/(label+'.tree')),str(invalid/(label+'.walk'))],capture_output=True,text=True,timeout=15);(invalid/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==1,'Invalid tree PHY accepted '+label);invalidChecks+=1
    # Fresh default/raw legacy regression, isolated from historical receipts.
    import verify_phy_raw_collision as regression
    regression.OUT=OUT/'regression';regression.main();regressionReceipts=sorted(regression.OUT.glob('verification-*/results.json'),key=lambda p:p.stat().st_mtime);regressionReceipt=regressionReceipts[-1];require(json.loads(regressionReceipt.read_text())['status']=='PASS','Reader regression status')
    from verify_ifm_draw_packet import check_ranges
    physics=next(Path(e['source']) for e in prior['libraries'] if Path(e['source']).name.lower()=='vphysics.dll');image,evidence=check_ranges(physics,'vphysics-disassembly.txt',[('original_complete_tree_traversal',0x1002ad00,0x1002b237),('original_complete_trace_tree_call',0x1002b453,0x1002b478)])
    for m in prior['models']:
        for field in ['phy','mdl']:require(sha(Path(m[field]))==m[field+'_sha256'],'Original asset changed')
    for e in prior['libraries']:require(sha(Path(e['source']))==e['sha256'] and sha(libraries/Path(e['source']).name)==e['sha256'],'Original/copy DLL changed')
    pinned={'bin/engine.dll':'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f','bin/tools/ifm.dll':'8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4','tf_movies/maps/mtt_soldier.bsp':'60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx':'5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee'};game=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
    for p,h in pinned.items():require(sha(game/p)==h,'Original benchmark/engine changed')
    sources=['source_phy_compact_geometry_candidate.h','source_physics_tree_traversal_candidate.h','physics_tree_traversal_test_fixture.h','probe_physics_tree_capture.cpp','probe_owned_phy_tree_traversal.cpp','test_physics_tree_traversal.cpp','build_physics_tree.cmd','verify_physics_tree.py','native_vphysics_library_fixture.h','physics_support_test_fixture.h']
    receipt=dict(status='PASS',models=models,totals=dict(totals),observed_traversal_statistics=dict(counters),differences=0,original_full_traces_all_unchanged=True,safety=safety.strip(),invalid_phy_checks=invalidChecks,reader_regression_receipt=str(regressionReceipt),reader_regression_sha256=sha(regressionReceipt),executables=executables,prior_receipt=str(PRIOR),prior_receipt_sha256=sha(PRIOR),libraries=prior['libraries'],library_copies=str(libraries),static_evidence=evidence,original_pinned_files=pinned,source_sha256={n:sha(ROOT/'work'/n) for n in sources},limits=['Actual original traversal and leaf functions run intact; only loaded-memory call bridges observe before/after states; no original disk writes','Owned traversal consumes owned file tree but REPLAYS original leaf poststates: this does NOT prove owned leaf hit updates/full tracer/renderer/editor integration','Independent Python traversal also replays observed poststates; it verifies pruning/order/state propagation, not those poststate calculations','Actual benchmark61 one-solid inputs only; deep pending stack/right ties and malformed trees additionally analytical owned tests, not synthetic original engine parity','Existing default/raw geometry newly recompiled and compared against actual original loaded ledges and prior converted meshes; original library/assets/session hashes unchanged','No editor build/UI changes, major dependencies, background helpers or overall project completion claim'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['status','totals','observed_traversal_statistics','differences','safety','invalid_phy_checks']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
