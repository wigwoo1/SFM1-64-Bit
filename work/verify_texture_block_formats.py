"""Native block-format file conversion and owned x64 compatibility checks."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,itertools,json,random,re,shutil,struct,subprocess,sys,tempfile
from verify_session_preview_aspect import ROOT,VS
from verify_texture_file_image import text
from verify_ifm_draw_packet import check_ranges
from verify_material_texture_converted_image import native_records
OUT=ROOT/'outputs/texture-block-formats-20261101'
LIB=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def block_size(w,h,fmt):return ((w+3)//4)*((h+3)//4)*(16 if fmt==15 else 8)
def make(minor,w,h,frames,fmt,flags,seed):
    header=64 if minor==1 else 80 if minor==2 else 88;b=bytearray(header);mips=max(w,h).bit_length()
    for at,n in [(0,0x00465456),(4,7),(8,minor),(12,header),(20,flags),(52,fmt),(57,0xffffffff)]:struct.pack_into('<I',b,at,n)
    for at,n in [(16,w),(18,h),(24,frames)]:struct.pack_into('<H',b,at,n)
    if minor>1:struct.pack_into('<H',b,63,1)
    if minor>2:struct.pack_into('<I',b,68,1);struct.pack_into('<2I',b,80,0x30,header)
    b[56]=mips;blocks={};rng=random.Random(seed)
    alpha_pairs=[(255,0),(0,255),(77,77),(0,0),(255,255),(64,192),(192,64),(1,2),(2,1)]
    for mip in reversed(range(mips)):
        for frame in range(frames):
            mw,mh=max(1,w>>mip),max(1,h>>mip);raw=bytearray()
            for block in range(((mw+3)//4)*((mh+3)//4)):
                a,c=(0xf800,0x07e0) if (seed+block+frame)%2==0 else (0x07e0,0xf800)
                selectors=sum(((i+frame+block)%4)<<(2*i) for i in range(16))
                if seed>=100:a,c,selectors=rng.randrange(65536),rng.randrange(65536),rng.getrandbits(32)
                if fmt==15:
                    aa,ab=alpha_pairs[seed%len(alpha_pairs)] if seed<100 else (rng.randrange(256),rng.randrange(256))
                    alpha_selectors=sum(((i+frame+block)%8)<<(3*i) for i in range(16)) if seed<100 else rng.getrandbits(48)
                    raw.extend(bytes([aa,ab])+alpha_selectors.to_bytes(6,'little'))
                raw.extend(struct.pack('<HHI',a,c,selectors))
            need(len(raw)==block_size(mw,mh,fmt),'authored compressed size');blocks[mip,frame]=bytes(raw);b.extend(raw)
    return b,blocks
def decode(raw,w,h,fmt):
    result=[None]*(w*h);step=16 if fmt==15 else 8
    for by in range((h+3)//4):
        for bx in range((w+3)//4):
            block=raw[(by*((w+3)//4)+bx)*step:][:step];at=8 if fmt==15 else 0;a,b,selectors=struct.unpack_from('<HHI',block,at)
            def color(v):
                r,g,bl=(v>>11)&31,(v>>5)&63,v&31
                return [(r<<3)|(r>>2),(g<<2)|(g>>4),(bl<<3)|(bl>>2),255]
            # Measured native CPU path calls its DXT1 palette even for DXT5.
            # This differs from standard GPU DXT5 for reversed colour endpoints.
            p=[color(a),color(b)];four=a>b
            p += [[(2*p[0][i]+p[1][i])//3 if four else (p[0][i]+p[1][i])//2 for i in range(3)]+[255],[(p[0][i]+2*p[1][i])//3 if four else [0,255,255][i] for i in range(3)]+[255 if four else 0]]
            alpha=None;alpha_bits=0
            if fmt==15:
                aa,ab=block[:2];alpha=[aa,ab]+[((8-i)*aa+(i-1)*ab)//7 if aa>ab else ((6-i)*aa+(i-1)*ab)//5 for i in range(2,8 if aa>ab else 6)]
                if aa<=ab:alpha += [0,255]
                alpha_bits=int.from_bytes(block[2:8],'little')
            for y in range(4):
                for x in range(4):
                    if bx*4+x<w and by*4+y<h:
                        i=y*4+x;c=p[(selectors>>(2*i))&3].copy()
                        if alpha is not None:c[3]=alpha[(alpha_bits>>(3*i))&7]
                        result[(by*4+y)*w+bx*4+x]=c
    return result
def converted(raw,w,h,fmt,target):
    if target==fmt:return raw
    pixels=decode(raw,w,h,fmt)
    if target==0:return bytes(c for p in pixels for c in p)
    if target==3:return bytes(c for r,g,b,a in pixels for c in [b,g,r])
    if target==12:return bytes(c for r,g,b,a in pixels for c in [b,g,r,a])
    raise RuntimeError('unmeasured target')
def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));(OUT/'active-run.txt').write_text(str(run));print('Verification directory: '+str(run),flush=True)
    idx=json.loads((ROOT/'outputs/material-texture-converted-image-20261101/latest-milestone.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'preceding checkpoint');prior=json.loads(Path(idx['receipt']).read_text());sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256']
    transitions=[]
    for name in ['texture_image_plan_candidate.h','material_texture_converted_image_candidate.h']:
        p=ROOT/'work'/name;backup=OUT/'preserved-source'/name;need(sha(backup)==sources[str(p)],'preserved source '+name)
        if '--native-only' not in sys.argv:
            transitions.append(dict(path=str(p),before=sources[str(p)],after=sha(p)));sources[str(p)]=sha(p);protected[str(backup)]=sha(backup)
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'preceding identities')
    for name in [Path(__file__).name,'probe_texture_block_primitives.cpp']+([] if '--native-only' in sys.argv else ['probe_block_format_admission.cpp']):sources[str(ROOT/'work'/name)]=sha(ROOT/'work'/name)
    _,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('whole_file_reader',0x1002fdb0,0x100301cf),('real_VTF_conversion',0x10079140,0x10079408),('CPU_dispatch',0x10048ec0,0x100496c0)])
    rows=[];fixtures=run/'fixtures';fixtures.mkdir()
    def add(fmt=15,target=0,minor=5,w=8,h=8,frames=1,flags=0,request=0,enabled=1,compression=1,picmip=0,full=1,seed=0):
        b,blocks=make(minor,w,h,frames,fmt,flags,seed);p=fixtures/(str(len(rows))+'.vtf');p.write_bytes(b);rows.append(dict(fmt=fmt,target=target,minor=minor,w=w,h=h,frames=frames,flags=flags,request=request,enabled=enabled,compression=compression,picmip=picmip,full=full,seed=seed,source=str(p),sha256=sha(p),blocks=blocks,config=[request,picmip,64,64,full,0,1,target,enabled,0,compression]))
    for minor,target,seed in itertools.product(range(1,6),[0,3,12,15],range(9)):add(minor=minor,target=target,seed=seed)
    for (w,h),frames,flags,request,target,picmip,full in itertools.product([(2,2),(4,8),(16,16)],[1,2],[0,0x1000,0x2000,0x3000],[0,0x100],[0,3,12,15],[0,1],[0,1]):add(w=w,h=h,frames=frames,flags=flags,request=request,target=target,picmip=picmip,full=full,seed=5)
    for enabled,compression,target in itertools.product([0,1,0x100,0x101],[0,1,0x100,0x101],[0,3,12,15]):add(enabled=enabled,compression=compression,target=target)
    for seed,target in itertools.product(range(100,228),[0,3,12]):add(seed=seed,target=target)
    query=run/'queries.bin';query.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<11I',*r['config'])+text('materials/fixture/blocks.vtf')+text('fixture/blocks')+text(r['source']) for r in rows))
    batch=run/'build.cmd';commands=['@echo off',f'cd /d "{ROOT}"'];jobs=[('native',VS.parent/'vcvars32.bat','probe_texture_file_final.cpp'),('primitive',VS.parent/'vcvars32.bat','probe_texture_block_primitives.cpp')]
    if '--native-only' not in sys.argv:jobs += [('owned',VS,'probe_material_texture_converted_image.cpp'),('admission',VS,'probe_block_format_admission.cpp'),('plan',VS,'probe_texture_image_plan.cpp')]
    for name,vc,source in jobs:commands += ['setlocal',f'call "{vc}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\{source} /Fo:"{run/(name+".obj")}" /Fe:"{run/(name+".exe")}" >"{run/(name+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1','endlocal']
    batch.write_text('\n'.join(commands+['exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1800:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');warnings={};executables={}
    for name,vc,_ in jobs:
        b=(run/(name+'.exe')).read_bytes();pe=struct.unpack_from('<I',b,60)[0];x86=name in ['native','primitive'];need(struct.unpack_from('<H',b,pe+4)[0]==(0x14c if x86 else 0x8664) and struct.unpack_from('<H',b,pe+24)[0]==(0x10b if x86 else 0x20b),'architecture '+name)
        warnings[name]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(name+'-build.log')).read_text());need(not warnings[name],'warnings '+name);executables[name]={'path':str(run/(name+'.exe')),'sha256':sha(run/(name+'.exe'))}
    primitive=[];native=[];expected_owned=None;observations=[];converted_count=selected_blocks=selected_bytes=0
    for repeat in range(2):
        dest=run/('primitive-'+str(repeat)+'.bin');execute([run/'primitive.exe',LIB,dest],'primitive-'+str(repeat));primitive.append(dest.read_bytes());expected_primitive=b''.join(struct.pack('<6I',fmt,d,target,0 if fmt==20 else block_size(d,d,fmt),int(fmt!=20),int(fmt==20)) for fmt in [13,15,20] for d in [1,2,4,8,16] for target in [0,3,12]);need(primitive[-1]==expected_primitive,'native primitive size/refusal trace')
        dest=run/('native-'+str(repeat)+'.bin');execute([run/'native.exe',LIB,query,dest],'native-'+str(repeat));native.append(dest.read_bytes());records=native_records(native[-1]);need(len(records)==len(rows),'native case count');wanted=bytearray();converted_count=selected_blocks=selected_bytes=0
        for serial,(r,(record,trace,pixels,aux)) in enumerate(zip(rows,records)):
            effective=r['flags']&0x2e87fbff if r['minor']<=3 else r['flags'];flags=effective|r['request'];enabled=bool(r['enabled']&255);target=r['target'] if enabled else 15;change=target!=15;skip=0
            for _ in range(r['picmip']):
                if not flags&0x200 and r['w']>>skip>4 and r['h']>>skip>4:skip+=1
            w,h=max(1,r['w']>>skip),max(1,r['h']>>skip);mips=max(w,h).bit_length();selected=1 if flags&0x100 or not r['full'] else mips;vtfflags=((effective&~0x1000)|0x2000) if change and target in [0,12] else effective&~0x3000 if change else effective
            expected=[serial,1,2,1,flags,0,target,r['w'],r['h'],w,h,selected,r['frames'],skip,w,h,target,vtfflags,r['frames'],mips,int(change),int(change),int(enabled),int(enabled and not change)]
            expected_pixels=bytearray();selected_pixels=bytearray()
            for m in range(mips):
                for f in range(r['frames']):
                    p=converted(r['blocks'][m+skip,f],max(1,w>>m),max(1,h>>m),15,target);block=struct.pack('<I',len(p))+p;expected_pixels.extend(block)
                    if m<selected:selected_pixels.extend(block);selected_blocks+=1;selected_bytes+=len(p)
            if record!=expected or pixels!=expected_pixels or aux!=bytes(12):
                (run/'mismatch.json').write_text(json.dumps({'row':{k:v for k,v in r.items() if k!='blocks'},'record':record,'expected':expected,'trace':trace.decode(),'actual_pixels':list(pixels),'expected_pixels':list(expected_pixels)},indent=2));raise RuntimeError('native DXT5 mismatch case '+str(serial))
            requested=15 if r['compression']&255 else 12 if flags&0x3000 else 3;source=Path(r['source']).read_bytes();header=struct.unpack_from('<I',source,12)[0];read_size=len(source)-sum(block_size(max(1,r['w']>>m),max(1,r['h']>>m),15) for m in range(skip))*r['frames']
            steps=['P GAME','O materials/fixture/blocks.vtf rb 0 GAME','A 0','R 2097152 336 '+str(min(336,len(source))),'S '+str(header),'OVERRIDE MISS','OVERRIDE MISS','HW W','HW H','HW D','S 0','A 0','R 2097152 '+str(read_size)+' '+str(read_size),'X','M '+str(r['enabled'])]
            if enabled:steps+=['FORMAT '+str(requested)+' 1 '+str(target)]
            if change:steps+=['SCRATCH ALLOC '+str(len(expected_pixels)-4*mips*r['frames']),'SCRATCH FREE']
            elif enabled:steps+=['HDR 0']
            need(trace.decode().splitlines()==steps,'ordered file/service/scratch trace')
            plan=[flags,0,target,r['w'],r['h'],1,w,h,1,selected,r['frames'],skip,0,0,0,1];wanted.extend(struct.pack('<2I3I16I5I',serial,1,r['flags'],effective,r['request'],*plan,target,vtfflags,int(change),int(change),int(change)))
            service=''.join(s+'\n' for s in steps if s.startswith(('M ','FORMAT ','HDR '))).encode();wanted.extend(struct.pack('<I',len(service))+service+struct.pack('<I',len(selected_pixels))+selected_pixels);converted_count+=int(change)
            if repeat==0:observations.append({'serial':serial,'seed':r['seed'],'record':record,'pixel_sha256':hashlib.sha256(pixels).hexdigest()})
        if expected_owned is None:expected_owned=bytes(wanted);(run/'expected-owned.bin').write_bytes(expected_owned)
        else:need(expected_owned==wanted,'repeat independent prediction')
        if '--native-only' not in sys.argv:
            dest=run/('owned-'+str(repeat)+'.bin');execute([run/'owned.exe',query,dest],'owned-'+str(repeat));need(dest.read_bytes()==expected_owned,'owned x64 exact DXT5 state/pixels/services')
    need(native[0]==native[1] and primitive[0]==primitive[1],'repeated native identity')
    safety=None
    if '--native-only' not in sys.argv:
        safety=execute([run/'owned.exe','--test'],'owned-safety').stdout;admission=execute([run/'admission.exe'],'block-admission').stdout;safety+=admission
        old=Path(prior['cpu_receipt']['receipt']).parent
        for repeat in range(2):
            dest=run/('owned-regression-'+str(repeat)+'.bin');execute([run/'owned.exe',old/'queries.bin',dest],'owned-regression-'+str(repeat));need(sha(dest)==prior['owned_trace_sha256'],'all preceding580 cases unchanged')
        plan_idx=json.loads((ROOT/'outputs/texture-file-image-20261101/latest-verification.json').read_text());plan_old=Path(plan_idx['receipt']).parent;dest=run/'plan-regression.bin';execute([run/'plan.exe',plan_old/'plans-query.bin',dest],'plan-regression');need(dest.read_bytes()==(plan_old/'plans-0.bin').read_bytes(),'all601 prior plans unchanged')
    for name in ['native']+([] if '--native-only' in sys.argv else ['owned']):
        before=sha(query);args=[run/(name+'.exe')]+([LIB] if name=='native' else [])+[query,query];p=execute(args,name+'-existing-output',False);need(p.returncode!=0 and sha(query)==before,'existing output preserved')
    need(all(sha(r['source'])==r['sha256'] for r in rows),'all input fixtures unchanged')
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'final source/protected identities')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());need(all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()),'editor dependencies unchanged');need(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'editor preserved')
    (run/'observations.json').write_text(json.dumps(observations,indent=2));result=dict(status='VERIFIED native DXT5 CPU file contract and format20 refusal' if '--native-only' in sys.argv else 'VERIFIED bounded native/owned x64 DXT5 conversion and format20 refusal',time_utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='PROGRESS',cases=len(rows),converted=converted_count,selected_frame_mip_blocks=selected_blocks,selected_pixel_bytes=selected_bytes,primitive_cases=45,repetitions=2,native_trace_sha256=sha(run/'native-0.bin'),owned_trace_sha256=sha(run/'expected-owned.bin'),executables=executables,compiler_warnings=warnings,owned_safety=safety,transitions=transitions,source_sha256=sources,protected_sha256=protected,authenticated_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in ranges],limits=['Original CPU DXT5 conversion uses the authenticated DXT1 colour palette even for reversed colour endpoints; DXT5 alpha endpoints/selectors applied independently. This is not a claim about standard GPU BC3 decoding.','Format20 native sizing returns0 and conversion to0/3/12 returnsfalse leaving destination unchanged across45 primitive cases (15 format20 refusals). The private full-reader preflight stopped at its zero-mip-size guard. Owned image admission remains refused; no decoded or loaded state fabricated. This does not mean ordinary DXT1 files carrying one-bit-alpha flags are unsupported.','Explicit hardware/format/enable/compression services, HDR0 and debug settings0. No actual active-device selection, GPU upload, other compressed/HDR/debug modes, original teardown/error-resource construction or production integration established.','No editor dependencies/runtime changed. Original installation/assets/sessions read-only; no original SFM process/computer use or publication.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');index={'receipt':str(receipt),'receipt_sha256':sha(receipt)};(OUT/('native-verification.json' if '--native-only' in sys.argv else 'latest-verification.json')).write_text(json.dumps(index,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['source_sha256','protected_sha256','authenticated_ranges','limits']},indent=2),flush=True)
if __name__=='__main__':main()
