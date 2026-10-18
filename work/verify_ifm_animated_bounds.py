"""Complete original hitbox-bounds kernel; bone setup and live inputs controlled."""
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
OUT=ROOT/'outputs/ifm-animated-bounds-20261015'
RANGES=[('complete_animated_hitbox_bounds',0x1023ef30,0x1023f310),
        ('local_model_scale_accessor',0x1023eee0,0x1023ef26),
        ('bounds_transform',0x1049e820,0x1049ea1c),
        ('corrected_transpose_coordinate_inverse',0x1008bac0,0x1008bbb2),
        ('renderable_bounds_dispatch',0x1023f310,0x1023f699),
        ('renderable_bounds_wrapper',0x10240520,0x102405a3)]


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
        selected=[r for r in rows if a<=r['address']<b];cursor=a
        for row in selected:
            require(row['address']==cursor,'Disassembly gap '+label);raw=bytes.fromhex(row['bytes']);o=image.offset(cursor);require(image.b[o:o+len(raw)]==raw,'Stale bytes '+label);cursor+=len(raw)
        require(cursor==b,'Incomplete range '+label);evidence.append(dict(name=label,start=hex(a),end_exclusive=hex(b),bytes=b-a,instructions=selected))
    for address,bits in [(0x10aadbf4,0x7f7fffff),(0x10aae558,0xff7fffff),(0x10a8dfd4,0x38d1b717)]:require(image.word(address)==bits,'Native constant changed '+hex(address))
    return evidence


