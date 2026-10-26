"""Owned x64 primary material-tree primitive versus unchanged original code.

No fake material objects: the reference executes the generic tree primitives
with opaque payload IDs. This is a container gate, not material construction.
"""
from pathlib import Path
from datetime import datetime,timezone
from collections import Counter
import hashlib,json,random,re,shutil,struct,subprocess,tempfile,time,zlib
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_material_keyvalues_parser import DUMP
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges

OUT=ROOT/'outputs/material-dictionary-tree-20261026'
NEW=['material_dictionary_tree_candidate.h','probe_material_dictionary_tree.cpp','build_material_dictionary_tree.cmd','verify_material_dictionary_tree.py']


def corpus():
    q=[];labels=[]
    def add(label,*values):q.append(values);labels.append(label)
    rng=random.Random(6401)
    for i in range(100):add('preserved-discovery',1,i+1,rng.randrange(12),rng.randrange(2))
    ids=list(range(1,101));rng.shuffle(ids)
    for i in ids:
        add('preserved-discovery',2,i,0,0);add('preserved-discovery',3,0,rng.randrange(12),rng.randrange(2))
    for scenario in range(5):
        label=['forward','reverse','shuffled','equal-keys','symbol-boundaries'][scenario];add(label,4,0,0,0)
        rows=list(range(300))
        if scenario==1:rows.reverse()
        if scenario==2:rng.shuffle(rows)
        for i in rows:
            key=(i if scenario<3 else 7 if scenario==3 else [0,65535,32768,32767,1,65534][i%6]);flag=i%2
            add(label,1,1000+i,key,flag);add(label,3,0,key,flag)
        rng.shuffle(rows) if scenario>=2 else None
        for i in rows:add(label,2,1000+i,0,0)
        # Absent removals and retained capacity after completely emptying.
        add(label,2,999999,0,0);add(label,3,0,65535,1)
    add('mixed',4,0,0,0);active=[];next_id=10000
    for i in range(2400):
        if i in [500,1400,2000]:add('mixed',4,0,0,0);active.clear()
        choice=rng.random()
        if choice<0.5:
            identity=rng.choice(active) if active and rng.random()<0.2 else next_id
            if identity==next_id:next_id+=1
            key=rng.choice([0,1,2,32767,32768,65534,65535]) if i%3 else rng.randrange(65536)
            flag=rng.randrange(2);add('mixed',1,identity,key,flag);active.append(identity)
        elif choice<0.82:
            identity=rng.choice(active) if active and rng.random()<0.9 else 999999
            add('mixed',2,identity,0,0)
            if identity in active:active.remove(identity)
        else:add('mixed',3,0,rng.choice([0,1,2,32767,32768,65534,65535]),rng.randrange(2))
    add('final-purge',4,0,0,0)
    return q,labels


def decode(raw):
    at=0;records=[]
    while at<len(raw):
        require(at+36<=len(raw),'State header extent');head=struct.unpack_from('<9I',raw,at);at+=36;n=head[6]
        require(n<=65000 and at+n*28<=len(raw),'State node extent');nodes=[struct.unpack_from('<7I',raw,at+28*i) for i in range(n)];at+=n*28
        records.append((head,nodes))
    return records


def independent(q,records):
    checks=0;model=Counter();capacity=256;slots=0;free=[]
    def check(b,msg):
        nonlocal checks
        require(b,msg);checks+=1
    for index,(query,(h,nodes)) in enumerate(zip(q,records)):
        op,payload,symbol,flag=query;result,root,count,freehead,n,cap=h[2:8]
        if op==1:
            expected=free.pop() if free else slots
            if expected==slots:
                slots+=1
                if slots>capacity:capacity=capacity*2 if capacity else 2
            check(result==expected,'Exact fresh/free slot allocation '+str(index));model[(payload,symbol,flag)]+=1
        elif op==2:
            matching=[key for key,v in model.items() if v and key[0]==payload]
            check((result!=65535)==bool(matching),'Absent removal result '+str(index))
            if result!=65535:
                removed=tuple(nodes[result][4:7]);check(removed in matching,'Erased matching opaque payload '+str(index));model[removed]-=1;free.append(result)
        elif op==3:
            matching=any(v and key[1:]==(symbol,flag) for key,v in model.items())
            check((result!=65535)==matching,'Find membership '+str(index))
            if result!=65535:check(nodes[result][5:7]==(symbol,flag),'Find exact source category/key '+str(index))
        else:model.clear();slots=capacity=0;free.clear();check(result==65535,'Purge result')
        check(h[:2]==(index,op) and count==sum(model.values()) and n==slots and cap==capacity,'Exact model/header/count/capacity '+str(index))
        check(freehead==(free[-1] if free else 65535),'Exact free-list head '+str(index))
        live={i for i,row in enumerate(nodes) if row[0]!=i}
        check(Counter(tuple(nodes[i][4:7]) for i in live)==+model,'Whole live payload multiset '+str(index))
        seen=set();todo=[(root,65535)] if root!=65535 else [];ordered=[]
        while todo:
            node,parent=todo.pop();check(node in live and node not in seen,'Acyclic live child index');seen.add(node);row=nodes[node];check(row[2]==parent and row[3]<=1,'Parent/color domain')
            for child in row[:2]:
                if child!=65535:todo.append((child,node))
        check(seen==live and len(live)==count,'Exact reachable live nodes')
        stack=[];current=root
        while stack or current!=65535:
            while current!=65535:stack.append(current);current=nodes[current][0]
            current=stack.pop();ordered.append((1-nodes[current][6],nodes[current][5]));current=nodes[current][1]
        check(ordered==sorted(ordered),'Source-category/symbol ordering')
        seen_free=set();current=freehead
        while current!=65535:
            check(current<len(nodes) and current not in seen_free and current not in live,'Free-list partition');seen_free.add(current);current=nodes[current][1]
        check(live|seen_free==set(range(n)),'All allocated slots accounted')
    return checks


