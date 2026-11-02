"""Target no-mip priority with the diagnostic device feature enabled."""
from pathlib import Path
import itertools,json,os,shutil,struct,subprocess,tempfile
from verify_anisotropic_gpu import ROOT,OUT,sha,need,put
from verify_texture_sampling_cross_api import fixture

def main():
    idx=json.loads((OUT/'latest-normal-verification.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'normal integration receipt');j=json.loads(Path(idx['receipt']).read_text());exe=j['executable'];need(sha(exe['path'])==exe['sha256'],'exact tested executable');base=Path(idx['receipt']).parent;oldidx=json.loads((ROOT/'outputs/source-border-20261101/latest-draw-verification.json').read_text());old=json.loads(Path(oldidx['receipt']).read_text());oldexe=old['executables']['draw'];need(sha(oldexe['path'])==oldexe['sha256'],'legacy draw executable');legacy=Path(oldidx['receipt']).parent;run=Path(tempfile.mkdtemp(prefix='no-mip-priority-',dir=OUT));print('No-mip directory: '+str(run),flush=True);rows=[]
    for fmt in [0,13,15]:
        mount=run/str(fmt);d=mount/'materials/fixture';d.mkdir(parents=True)
        for family,name in [(0,'base'),(1,'second')]:
            source=fixture(fmt,family);n=8*8*4 if fmt==0 else 4*(8 if fmt==13 else 16);b=source[:88]+source[-2*n:];b[56]=1;struct.pack_into('<I',b,20,0x110);(d/(name+'.vtf')).write_bytes(b)
        shutil.copyfile(next((base/'fixtures').glob('*/materials/fixture/mask.vtf')),d/'mask.vtf');shutil.copyfile(next((base/'fixtures').glob('*/materials/fixture/model.vmt')),d/'model.vmt')
        for gamma,frame,ch,uv,pattern in itertools.product(range(2),range(2),range(4),[(.1875,.6875),(.234375,.4609375)],[0,4]):
            bits=lambda f:struct.unpack('<I',struct.pack('<f',f))[0];rows.append(([fmt,frame,0,5,gamma,ch,bits(float(pattern)),0,3,bits(uv[0]),bits(uv[1])],mount))
    # Existing failure sweep needs a complete multi-level first fixture at mip1.
    # Keep every target no-mip case; prepend a separate valid warm-up fixture.
    bits=lambda f:struct.unpack('<I',struct.pack('<f',f))[0]
    rows.insert(0,([0,0,0,5,0,0,0,0,3,bits(.1875),bits(.6875)],base/'fixtures'/'0-16-16'))
    def query(p,rs):
        data=bytearray(struct.pack('<I',len(rs)))
        for q,m in rs:
            s=str(m).encode('ascii');data+=struct.pack('<11I',*q)+struct.pack('<I',len(s))+s
        p.write_bytes(data)
    query(run/'queries.bin',rows);previous=[]
    for q,m in rows:q=q.copy();q[3]=q[6]=0;previous.append((q,m))
    query(run/'legacy-query.bin',previous);outputs={};logs={}
    for label,enabled,program,shaders,queryfile in [('enabled',True,exe['path'],base,'queries.bin'),('disabled',False,exe['path'],base,'queries.bin'),('legacy',False,oldexe['path'],legacy,'legacy-query.bin')]:
        env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')}
        if enabled:env['SFM64_DIAGNOSTIC_ANISOTROPIC_TEXTURE_TEST']='1'
        output=run/(label+'.bin');p=subprocess.run([str(program),str(shaders/'vertex.spv'),str(shaders/'pixel.spv'),str(run/queryfile),str(output)],env=env,cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);logs[label]=p.stdout+p.stderr;(run/(label+'.log')).write_text(logs[label]);need(p.returncode==0,label+' failed '+logs[label][-1200:]);outputs[label]=output.read_bytes()
    need(outputs['enabled']==outputs['disabled']==outputs['legacy'],'no-mip flag takes priority: feature-on/off/old binary exact');need('aniso_enabled=1' in logs['enabled'] and 'aniso_enabled=0' in logs['disabled'],'actual logical feature control');result={'status':'VERIFIED no-mip texture flag priority with feature enabled: exact preceding fixed-mip pixels','target_no_mip_cases':len(rows)-1,'warmup_cases':1,'cases':len(rows),'executables':[exe,oldexe],'normal_verification':idx,'legacy_verification':oldidx,'source_sha256':{str(Path(__file__).resolve()):sha(__file__)},'pixel_sha256':sha(run/'enabled.bin'),'logs':logs,'scope':'Authored no-mip8x8 single-level RGBA/BC1/BC3 frames, independent gamma/channel/mask/composition through real normal producer and indexed draw. Does not prove general D3D9/Vulkan anisotropy equivalence.'};p=run/'results.json';put(p,result);put(OUT/'latest-nomip-verification.json',{'receipt':str(p),'receipt_sha256':sha(p)});print(result['status']+' cases='+str(len(rows)),flush=True)
if __name__=='__main__':main()
