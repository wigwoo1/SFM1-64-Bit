"""Actual authored D3D9/Vulkan border samples; original reset path static audit."""
from pathlib import Path
from datetime import datetime,timezone
import itertools,json,math,os,re,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS,DUMP,INCLUDE
from verify_source_border_common import OUT,sha,need,put,checkpoint
from verify_texture_sampling_cross_api import fixture,string
from verify_source_texture_sampling import SHADER
from verify_ifm_draw_packet import check_ranges

def main():
    run=Path(tempfile.mkdtemp(prefix='cross-api-',dir=OUT));print('GPU directory: '+str(run),flush=True)
    sources,protected,transitions=checkpoint()
    need(sha(SHADER)=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa','authenticated original ShaderAPI')
    image,ranges=check_ranges(SHADER,'shaderapidx9-disassembly.txt',[('pixel sampler initialization loop',0x100200c2,0x10020552)])
    instructions={r['address']:r['instruction'] for r in ranges[0]['instructions']}
    for at,parts in [(0x100204c8,['jne','1002053C']),(0x10020509,['mov','[eax+8],4']),(0x10020516,['mov','[ecx+0Ch],0']),(0x1002052e,['push','0']),(0x10020530,['push','4']),(0x10020532,['push','edi']),(0x10020534,['[edx+00000114h]']),(0x10020546,['[eax+00000230h]']),(0x1002054c,['jl','100200E0'])]:
        need(all(p in instructions[at] for p in parts),'literal border-reset instruction '+hex(at))
    audit=run/'native-reset-audit.local.json';put(audit,{'module':str(SHADER),'module_sha256':sha(SHADER),'method':'Static complete byte/disassembly identity check for bounded pixel-sampler initialization loop. NOT execution of whole reset routine; no original process or DLL loading.','ranges':ranges,'meaning':'When active, direct SetSamplerState(stage,4,0) or queued command17(stage,4,0); loop bound hardware object+0x230. Prior whole normal bind/apply proof emits six recorded address/filter states and preserves this border color. No claim every live original state observed.'});protected[str(audit)]=sha(audit)
    points=[]
    for frame,mip in itertools.product(range(2),range(4)):
        w=max(1,8>>mip);points.extend((frame,(x+.5)/w,(y+.5)/w,float(mip)) for y in range(w) for x in range(w))
    coords=[(-1.,.3125),(2.,.3125),(.3125,-1.),(.3125,2.),(-1.,2.),(0.,.3125),(1.,.3125),(.3125,0.),(.3125,1.),(-.03125,.3125),(.03125,.3125),(.96875,.3125),(1.03125,.3125),(.234375,.4609375)]
    points.extend((frame,u,v,lod) for frame in range(2) for u,v in coords for lod in [-.25,0.,.25,1.,1.25,2.75,7.])
    rows=[];fixtures={}
    for fmt in [13,15,0]:
        for family in range(8 if fmt!=0 else 1):
            path=run/f'authored-{fmt}-{family}.vtf';path.write_bytes(fixture(fmt,family));fixtures[str(path)]=sha(path)
            for srgb,linear,mip_linear,addresses in itertools.product(range(2),range(2),range(2),[(0,0),(2,2),(3,0),(0,3),(3,3),(3,2),(2,3)]):rows.append({'format':fmt,'family':family,'config':[srgb,linear,linear,mip_linear,*addresses,0,0xffffffff],'source':str(path)})
    put(run/'cases.json',{'rows':rows,'points':points});batches=[rows[a:a+128] for a in range(0,len(rows),128)];queries=[]
    for i,batch in enumerate(batches):
        p=run/f'queries-{i}.bin';p.write_bytes(struct.pack('<I',len(batch))+b''.join(struct.pack('<8I',*r['config'])+string('fixture/border/'+str(r['format'])+'/'+str(r['family']))+string(r['source'])+struct.pack('<I',len(points))+b''.join(struct.pack('<I3f',*point) for point in points) for r in batch));queries.append(p)
    dxc=Path(os.environ.get('SFM64_DXC_EXE',str(ROOT/'work/dependencies/dxc-v1.9.2609/bin/x64/dxc.exe')));shader=run/'sampling.spv';build=run/'build.cmd'
    commands=['@echo off',f'cd /d "{ROOT}"',f'"{dxc}" -spirv -fspv-target-env=vulkan1.0 -T cs_6_0 -E main work\\texture-binding-sample-probe.hlsl -Fo "{shader}" >"{run/"shader-build.log"}" 2>&1','if errorlevel 1 exit /b 1']
    for label,source,libs in [('vulkan','probe_vulkan_prepared_texture_binding.cpp',''),('d3d9','probe_texture_sampling_d3d9_x64.cpp','d3d9.lib d3dcompiler.lib user32.lib')]:commands+=['setlocal',f'call "{VS}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /I"{INCLUDE}" work\\{source} /Fo:"{run/(label+".obj")}" /Fe:"{run/(label+".exe")}" {libs} >"{run/(label+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1','endlocal']
    build.write_text('\n'.join(commands+['exit /b 0','']))
    def execute(args,label):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=300,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1600:]);return p.stdout+p.stderr
    execute(['cmd.exe','/d','/c',build],'build-driver');executables={};warnings={};imports={}
    for label in ['d3d9','vulkan']:
        p=run/(label+'.exe');data=p.read_bytes();pe=struct.unpack_from('<I',data,60)[0];need(struct.unpack_from('<H',data,pe+4)[0]==0x8664 and struct.unpack_from('<H',data,pe+24)[0]==0x20b,'AMD64 PE32+ '+label);executables[label]={'path':str(p),'sha256':sha(p)};warnings[label]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(label+'-build.log')).read_text());imports[label]=execute([DUMP,'/dependents',p],label+'-imports');need(not re.search(r'(?i)(?:ifm|studiorender|shaderapi|tier0|vstdlib|engine|materialsystem)\.dll',imports[label]),'no original runtime imports')
    need(not warnings['d3d9'] and len(warnings['vulkan'])==3 and all(c=='C4996' and "'getenv'" in m for c,m in warnings['vulkan']),'only inherited warnings');need(not (run/'shader-build.log').read_text().strip(),'no shader diagnostics')
    traces={};logs=[];adapter=None;defaults=0
    for repeat in range(2):
        values={'d3d9':[],'vulkan':[]}
        for i,(query,batch) in enumerate(zip(queries,batches)):
            trace=run/f'vulkan-{repeat}-{i}.bin';log=execute([run/'vulkan.exe',shader,query,trace],f'vulkan-{repeat}-{i}');logs.append(log);match=re.search(r'vendor=(\d+) device_id=(\d+)',log);need(match is not None,'GPU identity');current=tuple(map(int,match.groups()));need(adapter is None or adapter==current,'same GPU');adapter=current
            output=run/f'd3d9-{repeat}-{i}';log=execute([run/'d3d9.exe',query,output,*adapter],f'd3d9-{repeat}-{i}');logs.append(log);need('d3d9_pixel_border_defaults_checked=16 value=0 no_border_color_writes=1' in log,'actual default queries');defaults+=16
            need(all(not p.read_bytes().strip(b'\x00\r\n ') for p in output.glob('*-compiler.log')),'owned SM3 shader diagnostics')
            for label,path,header in [('vulkan',trace,10),('d3d9',output/'samples.bin',2)]:
                b=path.read_bytes();at=0
                for serial,row in enumerate(batch):
                    fields=struct.unpack_from('<'+'I'*header,b,at);at+=4*header;need(fields[0]==serial and fields[-1]==len(points),'trace framing '+label)
                    if label=='vulkan':need(fields[1:4]==(2,4,row['format']),'authored format retained')
                    floats=struct.unpack_from('<'+'f'*(len(points)*4),b,at);at+=len(points)*16;need(all(math.isfinite(x) and -.000001<=x<=1.000001 for x in floats),'finite normalized output');values[label].append(floats)
                need(at==len(b),'complete extent '+label)
            print('repeat '+str(repeat)+' batch '+str(i)+' passed',flush=True)
        traces[repeat]=values
    need(traces[0]==traces[1],'exact repeated float traces');need(traces[0]['d3d9']==traces[0]['vulkan'],'exact cross-API floats; investigate rather than relax')
    zeros=0;contrasts=0
    lookup={(r['format'],r['family'],tuple(r['config'])):i for i,r in enumerate(rows)}
    for i,row in enumerate(rows):
        q=row['config']
        if 3 not in q[4:6]:continue
        wrap=q.copy();wrap[4:6]=[0,0];other=traces[0]['vulkan'][lookup[(row['format'],row['family'],tuple(wrap))]]
        for n,(_,u,v,_) in enumerate(points):
            if (q[4]==3 and (u<=-1 or u>=2)) or (q[5]==3 and (v<=-1 or v>=2)):
                a=traces[0]['vulkan'][i][n*4:n*4+4];need(a==(0.,0.,0.,0.),'independent fully-outside transparent black');zeros+=4;contrasts+=int(tuple(other[n*4:n*4+4])!=a)
    need(zeros>10000 and contrasts>10000,'contrasting border samples')
    for group in [sources,protected,fixtures]:need(all(sha(p)==h for p,h in group.items()),'final identity checks')
    result={'status':'VERIFIED bounded authored border GPU contract; original reset loop statically authenticated, full reset execution/live state unverified','time_utc':datetime.now(timezone.utc).isoformat(),'cases':len(rows),'samples_per_case':len(points),'samples_per_api_per_run':len(rows)*len(points),'repetitions':2,'same_gpu_vendor_device':adapter,'exact_cross_api_repeat':True,'default_border_state_queries':defaults,'independent_outside_zero_component_checks':zeros,'outside_vs_repeat_contrasting_samples':contrasts,'executables':executables,'compiler_warnings':warnings,'imports':imports,'logs':logs,'source_sha256':sources,'protected_sha256':protected,'fixture_sha256':fixtures,'transitions':transitions,'limits':['Same installed GPU/driver only. No validation layer/device-loss/submit-failure proof.','Actual System D3D9 HAL with owned shaders/fixtures; Vulkan actual prepared binding. Original SFM not launched, loaded or controlled. Native audit is static, not execution of entire reset/device lifecycle.','Prior CPU-native wrap/filter/apply evidence establishes border flag override and address4 delivery under enabled reference config. Current reference color zero supported by original initialization and actual D3D9 defaults; other live original overrides not certified.']}
    receipt=run/'results.json';put(receipt,result);put(OUT/'latest-cross-api-verification.json',{'receipt':str(receipt),'receipt_sha256':sha(receipt)});print(json.dumps({k:result[k] for k in ['status','cases','samples_per_case','samples_per_api_per_run','independent_outside_zero_component_checks','outside_vs_repeat_contrasting_samples']},indent=2),flush=True)
if __name__=='__main__':main()
