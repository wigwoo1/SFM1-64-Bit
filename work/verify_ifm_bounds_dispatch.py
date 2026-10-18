"""Full original dispatcher/wrapper policy with controlled external providers."""
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
OUT=ROOT/'outputs/ifm-bounds-dispatch-20261015'
RANGES=[('complete_dispatcher',0x1023f310,0x1023f698),('complete_wrapper',0x10240520,0x102405a3),
        ('sequence_descriptor_lookup',0x105da450,0x105da4e4),('bounds_min',0x1011c4f0,0x1011c544),
        ('bounds_max',0x1011c550,0x1011c5a4),('typed_first_bone',0x105c3580,0x105c35ca),
        ('first_bone_position',0x100031c0,0x100031c4),('first_bone_orientation',0x104d45c0,0x104d45c4)]


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
    for label,a,b in RANGES:
        selected=[row for row in rows if a<=row['address']<b];cursor=a
        for row in selected:
            require(row['address']==cursor,'Disassembly gap '+label);raw=bytes.fromhex(row['bytes']);o=image.offset(cursor);require(image.b[o:o+len(raw)]==raw,'Stale bytes '+label);cursor+=len(raw)
        require(cursor==b,'Incomplete static range '+label);evidence.append(dict(name=label,start=hex(a),end_exclusive=hex(b),bytes=b-a,instructions=selected))
    # The comparison vector is in zero-initialized virtual data, not file bytes.
    # Verify that PE mapping explicitly; do not invent a file offset for BSS.
    nt=image.u32(60);optional=nt+24;sections=optional+image.u16(nt+20);zero_rva=0x10f4ed24-image.base;zero_section=None
    for index in range(image.u16(nt+6)):
        offset=sections+index*40;virtual_size=image.u32(offset+8);rva=image.u32(offset+12);raw_size=image.u32(offset+16)
        if rva+raw_size<=zero_rva and zero_rva+12<=rva+virtual_size:zero_section=dict(name='private_image_hull_comparison_zero_vector',address=hex(0x10f4ed24),section_rva=hex(rva),raw_size=raw_size,virtual_size=virtual_size,storage='zero-initialized PE virtual tail')
    require(zero_section is not None,'Hull comparison vector is not in the expected zero-initialized PE tail');evidence.append(zero_section)
    return evidence


