"""Authenticated native Phong pixel-selector slices vs independent x64/Python.

Controlled effective inputs, owned fog/hardware/cache binding services; no whole
native draw, material-state inference, GPU, editor change or SFM process use.
"""
from datetime import datetime, timezone
import hashlib
import itertools
import json
from pathlib import Path
import random
import re
import struct
import subprocess
import tempfile
from verify_session_preview_aspect import ROOT, VS
from verify_ifm_draw_packet import check_ranges
from vcs6_shader_cache_candidate import Vcs6ShaderCache
from inspect_generic_ambient_shader import disassemble

GAME=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
OUT=ROOT/'outputs/phong-pixel-variant-20261030'
RANGES=[('PS20_static_argument',0x100376f0,0x10037747),('PS30_static_argument',0x100377b0,0x10037810),
        ('PS20_dynamic_constructor',0x10049e00,0x10049e3d),('PS30_dynamic_constructor',0x10037810,0x10037844),
        ('PS20_static_fields_and_name',0x1003875c,0x10038ae6),('PS30_static_fields_and_bind',0x10038baf,0x10038f84),
        ('static_bind_common',0x10038f7d,0x10038f84),('PS20_dynamic_fields_and_argument',0x1003a8cb,0x1003a9c2),
        ('PS20_dynamic_bind_common',0x1003add4,0x1003add8),('PS30_dynamic_fields_and_argument',0x1003ac31,0x1003ad83),
        ('dynamic_program_gate',0x1003a7c7,0x1003a7df),('static_program_gate',0x100386b6,0x100386d7)]


def need(ok, why):
    if not ok:raise RuntimeError(why)


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def static_fields(version,x):
    b=lambda i:int(x[i]!=0)
    return [b(0),int(b(1) and not b(4)),int(b(2) and not b(4)),b(3),int(b(5) and b(6)),int(b(7) and b(6)),
            b(8),x[10],b(9),b(11),x[12],0,b(13) if version==30 else 0,b(14),b(15) if version==30 else 0,0]


def static_argument(version,f):
    # Independent coefficient form, not the owned Horner expression.
    weights=[1,2,4,8,16,32,64,128,896,1792,3584,0,0,10752,0,0]
    if version==30:weights[12],weights[13],weights[14]=10752,21504,43008
    return sum(a*b for a,b in zip(weights,f))*(80 if version==20 else 160)


def expected(serial,kind,x):
    version=30 if kind%2 else 20;calls=binds=0
    if kind<2:
        f=static_fields(version,x);value=static_argument(version,f);binds=1
    elif kind<4:
        suppressed=version==30 and x[6]!=0
        water=int(x[2]!=0 and not suppressed);fog=int(x[0]==2);lights=0 if suppressed else x[1]
        depth=int(x[3]!=0);shadows=int(x[4]!=0 and not suppressed);uber=int(version==30 and x[5]!=0)
        value=water+2*fog+4*lights+20*depth+40*shadows+80*uber
        f=[water,fog,lights,depth,shadows,uber]+[0]*10
        if version==20:f[4]=0
        else:f[2]=0
        calls=binds=1
    elif kind<6:
        f=x.copy();value=static_argument(version,f)
    else:
        version=30 if x[0]&255 else 20;value=version;f=[0]*16;calls=1
    return struct.pack('<22I',serial,kind,version,value,calls,binds,*f)


