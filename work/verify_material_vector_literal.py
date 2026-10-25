"""Private complete vector parser and strtod-prefix comparison, not VMT loading."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,json,random,re,struct,subprocess,tempfile,zlib
from verify_ifm_draw_packet import check_ranges
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
OUT=ROOT/'outputs/material-vector-literal-20261025'
def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));require(sha(LIB)==sha(PRIVATE)==PIN,'Library identity')
    im,evidence=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('complete_vector_start',0x10002980,0x100029ae),('complete_vector_parser',0x10002ec0,0x10002fdc),
        ('complete_strtod_wrapper',0x1007e38d,0x1007e4c7),('GetString_entry',0x10071470,0x100714ab),
        ('GetString_string_return',0x10071672,0x1007169e),('complete_GetName',0x100705f0,0x1007061d)])
    require(im.word(0x100716a0)==0x10071672,'GetString type1 dispatch')
    reciprocal=struct.unpack('<I',struct.pack('<f',1/255))[0];require(im.word(0x100956b0)==reciprocal,'Byte color reciprocal')
    cases=[];labels=[];rng=random.Random(2026102506)
    def add(op,text,seed=(0x3f800000,0xc0000000,0x40400000,0x40800000),label='controlled'):
        if isinstance(text,str):text=text.encode()
        cases.append((op,*seed,len(text),text));labels.append(label)
    prefixes=['',' ','\t','\n','\r','\v','\f',' -','+','-']
    numbers=['','0','-0','.','1.5','1e3','1d3','1D-3','1e','1e+','1e-','1+3','1-3','nan','inf','0x1p2','1.#INF','1\0e9','12suffix','-x','1e309','1e-999','-1e-999']
    for prefix in prefixes:
        for s in numbers:add(0,prefix+s,label='prefix_grammar')
    for opening in ['[','{']:
        for body in ['', '1','1 2','1 2 3','1 2 3 4','1 2 3 4 5','255 128 0 255','1 x','x','1,2','1\n2','1\r2','1\v2','1\f2','1 2\0ignored','1e 2','1+3','1d3 -1D-3','1e309 0','1e-999 -1e-999']:
            for close in ['',']','}','\n']:
                for prefix in ['',' \t','\n']:
                    add(1,prefix+opening+body+close,label='vector_grammar')
    for fill in range(256):
        seed=tuple(rng.getrandbits(32) for _ in range(4))
        for s in ['[1 2]','{255 128}','[1 x]','[x]','[1 2 3 4 5]']:
            add(1,s,seed,label='all_seed_words')
    for _ in range(3000):
        n=rng.randrange(5);text=rng.choice(['[','{'])+' '.join(rng.choice(numbers[1:]) for _ in range(n))+rng.choice([']','}','', ' tail'])
        add(1,text,label='random_literals')
    for _ in range(2000):
        text=rng.choice(prefixes)+str(rng.randrange(-10**15,10**15))+rng.choice(['e','d','D','E'])+str(rng.randrange(-600,601))+rng.choice(['','tail',']'])
        add(0,text,label='random_prefixes')
    for s in ['[0 0 0 0]','{255 255 255 255}','[1 2]','[1 x]','[1 2 3 4 5]','\n[1 2]']:
        add(1,s,label='analytical')
    for s in ['1d3','1e+','-1e-999','-0','-1e-9999','1e9999']:
        add(0,s,label='analytical')
    query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<6I',*row[:6])+row[6] for row in cases))
    data=[];executables=[]
    for tag,suffix,machine in [('native','x86',0x14c),('owned','x64',0x8664)]:
        exe=OUT/f'material_vector_{tag}_{suffix}.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(struct.unpack_from('<H',raw,pe+4)[0]==machine,'PE architecture '+tag)
        log=OUT/(tag+'-build.log');require(not re.search(r'\b(warning|error)\b',log.read_text(),re.I),'Clean build '+tag)
        output=run/(tag+'.bin');p=subprocess.run([str(exe),str(PRIVATE),str(query),str(output)],capture_output=True,text=True,timeout=60)
        (run/(tag+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,'Execution '+tag+': '+p.stdout+p.stderr+str(p.returncode))
        raw=output.read_bytes();require(len(raw)==32*len(cases),'Output frame count');data.append(raw)
        executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),build_log_sha256=sha(log)))
    differences=[dict(case=i,label=labels[i],input_hex=row[6].hex(),seed=row[1:5],native=data[0][i*32:i*32+32].hex(),owned=data[1][i*32:i*32+32].hex()) for i,row in enumerate(cases) if data[0][i*32:i*32+32]!=data[1][i*32:i*32+32]]
    analytical=0
    if not differences:
        def result(op,text):
            i=next(i for i,r in enumerate(cases) if r[0]==op and r[6]==text.encode())
            return struct.unpack_from('<8I',data[1],32*i)
        for text,expected in [('[0 0 0 0]',(1,4,0,0,0,0,0,0)),
            ('{255 255 255 255}',(1,4,0,0x3f800000,0x3f800000,0x3f800000,0x3f800000,0)),
            ('[1 2]',(1,2,0,0x3f800000,0x40000000,0,0x40800000,0)),
            ('[1 x]',(1,0,1,0x3f800000,0,0x40400000,0x40800000,0)),
            ('[1 2 3 4 5]',(1,4,0,0x3f800000,0x40000000,0x40400000,0x40800000,0)),
            ('\n[1 2]',(0,0,0,0x3f800000,0xc0000000,0x40400000,0x40800000,0))]:
            require(result(1,text)==expected,'Analytical vector '+repr(text));analytical+=1
        for text,value,consumed in [('1d3',1000.,3),('1e+',1.,1),('-1e-999',0.,7),('-0',-0.,2)]:
            raw=struct.pack('<8I',*result(0,text));require(raw[:8]==struct.pack('<d',value) and struct.unpack_from('<I',raw,8)[0]==consumed,'Analytical prefix '+text);analytical+=1
    require(sha(LIB)==sha(PRIVATE)==PIN,'Libraries unchanged')
    require(sha(ROOT/'work/material_string_value_candidate.h')=='4a9b16d46bd01e16f034922777e94cfb0d700ea614ab16198ec46c0bf8256007','Protected setter unchanged')
    require(sha(ROOT/'outputs/vtf-legacy-thumbnail-20261024/sfm64_session_ui_vtf_legacy.exe')=='2e79aed74e3831196a609346251680132f7beeadcc0cb810318f1fb9f5156ed2','Editor unchanged')
    archive=run/'native-output.zlib';archive.write_bytes(zlib.compress(data[0],9));require(zlib.decompress(archive.read_bytes())==data[0],'Archive roundtrip')
    r=dict(status='FAIL' if differences else 'PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(cases),exact_bytes=len(data[0]),differences=differences,
        executables=executables,query_sha256=sha(query),native_output_sha256=hashlib.sha256(data[0]).hexdigest(),owned_output_sha256=hashlib.sha256(data[1]).hexdigest(),
        static_ranges=evidence,analytical_checks=analytical,library_sha256=PIN,
        sources_sha256={name:sha(ROOT/'work'/name) for name in ['material_decimal_prefix_candidate.h','material_vector_literal_candidate.h','material_construction_native_fixture.h','probe_material_vector_literal.cpp','build_material_vector_literal.cmd','verify_material_vector_literal.py']},
        limitations=['Typed KeyValues string node supplied; no text KeyValues loading, name interning or factory execution.',
        'Warning count verified; host supplies name lookup and warning sink, original warning text formatting not tested.',
        'All scratch words retained for comparison, including unconsumed words; full dispatcher only observes count components.',
        'No live engine, production integration or full editor build.'])
    (run/'results.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(dict(receipt=str(run/'results.json'),cases=len(cases),differences=len(differences),first=differences[:3])))
    require(not differences,'Vector/prefix comparison')
if __name__=='__main__':main()
