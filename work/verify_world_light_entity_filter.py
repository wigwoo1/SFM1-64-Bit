"""Original complete filter/manager instructions vs owned AMD64 and independent rules."""
import hashlib,json,random,struct,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/world-light-entity-filter-20261015'
ENGINE=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
MAP=ENGINE.parent.parent/'tf_movies/maps/mtt_soldier.bsp'
DIGEST='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
MAP_SHA='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(pass_,message):
    if not pass_:raise RuntimeError(message)
def expected(f):
    entity,excluded,handle,mask,mode,context=f
    raw=(handle>>16)==0x4000
    classified=entity==0 or raw
    count=(2+int(mode==3)) if entity else 0
    return struct.pack('<9I',int(classified),int(raw),(0x1fff if handle==0xffffffff else handle&65535) if entity else 0xffffffff,int(mode==3 and classified and entity!=excluded),mode,count,*([entity]*count+[0]*(3-count)))
def main():
    require(sha(ENGINE)==DIGEST and sha(MAP)==MAP_SHA,'Pinned original changed/unavailable')
    smoke='--smoke' in sys.argv;run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));rows=[];categories=[]
    def add(entity,excluded,handle,mask=0x4481,mode=3,context=0,category='directed'):
        rows.append((entity,excluded,handle,mask,mode,context));categories.append(category)
    for entity in range(4):
        for excluded in range(4):
            for handle in [0,0xffff,0x3fffffff,0x40000000,0x40001fff,0x4000ffff,0x40010000,0x80000000,0xc0000000,0xffffffff]:
                for mode in [1,3]:add(entity,excluded,handle,mode=mode,context=0xa7b8c9da)
    if not smoke:
        for high in range(65536):add(1+high%3,high%4,(high<<16)|((high*8191)&65535),mask=(high*65537)&0xffffffff,context=high,category='exhaustive_high16')
        for low in range(65536):
            for excluded in [0,1,2]:add(1,excluded,0x40000000|low,mask=low*65537,context=0xffffffff-low,category='exhaustive_static_low16')
        rng=random.Random(0x1943e0)
        for i in range(1024):add(1+i%3,i%4,rng.getrandbits(32),mask=rng.getrandbits(32),mode=1,context=rng.getrandbits(32),category='world_only')
    query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<6I',*r) for r in rows));reference=b''.join(expected(r) for r in rows);(run/'independent_expected.bin').write_bytes(reference)
    executables=[];logs=[];outputs=[]
    for name,machine in [('filter_native_x86',0x14c),('filter_owned_x64',0x8664),('filter_safety_x64',0x8664)]:
        exe=OUT/(name+'.exe');b=exe.read_bytes();require(struct.unpack_from('<H',b,struct.unpack_from('<I',b,60)[0]+4)[0]==machine,'Machine architecture')
        dest=run/(name+'.bin');args=[] if 'safety' in name else [str(ENGINE),str(query),str(dest)]
        result=subprocess.run([str(exe),*args],capture_output=True,text=True,timeout=60);logs.append(result.stdout+result.stderr);require(result.returncode==0,'Probe failed: '+name+' '+logs[-1]);executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine)))
        if 'safety' not in name:outputs.append(dest.read_bytes());require(len(outputs[-1])==len(rows)*36,'Result extent')
    mismatch=[]
    for i,f in enumerate(rows):
        a,b,c=(x[i*36:(i+1)*36] for x in [*outputs,reference])
        if not a==b==c:
            mismatch.append(dict(case=i,fixture=f,native=struct.unpack('<9I',a),owned=struct.unpack('<9I',b),expected=struct.unpack('<9I',c)))
            if len(mismatch)==12:break
    (run/'comparison.json').write_text(json.dumps(dict(differences=mismatch),indent=2)+'\n');require(not mismatch,'Filter mismatch: '+str(run/'comparison.json'))
    invalid=[struct.pack('<6I',4,0,0,0,3,0),struct.pack('<6I',1,4,0,0,3,0),struct.pack('<6I',1,0,0,0,2,0),query.read_bytes()[:23]]
    invalidChecks=0
    for i,wire in enumerate(invalid):
        file=run/('invalid-%d.bin'%i);file.write_bytes(wire)
        for name in ['filter_native_x86','filter_owned_x64']:
            result=subprocess.run([str(OUT/(name+'.exe')),str(ENGINE),str(file),str(run/(name+'-invalid.bin'))],capture_output=True,text=True,timeout=30)
            require(result.returncode==1 and 'fault' not in result.stderr,'Invalid fixture must reject normally');invalidChecks+=1
    from pe_image import Image
    from verify_ifm_draw_packet import check_ranges
    image=Image(ENGINE)
    ranges=[('lighting_predicate',0x101943e0,0x10194411),('manager_getter',0x101c46e0,0x101c46e6),('entity_classification',0x101c4700,0x101c472e),('raw_classification',0x101c4730,0x101c4749),('index_decoder',0x101c4750,0x101c4776),('world_only_false',0x102727e0,0x102727e5),('world_only_type',0x10127d20,0x10127d26),('props_type',0x10218050,0x10218056),('entity_loop_static_inspection_only',0x1016ef6f,0x1016f061),('trace_client_registration_static_only',0x1030d2f0,0x1030d305),('trace_client_factory_static_only',0x1016d420,0x1016d426),('collidable_clip_dispatch_static_only',0x1016f300,0x1016f54c)]
    image,evidence=check_ranges(ENGINE,'engine-disassembly.txt',ranges)
    require(image.word(0x1040b598)==0x1035fcdc and image.word(0x1035fcdc+44)==0x101c4700 and image.word(0x1035fcdc+40)==0x101c4730 and image.word(0x1035fcdc+48)==0x101c4750,'Manager vtable')
    require([image.word(a) for a in [0x1035bb48,0x1035bb4c,0x1031dee4,0x1031dee8]]==[0x101943e0,0x10218050,0x102727e0,0x10127d20],'Original lighting filter vtables')
    require(image.string(0x10354658)=='EngineTraceClient004' and image.word(0x10407ebc)==0x103544ec and image.word(0x103544ec+20)==0x1016ece0 and image.word(0x103544ec+16)==0x1016f300,'Trace client/clip vtable boundary')
    # Read-only inventory: counts/stride only, not a static-prop collision loader.
    b=MAP.read_bytes();off,length,version,compression=struct.unpack_from('<4I',b,8+35*16);require(not version and not compression and off+length<=len(b),'Game lump');n=struct.unpack_from('<I',b,off)[0];require(4+n*16<=length,'Game lump directory');inventory=None
    for i in range(n):
        id,flags,v,o,size=struct.unpack_from('<IHHII',b,off+4+16*i)
        if id!=int.from_bytes(b'prps','little'):continue
        require(flags==0 and v==5 and o+size<=len(b),'Benchmark static-prop lump');models=struct.unpack_from('<I',b,o)[0];p=o+4+128*models;require(p+4<=o+size,'Model extent');leaves=struct.unpack_from('<I',b,p)[0];p+=4+2*leaves;require(p+4<=o+size,'Leaf extent');props=struct.unpack_from('<I',b,p)[0];require(p+4+props*60==o+size,'Version5 record extent');inventory=dict(version=v,model_names=models,leaf_references=leaves,props=props,record_stride=60,scope='Read-only counts/extent; not live engine entities or collision shapes')
    require(inventory is not None,'Benchmark static props missing')
    coverage=dict(total=len(rows),null_entity=sum(r[0]==0 for r in rows),static_marker=sum(r[2]>>16==0x4000 for r in rows),excluded_same=sum(r[0]==r[1] for r in rows),world_only=sum(r[4]==1 for r in rows),accepted=sum(struct.unpack_from('<I',reference,i*36+12)[0] for i in range(len(rows))))
    require(sha(ENGINE)==DIGEST and sha(MAP)==MAP_SHA,'Original changed')
    receipt=dict(status='PASS',smoke=smoke,cases=len(rows),bytes_per_result=36,differences=0,coverage=coverage,invalid_fixture_checks=invalidChecks,logs=logs,executables=executables,query_sha256=sha(query),engine_sha256=DIGEST,map_sha256=MAP_SHA,benchmark_inventory=inventory,static_evidence=evidence,source_sha256={n:sha(ROOT/'work'/n) for n in ['source_world_light_entity_filter_candidate.h','probe_world_light_entity_filter.cpp','test_world_light_entity_filter.cpp','build_world_light_entity_filter.cmd','verify_world_light_entity_filter.py']},limits=['Full original filter/getter/manager classifiers/index virtuals execute intact; entity GetRefEHandle virtual supplied with fixture handle','Native manager/file-backed vtables retained and mutation checked; no DllMain/import loading','Entity identities normalized across x86/AMD64; AMD64 labels1/2 intentionally share low32 but differ above32bits','Entity-loop range only statically inspected, not executed; no partition enumeration/prop shape clipping/trace merge','Benchmark inventory is read-only; no live handle allocation/registry/ownership or original scene parity','Existing BSP lighting adapter still refuses mode3; no editor integration or build, apps, original writes or dependencies'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['status','cases','differences','coverage','benchmark_inventory']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
