"""Read-only static installed-loader audit. Never loads or executes original DLL code."""
from pathlib import Path
import hashlib, json, re, struct, subprocess
from pe_image import Image

ROOT=Path(__file__).resolve().parent.parent
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
GAME=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
FILES={'ifm':GAME/'bin/tools/ifm.dll','engine':GAME/'bin/engine.dll'}
PIN={'ifm':'12504c248047e181d31a8500fb6693d1a2d3c805e841fc2227e41aa4e2d2b47e',
     'engine':'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'}
RANGES=[('ifm','model_info_acquisition',0x10989dd9,0x10989df6),
 ('ifm','inline_name_admission',0x102abd5f,0x102abdb0),
 ('ifm','puppet_model_lookup',0x1024277f,0x102427b2),
 ('ifm','effects_attribute_existence',0x1035dd50,0x1035dd91),
 ('ifm','no_puppet_continues_children',0x102abdf4,0x102abe36),
 ('engine','interface_registration',0x1030fab0,0x1030fac5),
 ('engine','interface_factory',0x1019f690,0x1019f696),
 ('engine','find_or_load_model',0x1019e8c0,0x1019e8f6),
 ('engine','loader_static_construction',0x10309b80,0x10309b8a),
 ('engine','loader_vptr',0x100e7eb0,0x100e7eba),
 ('engine','loader_virtual_wrapper',0x100e9700,0x100e9720),
 ('engine','inline_index_and_bounds',0x100e8e30,0x100e8eaf),
 ('engine','decimal_wrapper',0x102df23d,0x102df248),
 ('engine','base10_parser_call',0x102df227,0x102df23d),
 ('engine','raw_lump14_to_submodels',0x100e7f36,0x100e8162),
 ('engine','model_count_destination',0x100e0007,0x100e0011),
 ('engine','submodel_to_runtime_same_ordinal',0x100e8550,0x100e865f)]
def need(x,message):
    if not x:raise RuntimeError(message)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def audit(directory):
    directory=Path(directory);directory.mkdir(parents=True,exist_ok=False)
    images={k:Image(p) for k,p in FILES.items()}
    need(all(sha(p)==PIN[k] for k,p in FILES.items()),'pinned installed inputs changed')
    evidence=[];instructions={}
    for module,label,a,b in RANGES:
        q=subprocess.run([str(DUMP),'/disasm:bytes',f'/range:0x{a:X},0x{b-1:X}',str(FILES[module])],
                         capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW)
        (directory/(label+'.txt')).write_text(q.stdout,encoding='utf-8')
        rows=[];pending=None
        for line in q.stdout.splitlines():
            m=re.match(r'\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2} )+)(.*)',line)
            if m:
                va=int(m[1],16);pending=None
                if a<=va<b:
                    pending=dict(address=va,bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());rows.append(pending)
            elif pending and re.fullmatch(r'\s+(?:[0-9A-F]{2} ?)+',line):pending['bytes']+=bytes.fromhex(line).hex()
        cursor=a;im=images[module]
        for row in rows:
            raw=bytes.fromhex(row['bytes']);need(row['address']==cursor,'instruction gap '+label)
            need(im.b[im.offset(cursor):im.offset(cursor)+len(raw)]==raw,'stale disassembly bytes '+label)
            instructions[module,cursor]=row['instruction'];cursor+=len(raw)
        need(cursor==b,'incomplete range '+label)
        evidence.append(dict(module=module,label=label,start=hex(a),end_exclusive=hex(b),instructions=rows))
    engine=images['engine'];ifm=images['ifm']
    need(ifm.string(0x10ddb3c4)==engine.string(0x1035c9e4)=='VModelInfoClient004','model-info interface name')
    need(engine.word(0x1040a5c8)==0x1035c854,'factory object vptr')
    need(engine.word(0x1035c854+2*4)==0x1019f180 and engine.word(0x1035c854+40*4)==0x1019e8c0,'admission/lookup slots')
    need(engine.word(0x10401484)==0x105ce600 and engine.word(0x10342b8c+7*4)==0x100e9700,'loader object/virtual slot')
    for module,va,text in [('engine',0x100e8e9c,'mov         eax,edi'),
        ('engine',0x100e8e9e,'imul        eax,eax,144h'),
        ('engine',0x100e8ea4,'add         eax,dword ptr [esi+50h]'),
        ('engine',0x100e814d,'add         ecx,30h'),
        ('engine',0x100e8589,'imul        esi,esi,144h')]:
        need(instructions[module,va]==text,'index/stride instruction '+hex(va))
    # The code sets the count destination to this loader's +68 field. Conversion
    # starts at raw record0 and advances48 bytes; runtime construction starts at
    # ordinal0 and names nonzero records with that same ordinal via '*%d'.
    need(ifm.string(0x10adba88)=='effects' and ifm.string(0x10abd738)=='modelName','admission attribute names')
    need(instructions['ifm',0x102abdae].split()==['je','102ABDF4'],'absent effects branch bypasses allocation')
    need(engine.string(0x10342c38)=='*%d','inline model name format')
    need(engine.string(0x10343060)=='bad inline model number %i','inline bounds diagnostic')
    need(engine.word(0x100e0007+6)==0x105ce600+0x68,'raw model count belongs to loader')
    need(all(sha(p)==PIN[k] for k,p in FILES.items()),'original inputs changed during audit')
    result=dict(status='VERIFIED_STATIC',originals={k:dict(path=str(p),sha256=PIN[k]) for k,p in FILES.items()},
        complete_instruction_count=sum(len(e['instructions']) for e in evidence),evidence=evidence,
        conclusion='For admitted positive decimal inline names *n, installed loader selects runtime model[n]; raw lump14 conversion/runtime construction preserve ordinal, so use BSP model record n, without subtracting one.',
        bounds='world must exist;1<=n<model_count;world uses record0 separately',
        unavailable_inline_admission='CreatePuppets bypasses puppet allocation when model index is -1 and effects attribute is absent; children are still traversed. Presence of any effects attribute follows a different, unverified fallback.',
        limitations=['Static installed-byte evidence only; no original code execution or live SFM comparison.',
                    'The IFM file is the user-intentionally modified installation; audited spans are identified by its actual hash.',
                    'Strict owned parsing need not mimic native acceptance of malformed strings or original fatal-error behavior.'])
    (directory/'evidence.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    return result
if __name__=='__main__':
    import sys
    p=Path((ROOT/'outputs/source-inline-number-current.txt').read_text().strip())
    value=audit(p/'static-evidence')
    print(json.dumps({k:v for k,v in value.items() if k!='evidence'},indent=2))
