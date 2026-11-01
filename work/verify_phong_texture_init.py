"""Private complete Phong texture-init control flow; actual loading is a boundary."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,itertools,json,random,re,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS
from verify_ifm_draw_packet import check_ranges
from verify_shader_parameter_registry import decode

OUT=ROOT/'outputs/phong-texture-init-20261031'
STD=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll')
PARAMS=[('$flags',0,None),('$flags2',2,None),('$flashlighttexture',9,0x64),('$basetexture',6,0),
        ('$compress',14,4),('$stretch',15,8),('$phongexponenttexture',47,0x88),('$phong',48,0xa8),
        ('$lightwarptexture',43,0x8c),('$phongwarptexture',44,0x90),('$detail',17,0x18),
        ('$detailblendmode',54,0xb8),('$bumpmap',26,0x3c),('$bumpcompress',27,0x40),
        ('$bumpstretch',28,0x44),('$envmap',20,0x24),('$selfillumfresnel',34,0x70),
        ('$selfillummask',52,0x148),('$displacementmap',102,0x170)]
REQUIRED=[0,1,3,15]

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(ok,why):
    if not ok:raise RuntimeError(why)

def predict(row):
    serial,config,hdr,*words=row;s=[words[i:i+5] for i in range(0,95,5)];events=[]
    def event(k,p,a=0,b=0):events.append((k,p,a,b))
    def present(p):return s[p][0]!=0
    def defined(p):
        need(present(p),'prediction absent definition');v=int(bool(s[p][1]&255));event(1,p,v);return v
    def load(p,kind=0,flags=0):
        if present(p) and defined(p):event(6,p,kind,flags);s[p][3]=s[p][4]
    def setint(p,v):s[p][2]=v;event(2,p,v)
    load(2,flags=0x80000)
    alpha=0
    if defined(3):
        load(3,flags=0x80000);event(3,3);alpha=int(bool(s[3][3]&255));event(4,3,alpha)
        if present(4) and present(5) and defined(4) and defined(5):load(4,flags=0x80000);load(5,flags=0x80000)
    mask=bool(s[0][2]&64 and s[1][2]&0x400000)
    if not alpha:
        keep=bool(s[0][2]&64 and present(16) and s[16][2]) or mask
        if not keep:setint(0,s[0][2]&0xffffffbf)
        setint(0,s[0][2]&0xffefffff)
    for p in [6,8,9]:
        if present(p) and defined(p) and present(7) and defined(7):load(p)
    if present(10) and defined(10):load(10,flags=0x80000 if present(11) and s[11][2] in [2,3,4] else 0)
    if not config&256 and present(12) and defined(12):
        load(12,kind=1);setint(1,s[1][2]|16)
        if present(13) and present(14) and defined(13) and defined(14):load(13);load(14)
    if defined(15):event(5,15,hdr);load(15,kind=2,flags=0 if hdr else 0x80000)
    if mask:load(17)
    if present(18) and defined(18):load(18)
    data=struct.pack('<2I95I',serial,len(events),*[v for state in s for v in state])+b''.join(struct.pack('<4I',*e) for e in events)
    return data,s,events

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));(OUT/'active-run.txt').write_text(str(run));print('Verification directory: '+str(run),flush=True)
    index=json.loads((ROOT/'outputs/phong-material-features-20261031/latest-verification.json').read_text());need(sha(index['receipt'])==index['receipt_sha256'],'accepted feature receipt')
    previous=json.loads(Path(index['receipt']).read_text());sources=dict(previous['source_sha256']);protected=dict(previous['protected_sha256']);protected[index['receipt']]=index['receipt_sha256']
    need(all(sha(p)==h for p,h in sources.items()),'prior source identities');need(all(sha(p)==h for p,h in protected.items()),'prior protected identities')
    for name in ['phong_texture_init_candidate.h','probe_phong_texture_init.cpp',Path(__file__).name]:sources[str(ROOT/'work'/name)]=sha(ROOT/'work'/name)
    profile=OUT.parent/'phong-material-features-20261031/reference-profile-20261031.json';protected[str(profile)]=sha(profile)
    image,ranges=check_ranges(STD,'stdshader_dx9-disassembly.txt',[
        ('complete_texture_init',0x10037bd0,0x10037f33),('texture_wrapper',0x10064ba0,0x10064be6),
        ('bump_wrapper',0x10064bf0,0x10064c33),('cube_wrapper',0x10064c40,0x10064c87),
        ('schema_handoff',0x10052f10,0x100532a2),('phong_texture_dispatch',0x10055000,0x10055052)])
    meta=ROOT/'outputs/material-effective-parameters-20261017/verification-l11z2le6';m=json.loads((meta/'shader-metadata.json').read_text());need(m==decode((meta/'registry-0.bin').read_bytes()),'registry decode')
    material=next(x for x in m if x['name']=='VertexLitGeneric');loaded={};links={};immediates={}
    for row in next(x for x in ranges if x['name']=='schema_handoff')['instructions']:
        ins=row['instruction'];m=re.fullmatch(r'mov\s+(ecx|edx),dword ptr ds:\[([0-9A-F]+)h\]',ins)
        if m:loaded[m[1]]=int(m[2],16)
        m=re.fullmatch(r'mov\s+dword ptr \[eax(?:\+([0-9A-F]+)h?)?\],(ecx|edx)',ins)
        if m:links[int(m[1] or '0',16)]=loaded[m[2]]
        m=re.fullmatch(r'mov\s+dword ptr \[eax(?:\+([0-9A-F]+)h?)?\],([0-9A-F]+)h?',ins)
        if m:immediates[int(m[1] or '0',16)]=int(m[2],16)
    for name,i,offset in PARAMS:
        p=material['parameters'][i];need(p['name'].lower()==name,'parameter index '+name)
        if offset is not None:
            if i in [6,9]:need(immediates[offset]==i,'fixed native index '+name)
            else:need(links[offset]==image.base+p['rva']+20,'native info link '+name)
    batch=run/'build.cmd';commands=['@echo off',f'cd /d "{ROOT}"']
    for arch,vcvars in [('x86',VS.parent/'vcvars32.bat'),('x64',VS)]:
        commands += [f'call "{vcvars}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_phong_texture_init.cpp /Fo:"{run/(arch+".obj")}" /Fe:"{run/(arch+".exe")}" >"{run/(arch+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1']
    batch.write_text('\n'.join(commands+['exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1200:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver')
    rows=[];coverage={};rng=random.Random(2026103102)
    def full():return [[1,1,1,0,1] for _ in range(19)]
    def add(s=None,config=0,hdr=0,label='controlled'):
        if s is None:s=full()
        rows.append([len(rows),config,hdr,*[v for st in s for v in st]]);coverage[label]=coverage.get(label,0)+1
    # Analytical conditions are checked independently of the full trace comparison.
    s=full();s[0][2]=0x100040;s[1][2]=0;s[3][4]=0;s[16][2]=0;add(s);_,final,events=predict(rows[-1]);need(final[0][2]==0 and (2,0,0x100000,0) in events,'alpha-less mask clear')
    s=full();s[0][2]=0x100040;s[1][2]=0x400000;s[3][4]=0;s[16][2]=0;add(s);_,final,events=predict(rows[-1]);need(final[0][2]==64 and (6,17,0,0) in events,'explicit self mask preserves selfillum')
    s=full();s[7][2]=0;add(s);need((6,8,0,0) in predict(rows[-1])[2],'Phong definition, not integer truth')
    s=full();s[3][4]=0x100;add(s);need((4,3,0,0) in predict(rows[-1])[2],'texture query uses low byte')
    s=full();add(s,config=0x100,hdr=0x100);e=predict(rows[-1])[2];need((6,15,2,0) in e and not any(x[0]==6 and x[1]==12 for x in e),'HDR full DWORD and bump configuration')
    optional=[i for i in range(19) if i not in REQUIRED]
    for mask in range(1<<len(optional)):
        s=full();s[0][2]=0x100040;s[1][2]=0x400000
        for b,i in enumerate(optional):s[i][0]=(mask>>b)&1
        add(s,label='all_optional_declaration_patterns')
    checks=[2,3,4,5,6,7,8,9,10,12,13,14,15,17,18]
    for mask in range(1<<len(checks)):
        s=full();s[0][2]=0x100040;s[1][2]=0x400000
        for b,i in enumerate(checks):s[i][1]=(mask>>b)&1
        add(s,label='all_definition_patterns')
    for i,byte,high in itertools.product(checks,range(256),[0,0xa5b60000]):
        s=full();s[i][1]=high|byte;add(s,label='definition_low_bytes')
    for byte,high in itertools.product(range(256),[0,0x12340000]):
        s=full();s[3][4]=high|byte;add(s,label='texture_query_low_bytes')
    for flags,flags2,mode,config,hdr in itertools.product([0,64,0x100000,0x100040,0xffffffff],[0,0x400000],[0,1,2,3,4,5,0xffffffff],[0,0x100],[0,1,2,0x100,0x80000000]):
        s=full();s[0][2]=flags;s[1][2]=flags2;s[11][2]=mode;s[3][4]=0;s[16][2]=0;add(s,config,hdr,'flag_detail_config_hdr_matrix')
    for _ in range(2048):
        s=[[rng.randrange(2),rng.choice([0,1,255,0x100,0xffffffff]),rng.getrandbits(32),rng.getrandbits(32),rng.choice([0,1,255,0x100,0xffffffff])] for _ in range(19)]
        for i in REQUIRED:s[i][0]=1
        add(s,rng.getrandbits(32),rng.choice([0,1,2,0x100,0xffffffff]),'seeded')
    query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<98I',*row) for row in rows));wanted=bytearray();total_events=0
    for row in rows:
        data,_,events=predict(row);wanted.extend(data);total_events+=len(events)
    expected=run/'expected.bin';expected.write_bytes(wanted);executables={};warnings={}
    for arch,machine,magic in [('x86',0x14c,0x10b),('x64',0x8664,0x20b)]:
        exe=run/(arch+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic,'architecture '+arch)
        warnings[arch]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(arch+'-build.log')).read_text());need(not warnings[arch],'compiler warnings '+arch)
        executables[arch]={'path':str(exe),'sha256':sha(exe)}
        for repeat in range(2):
            dest=run/(arch+'-'+str(repeat)+'.bin');execute([exe,STD if arch=='x86' else run/'absent-original.dll',query,dest],arch+'-'+str(repeat));observed=dest.read_bytes()
            if observed!=wanted:
                first=next((i for i,(a,b) in enumerate(zip(observed,wanted)) if a!=b),min(len(observed),len(wanted)));cursor=0;case=None
                for row in rows:
                    length=len(predict(row)[0])
                    if cursor<=first<cursor+length:case={'serial':row[0],'row':row,'offset':first-cursor,'expected':list(wanted[cursor:cursor+length]),'observed':list(observed[cursor:cursor+length])};break
                    cursor+=length
                (run/'mismatch.json').write_text(json.dumps(case,indent=2));raise RuntimeError(arch+' trace mismatch byte '+str(first)+'; see mismatch.json')
    refusals=[]
    for i in REQUIRED:
        row=rows[0].copy();row[3+i*5]=0;bad=run/('missing-'+str(i)+'.bin');bad.write_bytes(struct.pack('<98I',*row))
        for arch in ['x86','x64']:
            label='missing-'+str(i)+'-'+arch;p=execute([run/(arch+'.exe'),STD if arch=='x86' else run/'absent-original.dll',bad,run/(label+'.bin')],label,False);need(p.returncode!=0,'required index refusal '+label);refusals.append(label)
    for arch in ['x86','x64']:
        before=sha(query);p=execute([run/(arch+'.exe'),STD if arch=='x86' else run/'absent-original.dll',query,query],arch+'-existing-output',False);need(p.returncode!=0 and sha(query)==before,'existing output intact');refusals.append(arch+'-existing-output')
    need(all(sha(p)==h for p,h in sources.items()),'source identities after');need(all(sha(p)==h for p,h in protected.items()),'protected identities after')
    result={'status':'VERIFIED','time_utc':datetime.now(timezone.utc).isoformat(),'cases':len(rows),'ordered_events':total_events,'trace_bytes':len(wanted),'trace_sha256':sha(expected),'coverage':coverage,'repetitions':2,'executables':executables,'compiler_warnings':warnings,'refusals':refusals,'parameters':PARAMS,'authenticated_ranges':[{k:v for k,v in e.items() if k!='instructions'} for e in ranges],'source_sha256':sources,'protected_sha256':protected,'limits':['Complete native stage plus all three load wrappers; texture and hardware services are controlled callbacks.','Actual file resolution, texture-manager loading/error outcomes, native method implementations, proxies and active benchmark shader remain UNVERIFIED.','Base texture GetTextureValue and virtual offset18 result are explicit callback inputs, not inferred from VTF flags.','Native globals contain a known bound private parameter array, service, config and hardware object. Arbitrary absent global services are outside this stage comparison.','No editor integration/rebuild, GPU, live SFM/computer use, original writes or publication.']}
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps({'receipt':str(receipt),'receipt_sha256':sha(receipt)},indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','ordered_events','trace_bytes','trace_sha256','coverage','executables','compiler_warnings','refusals']},indent=2),flush=True)

if __name__=='__main__':main()
