"""Owned HDR fixture -> immutable plane -> native x64 per-fragment proof.

No original application launch, original writes, publication or default renderer
changes. Frozen 2e-6 gate and the prior independently calibrated n.8 raster model.
"""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import math
import os
import re
import struct
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/source-lightmap-fragment-20261101'
VS=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat')
DUMP=VS.parents[2]/'Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe'
TOLERANCE=2e-6
NEW=['source_bsp_base_lightmap_candidate.h','d3d9_base_lightmap_pass_candidate.h',
     'test_source_bsp_base_lightmap.cpp','probe_d3d9_base_lightmap_x64.cpp',Path(__file__).name]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(value,message):
    if not value:raise RuntimeError(message)
def put(p,value):Path(p).write_text(json.dumps(value,indent=2)+'\n')
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]

# Reuse only the already frozen raster contract; no production decoder/sampler
# participates in the independent raw-byte and double-precision light oracle.
spec=importlib.util.spec_from_file_location('raster',ROOT/'work/verify_d3d9_interpolation_contract.py')
raster=importlib.util.module_from_spec(spec);spec.loader.exec_module(raster)
COLORS=[[(128,16,24,255),(32,80,16,0),(24,32,128,1),(192,128,48,254)],
        [(48,120,16,0),(160,24,48,255),(80,64,192,254),(64,176,32,1)]]
def decoded(face):return [[f32(c*2.**(p[3] if p[3]<128 else p[3]-256)/255.) for c in p[:3]]+[1.] for p in COLORS[face]]
def fixture(raw):
    b=bytearray(1036);b[:4]=b'VBSP';struct.pack_into('<I',b,4,20)
    def lump(n,data,version=0):
        struct.pack_into('<4I',b,8+n*16,len(b),len(data),version,0);b.extend(data)
    # Two independent source faces, separate texinfo, nonzero minima/offsets.
    positions=b''.join(struct.pack('<3f',*p,0.) for _ in range(2) for p in raw);lump(3,positions)
    edges=bytearray(4)
    for face in range(2):
        for k in range(3):edges.extend(struct.pack('<2H',face*3+k,face*3+(k+1)%3))
    lump(12,edges);lump(13,struct.pack('<6I',*range(1,7)))
    ti=bytearray(144)
    for face in range(2):
        at=face*72
        for offset in (0,20,32,52):struct.pack_into('<f',ti,at+offset,1.)
        struct.pack_into('<f',ti,at+44,-3.);struct.pack_into('<f',ti,at+60,-5.)
    lump(6,ti)
    td=bytearray(32)
    for offset in (16,20,24,28):struct.pack_into('<I',td,offset,64)
    lump(2,td);lump(43,b'owned/albedo\0');lump(44,bytes(4))
    faces=bytearray(112)
    for face in range(2):
        at=56*face;struct.pack_into('<IHHhh',faces,at+4,face*3,3,face,-1,0)
        faces[at+16:at+20]=bytes((0,255,255,255));struct.pack_into('<i',faces,at+20,(24,160)[face])
        struct.pack_into('<4i',faces,at+28,-3,-5,1,1)
    lump(7,faces,1);lump(58,faces,1)
    model=bytearray(48);struct.pack_into('<I',model,44,2);lump(14,model)
    lighting=bytearray([222]*256)
    for face in range(2):
        for k,color in enumerate(COLORS[face]):lighting[(24,160)[face]+4*k:(24,160)[face]+4*k+4]=bytes(color)
    lump(53,lighting,1)
    return b
def cases(run):
    rows=[]
    for serial in range(48):
        raw=[(0.,0.),(1.,0.),(0.,1.)] if serial%4<2 else [(-1.,-.5),(2.,-.5),(-1.,2.)]
        points=[(3.+(serial%3)/256,4.),(28.,5.+(serial%5)/256),(6.,28.)]
        weights=[1.,1.,1.] if serial%3==0 else [1.25,2.5,4.75]
        if serial%2:points[1],points[2]=points[2],points[1]
        vertices=[raster.vertex(x,y,w,[*uv,0.,1.]) for (x,y),w,uv in zip(points,weights,raw)]
        face=serial%2;albedo=[255,255,255,255] if serial%6<3 else [255,0,255,255]
        modulation=[1.,1.,1.] if serial%5<3 else [.5,1.25,.75]
        if serial==31:modulation=[0.,0.,0.]
        path=run/f'owned-{serial}.bsp';path.write_bytes(fixture(raw))
        rows.append(dict(vertices=vertices,face=face,albedo=albedo,modulation=modulation,path=str(path),
                         samples=[[x,y] for y in range(32) for x in range(32)]))
    return rows
