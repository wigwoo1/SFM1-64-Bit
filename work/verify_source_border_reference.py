"""Independent System D3D9 measurements for normal-draw border references.
No production precision/decoder adjustment and no tolerance expansion.
"""
from pathlib import Path
import itertools,json,struct
from verify_source_border_common import ROOT,OUT,sha,need,put
from verify_vulkan_prepared_texture_binding import fixture,pixel

def measure(rows,run,execute):
    idx=json.loads((OUT/'latest-cross-api-verification.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'cross-API measurement identity');proof=json.loads(Path(idx['receipt']).read_text());exe=proof['executables']['d3d9'];need(sha(exe['path'])==exe['sha256'],'tested owned D3D9 executable');p=ROOT/'work/probe_texture_sampling_d3d9_x64.cpp';need(sha(p)==proof['source_sha256'][str(p)],'tested D3D9 source')
    directory=run/'independent-d3d9';directory.mkdir();files={}
    for fmt in [0,3,12,13,15]:
        for prepared in [False,True]:
            # Fallback is the decoded RGBA route. Prepared BC keeps authored BC;
            # RGB/BGRA semantic RGBA values are independently authored controls.
            if prepared and fmt in [0,13,15]:b=fixture(fmt)
            else:
                b=bytearray(fixture(0)[:88])
                for mip in reversed(range(4)):
                    w=max(1,8>>mip)
                    for frame in range(2):
                        for y in range(w):
                            for x in range(w):b.extend(pixel(fmt,frame,mip,x,y))
            p=directory/f'reference-{fmt}-{int(prepared)}.vtf';p.write_bytes(b);files[(fmt,prepared)]=p
    groups={};lookups=[]
    for row in rows:
        q=row['config'];fmt,frame,bm,full,srgb,channel,lodbits,sm,presence,ubits,vbits=q;lod=struct.unpack('<f',struct.pack('<I',lodbits))[0];u,v=[struct.unpack('<f',struct.pack('<I',n))[0] for n in [ubits,vbits]];refs=[]
        for fl,source_mip,bit,selected_frame in [(row['flags'][0],bm,1,frame),(row['flags'][1],sm,2,1-frame)]:
            prepared=bool(presence&bit);base=min(source_mip if prepared else bm,3);chain=bool((3 if full==4 else full)&bit) and prepared and not(fl&(1|0x100));linear=not(fl&1) if prepared else True;border=prepared and bool(fl&0x20000000);cu=3 if border else 2 if prepared and fl&4 else 0;cv=3 if border else 2 if prepared and fl&8 else 0
            config=(srgb,int(linear),int(linear),int(chain),cu,cv,0,0xffffffff);path=files[(fmt,prepared)];key=(str(path),config);g=groups.setdefault(key,{'path':str(path),'config':config,'points':[],'index':{}});point=(selected_frame,u,v,min(3,base+max(0,lod)) if chain else float(base))
            if point not in g['index']:g['index'][point]=len(g['points']);g['points'].append(point)
            refs.append((key,g['index'][point]))
        lookups.append(refs)
    items=list(groups.items());values={};commands=[]
    def text(s):
        b=str(s).encode('ascii');return struct.pack('<I',len(b))+b
    for a in range(0,len(items),128):
        batch=items[a:a+128];query=directory/f'query-{a}.bin';query.write_bytes(struct.pack('<I',len(batch))+b''.join(struct.pack('<8I',*g['config'])+text('fixture/normal-border-reference')+text(g['path'])+struct.pack('<I',len(g['points']))+b''.join(struct.pack('<I3f',*p) for p in g['points']) for _,g in batch));output=directory/f'gpu-{a}';args=[exe['path'],query,output,*proof['same_gpu_vendor_device']];log=execute(args,'normal-reference-'+str(a)).stdout;need('d3d9_pixel_border_defaults_checked=16 value=0 no_border_color_writes=1' in log,'actual D3D9 defaults');commands.append({'command':list(map(str,args)),'log':log})
        b=(output/'samples.bin').read_bytes();at=0
        for i,(key,g) in enumerate(batch):
            n=len(g['points']);need(struct.unpack_from('<2I',b,at)==(i,n),'independent measurement framing');at+=8;values[key]=[struct.unpack_from('<4f',b,at+16*k) for k in range(n)];at+=n*16
        need(at==len(b),'complete independent measurement')
    changes=0;maximum=0.
    for row,refs in zip(rows,lookups):
        q=row['config'];u,v=[struct.unpack('<f',struct.pack('<I',n))[0] for n in q[9:11]];base,second=[values[key][n] for key,n in refs]
        # Existing orthogonal integer-code mask ramp remains an independent CPU
        # expression, separate from the newly measured texture channels.
        mask=[0.]*4;colors=[[64,64,128,255],[192,64,128,255],[64,192,128,255],[192,192,128,255]]
        import math
        x,y=u*2-.5,v*2-.5;ix,iy=math.floor(x),math.floor(y);fx,fy=x-ix,y-iy
        for dx,dy in itertools.product(range(2),repeat=2):
            c=colors[((iy+dy)%2)*2+(ix+dx)%2];weight=(fx if dx else 1-fx)*(fy if dy else 1-fy);mask=[a+weight*b/255 for a,b in zip(mask,c)]
        expected=base if q[5]==0 else mask if q[5]==1 else second if q[5]==2 else [a*b+.1*c for a,b,c in zip(base,second,mask)]
        ideal=row['expected'];maximum=max(maximum,max(abs(a-b) for a,b in zip(ideal,expected)));changes+=int(any(abs(a-b)>t for a,b,t in zip(ideal,expected,row['tolerance'])));row['rejected_ideal_expected']=ideal;row['expected']=expected
    proof={'method':'Independent owned x64 Windows D3D9 HAL shader measurements of same authored texels/format, selected frame, absolute mip and address/filter policy; CPU composition and mask ramp. Existing linear tolerance unchanged2e-5. Not original shader execution or universal device precision claim.','executable':exe,'gpu_receipt':idx,'groups':len(groups),'actual_samples':sum(len(g['points']) for g in groups.values()),'border_rows':len(rows),'ideal_rows_over_unchanged_budget':changes,'maximum_ideal_difference':maximum,'commands':commands,'fixture_sha256':{str(p):sha(p) for p in files.values()}}
    need(changes>0,'original ideal-reference rejection retained');put(directory/'reference.json',proof);return proof
