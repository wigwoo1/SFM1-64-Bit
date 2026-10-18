"""Original complete world trace compared with owned surface bookkeeping."""
import json
import random
import struct
import subprocess
import sys
import tempfile
from pathlib import Path
from verify_ifm_draw_packet import check_ranges
from verify_native_leaf_ambient_contract import ENGINE,DIGEST
from verify_source_world_light_overflow import MAP,MAP_SHA
from verify_swept_world_trace import PREPARED
from verify_dag_world_position import sha,require
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/world-trace-surface-20261015'
PRIOR=ROOT/'outputs/world-ray-trace-20261015/verification-2i321pi7'
RANGES=[('surface_lookup',0x10146440,0x1014646a),('box_line_kernel',0x10146850,0x10146c24),('general_brush_line_kernel',0x10147690,0x1014791f),('stationary_nested_surface_reset',0x1014ce00,0x1014ce3c),('displacement_surface_commit',0x1014d0a7,0x1014d0f8),('complete_world_wrapper',0x10149eb0,0x1014a12c)]
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def main():
    require(sha(ENGINE)==DIGEST and sha(MAP)==MAP_SHA and sha(PREPARED)=='76d24b8b60040b809791f36da772294c8ec107f084e9b30d245244f1cac7e02c','Pinned input changed');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));smoke='--smoke' in sys.argv;image,evidence=check_ranges(ENGINE,'engine-disassembly.txt',RANGES);require(image.word(0x10402fec)==0 and image.string(0x10350ee0)=='**displacement**','Original default/displacement surface')
    prior=json.loads((PRIOR/'results.json').read_text());require(prior['status']=='PASS' and sha(PRIOR/'queries.bin')=='e49f0bee17a906bab6dc4be7740fedcb60e55a079aef470461fcf3ec1f84cda9','Prior world query evidence');old=(PRIOR/'queries.bin').read_bytes();require(len(old)==19437*36,'Prior extent');rows=[old[i:i+36] for i in range(0,len(old),36)];categories=['prior']*len(rows)
    if smoke:rows=rows[:128]+rows[9504:9536];categories=['prior smoke']*len(rows)
    bsp=MAP.read_bytes();require(bsp[:4]==b'VBSP','Map magic')
    def lump(n):return struct.unpack_from('<4I',bsp,8+16*n)
    texOff,texSize,texVersion,texCompression=lump(6);require(texVersion==0 and texCompression==0 and texSize%72==0 and texOff+texSize<=len(bsp),'Texinfo extent');flags=[struct.unpack_from('<I',bsp,texOff+i*72+64)[0]&65535 for i in range(texSize//72)]
    def append(start,delta,category,mask=0x4481):rows.append(struct.pack('<6f3I',*map(f32,start),*map(f32,delta),mask,1,1));categories.append(category)
    # Trace the actual rays requested by the previous query fixtures. This test
    # remains world-only: it does not pretend to execute static-prop dispatch.
    visibility=ROOT/'outputs/world-light-visibility-20261015/verification-6trkd1er';queries=(visibility/'queries.bin').read_bytes();visibilityReceipt=json.loads((visibility/'results.json').read_text());require(visibilityReceipt['status']=='PASS' and sha(visibility/'queries.bin')==visibilityReceipt['query_sha256'],'Prior lighting query evidence')
    for i in range(0,len(queries)//124,256 if smoke else 1):
        h=struct.unpack_from('<7I',queries,i*124);v=struct.unpack_from('<18f',queries,i*124+52);position=v[0:3];normal=v[6:9];point=v[9:12]
        if h[0]==5:continue
        delta=[f32(f32(point[j]-f32(normal[j]*57016.3203125))-point[j]) for j in range(3)] if h[0]==3 else [f32(position[j]-point[j]) for j in range(3)]
        append(point,delta,'previous light query world ray')
    # Directed sky-face and ordinary-face rays in the real benchmark brushes.
    planeOff,planeBytes,_,_=lump(1);sideOff,sideBytes,_,_=lump(19);brushOff,brushBytes,_,_=lump(18);skyCandidates=0
    for i in range(brushBytes//12):
        first,count,contents=struct.unpack_from('<3I',bsp,brushOff+12*i)
        if not contents&0x4481:continue
        sides=[];low=[None]*3;high=[None]*3
        for j in range(count):
            pi,ti=struct.unpack_from('<Hh',bsp,sideOff+8*(first+j));n=struct.unpack_from('<3f',bsp,planeOff+20*pi);d=struct.unpack_from('<f',bsp,planeOff+20*pi+12)[0];sides.append((n,d,ti))
            for axis in range(3):
                if all(n[k]==0 for k in range(3) if k!=axis):
                    if n[axis]==1:high[axis]=d if high[axis] is None else min(high[axis],d)
                    if n[axis]==-1:low[axis]=-d if low[axis] is None else max(low[axis],-d)
        if any(x is None for x in low+high):continue
        center=[f32((low[k]+high[k])*.5) for k in range(3)]
        for n,d,ti in sides:
            if ti<0 or not flags[ti]&4:continue
            skyCandidates+=1;start=[f32(center[k]+n[k]*512) for k in range(3)];delta=[f32(center[k]-start[k]) for k in range(3)];append(start,delta,'directed sky brush face')
            if smoke and skyCandidates>=16:break
        if smoke and skyCandidates>=16:break
    # Strip the old preparation corpus to its43 complete input meshes.
    source=PREPARED.read_bytes();pos=0
    for i in range(43):
        power=struct.unpack_from('<I',source,pos)[0];cells=1<<power;pos+=60+16*(cells+1)**2+8*2*cells*cells
    prepared=run/'prepared.bin';prepared.write_bytes(source[:pos]);query=run/'queries.bin';query.write_bytes(b''.join(rows));outputs=[];executables=[];logs=[]
    for name,machine in [('surface_native_x86',0x14c),('surface_owned_x64',0x8664),('surface_safety_x64',0x8664)]:
        path=OUT/(name+'.exe');raw=path.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Executable architecture/manifest');executables.append(dict(path=str(path),sha256=sha(path),machine=hex(machine)))
        if 'safety' in name:args=[str(MAP)]
        else:args=[str(MAP),str(prepared),str(query),str(run/(name+'.bin'))]+([str(ENGINE)] if machine==0x14c else [])
        p=subprocess.run([str(path),*args],capture_output=True,text=True,timeout=120);(run/(name+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,name+' '+str(p.returncode)+' '+p.stdout+p.stderr);logs.append(p.stdout.strip())
        if 'safety' not in name:outputs.append((run/(name+'.bin')).read_bytes());require(len(outputs[-1])==len(rows)*76,'Output extent')
    different=[i for i in range(len(rows)) if outputs[0][i*76:(i+1)*76]!=outputs[1][i*76:(i+1)*76]];details=[dict(case=i,category=categories[i],query=struct.unpack('<6f3I',rows[i]),native=struct.unpack_from('<19I',outputs[0],i*76),owned=struct.unpack_from('<19I',outputs[1],i*76)) for i in different[:6]];(run/'comparison.json').write_text(json.dumps(dict(differences=len(different),details=details),indent=2)+'\n');require(not different,'Trace surface comparison differs: '+str(run/'comparison.json'))
    coverage=dict(default=0,displacement=0,bound_surface=0,sky=0,stationary=0,flags={})
    for i in range(len(rows)):
        identity,value=struct.unpack_from('<2I',outputs[0],i*76+68);coverage['stationary']+=struct.unpack('<6f3I',rows[i])[3:6]==(0.,0.,0.)
        if identity==0xffffffff:coverage['default']+=1;require(value==0,'Default flags')
        elif identity==0xfffffffe:coverage['displacement']+=1;require(value==0,'Displacement flags')
        else:coverage['bound_surface']+=1;require(identity<len(flags) and value==flags[len(flags)-1-identity],'Nonidentity texture/surface flag binding')
        coverage['sky']+=bool(value&4);coverage['flags'][str(value)]=coverage['flags'].get(str(value),0)+1
    priorMatch=None
    if not smoke:
        before=(PRIOR/'native_x86.bin').read_bytes();require(len(before)==19437*68,'Prior native result extent');priorMatch=b''.join(outputs[0][i*76:i*76+68] for i in range(19437))==before;require(priorMatch,'Existing68numeric bytes changed')
    require(coverage['bound_surface']>0 and coverage['sky']>0 and coverage['default']>0,'Missing surface coverage')
    if not smoke:require(coverage['displacement']>0,'Missing displacement surface coverage')
    require(sha(ENGINE)==DIGEST and sha(MAP)==MAP_SHA,'Original modified');sources={n:sha(ROOT/'work'/n) for n in ['source_collision_surface_candidate.h','source_bsp_trace_surface_flags_candidate.h','source_optimized_box_line_ray_candidate.h','source_convex_brush_ray_candidate.h','source_brush_line_dispatch_candidate.h','source_prepared_world_ray_candidate.h','source_world_ray_trace_candidate.h','source_bsp_world_light_trace_candidate.h','probe_swept_world_trace.cpp','test_world_trace_surface.cpp','build_world_trace_surface.cmd','verify_world_trace_surface.py']}
    receipt=dict(status='PASS',smoke=smoke,cases=len(rows),bytes_per_result=76,differences=0,coverage=coverage,prior_numeric_bytes_unchanged=priorMatch,logs=logs,executables=executables,query_sha256=sha(query),source_sha256=sources,engine_sha256=DIGEST,map_sha256=MAP_SHA,prepared_sha256=sha(prepared),static_bytes=sum(e['bytes'] for e in evidence),static_evidence=evidence,prior_world_receipt=str(PRIOR/'results.json'),prior_lighting_receipt=str(visibility/'results.json'),limits=['Full original world trace/brush/displacement surface bookkeeping executes in private PE with controlled host work-pool/memset; native prepared benchmark meshes vs owned BSP loader','Texture-info to runtime-surface mapping is a controlled reversed bijection, not real material-registry reconstruction; flags are actual BSP texinfo low16','76bytes include prior68numeric outputs plus normalized surface identity/flags; material name strings and surface properties are not verified','No live static-prop manager, entity filters, renderer/editor integration or actual full light query/collision composition in this test','No original writes, application launch or dependencies'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['status','smoke','cases','differences','coverage','prior_numeric_bytes_unchanged']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
