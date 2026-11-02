"""Original anisotropy state10 and raw configuration publication, offline CPU.
Controlled COM/Note/hardware services; no GPU, original process or runtime port claim.
"""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,itertools,json,random,re,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS,DUMP
from verify_source_texture_sampling import SHADER,MATERIAL,predict,signed
from verify_ifm_draw_packet import check_ranges
OUT=ROOT/'outputs/source-anisotropy-20261101'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def put(p,x):Path(p).write_text(json.dumps(x,indent=2)+'\n')
def u32(n):return n&0xffffffff
def effective(request,cap):
    request,cap=signed(request),signed(cap)
    if 1<request<=cap:return request
    quarter=cap//4 if cap>=0 else -((-cap)//4)
    return max(2,min(8,quarter))
def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
    before=json.loads((OUT/'before.json').read_text());sources=dict(before['source_sha256']);protected=dict(before['protected_sha256']);protected[before['prior']['receipt']]=before['prior']['receipt_sha256']
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'preceding current identities')
    for name in ['probe_source_anisotropy_x86.cpp',Path(__file__).name]:sources[str(ROOT/'work'/name)]=sha(ROOT/'work'/name)
    audit=json.loads((OUT/'native-audit.local.json').read_text());need(sha(SHADER)==audit['shader_sha256']=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa' and sha(MATERIAL)==audit['material_sha256']=='c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8','original module identities')
    for path,disassembly,key in [(SHADER,'shaderapidx9-disassembly.txt','shader_ranges'),(MATERIAL,'materialsystem-disassembly.txt','material_ranges')]:check_ranges(path,disassembly,[(r['name'],int(r['start'],16),int(r['end_exclusive'],16)) for r in audit[key]])
    for name in ['native-audit.local.json','paused-probe-before-review.cpp','paused-harness-review.json','before.json']:protected[str(OUT/name)]=sha(OUT/name)
    review=json.loads((OUT/'paused-harness-review.json').read_text());need(sha(review['backup'])==review['before_sha256'] and sha(review['path'])==review['after_sha256'],'paused source review transition')
    rows=[]
    def add(request,cap,stages,record,gate):rows.append([u32(request),u32(cap),u32(stages),record,gate,0 if len(rows)%2==0 else 0x8876086c])
    for request,cap,stages,record,gate in itertools.product(range(-2,36),range(33),[-2,0,1,4,16],range(2),range(3)):add(request,cap,stages,record,gate)
    edges=[-2147483648,-1,0,1,2,3,4,7,8,15,16,31,32,33,64,127,128,2147483647]
    for request,cap,stages,record,gate in itertools.product(edges,edges,[-1,0,1,8,16],range(2),range(3)):add(request,cap,stages,record,gate)
    rng=random.Random(2026110101)
    for _ in range(3000):add(rng.getrandbits(32),rng.getrandbits(32),rng.choice([-2,0,1,2,4,8,16]),rng.randrange(2),rng.randrange(3))
    need(len(rows)==50340,'bounded dense/edge/random count');query=run/'queries.bin';query.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<6I',*q) for q in rows));put(run/'queries.json',rows)
    build=run/'build.cmd';commands=['@echo off',f'cd /d "{ROOT}"']
    builds=[('aniso','probe_source_anisotropy_x86.cpp'),('sampling','probe_source_texture_sampling_x86.cpp'),('apply','probe_source_sampler_apply_x86.cpp')]
    for label,source in builds:commands+=['setlocal',f'call "{VS.parent/"vcvars32.bat"}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\{source} /Fo:"{run/(label+".obj")}" /Fe:"{run/(label+".exe")}" >"{run/(label+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1','endlocal']
    build.write_text('\n'.join(commands+['exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=300,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1800:])
        return p
    execute(['cmd.exe','/d','/c',build],'build-driver');executables={}
    for label,_ in builds:
        exe=run/(label+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==0x14c and struct.unpack_from('<H',b,pe+24)[0]==0x10b,'private x86 '+label);warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(label+'-build.log')).read_text());need(not warnings,'warning-free CPU oracle '+label);executables[label]={'path':str(exe),'sha256':sha(exe),'compiler_warnings':warnings}
    stats=[];traces={};examples=[]
    for repeat in range(2):
        publication=run/f'published-{repeat}.bin';output=run/f'anisotropy-{repeat}.bin';execute([run/'aniso.exe','material',MATERIAL,query,publication],f'publication-{repeat}');execute([run/'aniso.exe','shader',SHADER,publication,output],f'anisotropy-{repeat}');data=output.read_bytes();need(struct.unpack_from('<I',data)[0]==len(rows),'anisotropy output count');at=4;events=cache=notes=0;direct=queued=0
        for serial,q in enumerate(rows):
            header=struct.unpack_from('<12I',data,at);at+=48;n=header[-1];actual=[struct.unpack_from('<3I',data,at+12*i) for i in range(n)];at+=12*n;stage_count=struct.unpack_from('<I',data,at)[0];at+=4;values=struct.unpack_from('<'+'I'*stage_count,data,at);at+=4*stage_count;count=max(0,signed(q[2]));value=effective(q[0],q[1]);expected=[(stage,10,value) for stage in range(count) if q[4]!=1 and not(q[4]==2 and stage&1)]
            need(header[:7]==(serial,*q) and header[7:11]==(q[0],q[1],q[0],count),'raw request/getter/callback/stage observation');need(stage_count==count and values==(value,)*count,'all stage caches receive effective value including inactive stages')
            if actual!=expected:put(run/'mismatch.json',{'case':serial,'query':q,'actual':actual,'expected':expected});raise RuntimeError('state10 submission mismatch '+str(serial))
            events+=n;cache+=count;notes+=1;queued+=n if q[3] else 0;direct+=n if not q[3] else 0
        need(at==len(data),'complete anisotropy framing');need(notes==len(rows) and direct and queued,'actual notes and both submission paths');traces[str(output)]=sha(output);traces[str(publication)]=sha(publication)
        # Feed actual original-published requests into original CTexture -> original
        # ShaderAPI records, keeping the global limit distinct from per-texture filters.
        flags=[0,1,2,0x10,0x100,0x11,0x110,0x20000010,0x0c,0x2000000c]
        material_rows=[[flags[i%len(flags)],q[0],q[1],2,2 if i%3==0 else 1,0x20 if i%2 else 0,4] for i,q in enumerate(rows)]
        texture_query=run/f'texture-input-{repeat}.bin';texture_query.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<7I',*r) for r in material_rows));commands_path=run/f'texture-commands-{repeat}.bin';records_path=run/f'sampler-records-{repeat}.bin';execute([run/'sampling.exe','material',MATERIAL,texture_query,commands_path],f'texture-commands-{repeat}');execute([run/'sampling.exe','shader',SHADER,commands_path,records_path],f'sampler-records-{repeat}');cb=commands_path.read_bytes();rb=records_path.read_bytes();ca=ra=4;record_count=command_count=0
        need(struct.unpack_from('<I',cb)[0]==struct.unpack_from('<I',rb)[0]==len(rows),'texture chain count')
        for serial,row in enumerate(material_rows):
            head=struct.unpack_from('<9I',cb,ca);ca+=36;need(head[:8]==(serial,*row),'actual texture command row');n=head[8];actual=[struct.unpack_from('<3I',cb,ca+12*k) for k in range(n)];ca+=n*12;expected,want_records=predict(row);need(actual==expected,'original per-texture filter selection');rh=struct.unpack_from('<9I',rb,ra);ra+=36;need(rh[:8]==(serial,*row) and rh[8]==len(want_records),'record header');actual_records=[struct.unpack_from('<7I',rb,ra+28*k) for k in range(rh[8])];ra+=28*rh[8];need(actual_records==want_records,'original ShaderAPI address/min/mag/mip records');command_count+=n;record_count+=rh[8]
            if repeat==0 and row[1]==1 and row[2] in [8,16,32] and row[0] in [0,0x10] and rows[serial][2]==1 and rows[serial][3:5]==[0,0] and len(examples)<12:examples.append({'raw_force':1,'hardware_max':row[2],'texture_flags':row[0],'state10_limit':effective(row[1],row[2]),'records':want_records})
        need(ca==len(cb) and ra==len(rb),'complete original texture chain traces')
        # Same native six-state apply proof, driven by newly measured raw force.
        config=run/f'apply-config-{repeat}.bin';b=bytearray(struct.pack('<I',len(rows)))
        for serial,q in enumerate(rows):
            c=bytearray(100);struct.pack_into('<I',c,0x28,q[0]);c[0x3c]=c[0x48]=1;b+=struct.pack('<12I',serial,*([0]*11))+c+bytes(12)
        config.write_bytes(b);applied=run/f'apply-{repeat}.bin';execute([run/'apply.exe',SHADER,config,records_path,applied],f'apply-{repeat}');ab=applied.read_bytes();aa=4;ra=4;apply_events=binds=0
        need(struct.unpack_from('<I',ab)[0]==len(rows),'apply count')
        for serial,row in enumerate(material_rows):
            rh=struct.unpack_from('<9I',rb,ra);ra+=36;need(struct.unpack_from('<2I',ab,aa)==(serial,rh[8]),'apply row');aa+=8
            for frame in range(rh[8]):
                r=struct.unpack_from('<7I',rb,ra);ra+=28;need(struct.unpack_from('<I',ab,aa)[0]==r[0],'apply frame');aa+=4;u,v,w,mag,minimum,mip=r[1:];stage=(serial+frame)%16
                for variant in range(6):
                    enabled=variant%3!=1;active=variant%3!=2
                    for phase in range(4):
                        expected=[];getters=0
                        if enabled and phase!=1:
                            expected=[(0,stage,0x4000,0)];getters=2
                            if active:
                                if phase==0:expected += [(1,stage,k,value) for k,value in zip([1,2,3,6,5,7],[u,v,w,minimum,mag,mip])]
                                if phase==2:expected += [(1,stage,k,value) for k,value,old in zip([1,2,3,6,5,7],[u,v,w,0,0,0],[u,v,w,minimum,mag,mip]) if value!=old]
                        header=struct.unpack_from('<4I',ab,aa);aa+=16;actual=[struct.unpack_from('<4I',ab,aa+16*k) for k in range(header[3])];aa+=16*header[3];need(header==(variant,phase,getters,len(expected)) and actual==expected,'actual normal bind/apply record submission');need(all(e[0]!=1 or e[2]!=10 for e in actual),'normal bind preserves separately set state10');apply_events+=len(actual);binds+=1
        need(aa==len(ab) and ra==len(rb),'complete native apply chain')
        for p in [commands_path,records_path,applied]:traces[str(p)]=sha(p)
        stats.append({'cases':len(rows),'raw_publications':notes,'state10_events':events,'direct_state10_events':direct,'queued_state10_events':queued,'stage_cache_updates':cache,'texture_commands':command_count,'texture_records':record_count,'native_bind_calls':binds,'normal_apply_events':apply_events});print('PASS repeat '+str(repeat)+' '+json.dumps(stats[-1]),flush=True)
    need(stats[0]==stats[1],'repeat statistics')
    for prefix in ['published','anisotropy','texture-commands','sampler-records','apply']:need((run/(prefix+'-0.bin')).read_bytes()==(run/(prefix+'-1.bin')).read_bytes(),'exact repeated original traces '+prefix)
    before_hash=sha(query);p=execute([run/'aniso.exe','material',MATERIAL,query,query],'existing-output',False);need(p.returncode!=0 and sha(query)==before_hash,'existing output refusal')
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'final source/protected identities')
    result={'status':'VERIFIED original CPU state10 setter/raw config publication/texture-filter/apply dependency contract; native x64 anisotropic pixels NOT implemented or verified by this research','time_utc':datetime.now(timezone.utc).isoformat(),'previous_goal_turn':'PROGRESS','cases':len(rows),'repetitions':2,'stats':stats,'examples':examples,'executables':executables,'authenticated_native_audit':{'path':str(OUT/'native-audit.local.json'),'sha256':sha(OUT/'native-audit.local.json')},'source_sha256':sources,'protected_sha256':protected,'trace_sha256':traces,'review':review,'contract':{'raw_note':'The raw signed request is published to config+0x28 before computing device fallback. Do not publish fallback as the global force request.','effective_state10':'If 1 < signed(request) <= signed(hardware_max), retain request; otherwise clamp(trunc_toward_zero(hardware_max/4),2,8).','per_stage':'Store effective value at base+0x1a14+36*stage before deactivation gate. Active direct/queued submissions send state10; queued command17. No changed-state suppression in whole setter.','separate_filter_selection':'Per-texture point/no-mip priority and force/flag0x10/hardware-cap decisions stay separate. force1 is trilinear for ordinary textures, anisotropic for flagged textures when hardware maximum>1. Magnification uses separate original capability bit.','source_table':'ShaderApi030 slot152; interface this=base+0x2d0. Hardware table slot5 reads+0x510. Provider slot18 publishes raw force; slot8 returns singleton config.'},'limits':['Original engine hardware-cap discovery and real scheduling of SetAnisotropicLevel/OverrideConfig are not executed or claimed. Hardware caps/stage counts are controlled test inputs. Signed extremes/nonpositive caps/counts are synthetic branch tests, not valid driver configurations.','Original shader routine calls controlled Note callback; actual original Note publisher/getter execute in separate process because PE images share preferred base. Normalized traces compose those independently verified paths, not a live engine configuration update.','Device COM endpoints record arguments and return controlled HRESULT values, including failure values that the original void routine ignores. No real device/driver acceptance, anisotropic GPU pixels, full reset/queue flush or device failure recovery claimed.','Private x86 research only; no new production helper, architecture change or 32-bit editor runtime dependency. Original SFM not launched, controlled, enumerated or modified. Production sources and verified editor remain unchanged; no editor rebuild required for test-only additions.']}
    receipt=run/'results.json';put(receipt,result);put(OUT/'latest-verification.json',{'receipt':str(receipt),'receipt_sha256':sha(receipt)});print(json.dumps({k:result[k] for k in ['status','cases','repetitions','stats','examples']},indent=2),flush=True)
if __name__=='__main__':main()
