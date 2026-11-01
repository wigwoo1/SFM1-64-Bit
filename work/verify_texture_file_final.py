"""Real native CPU file-reader return/conversion; controlled hardware, no GPU."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,itertools,json,re,struct,subprocess,sys,tempfile
from verify_session_preview_aspect import ROOT,VS
from verify_ifm_draw_packet import check_ranges
from verify_texture_file_image import text
OUT=ROOT/'outputs/texture-file-final-20261101'
LIB=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def size(w,h,fmt):return ((w+3)//4)*((h+3)//4)*8 if fmt==13 else w*h*(3 if fmt==3 else 4)
def unpack(raw,w,h,fmt):
    if fmt!=13:
        step=3 if fmt==3 else 4
        return [(raw[i+2],raw[i+1],raw[i],255 if fmt==3 else raw[i+3]) if fmt in [3,12] else tuple(raw[i:i+4]) for i in range(0,len(raw),step)]
    pixels=[None]*(w*h)
    for by in range((h+3)//4):
        for bx in range((w+3)//4):
            a,b,bits=struct.unpack_from('<HHI',raw,(by*((w+3)//4)+bx)*8)
            def color(c):
                r,g,bl=(c>>11)&31,(c>>5)&63,c&31
                return ((r<<3)|(r>>2),(g<<2)|(g>>4),(bl<<3)|(bl>>2),255)
            p=[color(a),color(b)]
            p+=[tuple((2*p[0][k]+p[1][k])//3 if a>b else (p[0][k]+p[1][k])//2 for k in range(3))+(255,),tuple((p[0][k]+2*p[1][k])//3 for k in range(3))+(255,) if a>b else (0,255,255,0)]
            for y in range(4):
                for x in range(4):
                    if bx*4+x<w and by*4+y<h:pixels[(by*4+y)*w+bx*4+x]=p[(bits>>(2*(y*4+x)))&3]
    return pixels
def convert(raw,w,h,source,target):
    if source==target:return raw
    pixels=unpack(raw,w,h,source)
    if target==0:return bytes(c for p in pixels for c in p)
    if target==3:return bytes(c for r,g,b,a in pixels for c in [b,g,r])
    if target==12:return bytes(c for r,g,b,a in pixels for c in [b,g,r,a])
    raise RuntimeError('unsupported independent conversion target')
def fixture(minor,w,h,frames,fmt,flags,seed,aux,sorted_resources=True):
    mips=max(w,h).bit_length();low=bytes((i*19+seed*7)%256 for i in range(16)) if aux else b''
    header=64 if minor==1 else 80 if minor==2 else 112 if aux else 88
    b=bytearray(header)
    for at,n in [(0,0x00465456),(4,7),(8,minor),(12,header),(20,flags),(52,fmt),(57,0 if aux else 0xffffffff)]:struct.pack_into('<I',b,at,n)
    for at,n in [(16,w),(18,h),(24,frames)]:struct.pack_into('<H',b,at,n)
    if minor>1:struct.pack_into('<H',b,63,1)
    b[56]=mips;b[61]=b[62]=2 if aux else 0
    payload=struct.pack('<I',8)+b'abcdefgh' if aux else b''
    if minor>=3:
        struct.pack_into('<I',b,68,4 if aux else 1)
        entries=[(1,header),(0x30,header+len(low)+len(payload)),(0x02435243,0x1234abcd),(0x00434241,header+len(low))] if aux else [(0x30,header)]
        # GetResourceData masks the inline flag and binary-searches the table.
        # Include one explicit unsorted fixture below to retain that observation.
        if sorted_resources:entries.sort(key=lambda e:e[0]&0xffffff)
        for i,e in enumerate(entries):struct.pack_into('<2I',b,80+i*8,*e)
    b.extend(low+payload);blocks={}
    for mip in reversed(range(mips)):
        for frame in range(frames):
            mw,mh=max(1,w>>mip),max(1,h>>mip)
            if fmt==13:
                a,c=(0xf800,0x07e0) if seed%2==0 else (0x07e0,0xf800)
                raw=b''.join(struct.pack('<HHI',a,c,(0xe4e4e4e4+frame*0x11111111)&0xffffffff) for _ in range(((mw+3)//4)*((mh+3)//4)))
            else:
                raw=bytearray()
                for i in range(mw*mh):
                    r,g,bl,alpha=(i*37+mip*53+frame*71+seed)%256,(i*19+11)%256,(i*67+97)%256,(i*23+frame*41)%256
                    raw.extend([bl,g,r] if fmt==3 else [bl,g,r,alpha] if fmt==12 else [r,g,bl,alpha])
                raw=bytes(raw)
            b.extend(raw);blocks[mip,frame]=raw
    resources=(struct.pack('<3I',0x00434241,8,8)+b'abcdefgh' if sorted_resources else b'')+struct.pack('<3I',0x02435243,0x1234abcd,0)
    auxiliary=struct.pack('<3I',2,2,2 if sorted_resources else 1)+b''.join(low[i:i+3] for i in range(0,len(low),4))+resources if aux else bytes(12)
    return b,blocks,auxiliary
def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));(OUT/'active-run.txt').write_text(str(run));print('Verification directory: '+str(run),flush=True)
    idx=json.loads((ROOT/'outputs/texture-format-policy-20261101/latest-verification.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'accepted policy receipt');prior=json.loads(Path(idx['receipt']).read_text());sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256']
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'preceding identities')
    for n in ['probe_texture_file_final.cpp',Path(__file__).name]:sources[str(ROOT/'work'/n)]=sha(ROOT/'work'/n)
    _,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('whole_reader',0x1002fdb0,0x100301cf),('debug_override_dispatch',0x1002ee40,0x1002ef0f),('real_VTF_conversion',0x10079140,0x10079408),('whole_nearest_request',0x1002c800,0x1002c896),('whole_conversion_decision',0x1002d070,0x1002d129),('conversion_CPU',0x10048ec0,0x100496c0)])
    _,more=check_ranges(LIB,'materialsystem-disassembly.txt',[('masked_resource_binary_search',0x1007b4c0,0x1007b526),('DXT1_native_palette',0x10042c60,0x10042e4d)])
    ranges+=more
    rows=[];fixtures=run/'fixtures';fixtures.mkdir()
    def add(minor=2,w=8,h=8,frames=1,fmt=0,flags=0,request=0,target=3,enabled=1,compression=0,picmip=0,aux=False,seed=0,sorted_resources=True):
        b,blocks,auxiliary=fixture(minor,w,h,frames,fmt,flags,seed,aux,sorted_resources);serial=len(rows);p=fixtures/(str(serial)+'.vtf');p.write_bytes(b)
        config=[request,picmip,64,64,1,0,1,target,enabled,0,compression]
        rows.append(dict(serial=serial,minor=minor,w=w,h=h,frames=frames,fmt=fmt,flags=flags,request=request,target=target,enabled=enabled,compression=compression,picmip=picmip,source=str(p),sha256=sha(p),blocks=blocks,auxiliary=auxiliary,config=config))
    add()
    if '--smoke' not in sys.argv:
        for minor,fmt,target,flags in itertools.product(range(1,6),[0,3,12],[0,3,12],[0,0x1000,0x2000,0x3000]):add(minor=minor,fmt=fmt,target=target,flags=flags)
        for fmt,target,enabled,frames,(w,h),picmip in itertools.product([0,12,13],[0,3,12],[0,1,0x100,0x101],[1,2],[(4,8),(16,16)],[0,1]):add(minor=5,fmt=fmt,target=target,enabled=enabled,frames=frames,w=w,h=h,picmip=picmip,seed=1)
        for flags,request,fmt,target in itertools.product([0,0x800,0x20000],[0,0x1000,0x2000],[0,12,13],[3,12]):add(minor=5,fmt=fmt,target=target,flags=flags,request=request,aux=True)
        for minor,compression,enabled in itertools.product(range(1,6),[0,1,0x100,0x101],[0,1]):add(minor=minor,fmt=13,target=13,compression=compression,enabled=enabled)
        add(minor=5,aux=True,sorted_resources=False)
    q=run/'queries.bin';q.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<11I',*r['config'])+text('materials/fixture/final.vtf')+text('fixture/final')+text(r['source']) for r in rows))
    batch=run/'build.cmd';batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS.parent/"vcvars32.bat"}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_texture_file_final.cpp /Fo:"{run/"native.obj"}" /Fe:"{run/"native.exe"}" >"{run/"build.log"}" 2>&1','if errorlevel 1 exit /b 1','exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1800:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/'build.log').read_text());need(not warnings,'compiler warnings');exe=run/'native.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(struct.unpack_from('<H',raw,pe+4)[0]==0x14c,'private reference x86')
    observations=[];traces=[];converted=0
    for repeat in range(2):
        dest=run/('native-'+str(repeat)+'.bin');execute([exe,LIB,q,dest],'native-'+str(repeat));data=dest.read_bytes();traces.append(data);at=0;converted=0
        for r in rows:
            record=list(struct.unpack_from('<24I',data,at));at+=96;strings=[]
            for _ in range(3):n=struct.unpack_from('<I',data,at)[0];at+=4;strings.append(data[at:at+n]);at+=n
            trace,pixels,auxiliary=strings;effective=(r['flags']&0x2e87fbff if r['minor']<=3 else r['flags']);textureflags=effective|r['request'];enabled=bool(r['enabled']&255);target=r['target'] if enabled else r['fmt'];conversion=target!=r['fmt'];skip=0
            if not textureflags&0x200:
                for _ in range(r['picmip']):
                    if (r['w']>>skip)>4 and (r['h']>>skip)>4:skip+=1
            w,h=max(1,r['w']>>skip),max(1,r['h']>>skip);mips=max(w,h).bit_length();vtfflags=((effective&~0x1000)|0x2000) if conversion and target in [0,12] else (effective&~0x3000) if conversion else effective
            expected=[r['serial'],1,2,1,textureflags,0,target,r['w'],r['h'],w,h,mips,r['frames'],skip,w,h,target,vtfflags,r['frames'],mips,int(conversion),int(conversion),int(enabled),int(enabled and not conversion)]
            expected_pixels=b''.join(struct.pack('<I',size(max(1,w>>m),max(1,h>>m),target))+convert(r['blocks'][m+skip,f],max(1,w>>m),max(1,h>>m),r['fmt'],target) for m in range(mips) for f in range(r['frames']))
            if record!=expected or pixels!=expected_pixels or auxiliary!=r['auxiliary']:
                (run/'mismatch.json').write_text(json.dumps({'row':{k:v for k,v in r.items() if k not in ['blocks','auxiliary']},'record':record,'expected':expected,'trace':trace.decode(),'actual_pixels':list(pixels),'expected_pixels':list(expected_pixels),'auxiliary_equal':auxiliary==r['auxiliary']},indent=2));raise RuntimeError('native full file state/pixels mismatch case '+str(r['serial']))
            steps=trace.decode().splitlines();need(steps.count('M '+str(r['enabled']))==1 and len([s for s in steps if s.startswith('FORMAT ')])==int(enabled),'conversion gate/service count')
            requested=r['fmt'] if r['fmt']==13 and r['compression']&255 else 12 if textureflags&0x3000 else 3
            if enabled:need('FORMAT '+str(requested)+' 1 '+str(target) in steps,'actual request selected by native policy')
            need(steps.count('SCRATCH FREE')==int(conversion),'scratch retirement')
            if conversion:need('SCRATCH ALLOC '+str(sum(size(max(1,w>>m),max(1,h>>m),target) for m in range(mips))*r['frames']) in steps,'scratch full image extent')
            need(steps.count('X')==1 and steps.index('X')<steps.index('M '+str(r['enabled'])),'file closes before conversion')
            source=Path(r['source']).read_bytes();header=struct.unpack_from('<I',source,12)[0]
            wanted=len(source)-sum(size(max(1,r['w']>>m),max(1,r['h']>>m),r['fmt']) for m in range(skip))*r['frames']
            expected_steps=['P GAME','O materials/fixture/final.vtf rb 0 GAME','A 0','R 2097152 336 '+str(min(336,len(source))),'S '+str(header),'OVERRIDE MISS','OVERRIDE MISS','HW W','HW H','HW D','S 0','A 0','R 2097152 '+str(wanted)+' '+str(wanted),'X','M '+str(r['enabled'])]
            if enabled:expected_steps+=['FORMAT '+str(requested)+' 1 '+str(target)]
            if conversion:expected_steps+=['SCRATCH ALLOC '+str(sum(size(max(1,w>>m),max(1,h>>m),target) for m in range(mips))*r['frames']),'SCRATCH FREE']
            elif enabled:expected_steps+=['HDR 0']
            need(steps==expected_steps,'complete ordered file/service/scratch trace case '+str(r['serial']))
            converted+=int(conversion)
            if repeat==0:observations.append(dict(serial=r['serial'],record=record,trace=steps,pixel_bytes=len(pixels),pixel_sha256=hashlib.sha256(pixels).hexdigest()))
        need(at==len(data),'trace framing')
    need(traces[0]==traces[1],'full repeated trace');before=sha(q);p=execute([exe,LIB,q,q],'existing-output',False);need(p.returncode!=0 and sha(q)==before,'existing output preserved');need(all(sha(r['source'])==r['sha256'] for r in rows),'input fixtures preserved')
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'final identities')
    (run/'observations.json').write_text(json.dumps(observations,indent=2));result=dict(status='VERIFIED bounded real native file-reader CPU return and conversion; owned final-image bridge/GPU not implemented',time_utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='PROGRESS',cases=len(rows),converted=converted,repetitions=2,trace_bytes=len(traces[0]),trace_sha256=sha(run/'native-0.bin'),executable={'path':str(exe),'sha256':sha(exe),'architecture':'private x86 reference'},compiler_warnings=warnings,authenticated_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in ranges],source_sha256=sources,protected_sha256=protected,limits=['Original whole CPU file reader returns naturally after real VTF conversion. Guards, file close, scratch allocator/free LIFO, pixel bytes and auxiliary data checked. Hardware/enable/compression are explicit supplied services; debug override settings805/806=0.','Formats0/3/12 and DXT1 input, conversion targets0/3/12 and DXT1 preservation. Other compressed formats, HDR24/25 roundtrip, debug transforms, malformed files, error resources, GPU upload and native teardown remain unverified.','This is a private reference test, not an x64 implementation of the complete reader. Owned x64 policy/CPU leases remain separate. No production editor source/build/runtime, original files/process or publication changed.'])
    result['coverage']={'source_format_cases':{str(f):sum(r['fmt']==f for r in rows) for f in [0,3,12,13]},'selected_target_cases':{str(f):sum(r['target']==f for r in rows) for f in [0,3,12,13]},'two_frame_cases':sum(r['frames']==2 for r in rows),'picmip_requested_cases':sum(r['picmip']>0 for r in rows),'resource_ordering_negative_cases':int('--smoke' not in sys.argv)}
    result['limits'].append('Native resource lookup binary-searches types masked to24 bits. Combined auxiliary fixtures are sorted on that key. One deliberately unsorted table reproduces the native missing custom chunk both before and after conversion; this is not changed or silently treated as supported owned input.')
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n')
    if '--smoke' not in sys.argv:(OUT/'latest-verification.json').write_text(json.dumps({'receipt':str(receipt),'receipt_sha256':sha(receipt)},indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ['source_sha256','protected_sha256','authenticated_ranges','limits']},indent=2),flush=True)
if __name__=='__main__':main()
