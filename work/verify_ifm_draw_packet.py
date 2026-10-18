"""Original IFM draw packet and Source lighting consumption, separate PE oracles."""
import json
from pathlib import Path
import random
import re
import struct
import subprocess
import sys
import tempfile
from pe_image import Image
from verify_session_illumination_dag import IFM,IFM_SHA,SESSION,SESSION_SHA
from verify_native_leaf_ambient_contract import ENGINE,DIGEST
from verify_dag_world_position import sha,require
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/ifm-draw-packet-20261015'
IFM_RANGES=[('packet_construction_and_optional_dispatch',0x10243a57,0x10243b8c),
 ('position_accessor',0x1023b6c0,0x1023b6c4),('angles_accessor',0x1023b6d0,0x1023b6d4),
 ('model_accessor',0x1012c420,0x1012c424),('instance_accessor',0x1023b7b0,0x1023b7b5),
 ('draw_basis_scale',0x1007fd50,0x1007fde1),('local_scale_getter',0x1023eee0,0x1023ef26),
 ('transform_getter',0x104c27d0,0x104c27e2),('Euler_matrix',0x104992b0,0x1049940d),
 ('Euler_position_matrix',0x1049d830,0x1049d85b),('model_interface_acquisition',0x102baff0,0x102bb00e)]
ENGINE_RANGES=[('default_illumination_point',0x100d2780,0x100d27f4),('affine_transform',0x10244180,0x10244221),
 ('origin_selection_and_material_virtual',0x100d5760,0x100d57fe),
 ('interface_registration',0x10308e90,0x10308ea5),('model_vtable_constructor_prefix',0x100d8d80,0x100d8d9c),
 ('slot16_draw_wrapper_static_only',0x100d69f0,0x100d6ae6),
 ('slot18_model_matrix_binding_static_only',0x100d67d0,0x100d683f),
 ('slot19_draw_lighting_call_static_only',0x100dabe0,0x100dafc4),
 ('uncached_query_branch_static_only',0x100d5a83,0x100d5b95)]


def check_ranges(path,disassembly,ranges):
    image=Image(path);rows=[];pending=None
    with (ROOT/'work'/disassembly).open() as f:
        for line in f:
            m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
            if m:
                pending=None;address=int(m[1],16)
                if any(a<=address<b for _,a,b in ranges):pending=dict(address=address,bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());rows.append(pending)
            elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
    evidence=[]
    for label,a,b in ranges:
        selected=[r for r in rows if a<=r['address']<b];cursor=a
        for row in selected:
            require(row['address']==cursor,'Disassembly gap '+label);raw=bytes.fromhex(row['bytes']);offset=image.offset(cursor);require(image.b[offset:offset+len(raw)]==raw,'Stale bytes '+label);cursor+=len(raw)
        require(cursor==b,'Incomplete range '+label);evidence.append(dict(name=label,start=hex(a),end_exclusive=hex(b),bytes=b-a,instructions=selected))
    return image,evidence


