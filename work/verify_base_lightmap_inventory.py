"""Read-only world inventory; independent raw face/triangle/material reconciliation."""
from pathlib import Path,PurePosixPath
from datetime import datetime,timezone
from collections import Counter
import hashlib,json,math,os,re,struct,subprocess,tempfile,traceback
import verify_scene_base_lightmap as scene_fixture
import verify_source_base_lightmap as utility
import verify_texture_sampling_cross_api as texture_fixture

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/benchmark-lightmap-inventory-20261101'
MAP=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\maps\mtt_soldier.bsp')
CONTENT=MAP.parents[2]/'usermod'
NEW=['probe_base_lightmap_inventory_x64.cpp','test_base_lightmap_inventory.cpp',Path(__file__).name]
def need(v,s):
    if not v:raise RuntimeError(s)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def put(p,v):Path(p).write_text(json.dumps(v,indent=2)+'\n')
def lump(b,n):return struct.unpack_from('<2I',b,8+n*16)
def replace(b,n,data,version=0):struct.pack_into('<4I',b,8+n*16,len(b),len(data),version,0);b.extend(data)
def owned_cases(run):
    result=[]
    variants=['plain','shader_a','metadata_all','missing_a','styles','style_nonzero','no_styles','missing_offset','flags','bumped','ldr','unpaired','unavailable','lod_a','format3_a','degenerate','displacement','transition_a','alpha_a','binary32_cancellation']
    for name in variants:
        root=run/name;(root/'maps').mkdir(parents=True);m=root/'materials/owned';m.mkdir(parents=True)
        for family,mat in enumerate(('a','b')):
            v=bytearray(texture_fixture.fixture(0,family));struct.pack_into('<I',v,20,1);(m/(mat+'.vtf')).write_bytes(v)
            (m/(mat+'.vmt')).write_text('LightmappedGeneric { $basetexture owned/'+mat+' }')
        positions=[[[0.,0,0],[2.,0,0],[2.,2,0]],[[0.,0,0],[2.,2,0],[0.,2,0]],[[3.,0,0],[4.,0,0],[4.,1,0]]]
        if name=='degenerate':positions[2]=[[3.,0,0],[4.,0,0],[5.,0,0]]
        # Independently constructed arithmetic case, not original map geometry.
        if name=='binary32_cancellation':positions[2]=[[0.,0,0],[200.,200.-2.**-12,0],[199.,199.-2.**-12,0]]
        b=scene_fixture.fixture(positions);face,_=lump(b,58);ti,_=lump(b,6)
        if name=='shader_a':(m/'a.vmt').write_text('VertexLitGeneric { $basetexture owned/a }')
        if name=='metadata_all':
            for mat in ('a','b'):(m/(mat+'.vmt')).write_text('LightmappedGeneric { $basetexture owned/'+mat+' $surfaceprop metal %keywords inventory }')
        if name=='missing_a':(m/'a.vtf').unlink()
        if name=='styles':b[face+16:face+20]=bytes((0,2,255,255))
        if name=='style_nonzero':b[face+16]=2
        if name=='no_styles':b[face+16:face+20]=bytes((255,)*4)
        if name=='missing_offset':struct.pack_into('<i',b,face+20,-1)
        if name=='flags':struct.pack_into('<I',b,ti+64,0x400)
        if name in ('bumped','unavailable'):struct.pack_into('<I',b,ti+64,0x800)
        if name=='unavailable':struct.pack_into('<I',b,12+53*16,248)
        if name=='ldr':
            struct.pack_into('<4I',b,8+8*16,*struct.unpack_from('<4I',b,8+53*16));struct.pack_into('<I',b,12+53*16,0);struct.pack_into('<I',b,12+58*16,0)
        if name=='unpaired':struct.pack_into('<I',b,12+58*16,0)
        if name=='lod_a':
            v=bytearray((m/'a.vtf').read_bytes());v[88:88]=bytes(8);struct.pack_into('<I',v,12,96);struct.pack_into('<I',v,68,2);struct.pack_into('<I',v,84,96);struct.pack_into('<2I',v,88,0x02444f4c,0);(m/'a.vtf').write_bytes(v)
        if name=='format3_a':
            v=(m/'a.vtf').read_bytes();header=bytearray(v[:88]);struct.pack_into('<I',header,52,3);(m/'a.vtf').write_bytes(header+b''.join(v[i:i+3][::-1] for i in range(88,len(v),4)))
        if name=='displacement':
            pts=[[0.,0,0],[0.,2,0],[2.,2,0],[2.,0,0]]+positions[1]+positions[2];replace(b,3,b''.join(struct.pack('<3f',*p) for p in pts))
            edges=bytearray(4);surface=[];start=0
            for count in (4,3,3):
                for k in range(count):surface.append(len(edges)//4);edges.extend(struct.pack('<2H',start+k,start+(k+1)%count))
                start+=count
            replace(b,12,edges);replace(b,13,struct.pack('<10I',*surface))
            for f,(start,count) in enumerate(((0,4),(4,3),(7,3))):struct.pack_into('<IH',b,face+56*f+4,start,count)
            struct.pack_into('<h',b,face+12,0);info=bytearray(176);struct.pack_into('<I',info,20,1);replace(b,26,info)
            dv=bytearray(9*20)
            for k in range(9):struct.pack_into('<f',dv,k*20+8,1.)
            replace(b,33,dv);replace(b,48,bytes(16))
        if name=='transition_a':(m/'a.vmt').write_text('WorldVertexTransition { $basetexture owned/a $basetexture2 owned/b }')
        if name=='alpha_a':(m/'a.vmt').write_text('LightmappedGeneric { $basetexture owned/a $alphatest 1 $alphatestreference 0.5 }')
        (root/'maps/owned.bsp').write_bytes(b)
        admitted=3 if name=='plain' else 0 if name in ('metadata_all','ldr','unpaired','unavailable') else 1 if name in ('flags','bumped') else 2
        result.append(dict(name=name,root=root,admitted=admitted,triangles=10 if name=='displacement' else 2 if name in ('degenerate','binary32_cancellation') else 3))
    return result
def keyvalues(path):
    text=Path(path).read_text();tokens=[]
    for match in re.finditer(r'//[^\r\n]*|"([^"]*)"|([{}])|([^\s{}"]+)',text):
        if match[0].startswith('//'):continue
        tokens.append(next(x for x in match.groups() if x is not None))
    need(len(tokens)>2 and tokens[1]=='{','independent VMT root');at=2
    def body():
        nonlocal at
        entries=[]
        while at<len(tokens) and tokens[at]!='}':
            key=tokens[at];value=tokens[at+1];at+=2
            if value=='{':entries.append((key,'',body(),True))
            else:entries.append((key,value,[],False))
        need(at<len(tokens),'independent VMT termination');at+=1;return entries
    root=body();need(at==len(tokens),'independent VMT trailing input');return tokens[0],root
def material_reasons(m):
    shader,nodes=keyvalues(m['vmt']);need(shader==m['shader'] and [n[0] for n in nodes]==m['keys'],'authored VMT shader/keys exact');r=set()
    if shader.lower()!='lightmappedgeneric':r.add('material.shader')
    metadata={'%keywords','$surfaceprop'}
    if not nodes or len(nodes)>4 or sum(n[0].lower() not in metadata for n in nodes)>2:r.add('material.shape')
    bases=transforms=0;counts={key:0 for key in metadata}
    for key,value,children,block in nodes:
        key=key.lower()
        if block or children:r.add('material.block:'+key)
        if not value:r.add('material.empty_value:'+key)
        if key=='$basetexture':bases+=1
        elif key=='$basetexturetransform':transforms+=1
        elif key in metadata:
            counts[key]+=1
            if block or children or not value or len(value)>4095 or any(not 32<=ord(c)<=126 for c in value):r.add('material.metadata_value:'+key)
        else:r.add('material.parameter:'+key)
    if bases!=1:r.add('material.base_count')
    if transforms>1:r.add('material.transform_count')
    if any(n>1 for n in counts.values()):r.add('material.metadata_count')
    for key,value in m['prepared_features'].items():
        if value:r.add('material.'+key)
    if not m['prepared_texture']:r.add('texture.no_prepared_plan')
    elif m['texture_format'] not in (0,13,15):r.add('texture.format')
    need(r==set(m['reasons']),'independent material rejection reasons '+m['vmt'])
    plan=set()
    if not m['prepared_texture']:
        if m['prepared_features']['missing_base']:plan.add('source_missing')
        elif not m['source_texture']:plan.add('constant_or_untextured')
        else:
            raw=Path(m['source_texture']).read_bytes();fmt=struct.unpack_from('<I',raw,52)[0];w,h=struct.unpack_from('<2H',raw,16);flags=struct.unpack_from('<I',raw,20)[0];minor=struct.unpack_from('<I',raw,8)[0];levels=raw[56]
            if fmt not in (0,3,12,13,15):plan.add('source_format')
            if not w or not h or w>32768 or h>32768 or w&(w-1) or h&(h-1):plan.add('non_power_of_two_or_extent')
            if flags&0x4000:plan.add('envmap_flag')
            if not levels:plan.add('no_mips')
            if minor>1 and struct.unpack_from('<H',raw,63)[0]!=1:plan.add('non_2d_depth')
            if minor>=3:
                for n in range(struct.unpack_from('<I',raw,68)[0]):
                    if struct.unpack_from('<I',raw,80+n*8)[0]&0xffffff==0x444f4c:plan.add('lod_resource_entry')
            required=1 if flags&0x100 else max(w,h).bit_length()
            if required>levels:plan.add('incomplete_declared_chain')
    need(plan==set(m['plan_unavailable_reasons']),'independent unavailable VTF plan reasons '+m['vmt']);return r
def raw_faces(b,available):
    hdr=not lump(b,8)[1] and lump(b,53)[1];face_id=58 if hdr and lump(b,58)[1] else 7;light_id=53 if hdr else 8;paired=not hdr or face_id==58
    fo,_=lump(b,face_id);mo,_=lump(b,14);first,count=struct.unpack_from('<2i',b,mo+40);ti,_=lump(b,6);td,_=lump(b,2);st,_=lump(b,44);strings,_=lump(b,43);vo,_=lump(b,3);so,_=lump(b,13);eo,_=lump(b,12);dp,_=lump(b,26);tags,_=lump(b,48);light,_=lump(b,light_id)
    result=[];vertex_count=0
    for face in range(first,first+count):
        at=fo+56*face;edge_start,edges,info,disp=struct.unpack_from('<ihhh',b,at+4);tip=ti+info*72;flags=struct.unpack_from('<I',b,tip+64)[0];name_index=struct.unpack_from('<i',b,td+struct.unpack_from('<I',b,tip+68)[0]*32+12)[0];name_at=strings+struct.unpack_from('<I',b,st+name_index*4)[0];name=b[name_at:b.index(0,name_at)].decode();styles=list(b[at+16:at+20]);style_count=next((n for n,s in enumerate(styles) if s==255),4);offset=struct.unpack_from('<i',b,at+20)[0];points=[]
        for k in range(edges):
            se=struct.unpack_from('<i',b,so+(edge_start+k)*4)[0];v=struct.unpack_from('<H',b,eo+abs(se)*4+(2 if se<0 else 0))[0];points.append(struct.unpack_from('<3f',b,vo+v*12))
        if disp>=0:
            d=dp+disp*176;power=struct.unpack_from('<i',b,d+20)[0];start=struct.unpack_from('<I',b,d+16)[0];triangles=sum(not(struct.unpack_from('<H',b,tags+2*(start+n))[0]&32) for n in range(2*(1<<power)**2));vertex_count+=((1<<power)+1)**2
        else:
            triangles=0;double_triangles=0;vertex_count+=edges
            for k in range(1,edges-1):
                a,q,c=points[0],points[k],points[k+1];cross=[(q[1]-a[1])*(c[2]-a[2])-(q[2]-a[2])*(c[1]-a[1]),(q[2]-a[2])*(c[0]-a[0])-(q[0]-a[0])*(c[2]-a[2]),(q[0]-a[0])*(c[1]-a[1])-(q[1]-a[1])*(c[0]-a[0])];length=math.sqrt(sum(x*x for x in cross));need(not 0.9e-8<length<1.1e-8,'ambiguous independent degeneracy threshold');triangles+=length>=1e-8
                double_triangles+=length>=1e-8
                # Existing CPU loader is compiled /fp:strict on binary32 values.
                # Keep the rejected double prediction separately, without fitting
                # a threshold, changing geometry, or excluding source triangles.
                f=utility.f32;dq=[f(q[i]-a[i]) for i in range(3)];dc=[f(c[i]-a[i]) for i in range(3)];cross=[f(f(dq[1]*dc[2])-f(dq[2]*dc[1])),f(f(dq[2]*dc[0])-f(dq[0]*dc[2])),f(f(dq[0]*dc[1])-f(dq[1]*dc[0]))]
                binary32_length=f(math.sqrt(f(f(f(cross[0]*cross[0])+f(cross[1]*cross[1]))+f(cross[2]*cross[2]))));triangles+=int(binary32_length>=f(1e-8))-int(length>=1e-8)
        reasons=set()
        if not available:reasons.add('data.snapshot_unavailable')
        if not paired:reasons.add('data.unpaired_tables')
        if face_id!=58 or light_id!=53:reasons.add('data.requires_hdr_pair')
        if offset<0:reasons.add('data.missing_light_offset')
        if not style_count:reasons.add('data.no_styles')
        if style_count!=1:reasons.add('data.style_count')
        if style_count and styles[0]!=0:reasons.add('data.style_not_zero')
        if flags&0x800:reasons.add('data.bumped_planes')
        if flags:reasons.add('data.face_flags')
        if disp!=-1:reasons.add('data.displacement')
        if not triangles and available:reasons.add('data.no_emitted_triangle')
        if not reasons:
            w,h=struct.unpack_from('<2i',b,at+36)
            for t in range((w+1)*(h+1)):
                r,g,blue,e=struct.unpack_from('<4B',b,light+offset+t*4);value=utility.f32(max(r,g,blue)*2.**(e if e<128 else e-256)/255.)
                if value>float.fromhex('0x1.fffffep+127')/4:reasons.add('data.arithmetic_headroom')
        if available:
            minima=struct.unpack_from('<2i',b,at+28)
            for axis in range(2):
                axes=struct.unpack_from('<4f',b,tip+32+axis*16)
                if any(abs(sum(p[k]*axes[k] for k in range(3))+axes[3]-minima[axis])>8192 for p in points):reasons.add('data.luxel_range')
        result.append(dict(id=face,name=name,triangles=triangles,naive_double_triangles=triangles if disp>=0 else double_triangles,flags=flags,displacement=disp,offset=offset,styles=styles,planes=4 if flags&0x800 else 1,reasons=reasons))
    return result,vertex_count,face_id,light_id,paired
def reconcile(path,map_path):
    j=json.loads(Path(path).read_text());faces,vertices,fl,ll,paired=raw_faces(Path(map_path).read_bytes(),j['metadata_available']);need((fl,ll,paired)==(j['face_lump'],j['lighting_lump'],j['paired']),'dataset selection')
    need(len(faces)==len(j['faces']),'all raw faces accounted');material=[material_reasons(m) for m in j['materials']];reason_counts=Counter();face_totals=Counter();tri_totals=Counter();admitted=admitted_tri=data=drawn=0
    for raw,f in zip(faces,j['faces']):
        need((f['id'],f['source_material'],f['triangles'],f['flags'],f['displacement'],f['light_offset'],f['styles'],f['planes'])==(raw['id'],raw['name'],raw['triangles'],raw['flags'],raw['displacement'],raw['offset'],raw['styles'],raw['planes']),'independent raw face/triangle record '+str(raw['id']))
        m=j['materials'][f['material']];suffix='/materials/'+str(PurePosixPath(raw['name'].replace('\\','/')).with_suffix('.vmt'));need(m['vmt'].lower().endswith(suffix.lower()),'raw/prepared material path')
        need(set(f['data_reasons'])==raw['reasons'],'independent data refusal reasons '+str(raw['id']));combined=raw['reasons']|material[f['material']];need(set(f['combined_reasons'])==combined,'combined actual material/data gate')
        reason_counts.update(combined);face_totals[f['material']]+=1;tri_totals[f['material']]+=raw['triangles'];data+=not raw['reasons'];admitted+=not combined;admitted_tri+=raw['triangles'] if not combined else 0;drawn+=raw['triangles']!=0
    need(reason_counts==Counter(j['overlapping_combined_face_reasons']),'overlap reason totals')
    range_faces={r['id']:set() for r in j['ranges']};range_triangles=Counter()
    for f in j['faces']:
        need(len(f['ranges'])==(1 if f['triangles'] else 0),'one source material range per emitted face')
        for r in f['ranges']:range_faces[r].add(f['id']);range_triangles[r]+=f['triangles'];need(j['ranges'][r]['material']==f['material'],'face/range material reconciliation')
    for r in j['ranges']:need(r['faces']==sorted(range_faces[r['id']]) and r['triangles']==range_triangles[r['id']] and r['index_count']==r['triangles']*3,'range triangle/face reconciliation')
    for m in j['materials']:need(m['faces']==face_totals[m['id']] and m['triangles']==tri_totals[m['id']] and m['ranges']==sum(r['material']==m['id'] for r in j['ranges']),'material totals')
    summary=dict(materials=len(j['materials']),materials_admitted=sum(not r for r in material),ranges=len(j['ranges']),draws=sum(r['index_count']>0 for r in j['ranges']),vertices=vertices,indices=sum(f['triangles']*3 for f in faces),faces=len(faces),drawn_faces=drawn,data_only_admitted_faces=data,combined_admitted_faces=admitted,triangles=sum(f['triangles'] for f in faces),combined_admitted_triangles=admitted_tri)
    need(summary==j['summary'],'independent summary reconciliation');j['independent_precision_notes']=[dict(face=f['id'],binary32_triangles=f['triangles'],naive_double_triangles=f['naive_double_triangles']) for f in faces if f['triangles']!=f['naive_double_triangles']];return j
def main():
    before=json.loads((OUT/'before.json').read_text());parent=before['parent'];need(sha(parent['receipt'])==parent['receipt_sha256'],'parent receipt');prior=json.loads(Path(parent['receipt']).read_text());sources=dict(before['source_sha256']);protected=dict(before['protected_sha256']);protected[parent['receipt']]=parent['receipt_sha256']
    for group in (sources,protected,before['complete_editor_dependencies_sha256']):
        for p,h in group.items():need(sha(p)==h,'prior unchanged identity '+p)
    for name in NEW:
        p=ROOT/'work'/name;need(str(p) not in sources,'new inventory files only');sources[str(p)]=sha(p)
    run=Path(tempfile.mkdtemp(prefix='r-',dir=OUT));print(run,flush=True);fixtures=owned_cases(run);inputs={str(p):sha(p) for p in run.rglob('*') if p.is_file()};put(run/'protocol.json',dict(time_utc=datetime.now(timezone.utc).isoformat(),source_sha256=sources,scope='world admission inventory, no renderer changes',fixtures=[x['name'] for x in fixtures],benchmark=str(MAP)))
    clean={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')};exes={};warnings={};closures={}
    def execute(args,label):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,env=clean,capture_output=True,text=True,timeout=300,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed '+(p.stdout+p.stderr)[-2000:]);return p.stdout+p.stderr
    try:
        for name,source in [('inventory','probe_base_lightmap_inventory_x64.cpp'),('test','test_base_lightmap_inventory.cpp')]:
            closures[name]=utility.closure(ROOT/'work'/source)
            for p,h in closures[name].items():need(sources.get(p)==h,'registered source closure '+p)
            exe=run/(name+'.exe');batch=run/(name+'-build.cmd');batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{utility.VS}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\{source} /Fo:"{run/(name+".obj")}" /Fe:"{exe}" >"{run/(name+"-build.log")}" 2>&1','exit /b %errorlevel%','']))
            execute(['cmd.exe','/d','/c',batch],name+'-build-driver');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==0x8664 and struct.unpack_from('<H',b,pe+24)[0]==0x20b,'AMD64 PE32+');execute([utility.DUMP,'/imports',exe],name+'-imports');exes[name]=dict(path=str(exe),sha256=sha(exe));warnings[name]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(name+'-build.log')).read_text());print('Fresh CPU build '+name+' warnings='+str(len(warnings[name])),flush=True)
        test_logs=[];fixture_results=[]
        for n in range(2):test_logs.append(execute([run/'test.exe',fixtures[0]['root']],f'test-{n}'))
        for f in fixtures:
            target=run/('checked-'+f['name']);execute([run/'inventory.exe',f['root']/'maps/owned.bsp',f['root'],'owned',target,'owned'],'fixture-'+f['name']);j=reconcile(target/'inventory.json',f['root']/'maps/owned.bsp');need(j['summary']['combined_admitted_faces']==f['admitted'] and j['summary']['triangles']==f['triangles'],'predeclared fixture admitted/triangle totals '+f['name']);fixture_results.append(dict(name=f['name'],summary=j['summary']));print('Fixture verified '+f['name'],flush=True)
        discovery=run/'benchmark-input-discovery';execute([run/'inventory.exe',MAP,CONTENT,'mtt_soldier',discovery,'mounted'],'benchmark-input-discovery');discovered=json.loads((discovery/'inventory.json').read_text());paths={str(MAP),str(CONTENT/'gameinfo.txt')}|{p for m in discovered['materials'] for p in m['asset_files']};benchmark_inputs={str(Path(p)):sha(p) for p in paths};put(run/'benchmark-input-sha256.json',benchmark_inputs)
        benchmark=[]
        for n in range(2):
            target=run/f'benchmark-{n}';execute([run/'inventory.exe',MAP,CONTENT,'mtt_soldier',target,'mounted'],f'benchmark-{n}');j=reconcile(target/'inventory.json',MAP);need(j['summary']['faces']==8050 and j['summary']['materials']==59 and j['summary']['ranges']==59 and j['summary']['triangles']==27147,'prior benchmark dimensions');benchmark.append(j);print('Benchmark independent reconciliation '+str(n)+' passed',flush=True)
        need((run/'benchmark-0/inventory.json').read_bytes()==(run/'benchmark-1/inventory.json').read_bytes(),'exact benchmark repeatability')
        for group in (sources,protected,inputs,benchmark_inputs,before['complete_editor_dependencies_sha256']):
            for p,h in group.items():need(sha(p)==h,'final unchanged identity '+p)
        final=benchmark[0];plan_reasons=Counter(reason for m in final['materials'] for reason in m['plan_unavailable_reasons']);result=dict(status='VERIFIED',time_utc=datetime.now(timezone.utc).isoformat(),parent=parent,source_sha256=sources,protected_sha256=protected,source_transitions={},new_work_files=NEW,executable=exes,warnings=warnings,closures=closures,test_logs=test_logs,fixtures=fixture_results,benchmark_input_sha256=benchmark_inputs,benchmark_summary=final['summary'],benchmark_reasons=final['overlapping_combined_face_reasons'],texture_plan_reasons=dict(plan_reasons),benchmark_precision_notes=final['independent_precision_notes'],benchmark_inventory=dict(path=str(run/'benchmark-0/inventory.json'),sha256=sha(run/'benchmark-0/inventory.json')),complete_editor_preserved=prior['complete_editor_preserved'],complete_editor_dependencies_sha256=before['complete_editor_dependencies_sha256'],editor_rebuilt=False,new_gpu_tests=False,full_goal='active/incomplete',limits=['world base-lightmap contract only; no session model/inline inventory','no shader behavior/admission change','no new renderer image/native HDR equivalence'])
        put(run/'results.json',result);put(OUT/'latest-verification.json',dict(receipt=str(run/'results.json'),sha256=sha(run/'results.json')));print(json.dumps(dict(status='VERIFIED',summary=final['summary'],texture_plan_reasons=dict(plan_reasons))),flush=True)
    except Exception:put(run/'failure.json',dict(status='FAILED',traceback=traceback.format_exc(),source_sha256=sources,executable=exes));raise
if __name__=='__main__':main()
