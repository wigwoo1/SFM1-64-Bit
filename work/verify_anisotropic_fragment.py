"""Compare real Vulkan fragment gradients with preceding compute/D3D9 pixels."""
from pathlib import Path
from datetime import datetime,timezone
import json,os,re,struct,subprocess,tempfile
from verify_anisotropic_gpu import ROOT,OUT,VS,INCLUDE,sha,need,put

def main():
    idx=json.loads((OUT/'latest-gradient-verification.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'preceding gradients');prior=json.loads(Path(idx['receipt']).read_text());measurement=Path(idx['receipt']).parent;run=Path(tempfile.mkdtemp(prefix='fragment-isolation-',dir=OUT));print('Fragment directory: '+str(run),flush=True)
    for group in ['source_sha256','protected_sha256']:need(all(sha(p)==h for p,h in prior[group].items()),'preceding tested identities')
    sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256']
    for n in ['probe_vulkan_anisotropic_fragment_x64.cpp','texture-anisotropic-fragment-probe.hlsl',Path(__file__).name,'measure_anisotropic_lod.py']:sources[str(ROOT/'work'/n)]=sha(ROOT/'work'/n)
    dxc=Path(os.environ.get('SFM64_DXC_EXE',str(ROOT/'work/dependencies/dxc-v1.9.2609/bin/x64/dxc.exe')));lines=['@echo off',f'cd /d "{ROOT}"']
    for name,profile,entry in [('vertex','vs_6_0','vertexMain'),('pixel','ps_6_0','pixelMain')]:lines +=[f'"{dxc}" -spirv -fspv-target-env=vulkan1.0 -T {profile} -E {entry} work\\texture-anisotropic-fragment-probe.hlsl -Fo "{run/(name+".spv")}" >"{run/(name+"-shader.log")}" 2>&1','if errorlevel 1 exit /b 1']
    lines +=[f'call "{VS}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /I"{INCLUDE}" work\\probe_vulkan_anisotropic_fragment_x64.cpp /Fo:"{run/"fragment.obj"}" /Fe:"{run/"fragment.exe"}" >"{run/"build.log"}" 2>&1','if errorlevel 1 exit /b 1','exit /b 0',''];batch=run/'build.cmd';batch.write_text('\n'.join(lines))
    def execute(args,label):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=300,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed '+(p.stdout+p.stderr)[-1600:]);return p.stdout+p.stderr
    execute(['cmd.exe','/d','/c',batch],'build-driver');warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/'build.log').read_text());need(len(warnings)==3 and all(c=='C4996' and "'getenv'" in m for c,m in warnings),'same inherited warnings');need(all(not (run/(n+'-shader.log')).read_text().strip() for n in ['vertex','pixel']),'no owned shader diagnostics');exe=run/'fragment.exe';b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==0x8664,'AMD64 fragment probe');definitions=json.loads((measurement/'cases.json').read_text());rows=definitions['rows'];points=definitions['points'];queries=sorted(measurement.glob('queries-*.bin'),key=lambda p:int(p.stem.split('-')[-1]));traces=[];logs=[]
    for repeat in range(2):
        output=[]
        for i,query in enumerate(queries):
            p=run/f'fragment-{repeat}-{i}.bin';log=execute([exe,run/'vertex.spv',run/'pixel.spv',query,p],f'fragment-{repeat}-{i}');logs.append(log);need('vendor=4318 device_id=11544' in log and 'aniso_enabled=1' in log,'same enabled GPU');output.append(p.read_bytes());print(f'fragment repeat {repeat} batch {i} complete',flush=True)
        traces.append(output)
    need(traces[0]==traces[1],'exact repeated fragment traces');identical_compute=True;different_dd=0;max_dd=0.;components=0
    for i,b in enumerate(traces[0]):
        identical_compute &= b==(measurement/f'vulkan-0-{i}.bin').read_bytes();dd=(measurement/f'd3d9-0-{i}'/'samples.bin').read_bytes();va=da=0
        for serial in range(min(128,len(rows)-i*128)):
            vh=struct.unpack_from('<10I',b,va);va+=40;dh=struct.unpack_from('<2I',dd,da);da+=8;need(vh[0]==dh[0]==serial and vh[-1]==dh[-1]==len(points),'paired samples framing');vv=struct.unpack_from('<'+'f'*(4*len(points)),b,va);va+=16*len(points);dv=struct.unpack_from('<'+'f'*(4*len(points)),dd,da);da+=16*len(points)
            for x,y in zip(vv,dv):error=abs(x-y);different_dd+=int(error>2e-6);max_dd=max(max_dd,error);components+=1
        need(va==len(b) and da==len(dd),'full fragment/reference extent')
    lid=json.loads((OUT/'latest-lod-isolation.json').read_text());need(sha(lid['receipt'])==lid['receipt_sha256'],'solid mip observation');ldir=Path(lid['receipt']).parent;lo=run/'solid-mip-fragment.bin';logs.append(execute([exe,run/'vertex.spv',run/'pixel.spv',ldir/'queries.bin',lo],'solid-mip-fragment'));need(lo.read_bytes()==(ldir/'vulkan-0.bin').read_bytes(),'solid-mip fragment matches compute')
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'final source/protected identities')
    result={'status':'MEASURED actual fragment-stage anisotropic gradients and solid mip isolation; no Source parity claim from Vulkan sampler flags','time_utc':datetime.now(timezone.utc).isoformat(),'cases':len(rows),'samples_per_api_per_run':len(rows)*len(points),'repetitions':2,'fragment_compute_byte_identical':identical_compute,'fragment_d3d9_components':components,'fragment_d3d9_components_over_2e_6':different_dd,'fragment_d3d9_max_absolute_error':max_dd,'solid_mip_fragment_compute_exact':True,'executable':{'path':str(exe),'sha256':sha(exe)},'compiler_warnings':warnings,'prior_gradient_measurement':idx,'solid_mip_measurement':lid,'source_sha256':sources,'protected_sha256':protected,'logs':logs,'limits':['One installed GPU/driver. Real fragment render passes, float32 attachment readbacks, same explicit gradients/texture sampler and retained fence leases; no original shader/process.','No tolerance relaxation or arbitrary normalization used. Differences must gate default integration; actual Source parity unresolved.']};p=run/'results.json';put(p,result);put(OUT/'latest-fragment-verification.json',{'receipt':str(p),'receipt_sha256':sha(p)});print(json.dumps({k:result[k] for k in ['status','cases','fragment_compute_byte_identical','fragment_d3d9_components_over_2e_6','fragment_d3d9_max_absolute_error','solid_mip_fragment_compute_exact']},indent=2),flush=True)
if __name__=='__main__':main()
