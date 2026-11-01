"""Native thumbnail conversion/resource publication before GPU/format conversion."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,itertools,json,random,re,struct,subprocess,sys,tempfile
from verify_session_preview_aspect import ROOT,VS
from verify_texture_file_image import fixture,size,text
from verify_ifm_draw_packet import check_ranges
OUT=ROOT/'outputs/texture-file-postprocess-20261101'
LIB=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def decode_low(data,w,h,fmt):
    if not w:return b''
    if fmt in [0,12]:return b''.join(bytes((data[i+2],data[i+1],data[i])) if fmt==12 else data[i:i+3] for i in range(0,len(data),4))
    rgb=bytearray(w*h*3)
    for by in range((h+3)//4):
        for bx in range((w+3)//4):
            a,b,bits=struct.unpack_from('<HHI',data,(by*((w+3)//4)+bx)*8)
            def color(v):
                r,g,bl=(v>>11)&31,(v>>5)&63,v&31
                return [(r<<3)|(r>>2),(g<<2)|(g>>4),(bl<<3)|(bl>>2)]
            p=[color(a),color(b)];p+=[[(2*p[0][k]+p[1][k])//3 if a>b else (p[0][k]+p[1][k])//2 for k in range(3)],[(p[0][k]+2*p[1][k])//3 if a>b else 0 for k in range(3)]]
            if a<=b:p[3]=[0,255,255] # Native RGB thumbnail transparent selector.
            for y in range(4):
                for x in range(4):
                    xx,yy=bx*4+x,by*4+y
                    if xx<w and yy<h:rgb[(yy*w+xx)*3:(yy*w+xx+1)*3]=bytes(p[(bits>>(2*(y*4+x)))&3])
    return bytes(rgb)
def make(minor,w,h,lowfmt,frames,highfmt,extras,seed):
    original,main=fixture(minor,8,8,frames,highfmt,0,4);oldheader=struct.unpack_from('<I',original,12)[0];high=original[oldheader:]
    rng=random.Random(seed);low=b''
    if w:
        if lowfmt in [0,12]:low=bytes(rng.randrange(256) for _ in range(w*h*4))
        else:low=b''.join(struct.pack('<HHI',0xf800 if seed==0 else rng.randrange(65536),0x07e0 if seed==0 else rng.randrange(65536),0xe4e4e4e4 if seed==0 else rng.getrandbits(32)) for _ in range(((w+3)//4)*((h+3)//4)))
    resources=[]
    if 'crc' in extras:resources.append((0x02435243,struct.pack('<I',0x1234abcd)))
    if 'lod' in extras:resources.append((0x02444f4c,struct.pack('<I',0x0202)))
    if 'chunk4' in extras:resources.append((0x00434241,b'abcd'))
    if 'chunk8' in extras:resources.append((0x00434241,b'abcdefgh'))
    header=64 if minor==1 else 80 if minor==2 else 80+8*(1+bool(w)+len(resources))
    b=bytearray(original[:oldheader]);b=b[:header]+bytes(max(0,header-len(b)));struct.pack_into('<I',b,12,header);struct.pack_into('<I',b,57,lowfmt if w else 0xffffffff);b[61]=w;b[62]=h
    resource_payload=bytearray();entries=[];expected_resources=bytearray()
    for typ,payload in resources:
        if typ&0x02000000:entries.append((typ,struct.unpack('<I',payload)[0]));value=struct.unpack('<I',payload)[0];expected_resources.extend(struct.pack('<3I',typ,value,0))
        else:
            entries.append((typ,header+len(low)+len(resource_payload)));resource_payload.extend(struct.pack('<I',len(payload))+payload)
            if len(payload)==4:expected_resources.extend(struct.pack('<3I',typ|0x02000000,struct.unpack('<I',payload)[0],0))
            else:expected_resources.extend(struct.pack('<3I',typ,len(payload),len(payload))+payload)
    high_at=header+len(low)+len(resource_payload)
    if minor>=3:
        entries=([(1,header)] if w else [])+[(0x30,high_at)]+entries;struct.pack_into('<I',b,68,len(entries))
        for i,e in enumerate(entries):struct.pack_into('<2I',b,80+i*8,*e)
    need(minor>=3 or not resources,'legacy resource fixture')
    b.extend(low+resource_payload+high)
    return b,main,decode_low(low,w,h,lowfmt),bytes(expected_resources)
def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));(OUT/'active-run.txt').write_text(str(run));print('Verification directory: '+str(run),flush=True)
    idx=json.loads((ROOT/'outputs/material-texture-cpu-image-20261101/latest-verification.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'accepted CPU bridge receipt');prior=json.loads(Path(idx['receipt']).read_text());sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256']
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'preceding identities')
    for name in ['probe_texture_file_postprocess.cpp','material_texture_thumbnail_candidate.h','probe_material_texture_thumbnail.cpp',Path(__file__).name]:sources[str(ROOT/'work'/name)]=sha(ROOT/'work'/name)
    image,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('postprocess_to_safe_stop',0x10030097,0x1003017d),('thumbnail_publication',0x1002cec0,0x1002cf6a),('conversion_dispatch',0x10048ec0,0x100496c0),('VTF_resource_types',0x1007b210,0x1007b25b),('VTF_resource_data',0x1007bc00,0x1007bc70)])
    _,extra_ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('DXT1_palette',0x10042c60,0x10042e4d),('DXT1_RGB_conversion',0x10047af0,0x10047c57)])
    ranges+=extra_ranges
    _,sizing_ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('native_compressed_size',0x10049790,0x10049842)])
    ranges+=sizing_ranges
    rows=[];coverage={};fixtures=run/'fixtures';fixtures.mkdir()
    def add(minor=2,w=4,h=4,lowfmt=0,frames=1,highfmt=0,extras='',seed=0,label='controlled'):
        data,main,thumbnail,resources=make(minor,w,h,lowfmt,frames,highfmt,extras,seed);serial=len(rows);p=fixtures/(str(serial)+'.vtf');p.write_bytes(data);rows.append(dict(serial=serial,minor=minor,w=w,h=h,lowfmt=lowfmt,frames=frames,highfmt=highfmt,extras=extras,seed=seed,source=str(p),sha256=sha(p),main=main,thumbnail=thumbnail,resources=resources));coverage[label]=coverage.get(label,0)+1
    add(w=0,h=0);add()
    if '--smoke' not in sys.argv:
        for minor in range(1,6):add(minor=minor,w=0,h=0,label='absent_thumbnail')
        for minor,(w,h),lowfmt,frames,highfmt in itertools.product(range(1,6),[(2,2),(4,4),(8,4),(8,8),(16,16),(3,5)],[0,12,13],[1,2],[0,13]):
            if lowfmt==13 and (w,h)==(3,5):continue # Explicit owned-negative family below.
            add(minor,w,h,lowfmt,frames,highfmt,label='version_thumbnail_frame_format_matrix')
        for minor,extras,lowfmt in itertools.product(range(3,6),['crc','lod','crclod','chunk4','chunk8'],[0,12,13]):add(minor=minor,extras=extras,lowfmt=lowfmt,label='native_auxiliary_resources')
        for seed in range(1,65):add(minor=5,lowfmt=13,seed=seed,label='DXT1_endpoint_selector_patterns')
    q=run/'queries.bin';q.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<7I',0,0,64,64,1,0,1)+text('materials/fixture/post.vtf')+text('fixture/post')+text(r['source']) for r in rows))
    batch=run/'build.cmd';batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS.parent/"vcvars32.bat"}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_texture_file_postprocess.cpp /Fo:"{run/"native.obj"}" /Fe:"{run/"native.exe"}" >"{run/"native-build.log"}" 2>&1','if errorlevel 1 exit /b 1','exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1600:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/'native-build.log').read_text());need(not warnings,'compiler warnings')
    exe=run/'native.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(struct.unpack_from('<H',raw,pe+4)[0]==0x14c,'private x86 reference')
    observations=[];traces=[];owned_expected=None
    for repeat in range(2):
        dest=run/('native-'+str(repeat)+'.bin');execute([exe,LIB,q,dest],'native-'+str(repeat));data=dest.read_bytes();traces.append(data);at=0
        expected_owned=bytearray()
        for r in rows:
            record=list(struct.unpack_from('<28I',data,at));at+=112;strings=[]
            for _ in range(3):n=struct.unpack_from('<I',data,at)[0];at+=4;strings.append(data[at:at+n]);at+=n
            post=list(struct.unpack_from('<3I',data,at));at+=12
            for _ in range(2):n=struct.unpack_from('<I',data,at)[0];at+=4;strings.append(data[at:at+n]);at+=n
            filename,trace,pixels,thumb,resources=strings;skip=1 if 'lod' in r['extras'] else 0
            expected_pixels=b''.join(struct.pack('<I',len(r['main'][m,f]))+r['main'][m,f] for m in range(skip,4) for f in range(r['frames']))
            expected_count=sum(k in r['extras'] for k in ['crc','lod','chunk4','chunk8'])
            dim=8>>skip
            expected_record=[r['serial'],1,1,2,1,0,0,r['highfmt'],8,8,1,dim,dim,1,4-skip,r['frames'],skip,0x3e800000,0x3f000000,0x3f400000,1,dim,dim,1,r['highfmt'],r['frames'],0,4-skip]
            if record!=expected_record or post!=[r['w'],r['h'],expected_count] or pixels!=expected_pixels or thumb!=r['thumbnail'] or resources!=r['resources']:
                (run/'mismatch.json').write_text(json.dumps({'row':{k:v for k,v in r.items() if k not in ['main','thumbnail','resources']},'record':record,'post':post,'expected_post':[r['w'],r['h'],expected_count],'trace':trace.decode('ascii'),'actual_thumbnail':list(thumb),'expected_thumbnail':list(r['thumbnail']),'actual_resources':list(resources),'expected_resources':list(r['resources']),'pixels_equal':pixels==expected_pixels},indent=2));raise RuntimeError('postprocess mismatch case '+str(r['serial']))
            need(record[2:5]==[1,2,1] and trace.endswith(b'CPU POSTPROCESS ACCEPTED\n'),'complete file lifecycle')
            source=Path(r['source']).read_bytes();header=struct.unpack_from('<I',source,12)[0];wanted=len(source)-sum(len(r['main'][0,f]) for f in range(r['frames']))*skip
            expected_trace=['P GAME','O materials/fixture/post.vtf rb 0 GAME','A 0','R 2097152 336 '+str(min(336,len(source))),'S '+str(header),'OVERRIDE MISS','OVERRIDE MISS','HW W','HW H','HW D','S 0','A 0','R 2097152 '+str(wanted)+' '+str(wanted),'X','CPU POSTPROCESS ACCEPTED']
            need(filename==b'materials/fixture/post.vtf' and [s for s in trace.decode('ascii').splitlines() if s!='W']==expected_trace,'ordered native trace')
            available='chunk' not in r['extras'];expected_owned.extend(struct.pack('<2I',r['serial'],int(available)))
            if available:expected_owned.extend(struct.pack('<3I',r['w'],r['h'],len(thumb))+thumb)
            if repeat==0:observations.append(dict(serial=r['serial'],post=post,thumbnail_bytes=len(thumb),thumbnail_sha256=hashlib.sha256(thumb).hexdigest(),resource_bytes=len(resources)))
        need(at==len(data),'trace framing')
        if owned_expected is None:owned_expected=bytes(expected_owned)
        else:need(owned_expected==expected_owned,'repeated owned prediction')
    need(traces[0]==traces[1],'complete repeat trace');before=sha(q);p=execute([exe,LIB,q,q],'existing-output',False);need(p.returncode!=0 and sha(q)==before,'existing output preserved')
    owned_batch=run/'owned-build.cmd';owned_batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_material_texture_thumbnail.cpp /Fo:"{run/"owned.obj"}" /Fe:"{run/"owned.exe"}" >"{run/"owned-build.log"}" 2>&1','if errorlevel 1 exit /b 1','exit /b 0','']))
    execute(['cmd.exe','/d','/c',owned_batch],'owned-build-driver');owned=run/'owned.exe';owned_warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/'owned-build.log').read_text());need(not owned_warnings,'owned warnings');raw=owned.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(struct.unpack_from('<H',raw,pe+4)[0]==0x8664 and struct.unpack_from('<H',raw,pe+24)[0]==0x20b,'owned AMD64 PE32+')
    owned_logs=[]
    for repeat in range(2):
        target=run/('owned-'+str(repeat)+'.bin');p=execute([owned,q,target],'owned-'+str(repeat));need(target.read_bytes()==owned_expected,'owned exact thumbnail comparison');owned_logs.append(p.stdout)
    before=sha(q);p=execute([owned,q,q],'owned-existing-output',False);need(p.returncode!=0 and sha(q)==before,'owned existing output preservation')
    negative_rows=[]
    for minor,(w,h) in itertools.product([1,5],[(3,5),(5,5),(6,4)]):
        b,_,_,_=make(minor,w,h,13,1,0,'',0);source=fixtures/('negative-'+str(len(negative_rows))+'.vtf');source.write_bytes(b);negative_rows.append(source)
    negative=run/'owned-negative-query.bin';negative.write_bytes(struct.pack('<I',len(negative_rows))+b''.join(struct.pack('<7I',0,0,64,64,1,0,1)+text('materials/fixture/invalid.vtf')+text('fixture/invalid')+text(p) for p in negative_rows))
    target=run/'owned-negative.bin';p=execute([owned,negative,target],'owned-negative');need(target.read_bytes()==b''.join(struct.pack('<2I',i,0) for i in range(len(negative_rows))),'nonaligned compressed thumbnails refused')
    need(all(sha(r['source'])==r['sha256'] for r in rows),'fixture preservation')
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'final identities')
    (run/'observations.json').write_text(json.dumps(observations,indent=2));result=dict(status='VERIFIED bounded native thumbnail/resource publication; later conversion/GPU unverified',time_utc=datetime.now(timezone.utc).isoformat(),cases=len(rows),coverage=coverage,repetitions=2,trace_bytes=len(traces[0]),trace_sha256=sha(run/'native-0.bin'),executable={'path':str(exe),'sha256':sha(exe)},compiler_warnings=warnings,authenticated_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in ranges],source_sha256=sources,protected_sha256=protected)
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());need(all(sha(ROOT/n)==h for n,h in build['editor_source_dependencies'].items()),'editor dependencies unchanged');need(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'editor unchanged')
    result.update(status='VERIFIED bounded native thumbnail/resources and owned x64 thumbnail conversion; later conversion/GPU unverified',previous_goal_turn='PROGRESS',owned_executable={'path':str(owned),'sha256':sha(owned),'architecture':'AMD64 PE32+'},owned_compiler_warnings=owned_warnings,owned_logs=owned_logs,owned_trace_sha256=sha(run/'owned-0.bin'),editor_preservation={'dependencies':len(build['editor_source_dependencies']),'changed':[],'executable_sha256':build['executable_sha256'],'build_runtime_this_task':'NOT RUN; no changed dependency'},limits=['Controlled read-only file snapshots and supplied hardware/no override configuration. Native stage stops at1003017D before later texture format conversion and GPU execution.','Native thumbnail and auxiliary resource allocations/copies execute with tracked guards; host disposes allocations, not a proven full cache lifecycle.','Owned immutable resource validation remains unchanged. Unknown custom resources remain unavailable to the owned thumbnail path; native publication is verified separately.','No editor integration, live SFM/computer use, original writes or publication. Owned thumbnail preparation is isolated and transactional.'])
    result['owned_nonaligned_refusals']=len(negative_rows)
    result['limits'].append('DXT1 dimensions above4 must be block aligned: native compressed sizing floors blocks; an earlier nonaligned fixture exposed reads into the supplied guard bytes and shifted legacy main-image addressing. Owned conversion refuses these inputs; their native behavior is not reproduced.')
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n')
    if '--smoke' not in sys.argv:(OUT/'latest-verification.json').write_text(json.dumps({'receipt':str(receipt),'receipt_sha256':sha(receipt)},indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ['source_sha256','protected_sha256','authenticated_ranges']},indent=2),flush=True)
if __name__=='__main__':main()
