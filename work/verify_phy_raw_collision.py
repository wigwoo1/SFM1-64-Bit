"""Owned PHY raw topology/IVP point bits vs actual loaded original query ledges."""
import hashlib,json,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/phy-raw-collision-20261015'
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def summary(raw):
    at=2;solids=struct.unpack_from('<H',raw)[0];result=dict(solids=solids,convexes=0,triangles=0,point_records=0,nonzero_padding=0,virtual_edges=0)
    for s in range(solids):
        count=struct.unpack_from('<I',raw,at)[0];at+=4;result['convexes']+=count
        for c in range(count):
            h=struct.unpack_from('<4I',raw,at);points=struct.unpack_from('<I',raw,at+16)[0];at+=20;triangles=h[3]&0xffff
            for j in range(triangles):
                t=struct.unpack_from('<4I',raw,at+j*16)
                for corner in range(1,4):require((t[corner]&0xffff)<points,'Output vertex span');result['virtual_edges']+=bool(t[corner]&0x80000000)
            at+=triangles*16
            for j in range(points):result['nonzero_padding']+=struct.unpack_from('<I',raw,at+j*16+12)[0]!=0
            at+=points*16;result['triangles']+=triangles;result['point_records']+=points
    require(at==len(raw),'Raw output extent');return result
def source_word_destinations(data):
    # Shared point tables cause the same source word to appear in several
    # normalized ledge prefixes. Track every occurrence in the expected copy.
    destinations={};out=2;start=struct.unpack_from('<I',data)[0]
    for solid in range(struct.unpack_from('<I',data,8)[0]):
        compact=start+32;root=compact+struct.unpack_from('<i',data,compact+32)[0];pending=[root];out+=4
        while pending:
            node=pending.pop();right,relative=struct.unpack_from('<2i',data,node)
            if right:pending.extend([node+right,node+28]);continue
            ledge=node+relative;triangles=struct.unpack_from('<H',data,ledge+12)[0];points=ledge+struct.unpack_from('<i',data,ledge)[0];last=max(struct.unpack_from('<H',data,ledge+16+t*16+corner*4)[0] for t in range(triangles) for corner in [1,2,3])
            for source,dest,count in [(ledge,out,4),(ledge+16,out+20,triangles*4),(points,out+20+triangles*16,(last+1)*4)]:
                for j in range(count):destinations.setdefault(source+j*4,[]).append(dest+j*4)
            out+=20+triangles*16+(last+1)*16
        start+=4+struct.unpack_from('<I',data,start)[0]
    return destinations
