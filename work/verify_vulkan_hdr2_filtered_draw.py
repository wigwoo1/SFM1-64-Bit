"""Frozen full-image analytic oracle for the actual HDR2 filtered draw path."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,struct,subprocess,tempfile,os,math
from verify_texture_sampling_cross_api import fixture as texture_fixture
from verify_vulkan_hdr2_indexed_draw import oracle as point_oracle
ROOT=Path(__file__).resolve().parent.parent
CHANGED={'work/vulkan_hdr2_lightmap_binding_candidate.h','work/probe_vulkan_hdr2_indexed_draw_x64.cpp'}
FIRST=((1,0,0),(0,1,0),(0,0,1),(1,1,1))
LAST=((0,0,1),(1,0,0),(1,1,1),(0,1,0))
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(x,s):
    if not x:raise RuntimeError(s)
def pe(p):
    b=p.read_bytes();i=struct.unpack_from('<I',b,60)[0];need((struct.unpack_from('<H',b,i+4)[0],struct.unpack_from('<H',b,i+24)[0])==(0x8664,0x20b),'AMD64 PE32+')
def sample(face,s,t,encoding,linear,padding,zero):
    w=1 if zero and face==2 else 2;h=2;page_w=w+2*padding;page_h=h+2*padding
    if zero and face==2:s=t=0
    x=padding+s;y=padding+t
    def fetch(x,y):
        x=max(0,min(page_w-1,x));y=max(0,min(page_h-1,y));x-=padding;y-=padding
        if x<0 or y<0 or x>=w or y>=h or encoding==36:return (0,0,0)
        return tuple(c*.5 for c in (FIRST if face==0 else LAST)[y*w+x])
    if not linear:return fetch(math.floor(x+.5),math.floor(y+.5))
    x0=math.floor(x);y0=math.floor(y);fx=x-x0;fy=y-y0
    taps=(fetch(x0,y0),fetch(x0+1,y0),fetch(x0,y0+1),fetch(x0+1,y0+1))
    return tuple((taps[0][c]*(1-fx)+taps[1][c]*fx)*(1-fy)+(taps[2][c]*(1-fx)+taps[3][c]*fx)*fy for c in range(3))
def preview(face,u,v,zero):
    if zero and face==2:return (v,0,1-v)
    tl,tr,bl,br=FIRST if face==0 else LAST
    if v<=u:return tuple((1-u)*tl[c]+(u-v)*tr[c]+v*br[c] for c in range(3))
    return tuple((1-v)*tl[c]+u*br[c]+(v-u)*bl[c] for c in range(3))
def oracle(path,encoding,linear,padding,edge,zero,active):
    b=path.read_bytes();need(len(b)==2048,'full filtered capture length');maximum=0;count=0
    for y in range(16):
        for x in range(32):
            color=(0,0,0);face=None
            if 2<=y<10:
                for n in range(3):
                    if 2+10*n<=x<10+10*n:face=n
            if face is not None:
                u=(x+.5-(2+10*face))/8;v=(y+.5-2)/8
                if face==1:color=(1,1,1)
                elif not active:color=preview(face,u,v,zero)
                else:color=sample(face,2*u-.5 if edge else u,2*v-.5 if edge else v,encoding,linear,padding,zero)
            want=[int(math.floor(c*255+.5)) for c in color]+[255]
            for c in range(4):
                error=abs(b[(y*32+x)*4+c]-want[c]);bound=1 if 0<want[c]<255 else 0
                need(error<=bound,f'frozen filtered oracle mismatch {path} pixel{x},{y} ch{c}: {b[(y*32+x)*4+c]} !={want[c]} bound{bound}');maximum=max(maximum,error);count+=1
    return {'sha256':sha(path),'channels':count,'max_error_lsb':maximum}
def vtf():
    b=bytearray(texture_fixture(0,0)[:88]);struct.pack_into('<HH',b,16,2,2);struct.pack_into('<I',b,20,0x101);struct.pack_into('<H',b,24,1);b[56]=1;b.extend(bytes([255]*16));return b
def main():
    parent=Path((ROOT/'outputs/vulkan-hdr2-filter-current.txt').read_text());before=json.loads((parent/'before.json').read_text());editor=Path((parent/'reference-editor.txt').read_text());run=Path(tempfile.mkdtemp(prefix='run-',dir=parent));policy=parent/'frozen-test-policy.json';result={'status':'IN PROGRESS','time_utc':datetime.now(timezone.utc).isoformat(),'frozen_policy':{'path':str(policy),'sha256':sha(policy)},'commands':[]};env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')}
    def execute(args,label):
        args=list(map(str,args));p=subprocess.run(args,cwd=ROOT,env=env,capture_output=True,text=True,timeout=900,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr);result['commands'].append({'args':args,'exit_code':p.returncode});need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-2000:]);return p.stdout+p.stderr
    try:
        for p,h in before.items():
            if p not in CHANGED:need(sha(p)==h,'protected initial '+p)
            else:need(sha(parent/('before-'+Path(p).name))==h,'before backup '+p)
        old=(parent/'before-probe_vulkan_hdr2_indexed_draw_x64.cpp').read_bytes();current=(ROOT/'work/probe_vulkan_hdr2_indexed_draw_x64.cpp').read_bytes();need(current.replace(b'#ifndef SFM64_HDR2_INDEXED_NO_MAIN'+(b'\r\n' if b'\r\n' in old else b'\n'),b'')==old+b'#endif'+(b'\r\n' if b'\r\n' in old else b'\n'),'only old point-probe main guard changed')
        white=run/'owned-white.vtf';white.write_bytes(vtf());execute(['cmd.exe','/d','/c',ROOT/'work/build_vulkan_hdr2_filtered_draw_probe.cmd',run/'gpu'],'build-filtered');exe=run/'gpu/probe_vulkan_hdr2_filtered_draw_x64.exe';pe(exe)
        captured=[];outputs=[]
        for repeat in range(2):
            root=run/('captures-'+str(repeat));outputs.append(execute([exe,editor/'shaders',root,white],'gpu-'+str(repeat)));need('PASS hdr2_filtered_draw_cases=32' in outputs[-1],'complete filtered GPU case protocol');metrics={}
            for encoding in (113,36):
                for linear in (0,1):
                    for padding in (0,1):
                        for edge in (0,1):
                            for zero in (0,1):
                                case=f'case-{encoding}-{linear}-{padding}-{edge}-{zero}'
                                for active in (False,True):metrics[f'{case}-{active}']=oracle(root/case/('hdr2.rgba' if active else 'baseline.rgba'),encoding,linear,padding,edge,zero,active)
            captured.append(metrics);print('Filtered full capture oracle repeat'+str(repeat)+' passed',flush=True)
        need(outputs[0]==outputs[1] and captured[0]==captured[1],'byte-identical repeated filtered draws')
        # Authenticate the padding and filter discriminators as real nontrivial
        # changes, while all middle legacy rectangles must remain byte-identical.
        first=run/'captures-0'
        def image(l,p,e,z):return (first/f'case-113-{l}-{p}-{e}-{z}/hdr2.rgba').read_bytes()
        need(image(1,1,1,0)!=image(1,0,1,0),'padding edge blend discriminator');need(image(1,0,0,0)!=image(0,0,0,0),'linear/nearest discriminator')
        for data in [image(l,p,e,z) for l in (0,1) for p in (0,1) for e in (0,1) for z in (0,1)]:
            for y in range(2,10):need(data[(y*32+12)*4:(y*32+20)*4]==bytes([255]*32),'unchanged middle legacy rectangle')
        # Freshly compile and rerun the preceding point probe with the new API's
        # omitted filter parameter, using its original independent oracle.
        quadrant=bytearray(vtf());quadrant[-16:]=bytes([255,255,255,255,255,0,0,255,0,255,0,255,0,0,255,255]);q=run/'owned-quadrants.vtf';q.write_bytes(quadrant);execute(['cmd.exe','/d','/c',ROOT/'work/build_vulkan_hdr2_indexed_draw_probe.cmd',run/'point'],'build-point-regression');point=run/'point/probe_vulkan_hdr2_indexed_draw_x64.exe';pe(point);point_root=run/'point-captures';result['point_output']=execute([point,editor/'shaders',point_root,q],'point-regression')
        for encoding in (113,36):
            for transform in range(3):
                for active in (False,True):point_oracle(point_root/f'case-{encoding}-{transform}'/('hdr2.rgba' if active else 'baseline.rgba'),encoding,transform,active)
        for p,h in before.items():
            if p not in CHANGED:need(sha(p)==h,'protected final '+p)
        result.update(status='VERIFIED controlled varying-coordinate linear/nearest HDR2 sampling; native atlas/live viewport parity incomplete',executable={'path':str(exe),'sha256':sha(exe),'architecture':'AMD64 PE32+'},point_executable={'path':str(point),'sha256':sha(point)},gpu_output=outputs[0],captures=captured[0],warnings={n:re.findall(r'warning (C\d+): ([^\r\n]+)',(run/n/'build.log').read_text(errors='replace')) for n in ('gpu','point')},transitions={p:{'before':before[p],'after':sha(ROOT/p)} for p in CHANGED},protected_sha256={p:h for p,h in before.items() if p not in CHANGED},new_source_sha256={p:sha(ROOT/'work'/p) for p in ['probe_vulkan_hdr2_filtered_draw_x64.cpp','build_vulkan_hdr2_filtered_draw_probe.cmd','verify_vulkan_hdr2_filtered_draw.py']},editor_rebuilt=False,reference_editor={'path':str(editor/'sfm64_session_ui.exe'),'sha256':sha(editor/'sfm64_session_ui.exe')},limits=['Explicit per-face padding and placement; not discovered native atlas borders or placement equivalence.','Owned controlled linear UNORM capture; no original SFM shader/process execution, benchmark lighting or native output policy proof.','Linear filter support checked on actual device; capability denial is an injected property result.','No shaders/editor include dependency changed; prior complete17-shader editor preserved.'])
    except Exception as e:result.update(status='PARTIALLY VERIFIED',failure=repr(e));raise
    finally:
        p=run/'results.json';p.write_text(json.dumps(result,indent=2)+'\n');(parent/'latest.json').write_text(json.dumps({'receipt':str(p),'sha256':sha(p)},indent=2)+'\n')
    print(json.dumps({'status':result['status'],'receipt':str(p),'exe':result['executable'],'warnings':{k:len(v) for k,v in result['warnings'].items()}},indent=2))
if __name__=='__main__':main()
