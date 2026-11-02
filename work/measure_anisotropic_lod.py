"""Isolate real per-API anisotropic mip selection with authored solid mip colors."""
from pathlib import Path
import json,itertools,math,struct,subprocess,tempfile
from verify_anisotropic_gpu import ROOT,OUT,sha,need,put

def main():
    idx=json.loads((OUT/'latest-gradient-verification.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'actual gradient measurement');prior=json.loads(Path(idx['receipt']).read_text());run=Path(tempfile.mkdtemp(prefix='lod-isolation-',dir=OUT));print('LOD directory: '+str(run),flush=True)
    for label in ['vulkan','d3d9']:need(sha(prior['executables'][label]['path'])==prior['executables'][label]['sha256'],'measured executable '+label)
    b=bytearray(88)
    for at,n in [(0,0x00465456),(4,7),(8,5),(12,88),(52,0),(57,0xffffffff),(68,1),(80,0x30),(84,88)]:struct.pack_into('<I',b,at,n)
    for at,n in [(16,8),(18,8),(24,2),(63,1)]:struct.pack_into('<H',b,at,n)
    b[56]=4
    for mip in reversed(range(4)):
        w=max(1,8>>mip)
        for frame in range(2):b+=bytes([mip*64,mip*64,mip*64,255])*(w*w)
    path=run/'owned-solid-mips.vtf';path.write_bytes(b);points=[];meta=[]
    for frame,angle,major,ratio in itertools.product(range(2),[0.,.25,.5,.75,1.],[.5,1.,2.,4.],[1.,2.,4.,8.]):
        minor=major/ratio;points.append((frame,.3125,.3125,0.,major/8,major*angle/8,-minor*angle/8,minor/8));meta.append({'frame':frame,'slope':angle,'major':major,'ratio':ratio})
    rows=[(factor,[0,1,1,1,0,0,0,0xffffffff,struct.unpack('<I',struct.pack('<f',factor))[0],0]) for factor in [0.,2.,4.,8.,16.]]
    def text(s):
        v=str(s).encode('ascii');return struct.pack('<I',len(v))+v
    query=run/'queries.bin';query.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<10I',*q)+text('fixture/solid-mip-lod')+text(path)+struct.pack('<I',len(points))+b''.join(struct.pack('<I7f',*v) for v in points) for _,q in rows));traces={};logs={}
    def execute(args,label):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed '+(p.stdout+p.stderr)[-1200:]);logs[label]=p.stdout+p.stderr
    for repeat in range(2):
        vt=run/f'vulkan-{repeat}.bin';dd=run/f'd3d9-{repeat}';execute([prior['executables']['vulkan']['path'],Path(idx['receipt']).parent/'gradient.spv',query,vt],'vulkan-'+str(repeat));execute([prior['executables']['d3d9']['path'],query,dd,*prior['same_gpu_vendor_device']],'d3d9-'+str(repeat));values={}
        for label,p,header in [('vulkan',vt,10),('d3d9',dd/'samples.bin',2)]:
            b=p.read_bytes();at=0;values[label]=[]
            for i in range(len(rows)):
                h=struct.unpack_from('<'+'I'*header,b,at);at+=4*header;need(h[0]==i and h[-1]==len(points),'LOD sample framing');v=struct.unpack_from('<'+'f'*(len(points)*4),b,at);at+=len(points)*16;values[label].append(v)
            need(at==len(b),'full LOD output')
        traces[repeat]=values
    need(traces[0]==traces[1],'exact LOD repeatability');observations=[];metrics={}
    for i,(factor,_) in enumerate(rows):
        for n,m in enumerate(meta):
            footprint=m['major'] if not factor else m['major']/min(factor,m['ratio']);cheb=max(0.,min(3.,math.log2(footprint)));euclid=max(0.,min(3.,math.log2(footprint*math.sqrt(1+m['slope']**2))));row=dict(m,factor=factor,chebyshev_prediction=cheb,euclidean_prediction=euclid)
            for label in ['vulkan','d3d9']:
                measured=traces[0][label][i][4*n]*255/64;row[label+'_measured_lod']=measured;k=label+'-factor'+str(factor);a=metrics.setdefault(k,{'count':0,'max_chebyshev_error':0.,'max_euclidean_error':0.});a['count']+=1;a['max_chebyshev_error']=max(a['max_chebyshev_error'],abs(measured-cheb));a['max_euclidean_error']=max(a['max_euclidean_error'],abs(measured-euclid))
            observations.append(row)
    result={'status':'MEASURED solid-mip isolation, no presumed footprint formula or production correction','prior_gradient_measurement':idx,'executable_sha256':prior['executables'],'queries_sha256':sha(query),'fixture_sha256':sha(path),'cases':len(rows),'samples_per_case':len(points),'repetitions':2,'metrics':metrics,'observations':observations,'source_sha256':{str(Path(__file__).resolve()):sha(__file__)},'logs':logs,'limits':'Authored constant mip colors isolate filtering LOD. Formula residuals are observations, not a universal implementation claim or permission to fit arbitrary shaders.'};p=run/'results.json';put(p,result);put(OUT/'latest-lod-isolation.json',{'receipt':str(p),'receipt_sha256':sha(p)});print(json.dumps(metrics,indent=2),flush=True)
if __name__=='__main__':main()
