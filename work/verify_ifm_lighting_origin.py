"""Original draw-origin fragment versus owned selection/finite arithmetic.

Native world-position and renderable-bounds providers are controlled callbacks.
Full original transform-stage execution has a separate DAG world-position test.
"""
import itertools
import json
from pathlib import Path
import random
import re
import struct
import subprocess
import tempfile
from pe_image import Image
from verify_session_illumination_dag import IFM,IFM_SHA
from verify_dag_world_position import sha,require

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/ifm-lighting-origin-20261015'
RANGES=[('complete_origin_selection_fragment',0x10243967,0x10243a57),
        ('typed_DAG_attribute_getter',0x10130ed0,0x10130f15),
        ('child_count',0x104c2b00,0x104c2b07),('typed_child_lookup',0x104c2e70,0x104c2ec6),
        ('cached_studio_header',0x1023bf80,0x1023bfa4),
        ('studio_illumination_attachment_index',0x1023b390,0x1023b3a4),
        ('computeBounds_registration',0x105c2992,0x105c29b9),
        ('renderable_flag20_setter',0x1023b9a0,0x1023b9bc)]


def static_check():
    image=Image(IFM);rows=[];pending=None
    with (ROOT/'work/ifm-disassembly.txt').open() as f:
        for line in f:
            m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
            if m:
                pending=None;address=int(m[1],16)
                if any(a<=address<b for _,a,b in RANGES):pending=dict(address=address,bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());rows.append(pending)
            elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
    evidence=[]
    for name,a,b in RANGES:
        selected=[r for r in rows if a<=r['address']<b];cursor=a
        for r in selected:
            require(r['address']==cursor,'Disassembly gap '+name);raw=bytes.fromhex(r['bytes']);o=image.offset(cursor);require(image.b[o:o+len(raw)]==raw,'Stale disassembly '+name);cursor+=len(raw)
        require(cursor==b,'Incomplete static range '+name);evidence.append(dict(name=name,start=hex(a),end_exclusive=hex(b),bytes=b-a,instructions=selected))
    require(image.string(0x10b9465c)=='computeBounds','Model field name changed')
    require(struct.unpack_from('<f',image.b,image.offset(0x10a8a87c))[0]==.5,'Midpoint constant changed')
    require(image.word(0x10adb904+0x44)==0x10240520,'Renderable bounds vtable changed')
    return evidence