def main():
    require(sha(IFM)==IFM_SHA and sha(ENGINE)==DIGEST and sha(SESSION)==SESSION_SHA,'Pinned originals changed');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));smoke='--smoke' in sys.argv
    ifm,e1=check_ranges(IFM,'ifm-disassembly.txt',IFM_RANGES);engine,e2=check_ranges(ENGINE,'engine-disassembly.txt',ENGINE_RANGES)
    require(ifm.string(0x10ae74a8)=='VEngineModel016' and engine.string(0x10340d78)=='VEngineModel016','Interface name mismatch')
    require([ifm.word(0x10adb904+4*i) for i in [1,2,8,31]]==[0x1023b6c0,0x1023b6d0,0x1012c420,0x1023b7b0],'IFM accessor vtable mismatch')
    require([engine.word(0x1034086c+4*i) for i in [16,18,19]]==[0x100d69f0,0x100d67d0,0x100dabe0],'Engine interface vtable mismatch')
    # The broad disassembly starts one factory instruction inside preceding jump
    # table bytes. Verify its actual six bytes, not that misaligned instruction.
    offset=engine.offset(0x100d1650);factory=engine.b[offset:offset+6];require(factory==bytes.fromhex('b878b95c10c3'),'Engine factory singleton return')
    executables=[]
    for name,machine in [('packet_ifm_x86',0x14c),('packet_engine_x86',0x14c),('packet_owned_x64',0x8664),('packet_safety_x64',0x8664)]:
        path=OUT/(name+'.exe');raw=path.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine,'Executable architecture');require(b'asInvoker' in raw,'Non-elevated manifest');executables.append(dict(path=str(path),sha256=sha(path),machine=hex(machine)))
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' exit='+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout
    rows=[];metadata=[];models=[];rng=random.Random(2026101541)
    def append(mask=31,flags=1,skin=2,body=3,instance=7,mode=1,p=(10,20,30),a=(0,0,0),scale=2,origin=(7,8,9),local=(1,2,3),index=0,attachment=(1,0,0,100,0,1,0,200,0,0,1,300),label='controlled'):
        rows.append(struct.pack('<7I25f',mask,flags&0xffffffff,skin&0xffffffff,body&0xffffffff,instance,mode&0xffffffff,index&0xffffffff,*p,*a,scale,*origin,*local,*attachment));metadata.append(dict(mask=mask,flags=flags,mode=mode,index=index,label=label))
    append(label='analytical scaled explicit')
    append(mask=15,label='analytical scaled default')
    append(mask=15,scale=0,label='analytical zero scale retains translation')
    append(mask=15,scale=-2,label='analytical negative scale')
    append(mask=14,label='analytical missing model transform defaults scale1')
    append(mask=13,label='analytical unresolved transform defaults scale1')
    append(mask=11,label='analytical absent scale attribute defaults1')
    append(mode=0,label='analytical packet construction without dispatch')
    append(mask=15|32,index=34,label='analytical actual HWM index attachment success')
    append(mask=15,index=34,label='analytical attachment failure falls back')
    append(mask=31|32,index=34,label='analytical explicit wins but default query still occurs')
    if not smoke:
        for mask in range(64):
            for mode in [-2147483648,-1,0,1,2147483647]:
                for scale in [-2,-0.0,0.0,.125,1,1.0000001192092896,2]:append(mask=mask,mode=mode,scale=scale,flags=rng.getrandbits(32),skin=rng.getrandbits(32),body=rng.getrandbits(32),instance=rng.randrange(65536),label='all input-presence/scaling/dispatch states')
        for i in range(8192):append(mask=rng.randrange(64),flags=rng.getrandbits(32),skin=rng.getrandbits(32),body=rng.getrandbits(32),instance=rng.randrange(65536),mode=rng.choice([-1,0,1]),p=[rng.uniform(-8192,8192) for _ in range(3)],a=[rng.uniform(-36000,36000) for _ in range(3)],scale=rng.uniform(-16,16),origin=[rng.uniform(-8192,8192) for _ in range(3)],local=[rng.uniform(-128,128) for _ in range(3)],index=rng.choice([-2147483648,-1,0,1,34,257,2147483647]),label='varied finite packet')
        previous=json.loads((ROOT/'outputs/ifm-animated-bounds-20261015/verification-4io_ucwf/results.json').read_text())
        for m in previous['models']:
            path=Path(m['path']);require(sha(path)==m['sha256'],'MDL changed');raw=path.read_bytes();local=struct.unpack_from('<3f',raw,0x5c);extra=struct.unpack_from('<i',raw,0x190)[0];index=struct.unpack_from('<i',raw,extra+8)[0] if extra else 0
            models.append(dict(path=str(path),sha256=m['sha256'],illumination_point=local,attachment_index=index))
            for i in range(64):append(mask=i,local=local,p=[rng.uniform(-4096,4096) for _ in range(3)],a=[rng.uniform(-360,360) for _ in range(3)],scale=rng.choice([0,1,2,-2]),origin=[rng.uniform(-8192,8192) for _ in range(3)],index=index,label='actual MDL illumination metadata')
        prior=ROOT/'outputs/ifm-lighting-composite-20261015/verification-c8ig4w6e';r=json.loads((prior/'results.json').read_text());raw=(prior/'queries.bin').read_bytes();require(sha(prior/'queries.bin')==r['query_sha256'],'Origin corpus provenance');points=(prior/'lighting_native_x86.bin').read_bytes();offset=0;i=0
        while offset<len(raw):
            h=struct.unpack_from('<11I',raw,offset);values=struct.unpack_from('<49f',raw,offset+64);has=struct.unpack_from('<I',points,i*136)[0];origin=struct.unpack_from('<3f',points,i*136+4)
            # The actual getters established here bind pose38/44, not pose50/5C.
            root=struct.unpack_from('<III8f',raw,offset+260+h[5]*44);mask=1|(2 if root[2]&32 else 0)|(4 if root[2]&64 else 0)|(8 if h[0]&1 else 0)|(16 if has else 0)
            header2,index=struct.unpack_from('<Ii',raw,offset+56);mask|=32 if i%2 else 0
            append(mask=mask,p=values[18:21],a=values[21:24],scale=root[10],origin=origin,local=(1,2,3),mode=1,index=index if header2 else 0,label='previous original origin result and matching pose38/44/local-scale attributes')
            offset+=260+h[6]*44+h[7]*24+h[8]*48+h[9]*28;i+=1
        require(i==r['cases'] and offset==len(raw),'Origin fixture shape')
    query=run/'queries.bin';query.write_bytes(b''.join(rows));packets=[];logs=[]
    for name in ['packet_ifm_x86','packet_owned_x64']:
        target=run/(name+'.bin');logs.append(execute(name,['ifm',IFM,query,target],name));packets.append(target.read_bytes());require(len(packets[-1])==200*len(rows),'Packet output length')
    packet_differences=[i for i in range(len(rows)) if packets[0][i*200:(i+1)*200]!=packets[1][i*200:(i+1)*200]]
    consumed=[]
    for name in ['packet_engine_x86','packet_owned_x64']:
        target=run/('consume-'+name+'.bin');logs.append(execute(name,['engine',ENGINE,run/'packet_ifm_x86.bin',target],'consume-'+name));consumed.append(target.read_bytes());require(len(consumed[-1])==48*len(rows),'Consumer output length')
    consumer_differences=[i for i in range(len(rows)) if consumed[0][i*48:(i+1)*48]!=consumed[1][i*48:(i+1)*48]]
    details=[dict(stage='packet',case=i,metadata=metadata[i],native=struct.unpack_from('<50I',packets[0],i*200),owned=struct.unpack_from('<50I',packets[1],i*200)) for i in packet_differences[:3]]+[dict(stage='consumer',case=i,metadata=metadata[i],native=struct.unpack_from('<12I',consumed[0],i*48),owned=struct.unpack_from('<12I',consumed[1],i*48)) for i in consumer_differences[:3]]
    (run/'comparison.json').write_text(json.dumps(dict(packet_differences=len(packet_differences),consumer_differences=len(consumer_differences),details=details),indent=2)+'\n')
    for i,expected in enumerate([[7,8,9],[12,24,36],[10,20,30],[8,16,24],[11,22,33],[11,22,33],[11,22,33],[7,8,9],[101,202,303],[12,24,36],[7,8,9]]):require(list(struct.unpack_from('<3f',consumed[0],i*48+12))==expected,'Independent lighting point case '+str(i))
    coverage=dict(explicit=0,default=0,dispatch=0,no_dispatch=0,selected_scale=0,default_scale=0,attachment_success=0,attachment_fallback=0,no_attachment_query=0)
    for i,m in enumerate(metadata):
        p=packets[0][i*200:(i+1)*200];meta=struct.unpack_from('<14I',p,144);c=consumed[0][i*48:(i+1)*48]
        require(meta[3]==0 and meta[2]==1 and meta[6]==0xffffffff and meta[9]==0,'Independent packet invariants');require(meta[11]==bool(m['mode']),'Independent dispatch gate');require(struct.unpack_from('<I',c,36)[0]==1 and c[12:24]==c[24:36],'Original material-origin delivery')
        require(c[12:24]==(p[72:84] if m['mask']&16 else c[:12]),'Explicit point must pass unchanged; absent point uses default')
        require(struct.unpack_from('<II',c,40)==(int(m['index']>0),m['index']&0xffffffff if m['index']>0 else 0),'Independent attachment query')
        coverage['explicit' if m['mask']&16 else 'default']+=1;coverage['dispatch' if m['mode'] else 'no_dispatch']+=1;coverage['selected_scale' if m['mask']&7==7 else 'default_scale']+=1
        coverage['no_attachment_query' if m['index']<=0 else 'attachment_success' if m['mask']&32 else 'attachment_fallback']+=1
    require(all(coverage.values()),'Missing packet branch coverage');safety=execute('packet_safety_x64',[],'safety');require(sha(IFM)==IFM_SHA and sha(ENGINE)==DIGEST and sha(SESSION)==SESSION_SHA and all(sha(Path(m['path']))==m['sha256'] for m in models),'Original modified')
    receipt=dict(status='PASS' if not(packet_differences or consumer_differences) else 'FAIL',smoke=smoke,cases=len(rows),packet_comparisons=len(rows),consumer_comparisons=len(rows),packet_differences=len(packet_differences),consumer_differences=len(consumer_differences),details=details,coverage=coverage,executables=executables,logs=logs,safety=safety.strip(),ifm_sha256=IFM_SHA,engine_sha256=DIGEST,session_sha256=SESSION_SHA,models=models,query_sha256=sha(query),static_range_bytes=sum(e['bytes'] for e in e1+e2),static_evidence=dict(ifm=e1,engine=e2,factory_return_bytes=factory.hex()),
        limits=['Original IFM10243A57..10243B8B construction/optional slot16 dispatch executes, with original position/angle/model/instance getters and local scale getter/math; slot16 is a capture callback',
                'Engine original100D2780 and100D5760..100D57FD execute on reconstructed pointers to the normalized captured packet, including actual material-context slot150 call to a capture callback',
                'Two PE32 processes avoid same-preferred-base conflict; serialized normalized packet is a test bridge, not live engine factory interoperation',
                'VE... interface registration/vtables, normal slot16->18/19->lighting path and uncached cache-query branches checked statically only; setup, eligibility and cache dispatch not executed',
                'Owned packet uses native-width identity handles and value-owned matrix/optional point; it does not adopt original packed x86 layout',
                'Original object fields, attribute/datamodel resolution, dispatch mode and supplied poses are controlled; actual MDL illumination point and attachment index retained, attachment matrices/results supplied',
                'Prior origin corpus uses original conditional math on controlled/owned-evaluated inputs, not live original DMX/bone evaluation',
                'No editor integration, original write, application launch, dependency install or full editor build'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['status','smoke','cases','packet_differences','consumer_differences','details','coverage','safety']}));print('receipt='+str(run/'results.json'));require(receipt['status']=='PASS','Packet handoff differs')


if __name__=='__main__':main()
