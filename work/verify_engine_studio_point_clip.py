"""Actual original studio ClipRayToCollideable + engine contents callback + real physics vs owned composition."""
import collections,hashlib,json,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/engine-studio-point-clip-20261016'
ENGINE=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/bin/engine.dll')
ENGINE_SHA='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
POINT_PRIOR=ROOT/'outputs/phy-point-trace-20261016/verification-xfpsrv2x/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def encode(q,index,cache=0):
    q=bytearray(q);mask=struct.unpack_from('<I',q,48)[0]&~0x40000000;struct.pack_into('<I',q,48,mask);return bytes(q)+struct.pack('<4IQ',[0,4,256,384][index%4],cache,0x40000000|((index*1031)&0xffff),[0,1,32768,65535][index%4],0x12345678 if index%5 else 0)
def empty(row):
    b=bytearray(92);v=struct.unpack_from('<12fI',row);f=lambda x:struct.unpack('<f',struct.pack('<f',x))[0];start=[f(x+0.) for x in v[:3]];struct.pack_into('<6f',b,0,*start,*[f(start[j]+v[3+j]) for j in range(3)]);struct.pack_into('<f',b,44,1.);return bytes(b)
def ledge_game_offsets(b):
    compact=struct.unpack_from('<I',b)[0]+32;pending=[compact+struct.unpack_from('<i',b,compact+32)[0]];out=[]
    while pending:
        n=pending.pop();right,rel=struct.unpack_from('<2i',b,n)
        if right:pending.extend([n+right,n+28])
        else:out.append(n+rel+4)
    return out