def cache_directory(data):
    h=struct.unpack_from('<7I',data);need(h[0]==6 and h[1]%h[2]==0,'VCS6 header')
    entries=[struct.unpack_from('<II',data,28+8*i) for i in range(h[5])]
    need(entries[-1]==(0xffffffff,len(data)),'cache sentinel')
    keys=[k for k,_ in entries[:-1]];offsets=[p for _,p in entries]
    need(keys==sorted(set(keys)) and offsets==sorted(set(offsets)),'cache directory ordering')
    at=28+8*h[5];count=struct.unpack_from('<I',data,at)[0]
    pairs=[struct.unpack_from('<II',data,at+4+8*i) for i in range(count)]
    need([k for k,_ in pairs]==sorted(set(k for k,_ in pairs)),'cache alias ordering')
    need(offsets[0]==at+4+count*8 and all(k<h[1]//h[2] for k in keys),'cache directory bounds')
    primary={key:(offsets[i],offsets[i+1]) for i,key in enumerate(keys)};aliases=dict(pairs)
    need(all(k<h[1]//h[2] and k not in primary and target in primary for k,target in pairs),'cache aliases')
    return h,primary,aliases


def selected_shader(data,static,dynamic):
    h,primary,aliases=cache_directory(data)
    if static not in primary and static not in aliases:raise KeyError('missing static slot')
    need(0<=dynamic<h[2],'dynamic range');target=aliases.get(static,static);a,b=primary[target];payload=data[a:b]
    fixture=data[:20]+struct.pack('<II',2,h[6])+struct.pack('<II',0,48)+struct.pack('<II',0xffffffff,48+len(payload))+struct.pack('<I',0)+payload
    return Vcs6ShaderCache(fixture).shader(0,dynamic),target


def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));(OUT/'active-run.txt').write_text(str(run),encoding='utf-8')
    latest=json.loads((ROOT/'outputs/phong-ambient-pixel-20261030/latest-verification.json').read_text())
    need(sha(latest['receipt'])==latest['receipt_sha256'],'prior receipt');prior=json.loads(Path(latest['receipt']).read_text())
    for group in ['source_sha256','protected_sha256']:
        need(all(sha(p)==h for p,h in prior[group].items()),'prior '+group)
    sources=dict(prior['source_sha256'])
    for name in ['phong_pixel_variant_candidate.h','probe_phong_pixel_variant.cpp',Path(__file__).name]:
        p=ROOT/'work'/name;sources[str(p)]=sha(p)
    protected=dict(prior['protected_sha256']);protected[latest['receipt']]=latest['receipt_sha256']
    std=GAME/'bin/stdshader_dx9.dll';image,evidence=check_ranges(std,'stdshader_dx9-disassembly.txt',RANGES)
    names={0x100964fc:'phong_ps20b',0x100964d0:'phong_ps30',0x1008d0b8:'LIGHTWARPTEXTURE',0x1009653c:'RIMLIGHT',
           0x1008c1f8:'NUM_LIGHTS',0x1008cffc:'WRITEWATERFOGTODESTALPHA',0x1008c9d8:'WRITE_DEPTH_TO_DESTALPHA',
           0x1008b3ac:'FLASHLIGHTSHADOWS',0x1008b360:'UBERLIGHT',0x10090008:'TOOL_MODE'}
    need(all(image.string(a)==s for a,s in names.items()),'native semantic labels')
    batch=run/'build.cmd';lines=['@echo off',f'cd /d "{ROOT}"']
    for arch,vcvars in [('x86',VS.parent/'vcvars32.bat'),('x64',VS)]:
        lines += [f'call "{vcvars}" >nul','if errorlevel 1 exit /b 1',
                  f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_phong_pixel_variant.cpp /Fo:"{run/(arch+".obj")}" /Fe:"{run/(arch+".exe")}" >"{run/(arch+"-build.log")}" 2>&1',
                  'if errorlevel 1 exit /b 1']
    batch.write_text('\n'.join(lines+['exit /b 0','']),encoding='utf-8')
    def execute(args,label,success=True):
        p=subprocess.run([str(a) for a in args],cwd=ROOT,capture_output=True,text=True,timeout=180)
        (run/(label+'.log')).write_text(p.stdout+p.stderr,encoding='utf-8')
        if success:need(p.returncode==0,f'{label} exit{p.returncode}: {(p.stdout+p.stderr)[-1500:]}')
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver')
    rng=random.Random(2026103013);queries=bytearray();wanted=bytearray();counts={};serial=0
    def append(kind,x,label):
        nonlocal serial
        need(len(x)==16,'query size');queries.extend(struct.pack('<18I',serial,kind,*x));wanted.extend(expected(serial,kind,x));serial+=1;counts[label]=counts.get(label,0)+1
    for version,size,kind in [(20,21504,4),(30,86016,5)]:
        for index in range(size):
            f=[0]*16;v=index
            for i in range(7):f[i]=v%2;v//=2
            f[7]=v%7;v//=7
            for i in [8,9]:f[i]=v%2;v//=2
            f[10]=v%3;v//=3
            if version==20:f[13]=v
            else:f[12],f[13],f[14]=v&1,(v>>1)&1,(v>>2)&1
            need(static_argument(version,f)==index*(80 if version==20 else 160),'exhaustive index inverse')
            append(kind,f,'complete_static_encoded_domain')
    boolean_indices=[i for i in range(16) if i not in (10,12)]
    for kind in range(2):
        for bits in range(1<<14):
            x=[0]*16
            for bit,i in enumerate(boolean_indices):x[i]=(bits>>bit)&1
            x[10]=bits%7;x[12]=(bits//7)%3;append(kind,x,'all_static_boolean_combinations')
        for _ in range(1024):
            x=[rng.randrange(256) for _ in range(16)];x[10]=rng.randrange(7);x[12]=rng.randrange(3);append(kind,x,'static_raw_bytes')
    fogs=[0,1,2,3,0xffffffff,0x80000000,0x7fffffff]
    for kind in [2,3]:
        for fog,lights,water,depth,shadows,uber,preview in itertools.product(fogs,range(5),range(2),range(2),range(2),range(2),[0,1,2,0x80000000,0xffffffff]):
            append(kind,[fog,lights,water,depth,shadows,uber,preview]+[0]*9,'dynamic_conditions')
        for role,raw in itertools.product(range(2,6),range(256)):
            x=[2,4,1,1,1,1,0]+[0]*9;x[role]=raw;append(kind,x,'dynamic_raw_bytes')
        for _ in range(1024):
            append(kind,[rng.getrandbits(32),rng.randrange(5),*[rng.randrange(256) for _ in range(4)],rng.choice([0,1,0xffffffff])]+[0]*9,'dynamic_seeded')
    for low in range(256):
        for high in [0,0xa57c0000]:append(6,[high|low]+[0]*15,'hardware_low_byte_route')
    query=run/'queries.bin';query.write_bytes(queries);binaries={}
    for arch,machine,magic in [('x86',0x14c,0x10b),('x64',0x8664,0x20b)]:
        exe=run/(arch+'.exe');raw=exe.read_bytes();p=struct.unpack_from('<I',raw,60)[0]
        need(struct.unpack_from('<H',raw,p+4)[0]==machine and struct.unpack_from('<H',raw,p+24)[0]==magic,'architecture')
        need(not re.search(r'warning C\d+', (run/(arch+'-build.log')).read_text(errors='replace')),'compiler warnings')
        binaries[arch]=dict(path=str(exe),sha256=sha(exe))
        for repeat in range(2):
            target=run/(arch+'-'+str(repeat)+'.bin');execute([exe,std if arch=='x86' else run/'absent-original.dll',query,target],arch+'-'+str(repeat))
            actual=target.read_bytes();need(actual==wanted,f'{arch} complete fields/argument/callback trace mismatch')
    print('native/owned trace cases='+str(serial),flush=True)
    cache_results={};catalog=[]
    for version in [20,30]:
        cache_path=GAME/'platform/shaders/fxc'/('phong_ps20b.vcs' if version==20 else 'phong_ps30.vcs')
        protected[str(cache_path)]=sha(cache_path);data=cache_path.read_bytes();h,primary,aliases=cache_directory(data)
        need(h[2]==(80 if version==20 else 160) and h[1]//h[2]==(21504 if version==20 else 86016),'cache compiled selector dimensions')
        cache_results[str(version)]=dict(path=str(cache_path),sha256=sha(cache_path),static_count=h[1]//h[2],dynamic_count=h[2],primary=len(primary),aliases=len(aliases),missing_static=h[1]//h[2]-len(primary)-len(aliases))
        profiles=[]
        profiles.append(('controlled-zero',[0]*16))
        x=[0]*16;x[5]=x[6]=x[11]=x[14]=1;profiles.append(('conditional-warp-rim-half',x.copy()))
        if version==30:x[15]=1;profiles.append(('conditional-warp-rim-half-tool',x.copy()))
        for label,x in profiles:
            fields=static_fields(version,x);argument=static_argument(version,fields);static=argument//h[2]
            for dynamic in [0,4,16,h[2]-1]:
                item=dict(version=version,profile=label,effective_inputs=x,static_fields=fields,static_argument=argument,static_index=static,dynamic_index=dynamic)
                try:
                    code,target=selected_shader(data,static,dynamic);item.update(status='PRESENT',primary=target,sha256=hashlib.sha256(code).hexdigest())
                    asm=disassemble(code)
                    item['samplers']=re.findall(r'//\s+(\w+)\s+s(\d+)\s+1\s*$',asm,re.M)
                    stem=f'private-ps{version}-{label}-d{dynamic}';(run/(stem+'.asm')).write_text(asm,encoding='utf-8');(run/(stem+'.bin')).write_bytes(code)
                except KeyError:item['status']='MISSING_REFUSED'
                catalog.append(item)
        need(any(c['version']==version and c['profile']=='controlled-zero' and c['dynamic_index']==0 and c['status']=='PRESENT' for c in catalog),'zero reference cache slot')
    (run/'catalog.json').write_text(json.dumps(catalog,indent=2)+'\n',encoding='utf-8')
    need(all(sha(p)==h for p,h in sources.items()),'source identities unchanged')
    need(all(sha(p)==h for p,h in protected.items()),'protected identities unchanged')
    result=dict(status='VERIFIED',time_utc=datetime.now(timezone.utc).isoformat(),cases=serial,coverage=counts,trace_bytes=len(wanted),trace_sha256=hashlib.sha256(wanted).hexdigest(),
                repetitions=2,executables=binaries,authenticated_ranges=[{k:v for k,v in row.items() if k!='instructions'} for row in evidence],
                cache_dimensions=cache_results,catalog_sha256=sha(run/'catalog.json'),source_sha256=sources,protected_sha256=protected,
                limits=['Native static/dynamic slices execute with explicit private stack inputs, debug logging off, owned fog/hardware/bind services and early exits.',
                        'Complete static argument helpers execute unchanged; hardware route low byte is controlled, not actual hardware queried.',
                        'Original material initialization/proxy state, full100380F0 draw, active benchmark permutation and output rendering NOT verified.',
                        'Catalog profiles are conditional effective inputs, not inferred from authored benchmark VMTs; missing cache slots refused.',
                        'No editor dependency/integration/rebuild, GPU calls, live SFM/computer use, original writes or publication.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt)),indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:result[k] for k in ['status','cases','coverage','trace_bytes','trace_sha256','executables','cache_dimensions']},indent=2),flush=True)


if __name__=='__main__':main()
