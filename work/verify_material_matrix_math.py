"""Private matrix arithmetic comparison. Complete material loader remains separate."""
from datetime import datetime,timezone
from pathlib import Path
from collections import Counter
import json,random,re,struct,subprocess,tempfile
from verify_ifm_draw_packet import check_ranges
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
OUT=ROOT/'outputs/material-matrix-math-20261025'
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));require(sha(LIB)==sha(PRIVATE)==PIN,'Library identity')
    im,evidence=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('complete_matrix_multiply',0x1006cb00,0x1006cf01),('complete_translate',0x1006d040,0x1006d0b1),
        ('complete_scale',0x1006d240,0x1006d2b1),('complete_rotation',0x1006d190,0x1006d231)])
    offset=im.offset(0x100de848);require(im.b[offset:offset+8]==struct.pack('<d',0.017453292519943295),'Degree conversion constant')
    rng=random.Random(2026102508);cases=[];labels=[]
    identity=tuple(0x3f800000 if i%5==0 else 0 for i in range(16))
    def add(op,a,b=identity,alias=0,label='controlled'):
        a=tuple(a)+(0,)*(16-len(a));cases.append((op,alias,*a,*b));labels.append(label)
    for index in range(16):
        a=[0]*16;a[index]=bits(index+1)
        for alias in range(4):add(0,a,identity,alias,'identity_basis')
    for _ in range(2000):
        a=tuple(bits(rng.uniform(-1000,1000)) for _ in range(16));b=tuple(bits(rng.uniform(-1000,1000)) for _ in range(16))
        for alias in range(4):add(0,a,b,alias,'finite_multiply')
    special=[0,0x80000000,1,0x80000001,0x7fffff,0x800000,0x3f800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f800001]
    for _ in range(512):
        a=tuple(rng.choice(special) for _ in range(16));b=tuple(rng.choice(special) for _ in range(16))
        for alias in range(4):add(0,a,b,alias,'special_multiply')
    for op in [1,2]:
        for a in [(x,y,z) for x in special[:8] for y in special[:8] for z in [0,0x80000000,0x3f800000]]:add(op,a,label='translate_scale')
    angles=[0.,-0.,1.,-1.,90.,180.,270.,360.,-90.,0.00001,359.9999]+[rng.uniform(-36000,36000) for _ in range(4096)]
    for angle in angles:add(3,[bits(angle)],label='rotation')
    for _ in range(2048):
        p=[bits(rng.uniform(-10,10)) for _ in range(7)];p[4]=bits(rng.uniform(-3600,3600));add(4,p,label='composed_transform')
    query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<34I',*q) for q in cases));outputs=[];exes=[]
    for tag,suffix,machine in [('native','x86',0x14c),('owned','x64',0x8664)]:
        exe=OUT/f'material_matrix_math_{tag}_{suffix}.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(struct.unpack_from('<H',raw,pe+4)[0]==machine,'Architecture')
        log=OUT/(tag+'-build.log');require(not re.search(r'\b(warning|error)\b',log.read_text(),re.I),'Clean build '+tag)
        result=run/(tag+'.bin');p=subprocess.run([str(exe),str(PRIVATE),str(query),str(result)],capture_output=True,text=True,timeout=60)
        (run/(tag+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,'Execution '+tag+': '+str(p.returncode)+' '+p.stdout+p.stderr)
        data=result.read_bytes();require(len(data)==64*len(cases),'Output framing');outputs.append(data);exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),build_log_sha256=sha(log)))
    differences=[dict(case=i,label=labels[i],query=q,native=outputs[0][64*i:64*i+64].hex(),owned=outputs[1][64*i:64*i+64].hex()) for i,q in enumerate(cases) if outputs[0][64*i:64*i+64]!=outputs[1][64*i:64*i+64]]
    receipt=dict(status='FAIL' if differences else 'PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(cases),groups=dict(Counter(labels)),differences=differences,exact_bytes=len(outputs[0]),executables=exes,static_ranges=evidence,library_sha256=PIN,
        scope='Row-major arithmetic and calls composing texture transform from primitives; full material parser/factory not yet executed')
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(receipt=str(run/'results.json'),cases=len(cases),differences=len(differences),groups=dict(Counter(d['label'] for d in differences)),first=differences[:2])))
    require(not differences,'Matrix arithmetic parity')
if __name__=='__main__':main()
