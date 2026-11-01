"""Original CTexture commands -> original ShaderAPI sampler record, offline."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,itertools,json,random,re,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS,DUMP
from verify_ifm_draw_packet import check_ranges
OUT=ROOT/'outputs/source-sampling-policy-20261101'
MATERIAL=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')
SHADER=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def signed(n):return n if n<0x80000000 else n-0x100000000
def predict(q):
    flags,force,cap,status,frames,magcap,mips=q;events=[]
    if not status&2:return events,[]
    for frame in range(frames):
        events.append((0,0x2000+frame,0))
        wraps=[2]*3 if flags&0x20000000 else [0 if flags&bit else 1 for bit in [4,8,0x2000000]]
        events.extend((1,i,v) for i,v in enumerate(wraps))
        def query():events.append((4,cap,0));return signed(cap)>1
        if flags&1:minimum=maximum=0
        elif flags&0x100:minimum=maximum=1
        elif signed(force)>1 and query():minimum=maximum=6
        else:
            aniso=bool(flags&0x10) and query();tri=force==1 or bool(flags&2)
            minimum=6 if aniso else 5 if tri else 3;maximum=6 if aniso else 1
        events.extend([(2,minimum,0),(3,maximum,0)])
    wrap_map={0:3,1:1,2:4};minimum_map={0:(1,0),1:(2,0),3:(2,int(mips!=1)),5:(2,2 if mips!=1 else 0),6:(3,2 if mips!=1 else 0)}
    min_filter,mip_filter=minimum_map[minimum] if frames else (0,0)
    mag_filter=(3 if magcap&0x20 else 2) if frames and maximum==6 else maximum+1 if frames else 0
    records=[(0x2000+i,*(wrap_map[w] for w in wraps),mag_filter,min_filter,mip_filter) for i in range(frames)]
    return events,records
def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
    index=json.loads((ROOT/'outputs/texture-cross-api-correction-20261101/latest-milestone.json').read_text());need(sha(index['receipt'])==index['receipt_sha256'],'preceding checkpoint');old=json.loads(Path(index['receipt']).read_text());sources=dict(old['source_sha256']);protected=dict(old['protected_sha256']);protected[index['receipt']]=index['receipt_sha256']
    need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in protected.items()),'preceding source/protected identities')
    for n in ['probe_source_texture_sampling_x86.cpp',Path(__file__).name,'native_private_world_image_fixture.h','studio_texture_binding_candidate.h','vulkan_prepared_model_images_candidate.h','prepared-material-draw-probe.hlsl']:sources[str(ROOT/'work'/n)]=sha(ROOT/'work'/n)
    mat,e1=check_ranges(MATERIAL,'materialsystem-disassembly.txt',[('whole_texture_wrap_commands',0x1002c960,0x1002ca14),('whole_texture_filter_commands',0x1002ca20,0x1002cb06),('whole_frame_sampling_dispatch',0x1002ce70,0x1002cebe)])
    api,e2=check_ranges(SHADER,'shaderapidx9-disassembly.txt',[('whole_wrap_record_setter',0x1001e150,0x1001e1d4),('whole_min_record_setter',0x1001e1e0,0x1001e2a6),('whole_mag_record_setter',0x1001e2d0,0x1001e381)])
    need([api.word(0x100fcedc+i*4) for i in [79,80,81]]==[0x1001e1e0,0x1001e2d0,0x1001e150],'installed API vtable sampler slots')
    tables={'minimum':[api.word(0x1001e2a8+4*i) for i in range(7)],'magnification':[api.word(0x1001e384+4*i) for i in range(7)]};put=lambda p,v:Path(p).write_text(json.dumps(v,indent=2)+'\n');put(run/'native-vtable-and-tables.json',tables)
    protected.update({str(p):sha(p) for p in [MATERIAL,SHADER]})
    bits=[1,2,4,8,0x10,0x100,0x2000000,0x20000000];rows=[]
    for mask,force,cap,mips,magcap in itertools.product(range(256),[0,1,2,0xffffffff],[0xffffffff,1,2,16],[0,1,4],[0,0x20]):
        flags=sum(bit for i,bit in enumerate(bits) if mask&(1<<i));rows.append([flags,force,cap,2,1+(mask%2),magcap,mips])
    for flags,status,frames in itertools.product([0,1,2,4,8,0x10,0x100,0x2000000,0x20000000],[0,1,2,3,0xffffffff],[0,1,2,4,8]):rows.append([flags,2,2,status,frames,0x20,4])
    rng=random.Random(2026110112)
    for _ in range(1024):rows.append([rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.randrange(9),rng.randrange(256),rng.choice([0,1,2,4,255])])
    need(len(rows)<=65536,'query bound');query=run/'queries.bin';query.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<7I',*q) for q in rows));expected_material=bytearray(struct.pack('<I',len(rows)));expected_shader=bytearray(struct.pack('<I',len(rows)));commands=records_count=0
    for serial,q in enumerate(rows):
        events,records=predict(q);expected_material+=struct.pack('<9I',serial,*q,len(events))+b''.join(struct.pack('<3I',*e) for e in events);expected_shader+=struct.pack('<9I',serial,*q,len(records))+b''.join(struct.pack('<7I',*r) for r in records);commands+=len(events);records_count+=len(records)
    (run/'expected-material.bin').write_bytes(expected_material);(run/'expected-shader.bin').write_bytes(expected_shader)
    batch=run/'build.cmd';batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS.parent/"vcvars32.bat"}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_source_texture_sampling_x86.cpp /Fo:"{run/"native.obj"}" /Fe:"{run/"native.exe"}" >"{run/"build.log"}" 2>&1','if errorlevel 1 exit /b 1','exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1400:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');exe=run/'native.exe';b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==0x14c and struct.unpack_from('<H',b,pe+24)[0]==0x10b,'private x86 PE32 oracle, not owned editor');warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/'build.log').read_text());need(not warnings,'warning-free probe')
    traces={}
    for repeat in range(2):
        m=run/f'material-{repeat}.bin';s=run/f'shader-{repeat}.bin';execute([exe,'material',MATERIAL,query,m],f'material-{repeat}');need(m.read_bytes()==expected_material,'actual original CTexture commands vs independent prediction');execute([exe,'shader',SHADER,m,s],f'shader-{repeat}');need(s.read_bytes()==expected_shader,'actual original ShaderAPI records vs independent prediction');traces[str(m)]=sha(m);traces[str(s)]=sha(s);print('original material -> shader CPU records repeat '+str(repeat)+' PASS',flush=True)
    before=sha(query);p=execute([exe,'material',MATERIAL,query,query],'existing-output',False);need(p.returncode!=0 and sha(query)==before,'existing output refused unchanged')
    # Actual original state contradicts one shared sampler when texture flags
    # differ. This is a regression requirement, not a passing runtime claim.
    normal=predict([0,0,1,2,1,0,4])[1][0];point_clamp=predict([1|4,0,1,2,1,0,4])[1][0];need(normal!=point_clamp,'independent material-channel counterexample')
    counterexample={'base_flags':0,'second_flags':5,'controlled_force_word':0,'controlled_hardware_capability':1,'mips':4,'base_D3D_record':normal,'second_D3D_record':point_clamp,'editor_limitation':'vulkan_prepared_model_images_candidate.h writes only binding1 using base lease sampler for both texture images; per-channel policy cannot be represented in current four-binding layout. prepared-material-draw-probe.hlsl also samples all three images with binding1.','needed_correction':'Independent per-texture sampler descriptors and shader consumption (base, second, mask as used); accurate flags/config propagation, no global guessed setting.'}
    put(run/'shared-sampler-counterexample.json',counterexample)
    need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in protected.items()),'final source/protected identities')
    result={'status':'VERIFIED bounded original texture commands and D3D sampler records; editor shared-sampler correction required','time_utc':datetime.now(timezone.utc).isoformat(),'previous_goal_turn':'PROGRESS','cases':len(rows),'material_command_events':commands,'per_frame_D3D_sampler_records':records_count,'repetitions':2,'trace_sha256':traces,'executable':{'path':str(exe),'sha256':sha(exe),'architecture':'PRIVATE X86 RESEARCH ORACLE, NOT THE X64 APPLICATION'},'compiler_warnings':warnings,'authenticated_ranges':[{k:v for k,v in e.items() if k!='instructions'} for e in e1+e2],'vtable_entries':{'79':'0x1001e1e0','80':'0x1001e2d0','81':'0x1001e150'},'jump_tables':tables,'counterexample':counterexample,'source_sha256':sources,'protected_sha256':protected,'limits':['CTexture wrap/filter/full frame loop and ShaderAPI record setters execute original unchanged CPU instructions in separate private PE images. Normalized command transport and receiver/device capability/configuration values are controlled; no actual original device query or DllMain/import loading.','Original Source format-conversion enabled/compression/nearest-format policy remains a separate known gate, not inferred from GPU BC support. Current cvars were requested from user but are not fabricated.','No original SFM process/control, existing runtime/editor change or publication. This verifies command/state logic, not a newly implemented normal runtime sampler or original shader/GPU parity.','Four-binding editor composition has a proven representational limitation for independent samplers; preserve this requirement and correct the real descriptor/shader consumers next. No disconnected production planner was added.']}
    receipt=run/'results.json';put(receipt,result);put(OUT/'latest-verification.json',{'receipt':str(receipt),'receipt_sha256':sha(receipt)});print(json.dumps({k:result[k] for k in ['status','cases','material_command_events','per_frame_D3D_sampler_records','counterexample']},indent=2),flush=True)
if __name__=='__main__':main()
