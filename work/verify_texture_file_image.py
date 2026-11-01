"""Original CPU image loading with explicit missing overrides/hardware limits.

Private native code, fixtures and traces are local evidence only. No live SFM.
"""
from datetime import datetime, timezone
from pathlib import Path
import hashlib, itertools, json, re, struct, subprocess, sys, tempfile
from verify_session_preview_aspect import ROOT, VS
from verify_ifm_draw_packet import check_ranges
from verify_texture_file_header import vtf

OUT = ROOT / 'outputs/texture-file-image-20261101'
LIB = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(ok, message):
    if not ok: raise RuntimeError(message)
def text(s):
    b = str(s).encode('ascii'); return struct.pack('<I', len(b)) + b
def size(w, h, fmt): return w*h*4 if fmt in [0, 12] else ((w+3)//4)*((h+3)//4)*8

def fixture(minor, w, h, frames, fmt, flags, mips):
    b = vtf(minor, w, h, frames, fmt, flags)
    header = struct.unpack_from('<I', b, 12)[0]; b = b[:header]; b[56] = mips
    for offset, value in zip([32,36,40], [0.25,0.5,0.75]): struct.pack_into('<f', b, offset, value)
    pixels = {}
    for mip in reversed(range(mips)):
        for frame in range(frames):
            n = size(max(1,w>>mip), max(1,h>>mip),fmt)
            pixels[mip,frame] = bytes((i*17 + mip*43 + frame*71 + 3)%256 for i in range(n))
            b.extend(pixels[mip,frame])
    return b, pixels

def metadata(row):
    w,h,fmt,frames,flags = [row[k] for k in ['w','h','fmt','frames','flags']]
    effective = flags & 0x2e87fbff if row['minor']<=3 else flags
    effective |= row['request'] | (0x200 if row['logical'].startswith('materials/graffiti/') else 0)
    tw,th = w,h
    if row['clamp']:
        while tw>row['clamp'] or th>row['clamp']: tw//=2;th//=2
    if not effective&0x200:
        for _ in range(row['picmip']):
            if tw>4 and th>4: tw//=2;th//=2
    tw=min(tw,max(4,row['maxw']));th=min(th,max(4,row['maxh']))
    skip=0;aw,ah=w,h
    while aw>tw or ah>th: aw//=2;ah//=2;skip+=1
    levels=1
    if not effective&0x100:
        if row['full'] or effective&0x400: levels=max(aw,ah).bit_length()
        else:
            mw,mh=aw,ah
            while min(mw,mh)>32: levels+=1;mw//=2;mh//=2
    return [effective,0,fmt,w,h,1,aw,ah,1,levels,frames,skip,0x3e800000,0x3f000000,0x3f400000,1],skip

def main():
    OUT.mkdir(exist_ok=True); run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    (OUT/'active-run.txt').write_text(str(run));print('Verification directory: '+str(run),flush=True)
    idx=json.loads((ROOT/'outputs/texture-file-header-20261101/latest-verification.json').read_text())
    need(sha(idx['receipt'])==idx['receipt_sha256'],'accepted header receipt')
    prior=json.loads(Path(idx['receipt']).read_text());sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256']
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'preceding identities')
    for n in ['probe_texture_file_image.cpp','texture_image_plan_candidate.h','probe_texture_image_plan.cpp',Path(__file__).name]:sources[str(ROOT/'work'/n)]=sha(ROOT/'work'/n)
    image,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('reader_through_image_acceptance',0x1002fdb0,0x10030097),
        ('CTexture_metadata',0x1002e550,0x1002e5c0),('fresh_handle_clear',0x1002dd20,0x1002dd8d),
        ('frame_array',0x1002c7b0,0x1002c7f6),('mip_selection',0x1002f370,0x1002f682),
        ('texture_mip_count',0x1002c8a0,0x1002c91d),('override_lookup_boundary',0x100733b0,0x10073405),
        ('whole_VTF_unserialize',0x1007c810,0x1007caea),('VTF_image_read',0x100789d0,0x10078bbe),
        ('VTF_filesize',0x1007bbc0,0x1007bbf5),('VTF_mip_size',0x10078e90,0x10078ed1),
        ('VTF_image_pointer',0x1007aac0,0x1007aae9),('VTF_mip_count',0x10012f50,0x10012f54)])
    need(image.string(0x1009addc)=='materials/graffiti/','special texture policy prefix')
    fixtures=run/'fixtures';fixtures.mkdir();rows=[];coverage={}
    def add(minor=2,w=8,h=8,frames=1,fmt=0,flags=0,request=0,picmip=0,maxw=64,maxh=64,full=0,clamp=0,mips=None,mode=1,logical='materials/fixture/image.vtf',damage='',label='controlled'):
        if mips is None:mips=max(w,h).bit_length()
        b,pixels=fixture(minor,w,h,frames,fmt,flags,mips)
        if damage=='header_only':b=b[:struct.unpack_from('<I',b,12)[0]]
        if damage=='missing_pixel':b=b[:-1]
        if damage=='signature':b[:4]=b'BAD!'
        serial=len(rows);source=fixtures/(str(serial)+'.vtf');source.write_bytes(b)
        rows.append(dict(serial=serial,minor=minor,w=w,h=h,frames=frames,fmt=fmt,flags=flags,request=request,picmip=picmip,maxw=maxw,maxh=maxh,full=full,clamp=clamp,mips=mips,mode=mode,logical=logical,damage=damage,source=str(source),source_sha256=sha(source),size=len(b),pixels=pixels))
        coverage[label]=coverage.get(label,0)+1
    add()
    if '--smoke' not in sys.argv:
        for full in range(2):add(w=64,h=64,fmt=13,full=full,label='partial_versus_full_native_chain')
        for minor,(w,h),frames,fmt in itertools.product(range(1,6),[(2,2),(4,8),(8,8),(16,16)],[1,2],[0,12,13]):add(minor,w,h,frames,fmt,label='versions_dimensions_frames_formats')
        for minor,flags,request in itertools.product(range(1,6),[0,0x100,0x200,0x400,0x1000,0x2000,0x80000],[0,0x100,0x400,0x80000]):add(minor=minor,flags=flags,request=request,label='raw_normalized_requested_flags')
        for w,picmip,maxw,maxh,full,clamp in itertools.product([4,8,16,32],range(3),[4,64],[4,64],range(2),[0,4,8]):add(w=w,h=w,picmip=picmip,maxw=maxw,maxh=maxh,full=full,clamp=clamp,label='mip_config_hardware_limits')
        for minor in range(1,6):
            for damage in ['header_only','missing_pixel','signature']:add(minor=minor,damage=damage,label='failure_'+damage)
            for picmip in range(3):add(minor=minor,w=16,h=16,mips=1,picmip=picmip,label='inconsistent_declared_mips')
            add(minor=minor,logical='materials/graffiti/test.vtf',picmip=2,label='graffiti_skips_picmip')
        add(mode=0,label='missing_file');add(mode=2,logical='materials/fixture/image.hdr.vtf',label='HDR_fallback')
    query=run/'queries.bin';query.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<7I',r['request'],r['picmip'],r['maxw'],r['maxh'],r['full'],r['clamp'],r['mode'])+text(r['logical'])+text('fixture/image')+text(r['source']) for r in rows))
    batch=run/'build.cmd';batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS.parent/"vcvars32.bat"}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_texture_file_image.cpp /Fo:"{run/"native.obj"}" /Fe:"{run/"native.exe"}" >"{run/"build.log"}" 2>&1','if errorlevel 1 exit /b 1','exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1600:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/'build.log').read_text());need(not warnings,'compiler warnings')
    exe=run/'native.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(struct.unpack_from('<H',raw,pe+4)[0]==0x14c,'private reference architecture')
    traces=[];observations=[];successes=failures=0
    for repeat in range(2):
        target=run/('native-'+str(repeat)+'.bin');execute([exe,LIB,query,target],'native-'+str(repeat));data=target.read_bytes();traces.append(data);offset=0;successes=failures=0
        for r in rows:
            record=list(struct.unpack_from('<28I',data,offset));offset+=112;strings=[]
            for _ in range(3):n=struct.unpack_from('<I',data,offset)[0];offset+=4;strings.append(data[offset:offset+n]);offset+=n
            filename,trace,pixels=strings;steps=trace.decode('ascii').splitlines()
            meta,skip=metadata(r);header_ok=r['mode']!=0 and r['damage']!='signature';accepted=header_ok and not r['damage'] and skip<=r['mips']
            native_mips=max(r['w'],r['h']).bit_length()-skip
            expected_pixels=bytearray()
            if accepted:
                for mip in range(native_mips):
                    for frame in range(r['frames']):
                        n=size(max(1,r['w']>>(mip+skip)),max(1,r['h']>>(mip+skip)),r['fmt'])
                        # The native object recomputes its allocation mip count;
                        # only header-declared levels are filled by the read loop.
                        block=r['pixels'][mip+skip,frame] if mip<r['mips']-skip else bytes([0xa5])*n
                        expected_pixels.extend(struct.pack('<I',n)+block)
            expected_meta=meta if header_ok else [r['request'],0,0xffffffff,0,0,1,0,0,1,0,0,0,0,0,0,1]
            expected_vtf=[max(1,r['w']>>skip),max(1,r['h']>>skip),1,r['fmt'],r['frames'],r['flags']&0x2e87fbff if r['minor']<=3 else r['flags'],native_mips] if accepted else [0]*7
            if bool(record[1])!=bool(accepted) or record[5:21]!=expected_meta or record[21:]!=expected_vtf or pixels!=expected_pixels:
                (run/'mismatch.json').write_text(json.dumps({'row':{k:v for k,v in r.items() if k!='pixels'},'actual':record,'expected_meta':expected_meta,'expected_vtf':expected_vtf,'accepted':accepted,'trace':steps,'pixel_bytes':len(pixels),'expected_pixel_bytes':len(expected_pixels)},indent=2));raise RuntimeError('image state mismatch case '+str(r['serial']))
            need(record[2]==(2 if r['mode']==2 else 1),'open count');need(record[3]==(2 if header_ok else int(r['mode']!=0)),'read count');need(record[4]==int(r['mode']!=0),'native file closed')
            need(record[0]==r['serial'],'serial identity')
            after=r['logical'].replace('.hdr.vtf','.vtf') if r['mode']==2 else r['logical']
            need(filename.decode('ascii')==after,'filename after fallback')
            expected_steps=['P GAME','O '+r['logical']+' rb 0 GAME']
            if r['mode']==2:expected_steps+=['P GAME','O '+after+' rb 0 GAME']
            header=struct.unpack_from('<I',Path(r['source']).read_bytes(),12)[0]
            if r['mode']!=0:expected_steps+=['A 0','R 2097152 336 '+str(min(336,r['size'])),'S '+str(header)]
            if header_ok:
                wanted=header+sum(size(max(1,r['w']>>m),max(1,r['h']>>m),r['fmt'])*r['frames'] for m in range(skip,max(r['w'],r['h']).bit_length()))
                expected_steps+=['OVERRIDE MISS','OVERRIDE MISS','HW W','HW H','HW D','S 0','A 0','R 2097152 '+str(wanted)+' '+str(min(wanted,r['size']))]
            if r['mode']!=0:expected_steps+=['X']
            expected_steps+=['IMAGE ACCEPTED' if accepted else 'ERROR DISPATCH']
            need([s for s in steps if s!='W']==expected_steps,'ordered filesystem trace '+str(r['serial']))
            need(steps[-1]==('IMAGE ACCEPTED' if accepted else 'ERROR DISPATCH'),'stop identity')
            need(steps.count('OVERRIDE MISS')==2*int(header_ok),'actual mip selection override calls')
            need([s for s in steps if s.startswith('HW ')]==(['HW W','HW H','HW D'] if header_ok else []),'hardware query order')
            if repeat==0:observations.append({'serial':r['serial'],'record':record,'trace':steps,'image_bytes':len(pixels),'image_sha256':hashlib.sha256(pixels).hexdigest()})
            successes+=int(accepted);failures+=int(not accepted)
        need(offset==len(data),'trace framing')
    need(traces[0]==traces[1],'repeated complete trace identity')
    before=sha(query);p=execute([exe,LIB,query,query],'existing-output',False);need(p.returncode!=0 and sha(query)==before,'existing output unchanged')
    owned_batch=run/'owned-build.cmd';owned_batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_texture_image_plan.cpp /Fo:"{run/"owned.obj"}" /Fe:"{run/"owned.exe"}" >"{run/"owned-build.log"}" 2>&1','if errorlevel 1 exit /b 1','exit /b 0','']))
    execute(['cmd.exe','/d','/c',owned_batch],'owned-build-driver');owned=run/'owned.exe';owned_warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/'owned-build.log').read_text());need(not owned_warnings,'owned compiler warnings')
    raw=owned.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(struct.unpack_from('<H',raw,pe+4)[0]==0x8664 and struct.unpack_from('<H',raw,pe+24)[0]==0x20b,'owned native x64 architecture')
    plans=[];expected_plans=bytearray()
    for r,observed in zip(rows,observations):
        if observed['record'][3]!=2:continue
        q=[7,r['minor'],r['flags'],r['request'],r['w'],r['h'],r['frames'],r['fmt'],0x3e800000,0x3f000000,0x3f400000,r['picmip'],r['maxw'],r['maxh'],r['clamp'],r['full'],int(r['logical'].startswith('materials/graffiti/')),1,1,1,1]
        plans.append(q);expected_plans.extend(struct.pack('<17I',1,*observed['record'][5:21]))
    matched_plans=len(plans)
    for index,value in [(0,8),(1,6),(2,0x4000),(3,0x4000),(4,0),(4,3),(5,0),(6,0),(6,2049),(7,99),(8,0x7f800000),(8,0x7f800001),(11,4),(12,0),(13,0),(14,3),(17,0),(18,0),(19,0)]:
        q=plans[0].copy();q[index]=value;q[20]=0;plans.append(q);expected_plans.extend(struct.pack('<17I',0,*([0xaabbccdd]*16)))
    plan_query=run/'plans-query.bin';plan_query.write_bytes(b''.join(struct.pack('<21I',*q) for q in plans))
    for repeat in range(2):
        dest=run/('plans-'+str(repeat)+'.bin');execute([owned,plan_query,dest],'owned-plan-'+str(repeat));need(dest.read_bytes()==expected_plans,'owned plan vs actual native CTexture state')
    before=sha(plan_query);p=execute([owned,plan_query,plan_query],'owned-existing-output',False);need(p.returncode!=0 and sha(plan_query)==before,'owned existing output preserved')
    need(all(sha(r['source'])==r['source_sha256'] for r in rows),'snapshot inputs unchanged')
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'final protected/source identities')
    (run/'observations.json').write_text(json.dumps(observations,indent=2))
    result=dict(status='VERIFIED bounded native CPU image read; GPU and production integration UNVERIFIED',time_utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='PROGRESS',cases=len(rows),accepted=successes,error_dispatches=failures,coverage=coverage,repetitions=2,trace_bytes=len(traces[0]),trace_sha256=sha(run/'native-0.bin'),executable={'path':str(exe),'sha256':sha(exe),'architecture':'x86 private reference'},compiler_warnings=warnings,authenticated_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in ranges],source_sha256=sources,protected_sha256=protected,limits=['Controlled read-only filesystem snapshots, GAME path ID, scratch, hardware dimensions, positive picmip and absent texture overrides. Real native metadata initializer, mip selection and full VTF Unserialize execute.','Stop before conversion/resource publication/error-image creation/GPU. No editor integration or new production x64 loader.','Fresh CTexture objects only; guarded private allocations are discarded by host. Native cache teardown/lifetimes remain unverified.','No live original SFM/computer use, original writes, editor rebuild or publication.'])
    result.update(owned_plan_cases=len(plans),native_matched_plans=matched_plans,owned_refusals=len(plans)-matched_plans,owned_executable={'path':str(owned),'sha256':sha(owned),'architecture':'AMD64 PE32+'},owned_compiler_warnings=owned_warnings)
    result['status']='VERIFIED bounded native CPU image read and owned x64 metadata/mip plan; GPU/production integration UNVERIFIED'
    result['limits']+=['Owned x64 helper is an isolated fresh 2D plan with explicit known hardware/no LOD resource/no overrides. It does not parse files, validate pixel availability, create GPU resources or replace production state.','Native VTF allocation mip count differs from declared/read mip count. In inconsistent fixtures unfilled allocation bytes remain supplied allocator fill A5, explicitly compared, not treated as validated image data. Existing owned strict VTF validation unchanged.']
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n')
    if '--smoke' not in sys.argv:(OUT/'latest-verification.json').write_text(json.dumps({'receipt':str(receipt),'receipt_sha256':sha(receipt)},indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ['source_sha256','protected_sha256','authenticated_ranges']},indent=2),flush=True)

if __name__=='__main__':main()
