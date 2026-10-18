"""Original complete support mapping vs owned AMD64 and independent float rules."""
import collections,hashlib,json,random,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/physics-support-20261015'
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def frame(data):return struct.pack('<I',len(data))+data
def take(data,at):
    require(at+4<=len(data),'Missing frame length');n=struct.unpack_from('<I',data,at)[0];require(at+4+n<=len(data),'Truncated frame');return data[at+4:at+4+n],at+4+n
def split(data):
    result=[];at=0
    while at<len(data):row,at=take(data,at);result.append(row)
    return result
def encode(mode,points,direction,basis=None,transform=None,first=0,triangles=(),seeds=None,cache=None):
    basis=basis or [1,0,0,0,1,0,0,0,1];transform=transform or [1,0,0,0,0,1,0,0,0,0,1,0]
    b=struct.pack('<6I24f8H',mode,len(points),len(triangles),first,int(seeds is not None),0,*basis,*transform,*direction,*(seeds or [0]*8))
    if mode==0:b+=struct.pack('<1025H',*(cache or [0]*1025))
    return b+b''.join(struct.pack('<3f',*p) for p in points)+b''.join(struct.pack('<4I',*t) for t in triangles)
def expected(wire):
    mode,n,t,first,seeded,reserved=struct.unpack_from('<6I',wire);basis=struct.unpack_from('<9f',wire,24);transform=struct.unpack_from('<12f',wire,60);direction=struct.unpack_from('<3f',wire,108);seeds=struct.unpack_from('<8H',wire,120);at=136
    cache=list(struct.unpack_from('<1025H',wire,at)) if mode==0 else None
    if cache is not None:at+=2050
    points=[struct.unpack_from('<3f',wire,at+12*i) for i in range(n)];at+=12*n;triangles=[struct.unpack_from('<4I',wire,at+16*i) for i in range(t)]
    mul=lambda a,b:f32(a*b)
    add=lambda a,b:f32(a+b)
    dot=lambda p,d:add(add(mul(p[1],d[1]),mul(p[0],d[0])),mul(p[2],d[2]))
    if mode==2:
        best=[dot(points[j],direction) for j in range(4)];indices=list(range(4))
        for group in range(4,n,4):
            for j in range(4):
                value=dot(points[group+j],direction)
                if best[j]<value:best[j]=value;indices[j]=group+j
        oldBest=best[:];oldIndices=indices[:]
        for j in range(4):
            k=(j+2)%4
            if oldBest[j]<oldBest[k]:best[j]=oldBest[k];indices[j]=oldIndices[k]
        j=1 if best[0]<best[1] else 0;chosen=indices[j];return struct.pack('<I3f',chosen,*points[chosen])
    ivp=[direction[0],-direction[2],direction[1]];local=[add(add(mul(basis[j],ivp[0]),mul(basis[j+3],ivp[1])),mul(basis[j+6],ivp[2])) for j in range(3)]
    if mode==1:
        chosen=0;best=dot(points[0],local)
        for j in range(1,n):
            value=dot(points[j],local)
            if value>best:best=value;chosen=j
        feature=first+chosen
    else:
        def word(edge):return triangles[edge//3][edge%3+1]
        def vertex(edge):return word(edge)&0xffff
        def nextedge(edge):return edge//3*3+(edge%3+2)%3
        def opposite(edge):
            offset=(word(edge)>>16)&0x7fff
            if offset&0x4000:offset-=0x8000
            physical=edge//3*4+edge%3+1+offset;return physical//4*3+physical%4-1
        cache[-1]=(cache[-1]+1)&0xffff
        if cache[-1]==0:cache[:-1]=[0]*1024
        def visit(j):
            slot=(j&511)*2;seen=cache[slot]==cache[-1] and cache[slot+1]==j;cache[slot:slot+2]=[cache[-1],j];return seen
        seed=seeds[(1 if local[0]<0 else 0)|(2 if local[1]<0 else 0)|(4 if local[2]<0 else 0)] if seeded else 0;edge=(seed>>2)*3+(seed&3);chosen=vertex(edge);visit(chosen);best=dot(points[chosen],local)
        for iteration in range(t):
            edge=nextedge(edge);firstNeighbor=vertex(edge);improved=False;ring=set()
            while True:
                require(edge not in ring,'Reference ring');ring.add(edge);j=vertex(edge)
                if not visit(j):
                    value=dot(points[j],local)
                    if value>best:chosen=j;best=value;improved=True;break
                edge=nextedge(opposite(edge))
                if vertex(edge)==firstNeighbor:break
            if not improved and firstNeighbor!=chosen:break
        feature=chosen
    p=points[chosen];out=[add(add(add(mul(transform[j*4+1],p[1]),mul(transform[j*4],p[0])),mul(transform[j*4+2],p[2])),transform[j*4+3]) for j in range(3)]
    return struct.pack('<I3f',feature,*out)+(struct.pack('<1025H',*cache) if mode==0 else b'')
def tetra(ids=(0,1,2,3)):
    points=[[0,0,0] for _ in range(max(ids)+1)]
    for j,p in zip(ids,[(1,1,1),(-1,-1,1),(-1,1,-1),(1,-1,-1)]):points[j]=p
    faces=[(0,1,2),(0,3,1),(0,2,3),(1,3,2)];edges={}
    for j,face in enumerate(faces):
        for k in range(3):edges[(face[k],face[(k+1)%3])]=j*4+k+1
    triangles=[]
    for j,face in enumerate(faces):
        triangles.append([0]+[ids[face[k]]|(((edges[(face[(k+1)%3],face[k])]-j*4-k-1)&0x7fff)<<16) for k in range(3)])
    return points,triangles
def main():
    prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Prior trace provenance');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libraries=run/'libraries';libraries.mkdir()
    for e in prior['libraries']:
        source=Path(e['source']);require(sha(source)==e['sha256'],'Original library changed');shutil.copyfile(source,libraries/source.name)
    executables=[]
    for name,machine in [('simplex_native_x86',0x14c),('simplex_capture_x86',0x14c),('simplex_owned_x64',0x8664),('simplex_safety_x64',0x8664)]:
        p=OUT/(name+'.exe');b=p.read_bytes();pe=struct.unpack_from('<I',b,60)[0];require(struct.unpack_from('<H',b,pe+4)[0]==machine and b'asInvoker' in b,'Build architecture/manifest');executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    captured=[];observed=[];models=[];modeCounts=collections.Counter()
    for index,m in enumerate(prior['models']):
        case=run/('%03d'%index);case.mkdir();queries=Path(m['path'])/'queries.bin';old=Path(m['path'])/'native-traces.bin';require(sha(Path(m['phy']))==m['phy_sha256'] and sha(queries)==m['query_sha256'] and sha(old)==m['trace_sha256'],'Prior model/query/trace changed');trace=case/'traces.bin';record=case/'captures.bin'
        p=subprocess.run([str(OUT/'simplex_capture_x86.exe'),str(libraries),m['phy'],str(queries),str(trace),str(record)],capture_output=True,text=True,timeout=45,cwd=case);(case/'capture.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Support capture failed '+str(case)+' '+p.stdout+p.stderr);require(trace.read_bytes()==old.read_bytes(),'Support observation changed full original trace');raw=record.read_bytes();at=0;count=0
        while at<len(raw):
            query=struct.unpack_from('<I',raw,at)[0];at+=4;require(query<m['queries'],'Captured query index');row,at=take(raw,at);result,at=take(raw,at);captured.append(row);observed.append(result);count+=1;modeCounts[struct.unpack_from('<I',row)[0]]+=1
        models.append(dict(model=m['model'],path=str(case),queries=m['queries'],support_calls=count,captures_sha256=sha(record),traces_sha256=sha(trace)));print('PASS observed',index+1,'calls='+str(count),p.stdout.strip(),flush=True)
    require(captured,'No real support calls');rows=[];rng=random.Random(0x26d40)
    for mode in [2,1]:
        for j in range(4096):
            n=rng.choice([4,8,12,16,32]) if mode==2 else rng.randint(1,20);scale=rng.choice([1e-6,1,128]);points=[[f32(rng.randint(-3,3)*scale) for _ in range(3)] for _ in range(n)];d=[f32(rng.randint(-3,3)*scale) for _ in range(3)];basis=[f32(rng.randint(-3,3)/4) for _ in range(9)];transform=[f32(rng.randint(-4,4)*128) for _ in range(12)];rows.append(encode(mode,points,d,basis,transform,first=rng.randint(0,64)))
    for ids,amount in [((0,1,2,3),2048),((0,511,512,513),64),((32764,32765,32766,32767),8)]:
        points,triangles=tetra(ids)
        for j in range(amount):
            d=[f32(rng.randint(-3,3)) for _ in range(3)];generation=[0,1,0xfffe,0xffff][j%4];cache=[rng.getrandbits(16) for _ in range(1024)]+[generation];seeds=[rng.randrange(4)*4+rng.randrange(3) for _ in range(8)] if j%2 else None;rows.append(encode(0,points,d,triangles=triangles,seeds=seeds,cache=cache))
    # Lane-priority tie and signed-zero fixtures, plus rotated real raw contexts
    # replayed exactly as captured (cache generations/metadata included).
    rows.append(encode(2,[(0,0,0),(1,0,0),(0,0,0),(0,0,0),(1,1,0),(0,0,0),(0,0,0),(0,0,0)],(1,0,0)))
    for d in [(0.,-0.,0.),(-0.,0.,-0.)]:rows.append(encode(2,[(0.,-0.,0.),(-0.,0.,-0.),(1,1,1),(-1,-1,-1)],d))
    controlled=len(rows);rows+=captured;query=run/'queries.bin';query.write_bytes(b''.join(frame(row) for row in rows));reference=[expected(row) for row in rows];(run/'independent_expected.bin').write_bytes(b''.join(frame(row) for row in reference));outputs=[];logs=[]
    for name in ['simplex_native_x86','simplex_owned_x64']:
        dest=run/(name+'.bin');p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(query),str(dest)],capture_output=True,text=True,timeout=90);logs.append(p.stdout+p.stderr);require(p.returncode==0,'Direct support failed '+name+' '+logs[-1]);data=split(dest.read_bytes());require(len(data)==len(rows),'Support output extent');outputs.append(data)
    differences=[]
    for j,row in enumerate(rows):
        native,owned=outputs[0][j],outputs[1][j];actual=observed[j-controlled] if j>=controlled else native
        if not native==owned==reference[j]==actual:
            differences.append(dict(case=j,mode=struct.unpack_from('<I',row)[0],captured=j>=controlled,input=row.hex(),native=native.hex(),owned=owned.hex(),expected=reference[j].hex(),observed=actual.hex()))
            if len(differences)==8:break
    (run/'comparison.json').write_text(json.dumps(dict(differences=differences),indent=2)+'\n');require(not differences,'Support mismatch '+str(run/'comparison.json'))
    safety=subprocess.run([str(OUT/'simplex_safety_x64.exe')],capture_output=True,text=True,timeout=15);require(safety.returncode==0,'Safety '+safety.stdout+safety.stderr)
    invalid=run/'invalid';invalid.mkdir();bad=[('empty',b'',True),('truncated_length',b'\x01\x00',True),('truncated_payload',frame(rows[0])[:-1],True)]
    for label,at,bits in [('bad_mode',0,3),('zero_points',4,0),('reserved',20,1),('nan_direction',108,0x7fc00000),('inf_point',136,0x7f800000)]:
        row=bytearray(rows[0]);struct.pack_into('<I',row,at,bits);bad.append((label,frame(row),True))
    row=bytearray(rows[0]);struct.pack_into('<3f',row,108,1e30,1e30,1e30);struct.pack_into('<3f',row,136,1e30,1e30,1e30);bad.append(('arithmetic_overflow',frame(row),False));invalidChecks=0
    for label,data,both in bad:
        path=invalid/(label+'.bin');path.write_bytes(data)
        for name in ['simplex_owned_x64']+(['simplex_native_x86'] if both else []):
            p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(path),str(invalid/(label+'-'+name+'.out'))],capture_output=True,text=True,timeout=15);require(p.returncode==1 and 'fault' not in p.stderr,'Unsafe malformed fixture handling '+label+' '+name);invalidChecks+=1
    from verify_ifm_draw_packet import check_ranges
    image,evidence=check_ranges(Path(prior['libraries'][1]['source']),'vphysics-disassembly.txt',[('complete_support',0x10026d40,0x100271d6),('complete_packed_support',0x10026bc0,0x10026d37),('inverse_rotate_direction',0x1006f400,0x1006f48d),('transform_selected_vertex',0x100a8190,0x100a8237),('actual_leaf_setup_call',0x1002a375,0x1002a37a)])
    require([image.word(0x100f26f4+j*4) for j in range(4)]==[0,8,0xfffffffc,0xfffffffc],'Original next-edge address table');require([struct.unpack('<f',struct.pack('<I',image.word(0x100c8070+j*4)))[0] for j in range(4)]==[0,1,2,3],'Original packed lane indices')
    for e in prior['libraries']:require(sha(Path(e['source']))==e['sha256'] and sha(libraries/Path(e['source']).name)==e['sha256'],'Original/copy library changed')
    for m in prior['models']:
        for field in ['phy','mdl']:require(sha(Path(m[field]))==m[field+'_sha256'],'Original asset changed')
    receipt=dict(status='PASS',cases=len(rows),controlled_cases=controlled,captured_real_calls=len(captured),differences=0,captured_modes=dict(modeCounts),all_modes=dict(collections.Counter(struct.unpack_from('<I',row)[0] for row in rows)),safety=safety.stdout+safety.stderr,executables=executables,logs=logs,models=models,original_trace_cases=prior['totals']['traces'],all_original_trace_bytes_unchanged=True,libraries=prior['libraries'],current_library_copies=str(libraries),prior_receipt=str(PRIOR),prior_receipt_sha256=sha(PRIOR),query_sha256=sha(query),static_evidence=evidence,source_sha256={name:sha(ROOT/'work'/name) for name in ['source_physics_support_candidate.h','physics_support_test_fixture.h','native_physics_support_fixture.h','probe_physics_support.cpp','probe_physics_support_capture.cpp','test_physics_support.cpp','build_physics_support.cmd','verify_physics_support.py','native_vphysics_library_fixture.h']},limits=['Full original26D40/26BC0/6F400/A8190 execute in normal workspace DLL copies; owned arithmetic/packed scan/span scan/edge walk/cache agree with captured/direct calls','Normalized contexts capture original-prepared transforms, raw vertices/triangles/metadata/packed groups/cache; original PHY-to-runtime binding and packed-cache construction not reconstructed here','Only one original near-CALL2A375 altered in process memory; leaf setup calls real294F0 then clones motion support vtable with real method bridge; all original trace results unchanged','Full2050 cache bytes checked in edge-walk mode, return feature and12point bytes checked in every mode','No complete owned GJK/compact trace/normal/startsolid/filter/registry/editor integration proof; no original writes/editorbuild/UI/dependencies/idlehelpers'])
    receipt['invalid_fixture_checks']=invalidChecks
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({key:receipt[key] for key in ['status','cases','controlled_cases','captured_real_calls','differences','captured_modes','all_modes','safety','invalid_fixture_checks']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
