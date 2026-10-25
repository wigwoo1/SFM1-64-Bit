"""Check final wide-to-double rounding and status against unchanged native code."""
import json,random,re,struct,subprocess,tempfile
from datetime import datetime,timezone
from verify_ifm_draw_packet import check_ranges
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha

def rounded96(x):
    if (x&(1<<42)) and (x&((1<<42)-1)):x+=1<<43
    return (x&((1<<96)-1))&~((1<<42)-1),x>>96

def convert(m,exp):
    sign=(exp>>15)<<63;e=(exp&32767)-16383
    if e==-16383:return sign,2 if m else 0
    original=m<<16;x,carry=rounded96(original);e+=carry
    if e< -1076:return sign,2
    if e<=-1023:
        x,_=rounded96(original>>(-1023-((exp&32767)-16383)))
        return sign|(x>>44),2
    if e>=1024:return sign|0x7ff0000000000000,1
    return sign|((e+1023)<<52)|((x&((1<<95)-1))>>43),0

def main():
    out=ROOT/'outputs/material-value-construction-20261025';run=type(out)(tempfile.mkdtemp(prefix='double-verification-',dir=out))
    require(sha(LIB)==sha(PRIVATE)==PIN,'Library identity')
    im,evidence=check_ranges(LIB,'materialsystem-disassembly.txt',[('complete_wide_to_double',0x1008e861,0x1008edb2)])
    require([im.word(a) for a in range(0x101050f4,0x1010510c,4)]==[1024,(-1023)&0xffffffff,53,11,64,1023],'Installed conversion settings')
    rng=random.Random(2026102504);cases=[]
    mantissas=[1<<79,(1<<80)-1]+[(1<<79)+(q<<27)+r for q in [0,1,2,12345,(1<<52)-1] for r in [0,(1<<26)-1,1<<26,(1<<26)+1,(1<<27)-1]]
    for e in [-16382,-1100,-1077,-1076,-1075,-1074,-1073,-1024,-1023,-1022,-1,0,1,52,53,1022,1023,1024,16384]:
        for m in mantissas:
            for sign in [0,32768]:cases.append((m,e+16383+sign))
    for _ in range(30000):cases.append(((1<<79)|rng.getrandbits(79),rng.randrange(1,32768)|(rng.randrange(2)<<15)))
    cases += [(m,sign) for m in [0,1,(1<<79)-1] for sign in [0,32768]]
    query=run/'queries.bin';query.write_bytes(b''.join(m.to_bytes(10,'little')+struct.pack('<H',e) for m,e in cases))
    exe=out/'material_construction_native_x86.exe';result=run/'native.bin'
    p=subprocess.run([str(exe),'--convert-batch',str(PRIVATE),str(query),str(result)],capture_output=True,text=True,timeout=60)
    (run/'native.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Native conversion batch')
    data=result.read_bytes();require(len(data)==len(cases)*12,'Conversion framing');diff=[];model=bytearray()
    for i,(m,e) in enumerate(cases):
        expected=struct.pack('<Qi',*convert(m,e));model.extend(expected)
        if expected!=data[i*12:i*12+12]:diff.append(dict(case=i,m=hex(m),exp=e,native=data[i*12:i*12+12].hex(),model=expected.hex()))
    (run/'model.bin').write_bytes(model)
    owned_exe=out/'material_construction_owned_x64.exe';owned_result=run/'owned.bin'
    p=subprocess.run([str(owned_exe),'--convert-batch',str(PRIVATE),str(query),str(owned_result)],capture_output=True,text=True,timeout=60)
    (run/'owned.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Owned conversion batch')
    require(owned_result.read_bytes()==model,'Owned versus integer model')
    for tag,path,machine in [('native',exe,0x14c),('owned',owned_exe,0x8664)]:
        raw=path.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(struct.unpack_from('<H',raw,pe+4)[0]==machine,'Architecture')
        require(not re.search(r'\b(warning|error)\b',(out/(tag+'-build.log')).read_text(),re.I),'Clean build')
    require(sha(LIB)==sha(PRIVATE)==PIN,'Unchanged libraries')
    receipt=dict(status='PASS' if not diff else 'FAIL',cases=len(cases),differences=diff,time_utc=datetime.now(timezone.utc).isoformat(),executable=str(exe),executable_sha256=sha(exe),library_sha256=PIN,static_evidence=evidence,scope='Normalized 80-bit significands, signed zero and selected wide subnormals; final bits and status; Python model')
    receipt.update(owned_executable=str(owned_exe),owned_executable_sha256=sha(owned_exe),owned_output_sha256=sha(owned_result),scope='Normalized significands, signed zero, selected wide subnormals; bits and status; original x86, owned x64, Python model')
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(receipt=str(run/'results.json'),cases=len(cases),differences=len(diff),first=diff[:3])))
    require(not diff,'Final conversion model')
if __name__=='__main__':main()