def encode(rows,path):
    b=bytearray(struct.pack('<I',len(rows)))
    for r in rows:
        name=r['path'].encode();b.extend(struct.pack('<I',len(name))+name);b.extend(struct.pack('<I4B3f',r['face'],*r['albedo'],*r['modulation']))
        for v in r['vertices']:b.extend(struct.pack('<4f',*v['clip']))
    path.write_bytes(b)
def sample(plane,u,v):
    # Clamp individual neighboring texels, blend after decoding, no gamma/clamp.
    x,y=math.floor(u),math.floor(v);fx,fy=u-x,v-y
    def at(a,b):return plane[max(0,min(1,b))*2+max(0,min(1,a))]
    return [(1-fy)*((1-fx)*at(x,y)[k]+fx*at(x+1,y)[k])+fy*((1-fx)*at(x,y+1)[k]+fx*at(x+1,y+1)[k]) for k in range(3)]
def compare(rows,path,metadata):
    data=path.read_bytes();raw=metadata.read_bytes();at=mt=0;maximum=0.;pixels=covered=0;over=coverage=0;vertex_difference=0.;bright=0
    for serial,r in enumerate(rows):
        need(struct.unpack_from('<I',data,at)[0]==serial,'capture serial');at+=4
        actual=struct.unpack_from('<4096f',data,at);at+=16384
        need(struct.unpack_from('<4I',raw,mt)==(serial,r['face'],2,2),'source face/plane metadata');mt+=16
        uv=struct.unpack_from('<6f',raw,mt);mt+=24
        need(list(uv)==[x for v in r['vertices'] for x in v['attribute'][:2]],'raw luxels independent source geometry')
        plane=decoded(r['face']);stored=struct.unpack_from('<16f',raw,mt);mt+=64
        need(list(stored)==sum(plane,[]),'independent raw RGBExp32 byte decode')
        for p,(x,y) in enumerate(r['samples']):
            interp=raster.expected(r['vertices'],x,y);a=actual[p*4:p*4+4]
            want=[0.]*4
            if interp[3]!=0:
                covered+=1;light=sample(plane,*interp[:2]);want=[light[k]*(r['albedo'][k]/255.)*r['modulation'][k] for k in range(3)]+[1.]
                # A second oracle for the old vertex-color shortcut must disagree
                # at interior pixels, so a preview implementation cannot pass.
                vv=[]
                for v in r['vertices']:
                    value=sample(plane,*v['attribute'][:2]);vv.append(dict(clip=v['clip'],attribute=value+[1.]))
                vertex=raster.expected(vv,x,y)
                vertex_difference=max(vertex_difference,max(abs(light[k]-vertex[k]) for k in range(3)))
                bright+=max(want[:3])>1.
            pixels+=1;coverage+=(a[3]==0)!=(want[3]==0)
            for value,w in zip(a,want):
                need(math.isfinite(value),'finite captured component');error=abs(value-w);maximum=max(maximum,error);over+=error>TOLERANCE
    need(at==len(data) and mt==len(raw),'complete output framing')
    return dict(pixels=pixels,covered=covered,max_error=maximum,over_budget=over,coverage_mismatches=coverage,
                vertex_shortcut_max_difference=vertex_difference,unclamped_bright_pixels=bright,boundary_pixels_excluded=0)
def closure(path,found=None):
    if found is None:found={}
    path=Path(path).resolve()
    if str(path) in found:return found
    found[str(path)]=sha(path)
    for name in re.findall(r'^\s*#\s*include\s*"([^"]+)"',path.read_text(errors='replace'),re.M):
        p=path.parent/name
        if p.exists():closure(p,found)
    return found
