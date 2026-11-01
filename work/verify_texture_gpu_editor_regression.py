"""Complete editor regression after existing material-owner mip/frame extension."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile
from verify_session_preview_aspect import ROOT,SESSION,CONTENT
OUT=ROOT/'outputs/texture-gpu-bridge-20261101'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def main():
    run=Path(tempfile.mkdtemp(prefix='editor-runtime-',dir=OUT));print('Runtime directory: '+str(run),flush=True)
    idx=json.loads((OUT/'latest-verification.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'GPU receipt identity');gpu=json.loads(Path(idx['receipt']).read_text());owner=gpu['transition'];need(sha(owner['path'])==owner['after'],'tested owner source')
    baseline=ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe';candidate=ROOT/'outputs/sfm64-editor-texture-mips-x64-20261101/sfm64_session_ui.exe';published=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text())
    if sha(baseline)!=published['executable_sha256']:baseline=OUT/'preserved-canonical/sfm64_session_ui.exe'
    need(sha(baseline)==published['executable_sha256'],'preserved baseline identity');need(published['editor_source_dependencies']['work/vulkan_material_images_candidate.h']==owner['before'],'published preceding owner identity')
    olddeps=published['editor_source_dependencies'];changed=[n for n,h in olddeps.items() if sha(ROOT/n)!=h];need(changed==['work/vulkan_material_images_candidate.h'],'single existing editor dependency change')
    graph={}
    def visit(p):
        p=p.resolve();n=p.relative_to(ROOT).as_posix()
        if n in graph:return
        graph[n]=sha(p)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            if (p.parent/inc).exists():visit(p.parent/inc)
    for n in list(olddeps)+['work/verify_texture_gpu_editor_regression.py']:visit(ROOT/n)
    warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(candidate.parent/'build.log').read_text());need([list(x) for x in warnings]==published['compiler_warnings'],'same editor warnings');b=candidate.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==0x8664 and struct.unpack_from('<H',b,pe+24)[0]==0x20b,'complete editor AMD64 PE32+')
    shaders={str(p.relative_to(candidate.parent)):sha(p) for p in (candidate.parent/'shaders').glob('*.spv')};need(len(shaders)==15 and all(sha(baseline.parent/n)==h for n,h in shaders.items()),'unchanged15 owned shaders')
    protected={str(p):sha(p) for p in [SESSION,CONTENT/'gameinfo.txt',baseline]};session=run/'input-copy.dmx';shutil.copyfile(SESSION,session);protected[str(session)]=sha(session);foreign=run/'empty-working-directory';foreign.mkdir();clean={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')};frames={};commands=[]
    for mode in ['default','two-texture']:
        for label,exe in [('baseline',baseline),('candidate',candidate)]:
            directory=run/(mode+'-'+label);directory.mkdir();env=dict(clean,SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory))
            if mode=='two-texture':env['SFM64_DIAGNOSTIC_TWO_TEXTURE_TEST']='1'
            args=[str(exe),'--control-test','--session',str(session),'--content-directory',str(CONTENT),'--start-frame','119'];p=subprocess.run(args,cwd=foreign,env=env,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);log=p.stdout+p.stderr;(directory/'run.log').write_text(log);need(p.returncode==0 and 'model_gpu_presentation_passed=1 frames=5' in log and 'gpu_readback_copies=5 capture_frames=1' in log,mode+' '+label+' failed: '+log[-1600:]);frames[mode+'-'+label]={str(n):sha(directory/f'vulkan-retained-frame-{n}.rgba') for n in [0,119,120]};commands.append({'command':args,'exit_code':p.returncode,'frames':5,'mode':mode,'label':label});print(mode+' '+label+' passed',flush=True)
        need(frames[mode+'-baseline']==frames[mode+'-candidate'],mode+' benchmark pixel regression')
    p=subprocess.run([str(candidate),'--control-test','--session',str(session),'--content-directory',str(CONTENT),'--start-frame','119'],cwd=foreign,env=clean,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);log=p.stdout+p.stderr;(run/'clean-startup.log').write_text(log);need(p.returncode==0 and 'model_gpu_presentation_passed=1 frames=5' in log and 'gpu_readback_copies=0 capture_frames=0' in log,'clean candidate startup: '+log[-1600:]);need(not list(foreign.iterdir()),'unexpected normal-startup files');need(all(sha(p)==h for p,h in protected.items()),'protected originals and baseline preserved');need(all(sha(ROOT/n)==h for n,h in graph.items()),'sources unchanged during runtime')
    result=dict(status='VERIFIED complete editor build and25 GPU-frame regression',time_utc=datetime.now(timezone.utc).isoformat(),candidate={'path':str(candidate),'sha256':sha(candidate)},baseline={'path':str(baseline),'sha256':sha(baseline)},editor_build_script='work/build_sfm64_editor_x64.cmd',source='work/probe_vulkan_win32_surface_x64.cpp',editor_source_dependencies={k:v for k,v in graph.items() if k!='work/verify_texture_gpu_editor_regression.py'},changed_existing_editor_dependencies=changed,compiler_warnings=warnings,shader_sha256=shaders,gpu_frames=25,frame_sha256=frames,clean_startup='PASS no diagnostics/no unexpected files',protected_sha256=protected,commands=commands,gpu_receipt=idx,limits=['Owned editor hidden control tests only; original SFM not launched or controlled. Original session copied read-only; gameinfo unchanged.','Exact25-frame benchmark regression preserves existing single-mip editor path; independent GPU bridge tests establish new multi-mip/frame upload. Default editor texture preparation/shader lookup does not yet use the new plan path.','No original Source visual-equivalence claim or new interactive workflow acceptance.15 same compiler warnings retained. Canonical executable not replaced by this verifier.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-editor-verification.json').write_text(json.dumps({'receipt':str(receipt),'receipt_sha256':sha(receipt)},indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','candidate','gpu_frames','changed_existing_editor_dependencies','clean_startup']},indent=2),flush=True)
if __name__=='__main__':
    import struct
    main()
