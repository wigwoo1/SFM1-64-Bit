"""CShaderSystem normal/bump/cube load policy, typed transitions and owned x64."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,itertools,json,random,re,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS
from verify_ifm_draw_packet import check_ranges

OUT=ROOT/'outputs/shader-texture-services-20261101'
GAME=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
LIB=GAME/'bin/materialsystem.dll'
STD=GAME/'bin/stdshader_dx9.dll'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(ok,why):
    if not ok:raise RuntimeError(why)
def text(s):
    data=s.encode('ascii');return struct.pack('<I',len(data))+data
def predict(q,name,group):
    serial,kind,typ,meta,flags,flags2,config,hdr,suppress,result,present=q;trace='';token=2
    def find(f,g,n):return 'F '+str(f)+' '+('G:'+g if g is not None else 'NULL')+' '+n+'\n'
    if typ==3:token=3
    elif typ!=1:trace='E\n'
    elif kind==0:
        if name.startswith('\\') or len(name)>1 and name[1]=='\\':name=name[1:]
        trace='C '+str(config)+'\n'
        if config&1 and name.lower()=='env_cubemap':token=0xffffffff
        else:
            trace+=find(flags,group if present else None,name);token=result
            if not token:
                trace+='S '+str(suppress)+'\n'
                if not suppress&255 and name.lower()!='env_cubemap':trace+='W '+name+'\n'
                trace+='E\n';token=2
    elif kind==1:
        trace=find(0,group if present else None,name);token=result
        if not token:trace+='E\n';token=2
    else:
        if name.lower()=='env_cubemap':token=0xffffffff;flags2|=32
        else:
            trace='H '+str(hdr)+'\n'
            if hdr:name=(name+'.hdr')[:259]
            trace+=find(flags,'CubeMap textures',name);token=result
            if not token:trace+='E\n';token=2
    refs=[1+int(token==i) for i in [1,2,3]]
    record=[serial,meta|3,0,0,0,0,0,token,flags2,*refs]
    return struct.pack('<12I',*record)+text(trace)

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));(OUT/'active-run.txt').write_text(str(run));print('Verification directory: '+str(run),flush=True)
    idx=json.loads((ROOT/'outputs/phong-texture-init-20261031/latest-verification.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'accepted stage receipt');prior=json.loads(Path(idx['receipt']).read_text());sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256']
    need(all(sha(p)==h for p,h in sources.items()),'prior source identities');need(all(sha(p)==h for p,h in protected.items()),'prior protected identities');need(sha(LIB)=='c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8','material-system identity');protected[str(LIB)]=sha(LIB)
    def visit(p):
        p=Path(p).resolve();key=str(p)
        if key in sources:return
        sources[key]=sha(p)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(),re.M):
            child=p.parent/inc
            if child.exists():visit(child)
    for n in ['material_shader_texture_init_candidate.h','probe_shader_texture_services.cpp',Path(__file__).name]:visit(ROOT/'work'/n)
    for folder in ['material-texture-value-lifetime-20261027','material-texture-lookup-20261027']:
        index=json.loads((ROOT/'outputs'/folder/'latest-verification.json').read_text());need(sha(index['receipt'])==index['receipt_sha256'],'accepted preceding boundary '+folder);protected[index['receipt']]=index['receipt_sha256']
    image,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('normal_texture_load',0x10037a30,0x10037b20),('bump_texture_load',0x10037b20,0x10037b94),
        ('cube_texture_load',0x10037ba0,0x10037cc5),('config_mask_query',0x100375d0,0x100375e4),
        ('texture_init_service_binding',0x10036e36,0x10036e5e),('SetTexture',0x10019710,0x10019825),
        ('release_previous',0x10017ad0,0x10017b46),('Clear',0x1001a0a0,0x1001a168)])
    need(image.word(0x101017f8)==0x101018b8 and image.word(0x101018b8)==0x1009c9dc and image.word(0x101018bc)==0x1009c9b4,'actual static shader-system binding')
    need([image.word(0x1009c9dc+4*i) for i in range(3)]==[0x10037a30,0x10037b20,0x10037ba0],'primary load methods')
    need(image.word(0x1009c9b4+24)==0x100375d0,'secondary config method')
    locator=image.word(0x1009c9d8);descriptor=image.word(locator+12);need(image.string(descriptor+8)=='.?AVCShaderSystem@@' and image.word(locator+4)==0,'primary RTTI identity')
    need(image.string(0x10099024)=='env_cubemap' and image.string(0x100968b0)=='CubeMap textures' and image.string(0x1009ab74)=='.hdr','native literals')
    _,std_ranges=check_ranges(STD,'stdshader_dx9-disassembly.txt',[('texture_public_wrapper',0x10064a60,0x10064aa3),('Phong_dispatch',0x10055000,0x10055052)])
    binding={'global':'0x101017f8','object':'0x101018b8','primary_vtable':'0x1009c9dc','secondary_vtable':'0x1009c9b4','rtti':'.?AVCShaderSystem@@','normal':'0x10037a30','bump':'0x10037b20','cube':'0x10037ba0','manager_find_slot':7,'manager_error_slot':16,'normal_bypasses_public_FindTexture_slot83':True};(run/'service-binding.json').write_text(json.dumps(binding,indent=2))
    batch=run/'build.cmd';lines=['@echo off',f'cd /d "{ROOT}"']
    for arch,vcvars in [('x86',VS.parent/'vcvars32.bat'),('x64',VS)]:
        lines += [f'call "{vcvars}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_shader_texture_services.cpp /Fo:"{run/(arch+".obj")}" /Fe:"{run/(arch+".exe")}" >"{run/(arch+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1']
    batch.write_text('\n'.join(lines+['exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1200:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');print('Fresh probe builds completed',flush=True)
    rows=[];coverage={};rng=random.Random(2026110101)
    def add(kind=0,typ=1,name='fixture/albedo',group='Other textures',present=1,meta=64,flags=0x80000,flags2=0,config=0,hdr=0,suppress=0,result=1,label='controlled'):
        q=[len(rows),kind,typ,meta,flags,flags2,config,hdr,suppress,result,present];rows.append((q,name,group));coverage[label]=coverage.get(label,0)+1
    names=['fixture/albedo','env_cubemap','ENV_CUBEMAP','\\env_cubemap','a\\env_cubemap','\\\\server/path','/rooted/logical','fixture/existing.hdr','x','x'*255,'x'*256,'x'*259]
    for kind,name,result,present in itertools.product(range(3),names,range(4),range(2)):add(kind=kind,name=name,result=result,present=present,config=1,hdr=1,label='names_and_results')
    for typ,meta,kind in itertools.product([0,1,2,3,4,6],range(0,256,16),range(3)):add(kind=kind,typ=typ,meta=meta,label='initial_storage_and_metadata')
    for byte,high in itertools.product(range(256),[0,0xa5b60000]):
        add(name='env_cubemap',config=high|byte,label='config_low_bit')
        add(result=0,suppress=high|byte,label='suppress_low_byte')
        add(kind=2,hdr=high|byte,label='HDR_full_DWORD')
    for kind,result,flags,flags2,config,hdr,suppress in itertools.product(range(3),range(4),[0,0x80000,0xffffffff],[0,0x20,0xffffffff],[0,1],[0,0x100],[0,0x100]):
        add(kind=kind,result=result,flags=flags,flags2=flags2,config=config,hdr=hdr,suppress=suppress,label='raw_flag_and_gate_matrix')
    for _ in range(1024):add(kind=rng.randrange(3),typ=rng.choice([0,1,2,3,4,6]),name=rng.choice(names),present=rng.randrange(2),meta=rng.randrange(16)*16,flags=rng.getrandbits(32),flags2=rng.getrandbits(32),config=rng.getrandbits(32),hdr=rng.choice([0,rng.getrandbits(32)]),suppress=rng.choice([0,0x100,rng.getrandbits(32)]),result=rng.randrange(4),label='seeded')
    query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<11I',*q)+text(n)+text(g) for q,n,g in rows));wanted=b''.join(predict(*r) for r in rows);(run/'expected.bin').write_bytes(wanted);executables={};warnings={}
    for arch,machine,magic in [('x86',0x14c,0x10b),('x64',0x8664,0x20b)]:
        exe=run/(arch+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic,'architecture '+arch);warnings[arch]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(arch+'-build.log')).read_text());need(not warnings[arch],'compiler warnings '+arch);executables[arch]={'path':str(exe),'sha256':sha(exe)}
        for repeat in range(2):
            dest=run/(arch+'-'+str(repeat)+'.bin');execute([exe,LIB if arch=='x86' else run/'absent-original.dll',query,dest],arch+'-'+str(repeat));observed=dest.read_bytes()
            if observed!=wanted:
                at=next((i for i,(x,y) in enumerate(zip(observed,wanted)) if x!=y),min(len(observed),len(wanted)));cursor=0
                for row in rows:
                    expected=predict(*row)
                    if cursor<=at<cursor+len(expected):(run/'mismatch.json').write_text(json.dumps({'row':row,'relative_byte':at-cursor,'expected':list(expected),'observed':list(observed[cursor:cursor+len(expected)])},indent=2));break
                    cursor+=len(expected)
                raise RuntimeError(arch+' mismatch byte '+str(at)+'; see mismatch.json')
        print(arch+' complete repeated trace matches',flush=True)
    safe=execute([run/'x64.exe','--test'],'owned-safety');need('PASS owned_safety_checks=20' in safe.stdout,'owned transaction safety count')
    refusals=[]
    for arch in ['x86','x64']:
        before=sha(query);p=execute([run/(arch+'.exe'),LIB if arch=='x86' else run/'absent-original.dll',query,query],arch+'-existing-output',False);need(p.returncode!=0 and sha(query)==before,'existing output unchanged');refusals.append(arch+'-existing-output')
    need(all(sha(p)==h for p,h in sources.items()),'source identities after');need(all(sha(p)==h for p,h in protected.items()),'protected identities after')
    result={'status':'VERIFIED','time_utc':datetime.now(timezone.utc).isoformat(),'previous_goal_turn':'PROGRESS: complete Phong texture-init and wrappers verified','cases':len(rows),'coverage':coverage,'repetitions':2,'trace_bytes':len(wanted),'trace_sha256':sha(run/'expected.bin'),'executables':executables,'compiler_warnings':warnings,'owned_safety':safe.stdout,'refusals':refusals,'service_binding':binding,'authenticated_material_ranges':[{k:v for k,v in x.items() if k!='instructions'} for x in ranges],'authenticated_stdshader_ranges':[{k:v for k,v in x.items() if k!='instructions'} for x in std_ranges],'source_sha256':sources,'protected_sha256':protected,'limits':['Complete native normal/bump/cube service methods and native CMaterialVar constructors/SetTexture/Clear execute with controlled manager, error resource, config, HDR, debug and resource reference services.','Actual manager miss loading, error-texture construction, filesystem/GPU behavior, queued/parent-owned variables and native texture contents remain unverified.','Owned results retain validated immutable 2D CPU VTF fixture bytes. This is resource identity/value policy, not genuine cubemap VTF support or native GPU textures.','ASCII nonempty input name<=259 and group<=255 only; native unsafe empty/overlength/locale behavior is not inferred.','No production/editor integration, complete editor rebuild, original writes, live SFM/computer use or publication.']}
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps({'receipt':str(receipt),'receipt_sha256':sha(receipt)},indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','coverage','trace_bytes','trace_sha256','owned_safety','executables','compiler_warnings']},indent=2),flush=True)

if __name__=='__main__':main()