def main():
    require(sha(IFM)==IFM_SHA,'Original IFM changed');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));evidence=static_check();executables=[]
    for name,machine in [('bounds_native_x86',0x14c),('bounds_owned_x64',0x8664),('bounds_safety_x64',0x8664)]:
        path=OUT/(name+'.exe');b=path.read_bytes();pe=struct.unpack_from('<I',b,60)[0];require(b[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',b,pe+4)[0]==machine,'Executable architecture');executables.append(dict(executable=str(path),sha256=sha(path),machine=hex(machine)))
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=90);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' exit='+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout
    identity=[1,0,0,0,0,1,0,0,0,0,1,0];rows=[];labels=[];metadata=[];rng=random.Random(2026101519);f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
    def append(flags,tick,scale,world,bones,boxes,label):
        require(all(0<=b[0]<len(bones) for b in boxes),'Fixture bone range')
        rows.append(struct.pack('<IIIif12f',flags,len(bones),len(boxes),tick,scale,*world)+b''.join(struct.pack('<12f',*b) for b in bones)+b''.join(struct.pack('<i6f',*b) for b in boxes));labels.append(label);metadata.append(dict(flags=flags,bones=len(bones),boxes=len(boxes),tick=tick))
    box=(0,-2,-3,-4,2,3,4)
    append(127,50000,1,identity,[identity],[box],'identity')
    world=[2,0,0,10,0,2,0,20,0,0,2,30];bone=[2,0,0,16,0,2,0,28,0,0,2,40]
    append(127,50000,2,world,[bone],[box],'analytical scale correction')
    append(95,50000,1,identity,[identity],[box],'setup false filled')
    append(0,0,1,identity,[identity],[box],'no header')
    append(127,0,1,identity,[identity],[],'empty hitbox set')
    append(127,50000,1,identity,[identity]*256,[(255,*box[1:])],'last palette bone')
    for case in range(4096):
        flags=case%128;count=1+case%32;boxes=[(rng.randrange(count),*[rng.uniform(-32,0) for _ in range(3)],*[rng.uniform(0,32) for _ in range(3)]) for _ in range(case%15)]
        world=[rng.uniform(-2,2) for _ in range(12)];world[3],world[7],world[11]=[rng.uniform(-512,512) for _ in range(3)]
        bones=[[rng.uniform(-4,4) for _ in range(12)] for _ in range(count)]
        append(flags,rng.choice([-2147483648,-50000,0,50000,2147483647]),rng.choice([.125,.5,1,2,4,-.5,-1,-2]),world,bones,boxes,'controlled')
    # Independently read the same installed MDL49 fields that the checked C++ reader reads.
    models=[];game=IFM.parents[2]
    paths=[game/'tf/models/player'/(name+'.mdl') for name in ['soldier','pyro','scout','heavy','medic','engineer','sniper','spy','demo']]
    paths += [game/'tf_movies/models/player/hwm/pyro.mdl',game/'tf/models/player/hwm/pyro.mdl']
    for number,path in enumerate(paths):
        if not path.is_file():continue
        raw=path.read_bytes();require(raw[:4]==b'IDST' and struct.unpack_from('<i',raw,4)[0]==49,'MDL49 metadata');bones=struct.unpack_from('<i',raw,156)[0];sets,start=struct.unpack_from('<ii',raw,172);require(sets>0 and 0<=start<=len(raw)-12,'Actual hitbox set');count,relative=struct.unpack_from('<ii',raw,start+4);require(0<count<=256 and 0<=start+relative<=len(raw)-count*68,'Actual hitbox span');boxes=[struct.unpack_from('<i',raw,start+relative+i*68)+(struct.unpack_from('<6f',raw,start+relative+i*68+8)) for i in range(count)]
        dump=run/('mdl-'+str(number)+'.bin');execute('bounds_owned_x64',[path,dump],'mdl-'+str(number));expected=struct.pack('<II',bones,count)+b''.join(struct.pack('<i6f',*box) for box in boxes);require(dump.read_bytes()==expected,'Owned MDL reader differs from actual fields');models.append(dict(path=str(path),sha256=sha(path),bone_count=bones,first_set_boxes=count))
        for pose in range(16):
            world=identity.copy();scale=[.5,1,2,4][pose%4];world[0]=world[5]=world[10]=scale;world[3],world[7],world[11]=[rng.uniform(-2048,2048) for _ in range(3)];palette=[]
            for bone_id in range(bones):
                m=identity.copy();m[3],m[7],m[11]=[rng.uniform(-96,96) for _ in range(3)];palette.append(m)
            append(127,pose*417,scale,world,palette,boxes,path.name+' controlled pose')
    require(len(models)>=9,'Actual model metadata coverage')
    query=run/'queries.bin';query.write_bytes(b''.join(rows));outputs=[];logs=[]
    for name in ['bounds_native_x86','bounds_owned_x64']:
        target=run/(name+'.bin');logs.append(execute(name,[IFM,query,target],name));outputs.append(target.read_bytes());require(len(outputs[-1])==len(rows)*52,'Result size')
    different=[i for i in range(len(rows)) if outputs[0][52*i:52*(i+1)]!=outputs[1][52*i:52*(i+1)]];details=[dict(case=i,label=labels[i],metadata=metadata[i],native=struct.unpack_from('<13I',outputs[0],i*52),owned=struct.unpack_from('<13I',outputs[1],i*52)) for i in different[:6]]
    # Analytical cases have exact integer-valued expected bounds and setup contracts.
    for i,want in [(0,[-2,-3,-4,2,3,4]),(1,[2,2,2,10,14,18]),(2,[-2,-3,-4,2,3,4]),(5,[-2,-3,-4,2,3,4])]:
        values=struct.unpack_from('<I6fIIIfII',outputs[0],i*52);require(values[0]==1 and list(values[1:7])==want and values[7:10]==(1,256,256) and values[10]==5,'Analytical bounds/setup contract case '+str(i))
    for i in [3,4]:require(outputs[0][i*52:(i+1)*52]==struct.pack('<I6fIIIfII',0,11,13,17,19,23,29,0,0,0,0,0,0),'Early bypass changed outputs/callbacks')
    coverage=dict(success=0,early_bypass=0,ignored_false_setup=0,clock=0,scale_attribute=0,missing_or_invalid_model_scale_default=0)
    for i,m in enumerate(metadata):
        values=struct.unpack_from('<13I',outputs[0],i*52);success=bool(values[0]);coverage['success' if success else 'early_bypass']+=1
        if success:
            coverage['ignored_false_setup']+=not bool(m['flags']&32);coverage['clock']+=bool(m['flags']&64);coverage['scale_attribute']+=bool(values[12] and m['flags']&16);coverage['missing_or_invalid_model_scale_default']+=not bool((m['flags']&30)==30)
    safety=execute('bounds_safety_x64',[],'safety');require(all(coverage.values()),'Missing bounds coverage');require(sha(IFM)==IFM_SHA and all(sha(Path(m['path']))==m['sha256'] for m in models),'Original inputs changed')
    receipt=dict(status='PASS' if not different else 'FAIL',cases=len(rows),differences=len(different),details=details,coverage=coverage,models=models,executables=executables,safety=safety.strip(),logs=logs,static_byte_count=sum(b-a for _,a,b in RANGES),static_evidence=evidence,ifm_sha256=IFM_SHA,query_sha256=sha(query),
        limits=['Complete original1023EF30 hitbox-bounds routine and real inverse/composition/AABB math execute in private IFM image',
                'Original scale getter1023EEE0 and transform-handle accessor execute; datamodel/type/scale-attribute lookup controlled',
                'SetupBones palettes, renderable model matrix, clock and cached header data supplied by test; native animation not executed',
                'Actual MDL first-set metadata independently checked; all bone poses controlled, not live benchmark animation',
                'Checked reader supports MDL49, first set only and referenced bone indices0..255; other formats not claimed',
                'Outer1023F310 fallback dispatch, child-space transforms, hull/sequence bounds and full10240520 wrapper still incomplete',
                'Coordinate inverse preserves native geometric-mean correction/transpose behavior; no general inverse replacement',
                'No editor integration, original writes, apps, dependency changes or full editor rebuild'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(rows),differences=len(different),details=details[:2],coverage=coverage,models=len(models),receipt=str(run/'results.json'))));require(not different,'Animated hitbox bounds differ')


if __name__=='__main__':main()