def main():
    require(sha(IFM)==IFM_SHA,'Pinned original IFM changed');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));evidence=static_check()
    rng=random.Random(2026101517);rows=[];expected=[];metadata=[];coverage=dict(explicit_dag=0,child_zero_dag=0,present_no_dag=0,positive_attachment_no_override=0,disabled_bounds_no_override=0,bounds_compute=0,bounds_forced=0,valid_second_child_ignored=0)
    f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
    def append(fields,values):
        values=list(map(f32,values));rows.append(struct.pack('<6Ii2I15f',*fields,*values));present,explicit,children,first,second,header,index,compute,flags=fields;point=[123.25,-456.5,789.75];dag=0xffffffff;world=box=0;has=0
        if present:
            if explicit==1:dag=0;coverage['explicit_dag']+=1
            elif children and first==1:dag=1;coverage['child_zero_dag']+=1
            else:
                coverage['present_no_dag']+=1
                if children>1 and first!=1 and second==1:coverage['valid_second_child_ignored']+=1
            if dag!=0xffffffff:has=world=1;point=values[dag*3:dag*3+3]
        elif header and index>0:coverage['positive_attachment_no_override']+=1
        elif compute or flags&0x20:
            has=box=1;point=[f32(f32(values[12+i]+values[9+i])*.5) for i in range(3)];coverage['bounds_compute' if compute else 'bounds_forced']+=1
        else:coverage['disabled_bounds_no_override']+=1
        expected.append(struct.pack('<I3fIII',has,*point,world,box,dag));metadata.append(fields)
    for fields in itertools.product(range(2),range(3),range(3),range(3),range(3),range(2),[-2147483648,-1,0,1,257,2147483647],[0,1,255],[0,0x20,0xdf,0xff]):
        # Asymmetric vectors distinguish selected DAGs and a midpoint from either endpoint.
        append(fields,[7,-3,2,13,17,-19,-23,29,31,-41,-43,47,53,59,-61])
    for i in range(8192):
        fields=(rng.randrange(2),rng.randrange(3),rng.randrange(3),rng.randrange(3),rng.randrange(3),rng.randrange(2),rng.choice([-2147483648,-1,0,1,257,2147483647]),rng.randrange(256),rng.randrange(256));values=[rng.uniform(-65536,65536) for _ in range(15)]
        if i%8==0:values=[rng.choice([0.0,-0.0,1.0,-1.0,2**-126,-2**-126]) for _ in range(15)]
        append(fields,values)
    require(all(coverage.values()),'Missing branch coverage');query=run/'queries.bin';query.write_bytes(b''.join(rows));outputs=[];executables=[]
    for name,machine in [('origin_native_x86',0x14c),('origin_owned_x64',0x8664)]:
        exe=OUT/(name+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];require(b[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',b,pe+4)[0]==machine,'Executable architecture');target=run/(name+'.bin');p=subprocess.run([str(exe),str(IFM),str(query),str(target)],capture_output=True,text=True,timeout=60);(run/(name+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,name+' failed: '+p.stdout+p.stderr);actual=target.read_bytes();require(len(actual)==len(rows)*28,'Output length');outputs.append(actual);executables.append(dict(executable=str(exe),sha256=sha(exe),machine=hex(machine)))
    want=b''.join(expected);different=[i for i in range(len(rows)) if outputs[0][28*i:28*(i+1)]!=outputs[1][28*i:28*(i+1)] or outputs[0][28*i:28*(i+1)]!=want[28*i:28*(i+1)]]
    details=[dict(case=i,fields=metadata[i],native=struct.unpack_from('<7I',outputs[0],28*i),owned=struct.unpack_from('<7I',outputs[1],28*i),expected=struct.unpack_from('<7I',want,28*i)) for i in different[:8]]
    safety=OUT/'origin_safety_x64.exe';p=subprocess.run([str(safety)],capture_output=True,text=True,timeout=20);(run/'safety.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Safety test: '+p.stdout+p.stderr);executables.append(dict(executable=str(safety),sha256=sha(safety)))
    require(sha(IFM)==IFM_SHA,'Original IFM changed');receipt=dict(status='PASS' if not different else 'FAIL',cases=len(rows),output_bytes_per_case=28,differences=len(different),details=details,coverage=coverage,static_byte_count=sum(b-a for _,a,b in RANGES),static_evidence=evidence,executables=executables,safety=p.stdout.strip(),ifm_sha256=IFM_SHA,query_sha256=sha(query),
        limits=['Original10243967..10243A56 fragment runs in supplied frame, then test RET at next block10243A57',
                'Original typed reference getter, child count/typed child getter and cached studio header/header2 index getter execute',
                'Attribute lookup, datamodel resolution/type predicate, DAG world positions and renderable bounds are controlled callbacks',
                'Native fixture model/renderable/context/header/attribute inputs and sealed private executable bytes checked unchanged',
                'Selection/arithmetic only: no model geometry/bounds computation, native live input binding or full renderer executed',
                'computeBounds registration/default literal is static evidence; live runtime defaults not observed',
                'Renderable flag20 setter identified; broader ownership/lifetime and draw-category gating remain unverified',
                'Owned finite/overflow guards narrow corrupted-input domain; native corrupted-DMX equivalence not claimed',
                'No editor integration, original writes, app launches or full editor build'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(rows),differences=len(different),details=details[:2],coverage=coverage,receipt=str(run/'results.json'))));require(not different,'Origin selection differs')


if __name__=='__main__':main()