def main():
    prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Original geometry receipt');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libraries=run/'libraries';libraries.mkdir()
    for e in prior['libraries']:
        p=Path(e['source']);require(sha(p)==e['sha256'],'Original DLL changed');shutil.copyfile(p,libraries/p.name)
    executables=[]
    for name,machine in [('raw_native_x86',0x14c),('raw_owned_x64',0x8664),('geometry_legacy_owned_x64',0x8664)]:
        p=OUT/(name+'.exe');b=p.read_bytes();pe=struct.unpack_from('<I',b,60)[0];require(b[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',b,pe+4)[0]==machine and b'asInvoker' in b,'Architecture/manifest');executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    def execute(name,args,log):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=30);log.write_text(p.stdout+p.stderr);require(p.returncode==0,'Raw geometry probe failed '+str(log)+' '+p.stdout+p.stderr)
    models=[];totals=dict(solids=0,convexes=0,triangles=0,point_records=0,nonzero_padding=0,virtual_edges=0,raw_bytes=0)
    for index,m in enumerate(prior['models']):
        case=run/('%03d'%index);case.mkdir();phy=Path(m['phy']);oldMesh=Path(m['path'])/'native-mesh.bin';require(sha(phy)==m['phy_sha256'] and sha(oldMesh)==m['mesh_sha256'],'Original model/mesh provenance')
        for name,label in [('raw_native_x86','native'),('raw_owned_x64','owned')]:execute(name,[libraries,phy,case/(label+'-raw.bin'),case/(label+'-mesh.bin')],case/(label+'.log'))
        execute('geometry_legacy_owned_x64',[phy,case/'legacy-mesh.bin'],case/'legacy.log');a=(case/'native-raw.bin').read_bytes();b=(case/'owned-raw.bin').read_bytes()
        if a!=b:
            (case/'comparison.json').write_text(json.dumps(dict(native_bytes=len(a),owned_bytes=len(b),first_different=next((j for j,(x,y) in enumerate(zip(a,b)) if x!=y),None)),indent=2));raise RuntimeError('Raw geometry mismatch '+str(case))
        for label in ['native','owned','legacy']:require((case/(label+'-mesh.bin')).read_bytes()==oldMesh.read_bytes(),'Converted geometry regression '+str(case)+' '+label)
        info=summary(a)
        for key,value in info.items():totals[key]+=value
        totals['raw_bytes']+=len(a);models.append(dict(model=m['model'],phy=str(phy),phy_sha256=m['phy_sha256'],summary=info,path=str(case),raw_sha256=sha(case/'native-raw.bin'),mesh_sha256=sha(case/'native-mesh.bin')));print('PASS',index+1,m['model'],info,flush=True)
    valid=Path(prior['models'][0]['phy']).read_bytes();start=struct.unpack_from('<I',valid)[0];compact=start+32;root=compact+struct.unpack_from('<i',valid,compact+32)[0];leaf=root
    while struct.unpack_from('<i',valid,leaf)[0]:leaf+=28
    ledge=leaf+struct.unpack_from('<i',valid,leaf+4)[0];bad=[]
    def mutate(label,at,fmt,value,both=True):
        b=bytearray(valid);struct.pack_into(fmt,b,at,value);bad.append((label,bytes(b),both))
    bad.extend([('short_header',valid[:15],True),('short_solid',valid[:start+40],True),('no_script',valid[:start+4+struct.unpack_from('<I',valid,start)[0]],True)])
    for label,at,value in [('bad_id',4,1),('bad_header_size',0,0xffffffff),('no_solids',8,0),('bad_vphy',start+4,0),('bad_size',start,1),('root_outside',compact+32,0x7fffffff),('bad_ivps',compact+44,0),('bad_axis_map',start+28,1),('leaf_no_ledge',leaf+4,0),('points_outside',ledge,0x7fffffff),('bad_ledge_flags',ledge+8,0)]:mutate(label,at,'<I',value)
    mutate('version',start+8,'<H',1);mutate('model_type',start+10,'<H',1);mutate('triangle_count',ledge+12,'<H',0x7fff);point=ledge+struct.unpack_from('<i',valid,ledge)[0];mutate('nonfinite_vertex',point,'<I',0x7f800000)
    edge=struct.unpack_from('<I',valid,ledge+20)[0];mutate('opposite_outside',ledge+20,'<I',(edge&0xffff)|(0x4000<<16),False);mutate('opposite_to_header',ledge+20,'<I',(edge&0xffff)|(0x7fff<<16),False)
    invalid=run/'invalid';invalid.mkdir();checks=0
    for label,data,both in bad:
        path=invalid/(label+'.phy');path.write_bytes(data)
        names=['raw_owned_x64']+(['geometry_legacy_owned_x64'] if both else [])
        for name in names:
            args=[str(libraries),str(path),str(invalid/(label+'-raw.bin')),str(invalid/(label+'-mesh.bin'))] if name=='raw_owned_x64' else [str(path),str(invalid/(label+'-legacy.bin'))]
            p=subprocess.run([str(OUT/(name+'.exe')),*args],capture_output=True,text=True,timeout=15);require(p.returncode==1,'Malformed PHY accepted '+label+' '+name);checks+=1
    # Positive owned-only preservation fixtures. These do not claim that the
    # original runtime accepts arbitrary changed flags/header words. Converted
    # triangles must stay unchanged while every changed opaque bit survives.
    first=run/'000';baseline=(first/'native-raw.bin').read_bytes();converted=(first/'native-mesh.bin').read_bytes();destinations=source_word_destinations(valid);preserved=[]
    for label,sourceAt,bits in [('nan_padding_word',point+12,0x7fc12345),('virtual_edge_bit',ledge+20,edge|0x80000000),('opaque_triangle_header',ledge+16,0xdeadbeef)]:
        data=bytearray(valid);struct.pack_into('<I',data,sourceAt,bits);path=invalid/(label+'.phy');path.write_bytes(data);rawPath=invalid/(label+'-raw.bin');meshPath=invalid/(label+'-mesh.bin');execute('raw_owned_x64',[libraries,path,rawPath,meshPath],invalid/(label+'.log'));expected=bytearray(baseline)
        for resultAt in destinations[sourceAt]:struct.pack_into('<I',expected,resultAt,bits)
        require(rawPath.read_bytes()==expected and meshPath.read_bytes()==converted,'Raw opaque preservation failed '+label);preserved.append(dict(label=label,copied_occurrences=len(destinations[sourceAt])))
    from verify_ifm_draw_packet import check_ranges
    image,evidence=check_ranges(Path(prior['libraries'][1]['source']),'vphysics-disassembly.txt',[('original_query_ledge_array_access',0x10007040,0x1000705e),('original_game_data_getter',0x10007060,0x1000707d),('original_triangle_vertex_reader',0x10007080,0x10007189),('original_query_convex_count',0x1001de40,0x1001de44),('original_query_create',0x10009270,0x100092b0)])
    for m in prior['models']:
        for field in ['phy','mdl']:require(sha(Path(m[field]))==m[field+'_sha256'],'Original asset changed')
    for e in prior['libraries']:require(sha(Path(e['source']))==e['sha256'] and sha(libraries/Path(e['source']).name)==e['sha256'],'Original/copy DLL changed')
    receipt=dict(status='PASS',models=models,totals=totals,differences=0,converted_geometry_all_unchanged=True,invalid_fixture_checks=checks,executables=executables,libraries=prior['libraries'],current_library_copies=str(libraries),prior_receipt=str(PRIOR),prior_receipt_sha256=sha(PRIOR),static_evidence=evidence,source_sha256={name:sha(ROOT/'work'/name) for name in ['source_phy_compact_geometry_candidate.h','probe_phy_raw_collision.cpp','build_phy_raw_collision.cmd','verify_phy_raw_collision.py','probe_owned_phy_compact_geometry.cpp','native_vphysics_library_fixture.h']},limits=['Original VCollideLoad/CreateQueryModel run normally using copied original DLLs; actual loaded query ledges independently read at statically verified array field','Complete raw ledge headers/triangle words/point16byte records through largest referenced index agree, including opaque padding','Both default legacy reader and optional raw extraction freshly compiled and preserve all prior converted geometry bytes','Only61 available benchmark one-solid VPHY0100/type0/IVPS inputs verified; other formats and multisolid runtime behavior unverified','No original optional metadata construction/raw-tree runtime binding/GJK/trace/editor integration proof; no original writes or editorbuild/UI/dependencies/idlehelpers'])
    receipt['owned_opaque_preservation_checks']=preserved
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({key:receipt[key] for key in ['status','totals','differences','converted_geometry_all_unchanged','invalid_fixture_checks','owned_opaque_preservation_checks']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