def main():
    require(sha(ENGINE)==ENGINE_SHA,'Original engine changed');prior=json.loads(PRIOR.read_text());point=json.loads(POINT_PRIOR.read_text());require(prior['status']==point['status']=='PASS','Prior receipts');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libs=run/'libraries';libs.mkdir()
    for e in prior['libraries']:
        p=Path(e['source']);require(sha(p)==e['sha256'],'Original library changed');shutil.copyfile(p,libs/p.name)
    executables=[]
    for name,machine in [('studio_clip_native_x86',0x14c),('studio_clip_owned_x64',0x8664),('studio_clip_safety_x64',0x8664),('default-regression/point_trace_native_x86',0x14c),('default-regression/point_trace_owned_x64',0x8664),('default-regression/point_trace_safety_x64',0x8664)]:
        p=OUT/(name+'.exe');raw=p.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Architecture/manifest');executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    def execute(name,args,log):
        r=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=90);log.write_text(r.stdout+r.stderr);require(r.returncode==0,'Studio clip '+str(log)+' '+r.stdout+r.stderr);return r.stdout
    safety=execute('studio_clip_safety_x64',[prior['models'][0]['phy'],prior['models'][0]['mdl'],Path(prior['models'][0]['path'])/'queries.bin'],run/'safety.log');print(safety,flush=True);totals=collections.Counter();models=[]
    def compare(case,phy,mdl,rows):
        queries=case/'queries.bin';queries.write_bytes(b''.join(rows));paths=[]
        for name in ['studio_clip_native_x86','studio_clip_owned_x64']:
            output=case/(name+'.bin');execute(name,[ENGINE,libs,phy,mdl,queries,output],case/(name+'.log'));require(len(output.read_bytes())==len(rows)*92,'Studio result extent');paths.append(output)
        a,b=paths[0].read_bytes(),paths[1].read_bytes();ea,eb=Path(str(paths[0])+'.events').read_bytes(),Path(str(paths[1])+'.events').read_bytes()
        if a!=b or ea!=eb:
            difference=next((i for i,(x,y) in enumerate(zip(a,b)) if x!=y),None);(case/'difference.json').write_text(json.dumps(dict(first_byte=difference,query=difference//92 if difference is not None else None,field=difference%92 if difference is not None else None,events_equal=ea==eb),indent=2));raise RuntimeError('Studio clip mismatch '+str(case/'difference.json'))
        require(len(ea)==len(rows)*8,'Studio event extent');info=collections.Counter(queries=len(rows))
        for i,row in enumerate(rows):
            dispatched,hit=struct.unpack_from('<2I',ea,i*8);info['physics_dispatches']+=dispatched;info['hits']+=hit;cache=struct.unpack_from('<I',row,56)[0];info['missing_cache_queries']+=cache==1;info['empty_cache_queries']+=cache==2
            if cache:require(a[i*92:(i+1)*92]==empty(row) and dispatched==0,'Known absent cache behavior')
            if hit:
                surface=struct.unpack_from('<Q',a,i*92+60)[0];metadata=struct.unpack_from('<I',a,i*92+68)[0];entity=struct.unpack_from('<Q',a,i*92+80)[0];box=struct.unpack_from('<I',a,i*92+88)[0];handle,property,world=struct.unpack_from('<IIQ',row,60);require(surface==0xffffffffffffffff and metadata==property and entity==world and box==(handle&0xffff)+1,'Studio annotation contract')
        return dict(path=str(case),summary=dict(info),queries_sha256=sha(queries),semantic_trace_sha256=sha(paths[0]),events_sha256=sha(Path(str(paths[0])+'.events')))
    for index,m in enumerate(prior['models']):
        require(sha(Path(m['phy']))==m['phy_sha256'] and sha(Path(m['mdl']))==m['mdl_sha256'],'Original model changed');source=Path(point['models'][index]['path'])/'queries.bin';require(sha(source)==point['models'][index]['queries_sha256'],'Prior point query changed');raw=source.read_bytes();rows=[encode(raw[i:i+52],i//52) for i in range(0,len(raw),52)];old=Path(m['path'])/'queries.bin';base=old.read_bytes()[:52]
        for cache in [1,2]:
            for flagsIndex in [0,2]:rows.append(encode(base,flagsIndex,cache))
        case=run/('%03d'%index);case.mkdir();record=compare(case,m['phy'],m['mdl'],rows);record['model']=m['model'];models.append(record);totals.update(record['summary']);print('PASS actual engine+physics/owned',index+1,record['summary'],flush=True)
    # Valid disposable topology, modified ONLY gameData/MDL contents fields to
    # exercise original studio callback0 and one-based bone1 through physics.
    variations=[];m=prior['models'][0];phy=Path(m['phy']).read_bytes();mdl=Path(m['mdl']).read_bytes();bones,boneOffset=struct.unpack_from('<2I',mdl,156);require(bones>=1,'Bone callback test needs bone0');q=(Path(m['path'])/'queries.bin').read_bytes()
    for data in [0,1]:
        case=run/('contents-'+str(data));case.mkdir();pb=bytearray(phy);mb=bytearray(mdl)
        for offset in ledge_game_offsets(pb):struct.pack_into('<I',pb,offset,data)
        struct.pack_into('<I',mb,332,6);struct.pack_into('<I',mb,boneOffset+180,4);p=case/'owned-copy.phy';d=case/'owned-copy.mdl';p.write_bytes(pb);d.write_bytes(mb);rows=[]
        for i in range(0,len(q),52):
            for mask in [1,2,4,6,0x4481,0x80000000]:
                row=bytearray(q[i:i+52]);struct.pack_into('<I',row,48,mask);rows.append(encode(row,len(rows)))
        record=compare(case,p,d,rows);record['game_data']=data;variations.append(record);totals.update(record['summary']);print('PASS controlled contents',data,record['summary'],flush=True)
    require(variations[0]['semantic_trace_sha256']!=variations[1]['semantic_trace_sha256'],'Model/bone contents never distinguished')
    invalid=run/'invalid';invalid.mkdir();good=Path(prior['models'][0]['mdl']).read_bytes();validQ=encode((Path(prior['models'][0]['path'])/'queries.bin').read_bytes()[:52],0);invalidChecks=0
    def reject(label,mdlData,queryData):
        nonlocal invalidChecks
        md=invalid/(label+'.mdl');qq=invalid/(label+'.queries');md.write_bytes(mdlData);qq.write_bytes(queryData);r=subprocess.run([str(OUT/'studio_clip_owned_x64.exe'),str(ENGINE),str(libs),prior['models'][0]['phy'],str(md),str(qq),str(invalid/(label+'.out'))],capture_output=True,text=True,timeout=15);require(r.returncode==1,'Invalid owned studio accepted '+label);invalidChecks+=1
    reject('short_header',good[:399],validQ)
    for label,offset,value in [('version',4,47),('bone_count',156,0xffffffff),('bone_offset',160,0xffffffff)]:
        b=bytearray(good);struct.pack_into('<I',b,offset,value);reject(label,b,validQ)
    for label,offset,value in [('custom_flag',52,1),('hitbox_mask',48,0x40000000),('bad_cache',56,3),('bad_handle',60,0),('bad_surface',64,65536),('nan_start',0,0x7fc12345)]:
        b=bytearray(validQ);struct.pack_into('<I',b,offset,value);reject(label,good,b)
    # Fresh default-overload regression uses the new header in isolated binaries.
    import verify_phy_point_trace as regression
    regression.OUT=OUT/'default-regression';regression.main();rr=max(regression.OUT.glob('verification-*/results.json'),key=lambda p:p.stat().st_mtime);require(json.loads(rr.read_text())['status']=='PASS','Default contents regression')
    from verify_ifm_draw_packet import check_ranges
    image,evidence=check_ranges(ENGINE,'engine-disassembly.txt',[('complete_clip_collideable',0x1016f300,0x1016f54c),('complete_physics_dispatch',0x1016e170,0x1016e2c2),('complete_studio_contents',0x1016d6c0,0x1016d6ee),('complete_client_entity_annotation',0x1016d7b0,0x1016d839),('complete_shape_lookup',0x10147ee0,0x10147f38)])
    require(image.word(0x10354470)==0x1016d6c0 and image.word(0x103544ec+26*4)==0x1016d7b0 and image.string(0x103545f8)=='**studio**','Original studio bindings');require(sha(ENGINE)==ENGINE_SHA,'Original engine changed')
    for e in prior['libraries']:require(sha(Path(e['source']))==e['sha256'] and sha(libs/Path(e['source']).name)==e['sha256'],'Original/copy library changed')
    for m in prior['models']:
        for key in ['phy','mdl']:require(sha(Path(m[key]))==m[key+'_sha256'],'Original asset changed')
    sources=['source_studio_physics_contents_candidate.h','source_engine_studio_point_clip_candidate.h','source_phy_point_trace_candidate.h','probe_engine_studio_point_clip.cpp','test_engine_studio_point_clip.cpp','build_engine_studio_point_clip.cmd','verify_engine_studio_point_clip.py','source_physics_leaf_candidate.h','native_vphysics_library_fixture.h','native_private_world_image_fixture.h','native_physics_search_fixture.h','verify_phy_point_trace.py']
    result=dict(status='PASS',totals=dict(totals),models=models,controlled_contents_variations=variations,differences=0,safety=safety.strip(),invalid_owned_fixture_checks=invalidChecks,executables=executables,engine_sha256=ENGINE_SHA,libraries=prior['libraries'],library_copies=str(libs),prior_receipt=str(PRIOR),prior_receipt_sha256=sha(PRIOR),point_query_receipt=str(POINT_PRIOR),point_query_receipt_sha256=sha(POINT_PRIOR),default_regression_receipt=str(rr),default_regression_sha256=sha(rr),static_evidence=evidence,source_sha256={n:sha(ROOT/'work'/n) for n in sources},limits=['Original whole1016F300/1016E170/1016D6C0/1016D7B0 and static handle manager run intact in private engine PE; normal initialized workspace vphysics/tier0 DLLs execute full real physics with original engine callback, no synthetic collision results','Native model/header/cache/pose/root/entity-list property providers represent explicit host inputs; no actual engine model-cache/registry/material-system/spatial partition initialization is claimed','Owned parses actual supported MDL model/bone contents, derives PHY metadata/search/tree/hit, applies original studio/entity annotation; no recorded state or original runtime dependency','Bounded domain is studio solid6 static props, point rays, no custom-collision bit1/hitbox mask40000000; known missing/empty cache returns miss. Unresolved asset is NOT evidence of known runtime cache absence','Surface-property lookup ID/world entity ID supplied explicitly; original metadata stamping verified, material-registry resolution not implemented','Default raw TraceBox API remains contents1 and freshly regressed in all three cache modes; six fresh programs, no editor integration/build or GUI/dependency/original writes'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','totals','differences','safety','invalid_owned_fixture_checks']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
