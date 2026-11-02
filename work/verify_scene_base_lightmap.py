"""Retained owned BSP/VMT/VTF -> existing prepared owner -> indexed lightmaps."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,itertools,json,math,os,re,struct,subprocess,tempfile,traceback
import verify_source_base_lightmap as base
import verify_d3d9_interpolation_contract as raster
import verify_texture_sampling_cross_api as texture

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/scene-base-lightmap-20261101'
VS=base.VS
TOLERANCE=2e-6
MODIFIED=['d3d9_prepared_scene_candidate.h','source_bsp_base_lightmap_candidate.h']
NEW=['scene_base_lightmap_candidate.h','probe_d3d9_scene_base_lightmap_x64.cpp','test_scene_base_lightmap.cpp',Path(__file__).name]
COLORS=base.COLORS+[[ (96,32,16,0),(24,128,48,1),(160,48,96,255),(16,64,192,0) ]]
def need(v,s):
    if not v:raise RuntimeError(s)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def put(p,v):Path(p).write_text(json.dumps(v,indent=2)+'\n')
def f32(x):return base.f32(x)
def string(s):
    b=str(s).encode();return struct.pack('<I',len(b))+b
def decoded(face):return [[f32(c*2.**(p[3] if p[3]<128 else p[3]-256)/255.) for c in p[:3]]+[1.] for p in COLORS[face]]
def fixture(positions):
    b=bytearray(1036);b[:4]=b'VBSP';struct.pack_into('<I',b,4,20)
    def lump(n,data,version=0):struct.pack_into('<4I',b,8+n*16,len(b),len(data),version,0);b.extend(data)
    lump(3,b''.join(struct.pack('<3f',*v) for tri in positions for v in tri))
    edges=bytearray(4)
    for face in range(3):
        for k in range(3):edges.extend(struct.pack('<2H',face*3+k,face*3+(k+1)%3))
    lump(12,edges);lump(13,struct.pack('<9I',*range(1,10)))
    ti=bytearray(144)
    for n in range(2):
        at=n*72
        for offset in (0,20):struct.pack_into('<f',ti,at+offset,8.)
        for offset in (12,28):struct.pack_into('<f',ti,at+offset,8.)
        struct.pack_into('<f',ti,at+32,.5);struct.pack_into('<f',ti,at+52,.5)
        struct.pack_into('<f',ti,at+44,-2.5);struct.pack_into('<f',ti,at+60,-4.5)
        struct.pack_into('<I',ti,at+68,n)
    lump(6,ti)
    td=bytearray(64)
    for n in range(2):
        struct.pack_into('<I',td,n*32+12,n)
        for offset in (16,20,24,28):struct.pack_into('<I',td,n*32+offset,8)
    lump(2,td);lump(43,b'owned/b\0owned/a\0');lump(44,struct.pack('<2I',0,8))
    faces=bytearray(168)
    for face in range(3):
        at=56*face;struct.pack_into('<IHHhh',faces,at+4,face*3,3,1 if face==1 else 0,-1,0)
        faces[at+16:at+20]=bytes((0,255,255,255));struct.pack_into('<i',faces,at+20,(24,128,232)[face]);struct.pack_into('<4i',faces,at+28,-3,-5,1,1)
    lump(7,faces,1);lump(58,faces,1)
    model=bytearray(48);struct.pack_into('<I',model,44,3);lump(14,model)
    light=bytearray([222]*320)
    for face in range(3):
        for k,p in enumerate(COLORS[face]):at=(24,128,232)[face]+k*4;light[at:at+4]=bytes(p)
    lump(53,light,1);return b
def build_cases(run):
    rows=[]
    corners=[(2,2),(22,2),(22,22),(2,22),(24,24),(30,24),(30,30)]
    for serial,(fmt,frame,mip,perspective) in enumerate(itertools.product((0,13,15),(0,1),(0,1),(False,True))):
        root=run/f'owned-{serial}';(root/'maps').mkdir(parents=True);materials=root/'materials/owned';materials.mkdir(parents=True)
        flags=13 if serial%3==0 else 1
        paths={}
        for family,name in enumerate(('a','b')):
            raw=bytearray(texture.fixture(fmt,family));struct.pack_into('<I',raw,20,flags);p=materials/(name+'.vtf');p.write_bytes(raw);paths[name]=str(p)
        prefix=bytearray(texture.fixture(0,0));struct.pack_into('<I',prefix,20,1)
        for at in range(88+3,len(prefix),4):prefix[at]=255
        (materials/'prefix.vtf').write_bytes(prefix);paths['prefix']=str(materials/'prefix.vtf')
        transform='center 0 0 scale 0.73 0.61 translate 0.123 0.237'
        (materials/'a.vmt').write_text('LightmappedGeneric { $basetexture owned/a }')
        (materials/'b.vmt').write_text('LightmappedGeneric { $basetexture owned/b $basetexturetransform "'+transform+'" }')
        weights=[1.,1.5,2.,1.25,1.,1.5,2.] if perspective else [1.]*7
        points=[[f32(((x+.5)/16-1)*w),f32(((y+.5)/16-1)*w),f32((w-1)*4)] for (x,y),w in zip(corners,weights)]
        positions=[[points[k] for k in tri] for tri in ((0,1,2),(0,2,3),(4,5,6))]
        (root/'maps/owned.bsp').write_bytes(fixture(positions))
        camera=[1.,0,0,0,0,1.,0,0,0,0,0,.5,0,0,.25 if perspective else 0,1.]
        modulation=[.5,1.25,.75] if serial%3 else [1.,1.,1.]
        triangles=[]
        prefixpos=[[(x+.5)/16-1,(y+.5)/16-1,0.] for x,y in ((1,26),(5,26),(1,30))]
        for face,pts in [(-1,prefixpos)]+list(enumerate(positions)):
            vertices=[]
            for px,py,pz in pts:
                w=f32(1+f32(camera[14]*pz));clip=[f32(px-f32(w/32)),f32(-py+f32(w/32)),.5,w]
                uv=[f32(px+1),f32(py+1)] if face>=0 else [f32(.234),f32(.346)]
                luxel=[f32(px*.5+.5),f32(py*.5+.5)] if face>=0 else [0.,0.]
                vertices.append(dict(clip=clip,attribute=luxel+uv))
            material='prefix' if face==-1 else 'a' if face==1 else 'b'
            matrix=[f32(.73),0.,f32(.123),0.,f32(.61),f32(.237)] if material=='b' else [1.,0,0,0,1.,0]
            triangles.append(dict(face=face,material=material,vertices=vertices,matrix=matrix))
        rows.append(dict(root=str(root),format=fmt,flags=flags,frame=frame,mip=mip,perspective=perspective,camera=camera,modulation=modulation,paths=paths,triangles=triangles,positions=positions))
    return rows
def query(rows,path):path.write_bytes(struct.pack('<I',len(rows))+b''.join(string(r['root'])+struct.pack('<2I19f',r['frame'],r['mip'],*r['camera'],*r['modulation']) for r in rows))
def reference_query(rows,path):
    requests=[];mapping=[];lookups=[]
    for serial,r in enumerate(rows):
        row_lookup={}
        for triangle in r['triangles']:
            samples=[];pixels=[]
            for y in range(32):
                for x in range(32):
                    v=raster.expected(triangle['vertices'],x,y)
                    if not any(v):continue
                    need((x,y) not in row_lookup,'disjoint/edge-owned fixture triangles')
                    row_lookup[x,y]=(triangle['face'],v[:2]);pixels.append((x,y))
                    m=triangle['matrix'];u=f32(m[0]*v[2]+m[1]*v[3]+m[2]);vv=f32(m[3]*v[2]+m[4]*v[3]+m[5])
                    frame=0 if triangle['face']==-1 else r['frame'];samples.append(struct.pack('<I7f',frame,u,vv,0.,0.,0.,0.,0.))
            flags=1 if triangle['face']==-1 else r['flags'];mip=0 if triangle['face']==-1 else r['mip']
            q=[1,0,0,0,2 if flags&4 else 0,2 if flags&8 else 0,0,0xffffffff,0,mip]
            requests.append(struct.pack('<10I',*q)+string('owned/independent-base-sampling')+string(r['paths'][triangle['material']])+struct.pack('<I',len(samples))+b''.join(samples));mapping.append((serial,pixels))
        lookups.append(row_lookup)
    path.write_bytes(struct.pack('<I',len(requests))+b''.join(requests));return mapping,lookups
def compare(rows,path,reference,mapping,lookups,metadata):
    b=reference.read_bytes();at=0;sampled={}
    for serial,(row,pixels) in enumerate(mapping):
        need(struct.unpack_from('<2I',b,at)==(serial,len(pixels)),'independent reference framing');at+=8
        for point in pixels:sampled[row,point]=struct.unpack_from('<4f',b,at);at+=16
    need(at==len(b),'complete independent reference')
    b=path.read_bytes();raw=metadata.read_bytes();at=mt=0;maximum=0.;over=coverage=0;pixels=covered=0;mid=0
    for serial,r in enumerate(rows):
        need(struct.unpack_from('<I',b,at)[0]==serial,'scene capture serial');at+=4;actual=struct.unpack_from('<4096f',b,at);at+=16384
        need(struct.unpack_from('<3I',raw,mt)==(serial,12,12),'actual geometry dimensions');mt+=12
        geometry=struct.unpack_from('<60f',raw,mt);mt+=240
        wantpos=[[(x+.5)/16-1,(y+.5)/16-1,0.,f32(.234),f32(.346)] for x,y in ((1,26),(5,26),(1,30))]+[[*v,f32(v[0]+1),f32(1-f32(v[1]+1))] for tri in r['positions'] for v in tri]
        need(list(geometry)==[f32(x) for v in wantpos for x in v],'independent source geometry/base UV emission')
        need(struct.unpack_from('<12I',raw,mt)==(0,1,2,3,4,5,9,10,11,6,7,8),'actual regrouped global index window');mt+=48
        need(struct.unpack_from('<17I',raw,mt)==(4,0,3,0,0xffffffff,3,3,1,0,6,3,1,2,9,3,2,1),'actual split/material/source face identities');mt+=68
        for y in range(32):
            for x in range(32):
                p=(x,y);a=actual[4*(y*32+x):4*(y*32+x)+4];want=[0.]*4
                if p in lookups[serial]:
                    face,luxel=lookups[serial][p];albedo=sampled[serial,p];covered+=1
                    if face==-1:want=list(albedo)
                    else:
                        light=base.sample(decoded(face),*luxel);want=[albedo[k]*light[k]*r['modulation'][k] for k in range(3)]+[1.]
                        mid+=any(0<c<1 for c in albedo[:3])
                coverage+=(a[3]==0)!=(want[3]==0);pixels+=1
                for v,w in zip(a,want):need(math.isfinite(v),'finite scene pixel');error=abs(v-w);maximum=max(maximum,error);over+=error>TOLERANCE
    need(at==len(b) and mt==len(raw),'complete scene/metadata output')
    return dict(pixels=pixels,covered=covered,midvalue_srgb_albedo_pixels=mid,max_error=maximum,over_budget=over,coverage_mismatches=coverage,boundary_pixels_excluded=0)
def main():
    before=json.loads((OUT/'before.json').read_text());parent=before['parent'];need(sha(parent['receipt'])==parent['receipt_sha256'],'parent identity');prior=json.loads(Path(parent['receipt']).read_text())
    sources=dict(before['source_sha256']);protected=dict(before['protected_sha256']);protected[parent['receipt']]=parent['receipt_sha256'];transitions={}
    for name in MODIFIED:
        p=str(ROOT/'work'/name);need(p in sources,'tracked transition');transitions[p]={'before':sources[p],'after':sha(p)};sources[p]=sha(p)
    for name in NEW:
        p=ROOT/'work'/name;need(str(p) not in sources,'new source identity');sources[str(p)]=sha(p)
    for group in (sources,protected,before['complete_editor_dependencies_sha256']):
        for p,h in group.items():need(sha(p)==h,'before current identity '+p)
    run=Path(tempfile.mkdtemp(prefix='r-',dir=OUT));print(run,flush=True)
    rows=build_cases(run);put(run/'cases.json',rows);query(rows,run/'query.bin');mapping,lookups=reference_query(rows,run/'reference.bin')
    put(run/'protocol.json',dict(time_utc=datetime.now(timezone.utc).isoformat(),source_sha256=sources,transitions=transitions,tolerance=TOLERANCE,cases=len(rows),no_excluded_pixels=True,reference='fresh independent owned GPU texture sampling + frozen n.8 raster + independent raw RGBExp32 math'))
    inputs={str(p):sha(p) for p in run.rglob('*') if p.is_file()};clean={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')}
    def execute(args,label,timeout=600):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,env=clean,capture_output=True,text=True,timeout=timeout,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed '+(p.stdout+p.stderr)[-2200:]);return p.stdout+p.stderr
    exes={};warnings={};closures={}
    def compile(name,source,libs=''):
        closure=base.closure(ROOT/'work'/source)
        for p,h in closure.items():need(sources.get(p)==h,'unregistered dependency '+p)
        closures[name]=closure;exe=run/(name+'.exe');batch=run/(name+'-build.cmd')
        batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\{source} /Fo:"{run/(name+".obj")}" /Fe:"{exe}" {libs} >"{run/(name+"-build.log")}" 2>&1','exit /b %errorlevel%','']))
        execute(['cmd.exe','/d','/c',batch],name+'-build-driver');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];need(struct.unpack_from('<H',binary,pe+4)[0]==0x8664 and struct.unpack_from('<H',binary,pe+24)[0]==0x20b,'AMD64 PE32+')
        execute([base.DUMP,'/imports',exe],name+'-imports');exes[name]=dict(path=str(exe),sha256=sha(exe));warnings[name]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(name+'-build.log')).read_text());print('Fresh build '+name+' warnings='+str(len(warnings[name])),flush=True);return exe
    try:
        cpu=compile('scene-cpu','test_scene_base_lightmap.cpp');decoder=compile('decoder-cpu','test_source_bsp_base_lightmap.cpp')
        gpu=compile('scene-gpu','probe_d3d9_scene_base_lightmap_x64.cpp','d3d9.lib d3dcompiler.lib user32.lib');reference=compile('reference','probe_anisotropic_sampling_d3d9_x64.cpp','d3d9.lib d3dcompiler.lib user32.lib')
        execute([reference,run/'reference.bin',run/'reference',4318,11544],'reference')
        logs=[];metrics=[]
        for repeat in range(2):
            logs.append(execute([cpu,rows[0]['root']],f'cpu-{repeat}'));logs.append(execute([decoder],f'decoder-{repeat}'))
            logs.append(execute([gpu,run/'query.bin',run/f'g{repeat}'],f'gpu-{repeat}'));m=compare(rows,run/f'g{repeat}/pixels.bin',run/'reference/samples.bin',mapping,lookups,run/f'g{repeat}/metadata.bin');put(run/f'g{repeat}-metrics.json',m);metrics.append(m)
            need(m['over_budget']==0 and m['coverage_mismatches']==0 and m['midvalue_srgb_albedo_pixels']>0,'frozen scene lightmap pixel gate failed');print('Scene bridge repeat '+str(repeat)+' verified '+str(m),flush=True)
        need((run/'g0/pixels.bin').read_bytes()==(run/'g1/pixels.bin').read_bytes(),'exact scene pixel repeats')
        # Fresh old operator: source/material-validation header changed explicitly.
        operator=compile('operator-gpu','probe_d3d9_base_lightmap_x64.cpp','d3d9.lib d3dcompiler.lib user32.lib');old=ROOT/'outputs/source-lightmap-fragment-20261101/r-ev8g37p4'
        for n in range(2):execute([operator,old/'query.bin',run/f'operator-{n}'],f'operator-{n}');need((run/f'operator-{n}/pixels.bin').read_bytes()==(old/'g0/pixels.bin').read_bytes(),'old48 operator pixels exact')
        # Fresh98 diagnostic with unchanged native reference and original fixtures.
        shading=compile('shading-gpu','probe_d3d9_shading_consumption_x64.cpp','d3d9.lib d3dcompiler.lib user32.lib');pointer=json.loads((ROOT/'outputs/d3d9-shading-consumption-20261101/latest-diagnostic.json').read_text());d=json.loads(Path(pointer['receipt']).read_text());old_diag=Path(d['executable']['path']).parent
        for n in range(2):execute([shading,old_diag/'q.bin',old_diag/'owned.vtf',old_diag/'owned.bsp',run/f'shading-{n}'],f'shading-{n}');need((run/f'shading-{n}/pixels.bin').read_bytes()==(old_diag/'d0/pixels.bin').read_bytes(),'old98 shading pixels exact')
        # Existing full730-case gate, with explicit new source transitions.
        import verify_d3d9_prepared_scene as default
        previous=json.loads((ROOT/'outputs/bsp-lightmap-scene-20261101/milestone.json').read_text());index=json.loads(Path(previous['indexed_gpu_receipt']['receipt']).read_text());directory=run/'default';directory.mkdir()
        put(directory/'before.json',dict(parent=index['parent'],source_sha256=sources,protected_sha256=protected,actual_parent=parent,explicit_transitions=transitions));default.OUT=directory;default.main();default_ref=json.loads((directory/'latest-verification.json').read_text());need(sha(default_ref['receipt'])==default_ref['receipt_sha256'],'fresh default receipt');default_result=json.loads(Path(default_ref['receipt']).read_text());need(default_result['cases']==730 and all(m['over_2e_6']==0 for m in default_result['metrics'].values()),'default730 gate')
        for group in (sources,protected,inputs,before['complete_editor_dependencies_sha256']):
            for p,h in group.items():need(sha(p)==h,'final frozen identity '+p)
        result=dict(status='VERIFIED',time_utc=datetime.now(timezone.utc).isoformat(),parent=parent,source_sha256=sources,protected_sha256=protected,source_transitions=transitions,new_work_files=NEW,executable=exes,warnings=warnings,closures=closures,metrics=metrics,logs=logs,default_gpu_receipt=default_ref,complete_editor_preserved=prior['complete_editor_preserved'],complete_editor_dependencies_sha256=before['complete_editor_dependencies_sha256'],editor_rebuilt=False,full_goal='active/incomplete',limits=['normalized explicit float operator; native SFM scale/HDR packing/tonemapping unverified','strict paired HDR/static-style0/base-plane/plain material subset','no default editor path or full benchmark image'])
        put(run/'results.json',result);put(OUT/'latest-verification.json',dict(receipt=str(run/'results.json'),sha256=sha(run/'results.json')));print(json.dumps(dict(status='VERIFIED',run=str(run),metrics=metrics[0])),flush=True)
    except Exception:
        put(run/'failure.json',dict(status='FAILED',time_utc=datetime.now(timezone.utc).isoformat(),traceback=traceback.format_exc(),source_sha256=sources,executable=exes));raise
if __name__=='__main__':main()
