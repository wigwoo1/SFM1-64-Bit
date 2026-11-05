"""Actual prepared indexed HDR2 draws with independently predicted full images."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,struct,subprocess,tempfile,os
from verify_texture_sampling_cross_api import fixture as texture_fixture
ROOT=Path(__file__).resolve().parent.parent
MANIFEST='work/editor_shader_manifest.txt'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(x,s):
    if not x:raise RuntimeError(s)
def pe(p):
    b=p.read_bytes();i=struct.unpack_from('<I',b,60)[0];need((struct.unpack_from('<H',b,i+4)[0],struct.unpack_from('<H',b,i+24)[0])==(0x8664,0x20b),'AMD64 PE32+')
def oracle(path,encoding,transform,active):
    b=path.read_bytes();need(len(b)==32*16*4,'complete capture length');checked=0;maximum=0
    # Frozen expectations: half light=.5, explicit tint(.5,.75,1), exposure1.
    # Base sRGB reads are endpoint0/1. Stored V=.75 becomes source V=.25 only
    # on the new route. Target is linear RGBA8 UNORM. One LSB is allowed ONLY
    # for HDR fractional64/128 quantization; endpoints/coverage remain exact.
    legacy_base=[(0,255,0),(0,0,255),(255,255,255)][transform]
    hdr_base=[(1,1,1),(1,0,0),(0,1,0)][transform]
    for y in range(16):
        for x in range(32):
            want=[0,0,0,255];face=None
            if 2<=y<10:
                for n in range(3):
                    if 2+10*n<=x<10+10*n:face=n
            if face is not None:
                if face==1:want[:3]=legacy_base
                elif active:
                    channel=0 if face==0 else 2
                    if encoding==113:want[channel]=(64 if channel==0 else 128)*hdr_base[channel]
                else:
                    channel=0 if face==0 else 2;want[channel]=legacy_base[channel]
            for c in range(4):
                got=b[(y*32+x)*4+c];error=abs(got-want[c]);tolerance=1 if active and face in (0,2) and want[c] in (64,128) else 0
                need(error<=tolerance,f'independent pixel mismatch {path.name} format{encoding} transform{transform} pixel{x},{y} channel{c}: {got} != {want[c]} (bound{tolerance})');maximum=max(maximum,error);checked+=1
    return {'checked_channels':checked,'max_error_lsb':maximum,'sha256':sha(path)}
def main():
    parent=Path((ROOT/'outputs/vulkan-hdr2-draw-current.txt').read_text());before=json.loads((parent/'before.json').read_text());run=Path(tempfile.mkdtemp(prefix='run-',dir=parent));result={'status':'IN PROGRESS','time_utc':datetime.now(timezone.utc).isoformat(),'commands':[]};env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')}
    def execute(args,label):
        args=list(map(str,args));p=subprocess.run(args,cwd=ROOT,env=env,capture_output=True,text=True,timeout=900,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr);result['commands'].append({'args':args,'exit_code':p.returncode});need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-2000:]);return p.stdout+p.stderr
    try:
        for p,h in before.items():
            if p!=MANIFEST:need(sha(p)==h,'initial protected '+p)
        old=(parent/'before-editor_shader_manifest.txt').read_bytes();need(hashlib.sha256(old).hexdigest()==before[MANIFEST],'saved manifest identity');new=(ROOT/MANIFEST).read_bytes();need(new==old+b'vs_6_0 sfm64-hdr2-lightmap-vertex-candidate\nps_6_0 sfm64-hdr2-lightmap-fragment-candidate\n','only two authored manifest additions')
        vtf=bytearray(texture_fixture(0,0)[:88]);struct.pack_into('<HH',vtf,16,2,2);struct.pack_into('<I',vtf,20,0x101);struct.pack_into('<H',vtf,24,1);vtf[56]=1;vtf.extend(bytes([255,255,255,255,255,0,0,255,0,255,0,255,0,0,255,255]));source=run/'owned-quadrants.vtf';source.write_bytes(vtf)
        execute(['cmd.exe','/d','/c',ROOT/'work/build_vulkan_hdr2_indexed_draw_probe.cmd',run/'gpu'],'build-probe');exe=run/'gpu/probe_vulkan_hdr2_indexed_draw_x64.exe';pe(exe)
        # Manifest changed: build the entire editor and shader package in its
        # normal build, preserving every canonical/published executable.
        editor=run/'editor-x64';execute(['cmd.exe','/d','/c',ROOT/'work/build_sfm64_editor_x64.cmd',editor],'complete-editor');binary=editor/'sfm64_session_ui.exe';pe(binary);need(len(list((editor/'shaders').glob('*.spv')))==17,'17 complete shader outputs');print('Complete editor and17 shaders built',flush=True)
        captures=[];logs=[]
        for repeat in range(2):
            root=run/('captures-'+str(repeat));logs.append(execute([exe,editor/'shaders',root,source],'gpu-'+str(repeat)));need('PASS hdr2_indexed_draw_cases=6' in logs[-1],'actual indexed draw complete marker');metrics={}
            for encoding in (113,36):
                for transform in range(3):
                    for active in (False,True):
                        p=root/f'case-{encoding}-{transform}'/('hdr2.rgba' if active else 'baseline.rgba');metrics[f'{encoding}-{transform}-{active}']=oracle(p,encoding,transform,active)
            captures.append(metrics);print('GPU full image oracle repeat'+str(repeat)+' passed',flush=True)
        need(logs[0]==logs[1] and captures[0]==captures[1],'identical repeated GPU captures/protocol')
        for p,h in before.items():
            if p!=MANIFEST:need(sha(p)==h,'final protected '+p)
        prior=ROOT/'outputs/vulkan-draw-pipeline-vke8p9p3/run-39k327ff/editor-x64/shaders';old_shaders={p.name:sha(p) for p in prior.glob('*.spv')};need(all(sha(editor/'shaders'/n)==h for n,h in old_shaders.items()),'all15 preceding shaders unchanged')
        result.update(status='VERIFIED controlled prepared indexed HDR2 point-sampling draw; production admission/native viewport parity unfinished',executable={'path':str(exe),'sha256':sha(exe),'architecture':'AMD64 PE32+'},editor={'path':str(binary),'sha256':sha(binary),'architecture':'AMD64 PE32+','shader_diagnostics':(editor/'shader-build.log').read_text(),'runtime_tested':False},gpu_output=logs[0],captures=captures[0],warnings={name:re.findall(r'warning (C\d+): ([^\r\n]+)',p.read_text(errors='replace')) for name,p in [('gpu',run/'gpu/build.log'),('editor',editor/'build.log')]},protected_sha256={p:h for p,h in before.items() if p!=MANIFEST},manifest_transition={'before':before[MANIFEST],'after':sha(ROOT/MANIFEST)},source_sha256={n:sha(ROOT/'work'/n) for n in ['vulkan_hdr2_lightmap_binding_candidate.h','probe_vulkan_hdr2_indexed_draw_x64.cpp','sfm64-hdr2-lightmap-vertex-candidate.hlsl','sfm64-hdr2-lightmap-fragment-candidate.hlsl','build_vulkan_hdr2_indexed_draw_probe.cmd','verify_vulkan_hdr2_indexed_draw.py']},limits=['Actual controlled prepared-bridge indexed draw, not default editor admission or mtt_soldier visual equivalence.','Point samples and constant source-coordinate planes; interpolation/filter/atlas-edge parity remains unverified.','Linear RGBA8 UNORM output declared explicitly; no live Source tone mapping/exposure inference.','No Vulkan validation layer enabled; faults injected around real Vulkan resources.'])
    except Exception as e:result.update(status='PARTIALLY VERIFIED',failure=repr(e));raise
    finally:
        p=run/'results.json';p.write_text(json.dumps(result,indent=2)+'\n');(parent/'latest.json').write_text(json.dumps({'receipt':str(p),'sha256':sha(p)},indent=2)+'\n')
    print(json.dumps({'status':result['status'],'receipt':str(p),'exe':result['executable'],'editor':result['editor'],'warnings':{k:len(v) for k,v in result['warnings'].items()}},indent=2))
if __name__=='__main__':main()
