"""Original loader/core support metadata vs owned PHY binding and scalar rules."""
import collections,hashlib,json,random,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/physics-support-metadata-20261015'
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
RAW=ROOT/'outputs/phy-raw-collision-20261015/verification-_7ukuat5/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def raw_shapes(data):
    solids=[];at=2
    for s in range(struct.unpack_from('<H',data)[0]):
        count=struct.unpack_from('<I',data,at)[0];at+=4;solid=[]
        for c in range(count):
            h=list(struct.unpack_from('<4I',data,at));n=struct.unpack_from('<I',data,at+16)[0];at+=20;t=h[3]&0xffff;tri=[list(struct.unpack_from('<4I',data,at+j*16)) for j in range(t)];at+=t*16;points=[list(struct.unpack_from('<4I',data,at+j*16)) for j in range(n)];at+=n*16;solid.append((h,tri,points))
        solids.append(solid)
    require(at==len(data),'Raw shape extent');return solids
def encode(shape,initial,ordinal):
    h,tri,points=shape;return struct.pack('<I',ordinal)+initial+struct.pack('<6I',*h,len(tri),len(points))+b''.join(struct.pack('<4I',*t) for t in tri)+b''.join(struct.pack('<4I',*p) for p in points)
def expected(wire):
    ordinal=struct.unpack_from('<I',wire)[0];payload=bytearray(wire[4:24]);h=struct.unpack_from('<4I',wire,24);t,n=struct.unpack_from('<2I',wire,40);tri=[struct.unpack_from('<4I',wire,48+j*16) for j in range(t)];at=48+t*16;points=[struct.unpack_from('<3f',wire,at+j*16) for j in range(n)];payload[:4]=b'\0'*4
    if not h[2]&3 and t<=512:
        used={word&0xffff for triangle in tri for word in triangle[1:]};lo,hi=min(used),max(used)
        if hi-lo+1<128:
            runs=[];length=0;present=True
            for j in range(lo,hi+1):
                if (j in used)!=present:runs.append(length);length=0;present=not present
                length+=1
            runs.append(length)
            if len(runs)<=14:
                struct.pack_into('<H',payload,4,lo)
                if len(runs)==1:payload[2]=2;struct.pack_into('<H',payload,0,runs[0])
                else:payload[2]=4;payload[3]=len(runs);struct.pack_into('<H',payload,0,sum(runs[::2]));payload[6:6+len(runs)]=bytes(runs)
                return struct.pack('<I',ordinal)+payload
    payload[2]=1
    def vertex(edge):return tri[edge//3][edge%3+1]&0xffff
    def previous(edge):return edge//3*3+(edge%3+2)%3
    def opposite(edge):
        offset=(tri[edge//3][edge%3+1]>>16)&0x7fff
        if offset&0x4000:offset-=0x8000
        physical=edge//3*4+edge%3+1+offset;return physical//4*3+physical%4-1
    for octant in range(8):
        d=[-1. if octant&(1<<j) else 1. for j in range(3)]
        dot=lambda p:f32(f32(f32(p[1]*d[1])+f32(p[0]*d[0]))+f32(p[2]*d[2]))
        edge=0;chosen=vertex(edge);chosenTriangle=0;best=dot(points[chosen])
        for iteration in range(t):
            edge=previous(edge);first=vertex(edge);improved=False;seen=set()
            while True:
                require(edge not in seen,'Independent seed ring');seen.add(edge);id=vertex(edge);value=dot(points[id])
                if value>best:best=value;chosen=id;chosenTriangle=edge//3;improved=True;break
                edge=previous(opposite(edge))
                if vertex(edge)==first:break
            if not improved and first!=chosen:break
        corner=next(j for j in range(3) if tri[chosenTriangle][j+1]&0xffff==chosen);struct.pack_into('<H',payload,4+octant*2,(chosenTriangle*4+corner)&0xffff)
    return struct.pack('<I',ordinal)+payload
def main():
    prior=json.loads(PRIOR.read_text());rawPrior=json.loads(RAW.read_text());require(prior['status']==rawPrior['status']=='PASS','Prior provenance');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libraries=run/'libraries';libraries.mkdir()
    for e in prior['libraries']:
        p=Path(e['source']);require(sha(p)==e['sha256'],'Original DLL changed');shutil.copyfile(p,libraries/p.name)
    executables=[]
    for name,machine in [('metadata_native_x86',0x14c),('metadata_kernel_native_x86',0x14c),('metadata_kernel_owned_x64',0x8664),('metadata_phy_owned_x64',0x8664),('metadata_safety_x64',0x8664)]:
        p=OUT/(name+'.exe');b=p.read_bytes();pe=struct.unpack_from('<I',b,60)[0];require(struct.unpack_from('<H',b,pe+4)[0]==machine and b'asInvoker' in b,'Build architecture/manifest');executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    models=[];allShapes=[];rows=[];rng=random.Random(0x26450);actualRecords=0;tableModes=collections.Counter();meaningful=0
    for index,(m,rawModel) in enumerate(zip(prior['models'],rawPrior['models'])):
        require(m['model']==rawModel['model'],'Raw model order');phy=Path(m['phy']);require(sha(phy)==m['phy_sha256'],'Original model changed');rawPath=Path(rawModel['path'])/'native-raw.bin';require(sha(rawPath)==rawModel['raw_sha256'],'Raw corpus changed');shapes=raw_shapes(rawPath.read_bytes());allShapes.extend(shape for solid in shapes for shape in solid);case=run/('%03d'%index);case.mkdir()
        for name,args in [('metadata_native_x86',[libraries,phy,case/'native.bin']),('metadata_phy_owned_x64',[phy,case/'owned.bin'])]:
            p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=30);(case/(name+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,'Metadata PHY probe '+str(case)+' '+p.stdout+p.stderr)
        a=(case/'native.bin').read_bytes();b=(case/'owned.bin').read_bytes();require(a[:2]==b[:2] and struct.unpack_from('<H',a)[0]==len(shapes),'Metadata solid count');na=nb=2;counts=[]
        for solid in shapes:
            ca,ra=struct.unpack_from('<2I',a,na);cb,rb=struct.unpack_from('<2I',b,nb);na+=8;nb+=8;require(ca==cb==len(solid) and ra==rb==(len(solid) if 0<len(solid)<=32 else 0),'Metadata table count/cutoff');counts.append(ra)
            for j in range(ra):
                ordinal=struct.unpack_from('<I',a,na)[0];actual=a[na+4:na+24];rebuilt=a[na+24:na+44];owned=b[nb+4:nb+24];require(ordinal==struct.unpack_from('<I',b,nb)[0]==j,'Metadata ledge order/identity');wire=encode(solid[j],bytes(20),j);reference=expected(wire)[4:];require(rebuilt==owned==reference,'Metadata owned/rebuilt/reference mismatch '+str(case)+' record='+str(j));flags=owned[2];mask=range(20) if flags==1 else range(6+(owned[3] if flags==4 else 0));require(all(actual[k]==owned[k] for k in mask),'Actual allocated metadata meaningful fields differ');meaningful+=len(mask);tableModes[flags]+=1;actualRecords+=1;rows.append(wire);na+=44;nb+=24
        require(na==len(a) and nb==len(b),'Metadata table output extent');models.append(dict(model=m['model'],phy=str(phy),phy_sha256=m['phy_sha256'],records=counts,path=str(case),native_sha256=sha(case/'native.bin'),owned_sha256=sha(case/'owned.bin')));print('PASS bound',index+1,'records='+str(sum(counts)),flush=True)
    # Same real ledges with random initial output bytes, eligibility flags and
    # finite transformed point coordinates exercise preserved bytes and seeds.
    for shape in allShapes:
        for variation in range(12):
            h,tri,points=shape;h=h[:];h[2]=(h[2]&~3)|(variation%4);changed=[]
            for p in points:
                v=struct.unpack('<4f',struct.pack('<4I',*p));scale=[1.,.00001,128.][variation%3];q=[f32(v[j]*scale+(variation-6)) for j in range(3)];changed.append(list(struct.unpack('<4I',struct.pack('<3fI',*q,p[3]))))
            rows.append(encode((h,tri,changed),bytes(rng.getrandbits(8) for _ in range(20)),rng.getrandbits(32)))
    # Real cube topology remapped to exact span/run boundaries.
    base=next(shape for shape in allShapes if len({w&0xffff for t in shape[1] for w in t[1:]})==8);unique=sorted({w&0xffff for t in base[1] for w in t[1:]})
    for ids in [list(range(8)),[0,1,3,5,7,9,11,13],list(range(0,16,2)),[0,1,2,3,4,5,6,126],[0,1,2,3,4,5,6,127],[32760+j for j in range(8)]]:
        mapping=dict(zip(unique,ids));points=[[0,0,0,0] for _ in range(max(ids)+1)]
        for old,new in mapping.items():points[new]=base[2][old][:]
        tri=[[t[0]]+[(w&0xffff0000)|mapping[w&0xffff] for w in t[1:]] for t in base[1]];rows.append(encode((base[0][:],tri,points),bytes([0xa7]*20),len(rows)))
    for count in [512,513]:
        h,tri,points=base;h=h[:];h[3]=(h[3]&0xffff0000)|count;extra=tri+[tri[0][:] for _ in range(count-len(tri))];rows.append(encode((h,extra,points),bytes([0xa7]*20),len(rows)))
    inputs=run/'queries.bin';inputs.write_bytes(b''.join(struct.pack('<I',len(row))+row for row in rows));reference=b''.join(expected(row) for row in rows);(run/'expected.bin').write_bytes(reference);outputs=[];logs=[]
    for name in ['metadata_kernel_native_x86','metadata_kernel_owned_x64']:
        dest=run/(name+'.bin');p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(inputs),str(dest)],capture_output=True,text=True,timeout=60);logs.append(p.stdout+p.stderr);require(p.returncode==0,'Metadata kernel '+name+' '+logs[-1]);outputs.append(dest.read_bytes());require(len(outputs[-1])==len(rows)*24,'Kernel result extent')
    differences=[]
    for j,row in enumerate(rows):
        a,b,c=(data[j*24:(j+1)*24] for data in [*outputs,reference])
        if not a==b==c:
            differences.append(dict(case=j,input=row.hex(),native=a.hex(),owned=b.hex(),expected=c.hex()))
            if len(differences)==8:break
    (run/'comparison.json').write_text(json.dumps(dict(differences=differences),indent=2)+'\n');require(not differences,'Metadata kernel mismatch '+str(run/'comparison.json'))
    safety=subprocess.run([str(OUT/'metadata_safety_x64.exe')],capture_output=True,text=True,timeout=15);require(safety.returncode==0,'Metadata safety '+safety.stdout+safety.stderr)
    from verify_ifm_draw_packet import check_ranges
    image,evidence=check_ranges(Path(prior['libraries'][1]['source']),'vphysics-disassembly.txt',[('complete_record_builder',0x10026450,0x100266d1),('complete_directional_seed_search',0x10026310,0x1002644d),('complete_run_encoder',0x10024e00,0x10024e70),('complete_metadata_table_constructor',0x100072d0,0x1000737e),('metadata_field_getter',0x100a9ff0,0x100a9ff4),('compact_field_getter',0x100184f0,0x100184f4)])
    require(image.word(0x100c5794+12*4)==0x100184f0 and image.word(0x100c5794+14*4)==0x100a9ff0,'Shape vtable getters')
    for m in prior['models']:
        for field in ['phy','mdl']:require(sha(Path(m[field]))==m[field+'_sha256'],'Original asset changed')
    for e in prior['libraries']:require(sha(Path(e['source']))==e['sha256'] and sha(libraries/Path(e['source']).name)==e['sha256'],'Original/copy DLL changed')
    receipt=dict(status='PASS',cases=len(rows),differences=0,actual_metadata_records=actualRecords,meaningful_allocated_bytes_compared=meaningful,actual_modes=dict(tableModes),kernel_modes=dict(collections.Counter(reference[j*24+6] for j in range(len(rows)))),safety=safety.stdout+safety.stderr,executables=executables,logs=logs,models=models,libraries=prior['libraries'],current_library_copies=str(libraries),prior_receipt=str(PRIOR),prior_receipt_sha256=sha(PRIOR),raw_receipt=str(RAW),raw_receipt_sha256=sha(RAW),input_sha256=sha(inputs),static_evidence=evidence,source_sha256={n:sha(ROOT/'work'/n) for n in ['source_physics_support_metadata_candidate.h','physics_support_metadata_test_fixture.h','probe_physics_support_metadata.cpp','probe_physics_support_metadata_kernel.cpp','probe_owned_phy_support_metadata.cpp','test_physics_support_metadata.cpp','build_physics_support_metadata.cmd','verify_physics_support_metadata.py','source_phy_compact_geometry_candidate.h','native_vphysics_library_fixture.h']},limits=['Actual original loader metadata and independently rebuilt zero-initialized original26450 records compared with owned file-to-metadata path; original pointer identity normalized to ledge ordinal','Unwritten native allocator tail bytes are opaque and excluded only from allocated-record comparison; direct native/owned/reference kernel compares all24bytes with supplied initial tails','All61benchmark one-solid inputs have1..15convexes; whole-table0/32/33boundaries owned analytical plus original static code, not native allocation-boundary execution','No original code patched; copied DLLs initialized normally; original assets unchanged; metadata construction is not full trace/editor integration'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['status','cases','differences','actual_metadata_records','meaningful_allocated_bytes_compared','actual_modes','kernel_modes','safety']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
