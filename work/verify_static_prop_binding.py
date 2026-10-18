"""Checked sprp5 records and owned collision bindings vs intact original init/getters.

Original engine runs only in test-only private x86 PE with explicit host model,
cache scope, bounds, illumination, and command-line providers. No live partition,
asset resolution, production x86 dependency or full SFM runtime claim.
"""
import collections,hashlib,json,math,random,struct,subprocess,tempfile
from pathlib import Path
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/static-prop-binding-20261016'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
PINS={'bin/engine.dll':'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f',
      'bin/tools/ifm.dll':'8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4',
      'tf_movies/maps/mtt_soldier.bsp':'60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea',
      'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx':'5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee'}
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def main():
    for p,s in PINS.items():require(sha(GAME/p)==s,'Pinned original changed: '+p)
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));engine=GAME/'bin/engine.dll';bsp=GAME/'tf_movies/maps/mtt_soldier.bsp'
    ranges=[('init',0x101c4cf0,0x101c516f),('ref_handle',0x101c4300,0x101c4304),('collideable_entity',0x101c4bd0,0x101c4bd4),('zero_flags',0x101c4bb0,0x101c4bb3),('model_origin_angles_solid',0x101c4470,0x101c44c5),('entity_predicate',0x101c4700,0x101c472e),('index_decoder',0x101c4750,0x101c4776),('manager_live_resolver',0x101c5e70,0x101c5ed0),('lump_loader',0x101c7160,0x101c73dd),('angle_matrix',0x10244e20,0x10244f7d),('angle_position_matrix',0x10246e20,0x10246e4b)]
    image,evidence=check_ranges(engine,'engine-disassembly.txt',ranges);(run/'original-code-evidence.json').write_text(json.dumps(evidence,indent=2))
    require([image.word(0x101c73e0+i*4) for i in range(7)]==[0x101c7206,0x101c7232,0x101c724e,0x101c726a,0x101c727f,0x101c7291,0x101c72a0],'Version dispatcher changed')
    require([image.word(0x1035f90c+4*i) for i in [0,7,8,9,11,12]]==[0x101c4bd0,0x101c4470,0x101c4480,0x101c4490,0x101c44c0,0x101c4bb0] and image.word(0x1035fa10+8)==0x101c4300,'Prop vtable changed')
    b=bsp.read_bytes();g=struct.unpack_from('<I',b,8+35*16)[0];entries=[struct.unpack_from('<IHHII',b,g+4+i*16) for i in range(struct.unpack_from('<I',b,g)[0])];entry=next(e for e in entries if e[0]==0x73707270);_,flags,version,offset,length=entry;require(flags==0 and version==5,'Benchmark prop version');p=offset;models=struct.unpack_from('<I',b,p)[0];p+=4;names=[];canonical=bytearray(struct.pack('<I',models))
    for i in range(models):
        name=b[p:p+128].split(b'\0')[0];p+=128;names.append(name.decode('ascii'));canonical+=struct.pack('<I',len(name))+name
    leaves=struct.unpack_from('<I',b,p)[0];p+=4;canonical+=struct.pack('<I',leaves)+b[p:p+leaves*2];p+=leaves*2;count=struct.unpack_from('<I',b,p)[0];p+=4;records=[b[p+i*60:p+(i+1)*60] for i in range(count)];canonical+=struct.pack('<I',count)+b[p:p+count*60];require(p+count*60==offset+length and (models,leaves,count)==(79,1083,485),'Benchmark prop extents');inactive=[]
    for i,r in enumerate(records):
        bad=[j for j in range(3) if not math.isfinite(struct.unpack_from('<f',r,44+4*j)[0])]
        if bad:require(not(r[31]&2),'Active NaN in source');inactive.append(dict(prop=i,components=bad,words=[hex(struct.unpack_from('<I',r,44+4*j)[0]) for j in bad]))
    executables=[]
    for name,machine in [('prop_binding_native_x86',0x14c),('prop_binding_owned_x64',0x8664),('prop_binding_safety_x64',0x8664)]:
        exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Probe architecture/manifest');executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine)))
    def execute(name,args,log):
        r=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=60);log.write_text(r.stdout+r.stderr);require(r.returncode==0,'Probe failed '+str(log)+' '+r.stdout+r.stderr);return r.stdout
    safety=execute('prop_binding_safety_x64',[bsp,run/'records.bin'],run/'safety.log');require((run/'records.bin').read_bytes()==canonical,'All owned parsed records differ from independent raw disk transcript');print(safety,flush=True)
    rows=[];expected=bytearray();categories=collections.Counter()
    def add(index,model,r,kind):
        require(len(r)==60,'Record query extent');rows.append(struct.pack('<IQ',index,model)+r);solid=r[30];angles=r[12:24] if solid==6 else bytes(12);expected.extend(struct.pack('<IIIQ',0x40000000|index,solid,0,model)+r[:12]+angles+struct.pack('<4I',index,1,1,1));categories[kind]+=1
    for i,r in enumerate(records):add(i,0x1234567800000001+struct.unpack_from('<H',r,24)[0],r,'benchmark')
    # All flag-byte values, supported solid types, and full low16 index range
    # boundaries. No inferred disk flags -> solid flags; original getter decides.
    for solid in [0,2,6]:
        for flag in range(256):
            for index in [0,1,65535]:
                r=bytearray(60);struct.pack_into('<6f',r,0,-0.,12.5,-33.25,-0.,90.,-15.);r[30:32]=bytes([solid,flag]);struct.pack_into('<3f',r,44,1.,2.,3.);add(index,0xaabbccdd12345678,bytes(r),'directed')
    rng=random.Random(2026101623)
    for _ in range(4096):
        r=bytearray(60);struct.pack_into('<6f',r,0,*[rng.uniform(-8192,8192) for _ in range(3)],*[rng.uniform(-360,360) for _ in range(3)]);r[30:32]=bytes([rng.choice([0,2,6]),rng.randrange(256)]);struct.pack_into('<3f',r,44,*[rng.uniform(-8192,8192) for _ in range(3)]);struct.pack_into('<HH',r,26,rng.randrange(65536),rng.randrange(65536));add(rng.randrange(65536),rng.getrandbits(64)|1,bytes(r),'random')
    (run/'queries.bin').write_bytes(b''.join(rows));(run/'independent-expected.bin').write_bytes(expected)
    for name in ['prop_binding_native_x86','prop_binding_owned_x64']:
        output=run/(name+'.bin');message=execute(name,[engine,bsp,run/'queries.bin',output],run/(name+'.log'));actual=output.read_bytes();require(actual==expected,'Native/owned/independent binding mismatch '+name);print(message,flush=True)
    # Repeat entire native program: no live engine process or warm state needed.
    execute('prop_binding_native_x86',[engine,bsp,run/'queries.bin',run/'native-repeat.bin'],run/'native-repeat.log');require((run/'native-repeat.bin').read_bytes()==expected,'Repeated native result changed')
    originals=[]
    for p,s in PINS.items():require(sha(GAME/p)==s,'Original changed after verification');originals.append(dict(path=str(GAME/p),sha256=s,unchanged=True))
    sources={str(p.relative_to(ROOT)):sha(p) for p in [ROOT/'work'/n for n in ['source_bsp_static_props_candidate.h','probe_static_prop_binding.cpp','test_static_prop_binding.cpp','build_static_prop_binding.cmd','verify_static_prop_binding.py','native_private_world_image_fixture.h','source_world_light_entity_filter_candidate.h']]}
    result=dict(status='PASS',path=str(run),benchmark=dict(models=models,leaf_references=leaves,props=count,solid_types=dict(collections.Counter(r[30] for r in records)),inactive_nonfinite_lighting_origins=inactive,raw_record_transcript_sha256=sha(run/'records.bin')),queries=len(rows),categories=dict(categories),semantic_bytes_per_query=60,query_sha256=sha(run/'queries.bin'),result_sha256=sha(run/'prop_binding_owned_x64.bin'),safety=safety.strip(),repeated_native_identical=True,executables=executables,originals=originals,sources=sources,limitations=['Original initialization/getters execute intact in a private engine image; model/header/bounds/illumination/cache-scope/command providers are controlled fixtures.','Native manager live lookup checked; owned output resolver booleans are expected assertions, not an implemented spatial/entity registry.','Owned parsed records and collision binding only; no spatial partition enumeration, asset lifetime, PHY registry or scene integration.','Static prop index/disk order establishes identity only; never substituted for partition order.','VBSP20 leaf-v1 / uncompressed sprp5 and solid types0/2/6 only; rendering, other versions and general collision unverified.','18 unresolved loose PHY names remain unresolved; missing files do not establish runtime cache absence.'])
    (run/'results.json').write_text(json.dumps(result,indent=2));print('PASS receipt '+str(run/'results.json'),flush=True)
if __name__=='__main__':main()