def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    ptr=json.loads((ROOT/'outputs/queued-material-lifetime-20261026/latest-verification.json').read_text());receipt=Path(ptr['receipt']);require(sha(receipt)==ptr['receipt_sha256'],'Prior queued receipt');prior=json.loads(receipt.read_text())
    for p,h in prior['sources'].items():require(sha(Path(p))==h,'Accepted source '+p)
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Original '+p)
    for p,h in prior['private_library_hashes'].items():require(sha(Path(p))==h,'Private original '+p)
    require(sha(LIB)==sha(PRIVATE)==PIN,'Original tree image')
    sources=source_closure([Path(p) for p in prior['sources']]+[ROOT/'work'/n for n in NEW])
    _,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('two_tree_constructor',0x10012910,0x10012997),('primary_purge',0x10012840,0x100128a4),('secondary_purge',0x100128b0,0x10012910),
        ('primary_comparator',0x10007c60,0x10007c7f),('node_allocation',0x10007f10,0x10008022),('insert_position',0x10007e20,0x10007f08),
        ('insert_link',0x10008c30,0x10008ca4),('insert_rebalance',0x10008210,0x100087f6),('remove_rebalance',0x10008cb0,0x10009751),
        ('remove_unlink_successor_nil_skip',0x10009760,0x10009a12),('remove_by_payload',0x10009ac0,0x10009b4a),('find',0x10010480,0x10010576),('actual_tree_storage_growth',0x1003c840,0x1003c8e2)])
    started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_dictionary_tree.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=120)
    (run/'build-driver.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Fresh original/owned tree builds')
    executables=[];logs={}
    for tag,machine in [('reference_x86',0x14c),('owned_x64',0x8664)]:
        exe=OUT/('tree_'+tag+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if machine==0x14c else 0x20b) and b'asInvoker' in raw,'Fresh architecture/manifest '+tag)
        text=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',text,re.M|re.I);require({s.lower() for s in modules}=={'kernel32.dll'},'Runtime independence/imports '+tag);executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
    for name in ['native-build.log','owned-build.log']:
        p=OUT/name;require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',p.read_text(),re.I),'Clean build '+name);logs[str(p)]=sha(p)
    q,labels=corpus();query=run/'query.bin';query.write_bytes(b''.join(struct.pack('<4I',*row) for row in q));query_hash=sha(query)
    require(query_hash=='341759ac1da3495ddbf053ad8c07fa98c06b74a2e3cf09bf078c0da6d6f3644a','All expanded discovery queries preserved')
    first=OUT/'discovery-c5b5rwhg';require(query.read_bytes().startswith((first/'query.bin').read_bytes()),'Preserved initial300 query prefix')
    (run/'groups.json').write_text(json.dumps(dict(groups=dict(Counter(labels)),labels=labels),indent=2)+'\n');outputs=[];runtime={}
    for tag,argument in [('reference_x86',PRIVATE),('owned_x64',run/'no-original-engine.dll')]:
        dest=run/(tag+'.bin');exe=OUT/('tree_'+tag+'.exe');p=subprocess.run([str(exe),str(argument),str(query),str(dest)],cwd=run,capture_output=True,text=True,timeout=60)
        (run/(tag+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,tag+' execution '+str(p.returncode)+'; inspect log');runtime[tag]=p.stdout+p.stderr;outputs.append(dest.read_bytes())
    records=[decode(raw) for raw in outputs];require(len(records[0])==len(records[1])==len(q),'Complete state count')
    differences=[i for i,(a,b) in enumerate(zip(*records)) if a!=b];(run/'comparison.json').write_text(json.dumps(dict(operations=len(q),differences=len(differences),first=[dict(operation=i,query=q[i],native=records[0][i],owned=records[1][i]) for i in differences[:1]]),indent=2)+'\n')
    require(not differences and outputs[0]==outputs[1],'Exact complete original/owned tree states')
    require(hashlib.sha256(outputs[0]).hexdigest()=='1e3dee918a70f075306457d097b1ec859782961abbd2d5cd093cc3f7f7f38f57','Original expanded output unchanged across owned correction')
    require(all(raw.startswith((first/'native.bin').read_bytes()) for raw in outputs),'Preserved all300 initial complete outputs')
    checks=independent(q,records[0]);counts=re.search(r'native_allocations=(\d+) releases=(\d+)',runtime['reference_x86']);require(counts and counts[1]==counts[2],'Original heap cleanup')
    p=subprocess.run([str(OUT/'tree_owned_x64.exe'),'--test'],cwd=run,capture_output=True,text=True,timeout=60);(run/'safety.log').write_text(p.stdout+p.stderr);require(p.returncode==0 and 'owned_tree_safety_checks=65803 max_nodes=65535 high_payload_bits=RETAINED' in p.stdout,'Owned allocation/limit/lifetime-independent safety')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());graph=set()
    def visit(p):
        key=p.relative_to(ROOT).as_posix()
        if key in graph:return
        graph.add(key)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            child=p.parent/inc
            if child.is_file():visit(child.resolve())
    visit((ROOT/'work/probe_vulkan_win32_surface_x64.cpp').resolve());require(graph==set(build['editor_source_dependencies']) and all(sha(ROOT/k)==h for k,h in build['editor_source_dependencies'].items()),'Whole editor graph unchanged');require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor executable unchanged')
    for p,h in sources.items():require(sha(Path(p))==h,'Source drift '+p)
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Original drift '+p)
    for p,h in prior['private_library_hashes'].items():require(sha(Path(p))==h,'Private original drift '+p)
    require(sha(query)==query_hash and not (run/'no-original-engine.dll').exists(),'Immutable input/owned absent engine path')
    capture=run/'complete-states.bin.zlib';capture.write_bytes(zlib.compress(outputs[0],9));require(zlib.decompress(capture.read_bytes())==outputs[0],'Lossless complete evidence')
    limits=['Owned x64 primary material dictionary TREE primitive only, with real64-bit opaque payloads; does not construct/own materials, source trees, references, forwarding objects, resources or queued calls',
        'Original generic comparator/allocation/link/rebalance/remove/find/purge run RX unchanged with controlled allocator/lock services; opaque32-bit payload IDs are not fake IMaterial objects and are never dereferenced',
        'Whole roots/counts/capacity/free-list/allocated slot links/colors/payload IDs/symbols/source flags and effective nil color compared after every operation, including original nil-child deletion-repair skip; no stronger balance claim',
        'Owned65535 index-bound and allocation-failure refusals are safe explicit policy; native maximum/OOM behavior not dynamically tested here',
        'Normal material dictionary2324-operation object/lifetime trace and queued/resource/renderer/editor integration remain separate next-stage work; their source closures unchanged, their earlier tests not rerun',
        'No original writes, live SFM, GPU, Blender or full-editor build/run; native oracle uses only private original image, owned executable has no original runtime dependency']
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),scope='VERIFIED owned x64 primary material dictionary tree primitive; full material lifetime incomplete',previous_goal_turn='progress',operations=len(q),groups=dict(Counter(labels)),exact_bytes=len(outputs[0]),output_sha256=hashlib.sha256(outputs[0]).hexdigest(),query_sha256=query_hash,compressed_states_sha256=sha(capture),independent_checks=checks,owned_safety=(run/'safety.log').read_text(),native_allocations=int(counts[1]),native_releases=int(counts[2]),executables=executables,build_logs=logs,sources=sources,original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],static_ranges=ranges,prior_receipt=str(receipt),prior_receipt_sha256=sha(receipt),editor_dependencies_unchanged=len(graph),editor_sha256=build['executable_sha256'],limits=limits)
    dest=run/'results.json';dest.write_text(json.dumps(result,indent=2)+'\n');replay=OUT/('replay-'+run.name.removeprefix('verification-'));replay.mkdir()
    for file in [Path(p) for p in sources]+[Path(e['path']) for e in executables]+[Path(p) for p in logs]+[dest,query,capture]+list(run.glob('*.log'))+[run/'groups.json',run/'comparison.json']:
        target=replay/file.relative_to(ROOT);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(file,target)
    manifest={p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()};(replay/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    ptr=dict(receipt=str(dest),receipt_sha256=sha(dest),replay=str(replay),replay_manifest_sha256=sha(replay/'manifest.json'));(OUT/'latest-verification.json').write_text(json.dumps(ptr,indent=2)+'\n');print(json.dumps(dict(status=result['status'],receipt=str(dest),operations=len(q),exact_bytes=len(outputs[0]),independent_checks=checks)))


if __name__=='__main__':main()
