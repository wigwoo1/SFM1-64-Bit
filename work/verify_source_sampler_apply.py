"""Observe final original pixel BindTexture submissions, direct/queued/cache modes."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS
from verify_source_texture_sampling import SHADER
from verify_ifm_draw_packet import check_ranges
OUT=ROOT/'outputs/source-sampler-apply-20261101'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def put(p,x):Path(p).write_text(json.dumps(x,indent=2)+'\n')
def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
    idx=json.loads((ROOT/'outputs/source-sampler-config-20261101/latest-milestone.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'previous checkpoint');prior=json.loads(Path(idx['receipt']).read_text());sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256']
    for group in [sources,protected]:need(all(sha(p)==v for p,v in group.items()),'preceding identities')
    for name in ['probe_source_sampler_apply_x86.cpp',Path(__file__).name]:sources[str(ROOT/'work'/name)]=sha(ROOT/'work'/name)
    audit=json.loads((OUT/'native-audit.local.json').read_text());need(sha(SHADER)==audit['module_sha256']=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa','original ShaderAPI identity');_,ranges=check_ranges(SHADER,'shaderapidx9-disassembly.txt',[(r['name'],int(r['start'],16),int(r['end_exclusive'],16)) for r in audit['ranges']]);protected[str(OUT/'native-audit.local.json')]=sha(OUT/'native-audit.local.json')
    verified=json.loads(Path(prior['verification']['receipt']).read_text());source_run=Path(prior['verification']['receipt']).parent;config_path=source_run/'config-0.bin';record_path=source_run/'records-0.bin';need(all(sha(p)==verified['trace_sha256'][str(p)] for p in [config_path,record_path]),'actual original config/record inputs');configs=config_path.read_bytes();records=record_path.read_bytes();count=struct.unpack_from('<I',configs)[0];need(count==struct.unpack_from('<I',records)[0]==50305,'input case count')
    batch=run/'build.cmd';batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS.parent/"vcvars32.bat"}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_source_sampler_apply_x86.cpp /Fo:"{run/"apply.obj"}" /Fe:"{run/"apply.exe"}" >"{run/"build.log"}" 2>&1','if errorlevel 1 exit /b 1','exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=300,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1800:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');exe=run/'apply.exe';b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==0x14c and struct.unpack_from('<H',b,pe+24)[0]==0x10b,'private x86 only');warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/'build.log').read_text());need(not warnings,'warning-free apply oracle')
    observations={};stats=[]
    for repeat in range(2):
        trace=run/f'apply-{repeat}.bin';p=execute([exe,SHADER,config_path,record_path,trace],f'apply-{repeat}');data=trace.read_bytes();need(struct.unpack_from('<I',data)[0]==count,'output count');at=4;ca=4;ra=4;bindings=events=0;zero_filter_calls=0;cached_skips=0;queued_events=0
        for serial in range(count):
            config_header=struct.unpack_from('<12I',configs,ca);ca+=48;configuration=configs[ca:ca+100];ca+=112;need(config_header[0]==serial,'config serial');record_header=struct.unpack_from('<9I',records,ra);ra+=36;need(record_header[0]==serial,'record serial');n=record_header[-1];need(struct.unpack_from('<2I',data,at)==(serial,n),'output identity');at+=8
            for frame in range(n):
                r=struct.unpack_from('<7I',records,ra);ra+=28;need(struct.unpack_from('<I',data,at)[0]==r[0],'frame identity');at+=4;u,v,w,mag,minimum,mip=r[1:]
                for variant in range(6):
                    enabled=variant%3!=1;active=variant%3!=2;stage=(serial+frame)%16
                    def values(filter_on,mips_on):return [u,v,w,minimum if filter_on else 0,mag if filter_on else 0,(mip if filter_on else 1) if mips_on else 0]
                    first=values(bool(configuration[0x3c]),bool(configuration[0x48]));changed=values(not configuration[0x3c],not configuration[0x48])
                    for phase in range(4):
                        expected=[];getters=0
                        if enabled and phase!=1:
                            expected=[(0,stage,0x4000,0)];getters=2
                            if active:
                                if phase==0:expected += [(1,stage,key,value) for key,value in zip([1,2,3,6,5,7],first)]
                                if phase==2:expected += [(1,stage,key,value) for key,value,previous in zip([1,2,3,6,5,7],changed,first) if value!=previous]
                        header=struct.unpack_from('<4I',data,at);at+=16;actual=[struct.unpack_from('<4I',data,at+16*i) for i in range(header[3])];at+=16*header[3]
                        if header!=(variant,phase,getters,len(expected)) or actual!=expected:
                            put(run/'mismatch.json',{'serial':serial,'frame':frame,'record':r,'configuration_filter':configuration[0x3c],'configuration_mips':configuration[0x48],'variant':variant,'phase':phase,'header':header,'expected_header':[variant,phase,getters,len(expected)],'actual':actual,'expected':expected});raise RuntimeError('original device submission mismatch '+str((serial,frame,variant,phase)))
                        bindings+=1;events+=len(actual);zero_filter_calls+=sum(e[0]==1 and e[2] in [5,6] and e[3]==0 for e in actual);cached_skips+=int(phase==1 and not actual);queued_events+=len(actual) if variant>=3 else 0
        need(at==len(data) and ca==len(configs) and ra==len(records),'complete trace framing');observations[str(trace)]=sha(trace);stats.append({'bind_calls':bindings,'events':events,'zero_filter_arguments':zero_filter_calls,'same_texture_unforced_skips':cached_skips,'queued_events':queued_events});print('PASS final original device submissions repeat '+str(repeat)+' '+json.dumps(stats[-1]),flush=True)
    need(stats[0]==stats[1] and (run/'apply-0.bin').read_bytes()==(run/'apply-1.bin').read_bytes(),'exact repeats');before=sha(config_path);p=execute([exe,SHADER,config_path,record_path,config_path],'existing-output',False);need(p.returncode!=0 and sha(config_path)==before,'existing trace preserved')
    for group in [sources,protected]:need(all(sha(p)==v for p,v in group.items()),'final protected/source identities')
    result={'status':'VERIFIED final original pixel texture binding/device argument stream under controlled direct and queued endpoints; original GPU acceptance and x64 default policy remain separate','time_utc':datetime.now(timezone.utc).isoformat(),'previous_goal_turn':'PROGRESS','cases':count,'repetitions':2,'stats':stats,'executable':{'path':str(exe),'sha256':sha(exe),'architecture':'PRIVATE X86 RESEARCH ONLY','compiler_warnings':warnings},'authenticated_ranges':[{k:v for k,v in r.items() if k!='instructions'} for r in ranges],'source_sha256':sources,'protected_sha256':protected,'trace_sha256':observations,'source_native_config':prior['verification'],'limits':['Complete original pixel BindTexture plus complete sampler wrapper/dispatcher execute unchanged private bytes. Actual original config/ShaderAPI record traces are inputs. Fake COM endpoints return success and record calls; no actual device/driver acceptance or GPU semantics established.','Direct and queued records, enabled/disabled stages, deactivation, unforced same-texture skip, forced rebind with config toggles, and duplicate state suppression checked. Queue has adequate bounded capacity; flush/replenish/error paths not exercised.','Non-null single-copy texture tokens with memory-budget bypass and controlled frame epoch; texture allocation/update scheduling, null/multicopy textures, sRGB/border color/max-aniso state and later changes not covered.','Filtering disabled produces exact numeric0 min/mag arguments, not assumed to equal Vulkan point filtering1. Mipmap disabled forces mip0; filter disabled with mip enabled forces mip1. User reported settings1 produce original stored min/mag/mip unchanged.','Original SFM and files untouched, no production/editor source changes or rebuild/publication. Native x86 probe is not the native x64 application.']}
    receipt=run/'results.json';put(receipt,result);put(OUT/'latest-verification.json',{'receipt':str(receipt),'receipt_sha256':sha(receipt)});print(json.dumps({k:result[k] for k in ['status','cases','repetitions','stats']},indent=2),flush=True)
if __name__=='__main__':main()