def main():
    require(sha(IFM)==IFM_SHA,'Original IFM changed');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));evidence=static_check();executables=[]
    for name,machine in [('dispatch_native_x86',0x14c),('dispatch_owned_x64',0x8664),('bounds_safety_x64',0x8664)]:
        path=OUT/(name+'.exe');binary=path.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(binary[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',binary,pe+4)[0]==machine,'Executable architecture');require(b'asInvoker' in binary,'Non-elevated execution manifest missing');executables.append(dict(executable=str(path),sha256=sha(path),machine=hex(machine)))
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=90);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' exit='+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout
    identity=[1,0,0,0,0,1,0,0,0,0,1,0];box=[-1,-2,-3,1,2,3];rows=[];metadata=[];rng=random.Random(2026101521)
    def append(flags,category,sequence,mode,children,render_flags,values,sequences,label):
        require(len(values)==55 and len(sequences)>0,'Fixture field length')
        rows.append(struct.pack('<IiiiiII55f',flags,category,sequence,mode,children,len(sequences),render_flags,*values)+b''.join(struct.pack('<6f',*b) for b in sequences));metadata.append(dict(label=label,flags=flags,category=category,sequence=sequence,mode=mode,children=children,render_flags=render_flags,sequences=len(sequences)))
    def values(model=box,animated=box,hull=box,view=box,child=identity,wrapper=identity,position=(0,0,0),orientation=(0,0,0,1)):
        return list(model)+list(animated)+list(hull)+list(view)+list(child)+list(wrapper)+list(position)+list(orientation)
    shift=identity.copy();shift[3]=10;outer=identity.copy();outer[3]=100
    append(0,3,0,0,0,0,values(),[box],'default')
    append(1|256,1,0,0,0,0,values(child=shift),[box],'category1 child translation')
    append(1|2|4|8|16,3,0,1,1,0,values(wrapper=outer),[box],'animated success then outer wrapper')
    append(1|2|4|8|32|128|512|1024,3,99,0,1,0,values(hull=[0]*6,position=(10,0,0)),[box],'first bone DIRECT matrix witness')
    append(2,3,0,1,1,0,values(wrapper=outer),[box],'default wrapper raw untyped node')
    append(1|32|128|512|1024,3,-1,0,1,0x20,values(position=(10,0,0)),[box],'engine fallback skips first bone')
    append(1|32|128,3,0,0,0,0,values(hull=[0]*6,view=[-0.0]*6),[[0.0]*6],'signed-zero hull/view and union ties')
    for flags,category,mode in itertools.product(range(2048),[0,1,3,7],[-1,0,1]):
        hull=[0.0]*6 if flags%3==0 else box
        append(flags,category,[-2,-1,0,1,99][(flags//128)%5],mode,[-1,0,1,2][(flags//32)%4],[0,0x20,0xff,0xdf][(flags//16)%4],values(hull=hull,child=shift,wrapper=outer,position=(10,0,0)),[box,[-9,-8,-7,9,8,7]],'exhaustive state')
    def random_box():return [rng.uniform(-64,0) for _ in range(3)]+[rng.uniform(0,64) for _ in range(3)]
    for i in range(4096):
        append(rng.randrange(2048),rng.choice([1,3,7]),rng.choice([-2147483648,-2,-1,0,1,2147483647]),rng.choice([-2147483648,-1,0,1,2147483647]),rng.choice([-1,0,1,2]),rng.randrange(256),values(model=random_box(),animated=random_box(),hull=[0]*6 if i%4==0 else random_box(),view=random_box(),child=[rng.uniform(-3,3) for _ in range(12)],wrapper=[rng.uniform(-3,3) for _ in range(12)],position=[rng.uniform(-32,32) for _ in range(3)],orientation=[rng.uniform(-1.2,1.2) for _ in range(4)]),[random_box() for _ in range(3)],'varied finite transforms')
    previous=json.loads((ROOT/'outputs/ifm-animated-bounds-20261015/verification-4io_ucwf/results.json').read_text());models=[]
    for i,m in enumerate(previous['models']):
        path=Path(m['path']);require(sha(path)==m['sha256'],'Original model metadata changed');raw=path.read_bytes();count,offset=struct.unpack_from('<ii',raw,188);require(0<=count<=4096 and 0<=offset<=len(raw)-count*212,'Raw sequence span');hull=list(struct.unpack_from('<6f',raw,128));view=list(struct.unpack_from('<6f',raw,104));sequences=[list(struct.unpack_from('<6f',raw,offset+j*212+32)) for j in range(count)];expected=struct.pack('<I12f',count,*hull,*view)+b''.join(struct.pack('<6f',*b) for b in sequences);dump=run/('mdl-'+str(i)+'.bin');execute('dispatch_owned_x64',[path,dump],'mdl-'+str(i));require(dump.read_bytes()==expected,'MDL bounds reader differs from raw fields');models.append(dict(path=str(path),sha256=m['sha256'],local_sequence_count=count))
        if count:
            for j in range(16):append(1|2|4|8|32|128|512|1024|(256 if j&1 else 0),3,[-2,-1,0,count-1,count+7][j%5],[-1,0,1][j%3],j%3,0,values(hull=hull,view=view,child=shift,wrapper=outer,position=(float(j),0,0)),sequences,path.name+' stored metadata')
    query=run/'queries.bin';query.write_bytes(b''.join(rows));outputs=[];logs=[]
    for name in ['dispatch_native_x86','dispatch_owned_x64']:
        target=run/(name+'.bin');logs.append(execute(name,[IFM,query,target],name));outputs.append(target.read_bytes());require(len(outputs[-1])==len(rows)*104,'Dispatch output length')
    different=[i for i in range(len(rows)) if outputs[0][104*i:104*(i+1)]!=outputs[1][104*i:104*(i+1)]];details=[dict(case=i,metadata=metadata[i],native=struct.unpack_from('<26I',outputs[0],104*i),owned=struct.unpack_from('<26I',outputs[1],104*i)) for i in different[:6]]
    for i,core,wrapper in [(0,[-16,-16,0,16,16,72],[-16,-16,0,16,16,72]),(1,[9,-2,-3,11,2,3],[9,-2,-3,11,2,3]),(2,box,[99,-2,-3,101,2,3]),(3,[-1,-2,-3,11,2,3],[-1,-2,-3,11,2,3]),(4,[-16,-16,0,16,16,72],[84,-16,0,116,16,72]),(5,box,box)]:
        require(list(struct.unpack_from('<6f',outputs[0],i*104))==core and list(struct.unpack_from('<6f',outputs[0],i*104+52))==wrapper,'Analytical dispatch output case '+str(i))
    require(struct.unpack_from('<6I',outputs[0],6*104)==(0x80000000,)*6,'Native signed-zero retention witness')
    coverage=dict(default_or_unsupported=0,engine_fallback=0,animated_success=0,studio_sequence=0,lazy_load=0,child_adjust_called=0,wrapper_transform=0,first_bone_query=0)
    for i,m in enumerate(metadata):
        for phase in range(2):
            events=struct.unpack_from('<7I',outputs[0],i*104+phase*52+24);sync,animated,load,model,child,wrap,bone=events
            coverage['default_or_unsupported']+=not sync and not model;coverage['engine_fallback']+=bool(model);coverage['animated_success']+=bool(animated and not model and not child);coverage['studio_sequence']+=bool(sync and not model and child);coverage['lazy_load']+=bool(load);coverage['child_adjust_called']+=bool(child);coverage['wrapper_transform']+=bool(wrap);coverage['first_bone_query']+=bool(bone)
    safety=execute('bounds_safety_x64',[],'safety');require(all(coverage.values()),'Missing dispatcher branch coverage');require(sha(IFM)==IFM_SHA and all(sha(Path(m['path']))==m['sha256'] for m in models),'Original inputs changed')
    receipt=dict(status='PASS' if not different else 'FAIL',cases=len(rows),comparisons=len(rows)*2,output_bytes_per_case=104,differences=len(different),details=details,coverage=coverage,models=models,executables=executables,safety=safety.strip(),logs=logs,ifm_sha256=IFM_SHA,query_sha256=sha(query),static_byte_count=sum(b-a for _,a,b in RANGES),static_evidence=evidence,
        limits=['Complete original1023F310 dispatcher and10240520 wrapper execute with original sequence lookup, typed first-bone getter, quaternion matrix, union and affine bounds math',
                'Synchronization, animated-bounds outcome, lazy cache loader, studio readiness, engine-model bounds and child/wrapper transform providers are controlled',
                'Child-adjust callback runs original1049E820 with supplied matrix; original child-frame selection1023BCD0 and wrapper matrix construction1023B640 are not executed',
                'Classic local sequence tables only: virtual-model/include-model sequence remapping not executed',
                'First-bone direct-transform witness retained; an inverse is computed into unused native scratch and is not the matrix passed to bounds helper',
                'Read-only MDL metadata cases use controlled poses/settings; no original live DMX, asset search path, bone setup or rendering execution',
                'Native object cache pointer may be initialized by controlled loader; other input fields and sealed code checked unchanged',
                'Owned finite/format guards narrow malformed-input domain; no native corrupted-asset equivalence claimed',
                'No editor integration, original writes, application launches or full editor build'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(rows),comparisons=len(rows)*2,differences=len(different),details=details[:2],coverage=coverage,models=len(models),receipt=str(run/'results.json'))));require(not different,'Dispatcher/wrapper differs')


if __name__=='__main__':main()