def main():
    before=json.loads((OUT/'before.json').read_text());parent=before['parent'];need(sha(parent['receipt'])==parent['receipt_sha256'],'parent receipt')
    prior=json.loads(Path(parent['receipt']).read_text());sources=dict(before['source_sha256']);protected=dict(before['protected_sha256']);protected[parent['receipt']]=parent['receipt_sha256']
    for group in (sources,protected,before['complete_editor_dependencies_sha256']):
        for p,h in group.items():need(sha(p)==h,'before identity '+p)
    for name in NEW:
        p=ROOT/'work'/name;need(str(p) not in sources,'new files only');sources[str(p)]=sha(p)
    test_closures={name:closure(ROOT/'work'/name) for name in ['test_source_bsp_base_lightmap.cpp','probe_d3d9_base_lightmap_x64.cpp']}
    for group in test_closures.values():
        for p,h in group.items():need(p in sources and sources[p]==h,'unregistered test dependency '+p)
    run=Path(tempfile.mkdtemp(prefix='r-',dir=OUT));print(run,flush=True)
    rows=cases(run);put(run/'cases.json',rows);encode(rows,run/'query.bin')
    put(run/'protocol.json',dict(time_utc=datetime.now(timezone.utc).isoformat(),contract_sha256=sha(OUT/'CONTRACT.md'),
        tolerance=TOLERANCE,raster='previously independently calibrated NVIDIA nearest-even n.8',light_filter='four point fetches; explicit FP32 bilinear',
        base_srgb='enabled; endpoint nonwhite fixture excludes transfer approximation from lightmap proof',cases=48,source_sha256=sources,test_closures=test_closures))
    clean={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')}
    def execute(args,label):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,env=clean,capture_output=True,text=True,timeout=300,creationflags=subprocess.CREATE_NO_WINDOW)
        (run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1800:]);return p.stdout+p.stderr
    exes={};warnings={}
    for name,source,libs in [('cpu','test_source_bsp_base_lightmap.cpp',''),('gpu','probe_d3d9_base_lightmap_x64.cpp','d3d9.lib d3dcompiler.lib user32.lib')]:
        exe=run/(name+'.exe');batch=run/(name+'-build.cmd')
        batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS}" >nul','if errorlevel 1 exit /b 1',
            f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\{source} /Fo:"{run/(name+".obj")}" /Fe:"{exe}" {libs} >"{run/(name+"-build.log")}" 2>&1','exit /b %errorlevel%','']))
        execute(['cmd.exe','/d','/c',batch],name+'-build-driver');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0]
        need(struct.unpack_from('<H',binary,pe+4)[0]==0x8664 and struct.unpack_from('<H',binary,pe+24)[0]==0x20b,'AMD64 PE32+')
        execute([DUMP,'/imports',exe],name+'-imports');exes[name]=dict(path=str(exe),sha256=sha(exe));warnings[name]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(name+'-build.log')).read_text())
    logs=[];metrics=[]
    for n in range(2):
        logs.append(execute([run/'cpu.exe'],f'cpu-{n}'))
        logs.append(execute([run/'gpu.exe',run/'query.bin',run/f'g{n}'],f'gpu-{n}'));m=compare(rows,run/f'g{n}/pixels.bin',run/f'g{n}/metadata.bin');metrics.append(m)
        put(run/f'g{n}-metrics.json',m)
        need(m['over_budget']==0 and m['coverage_mismatches']==0,'frozen analytic pixel gate failed; preserve and investigate')
        need(m['vertex_shortcut_max_difference']>.02 and m['unclamped_bright_pixels']>0,'per-fragment/bright discrimination absent')
        print('CPU and GPU repeat '+str(n)+' passed',flush=True)
    need((run/'g0/pixels.bin').read_bytes()==(run/'g1/pixels.bin').read_bytes(),'repeatable GPU pixels')
    # No existing source or editor dependency may change in this bounded proof.
    for group in (sources,protected,before['complete_editor_dependencies_sha256']):
        for p,h in group.items():need(sha(p)==h,'after identity '+p)
    editor=prior['complete_editor_preserved'];need(sha(editor['path'])==editor['sha256'],'complete editor preserved')
    result=dict(status='VERIFIED',time_utc=datetime.now(timezone.utc).isoformat(),parent=parent,executable=exes,warnings=warnings,metrics=metrics,
        logs=logs,test_closures=test_closures,source_sha256=sources,protected_sha256=protected,complete_editor_preserved=editor,
        complete_editor_dependencies_sha256=before['complete_editor_dependencies_sha256'],editor_rebuilt=False,full_goal='active/incomplete',
        limits=['isolated float diffuse operator only','no VTF asset/material bridge','no default renderer change','no SFM scale/tone-map/packed filtering parity','inline scene lightmap transport absent'])
    put(run/'results.json',result);put(OUT/'latest-verification.json',dict(receipt=str(run/'results.json'),sha256=sha(run/'results.json')))
    print(json.dumps(dict(status=result['status'],run=str(run),metrics=metrics[0],warnings={k:len(v) for k,v in warnings.items()})),flush=True)
if __name__=='__main__':main()
