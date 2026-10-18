"""Actual owned BSP trace inside lighting vs two original-execution stages."""
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from verify_native_leaf_ambient_contract import ENGINE,DIGEST
from verify_source_world_light_overflow import MAP,MAP_SHA
from verify_swept_world_trace import PREPARED
from verify_dag_world_position import sha,require
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/world-trace-surface-20261015'
PRIOR=ROOT/'outputs/world-light-visibility-20261015/verification-6trkd1er'
def main():
    require(sha(ENGINE)==DIGEST and sha(MAP)==MAP_SHA and sha(PREPARED)=='76d24b8b60040b809791f36da772294c8ec107f084e9b30d245244f1cac7e02c','Pinned input changed');run=Path(tempfile.mkdtemp(prefix='lighting-verification-',dir=OUT));smoke='--smoke' in sys.argv;prior=json.loads((PRIOR/'results.json').read_text());require(prior['status']=='PASS' and sha(PRIOR/'queries.bin')==prior['query_sha256'],'Prior query receipt');raw=(PRIOR/'queries.bin').read_bytes();original=(PRIOR/'visibility_native_x86.bin').read_bytes();require(len(raw)==prior['cases']*124 and len(original)==prior['cases']*100,'Prior query extent')
    nativeQuery=Path(prior['executables'][0]['path']);require(sha(nativeQuery)==prior['executables'][0]['sha256'],'Native query executable changed')
    rows=[];indices=[];geometry=[];lookup={};rayForCase=[]
    for i in range(prior['cases']):
        row=raw[i*124:(i+1)*124];h=struct.unpack_from('<7I',row)
        if h[1]&4:continue
        if smoke and i>=1000 and i%257:continue
        rows.append(bytearray(row));indices.append(i);traceCalls=struct.unpack_from('<I',original,i*100+20)[0];require(traceCalls<=1,'Prior trace count')
        if traceCalls:
            q=original[i*100+52:i*100+76]+struct.pack('<3I',0x4481,1,1)
            if q not in lookup:lookup[q]=len(geometry);geometry.append(q)
            rayForCase.append(lookup[q])
        else:rayForCase.append(None)
    require(rows and geometry,'Missing BSP lighting cases')
    source=PREPARED.read_bytes();pos=0
    for i in range(43):power=struct.unpack_from('<I',source,pos)[0];cells=1<<power;pos+=60+16*(cells+1)**2+8*2*cells*cells
    prepared=run/'prepared.bin';prepared.write_bytes(source[:pos]);rays=run/'rays.bin';rays.write_bytes(b''.join(geometry));executables=[];logs=[];traces=[]
    def execute(path,args,label):
        p=subprocess.run([str(path),*map(str,args)],capture_output=True,text=True,timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' exit '+str(p.returncode)+' '+p.stdout+p.stderr);logs.append(p.stdout.strip())
    def record(path,machine):
        binary=path.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(binary[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture');executables.append(dict(path=str(path),sha256=sha(path),machine=hex(machine)))
    for name,machine in [('surface_native_x86',0x14c),('surface_owned_x64',0x8664)]:
        path=OUT/(name+'.exe');record(path,machine);target=run/(name+'.bin');execute(path,[MAP,prepared,rays,target]+([ENGINE] if machine==0x14c else []),name);traces.append(target.read_bytes());require(len(traces[-1])==len(geometry)*76,'Geometry output extent')
    require(traces[0]==traces[1],'Original vs owned geometry stages differ')
    for i,ray in enumerate(rayForCase):
        if ray is None:continue
        answer=traces[0][ray*76:(ray+1)*76];fraction=answer[:4];end=answer[56:68];surface=struct.unpack_from('<I',answer,72)[0];rows[i][48:52]=fraction;struct.pack_into('<I',rows[i],20,surface);rows[i][112:124]=end
    query=run/'queries.bin';query.write_bytes(b''.join(rows));nativeOut=run/'original-query.bin';record(nativeQuery,0x14c);execute(nativeQuery,[ENGINE,query,nativeOut],'original-query');owned=OUT/'bsp_lighting_owned_x64.exe';record(owned,0x8664);ownedOut=run/'actual-owned-bsp-query.bin';execute(owned,[MAP,query,ownedOut],'actual-owned-bsp-query');a=nativeOut.read_bytes();b=ownedOut.read_bytes();require(len(a)==len(rows)*100 and len(b)==len(a),'Query output extent')
    different=[i for i in range(len(rows)) if a[i*100:(i+1)*100]!=b[i*100:(i+1)*100]];details=[dict(case=i,prior=indices[i],native=struct.unpack_from('<25I',a,i*100),owned=struct.unpack_from('<25I',b,i*100)) for i in different[:6]];(run/'comparison.json').write_text(json.dumps(dict(differences=len(different),details=details),indent=2)+'\n');require(not different,'Actual BSP lighting differs: '+str(run/'comparison.json'))
    coverage=dict(trace_calls=0,sky_visible=0,sky_blocked=0,point_visible=0,point_zero=0,debug=0,nonzero_surface_flags=0)
    for i,row in enumerate(rows):
        h=struct.unpack_from('<7I',row);result=struct.unpack_from('<5f8I12f',a,i*100);coverage['trace_calls']+=result[5];coverage['debug']+=result[6];coverage['nonzero_surface_flags']+=bool(h[5]) and rayForCase[i] is not None
        if h[0]==3:coverage['sky_visible' if result[4]!=0 else 'sky_blocked']+=1
        elif h[0]!=5:coverage['point_visible' if result[4]!=0 else 'point_zero']+=1
    require(coverage['trace_calls']>0 and coverage['sky_visible']>0 and coverage['sky_blocked']>0,'BSP lighting branch coverage')
    require(sha(ENGINE)==DIGEST and sha(MAP)==MAP_SHA,'Original modified');receipt=dict(status='PASS',smoke=smoke,cases=len(rows),unique_geometry_rays=len(geometry),bytes_per_result=100,differences=0,coverage=coverage,executables=executables,logs=logs,query_sha256=sha(query),ray_sha256=sha(rays),prior_receipt=str(PRIOR/'results.json'),prior_receipt_sha256=sha(PRIOR/'results.json'),engine_sha256=DIGEST,map_sha256=MAP_SHA,source_sha256={n:sha(ROOT/'work'/n) for n in ['source_bsp_world_light_trace_candidate.h','source_bsp_trace_surface_flags_candidate.h','source_world_light_visibility_candidate.h','probe_bsp_world_light_query.cpp','verify_bsp_world_light_query.py']},limits=['Owned AMD64 query invokes actual owned BSP brush/displacement collision and resolves surface flags in-process; fixture trace answers are checked but never returned as computation','Original comparison uses two separately executed stages: original full world trace then original query with recorded trace results; not one reentrant original query/world call chain','World-only mode1 only; mode3 static-prop/entity traces fail explicitly, remain unimplemented','Runtime surface mapping is a controlled reversed bijection; original material registry/name/property binding not reconstructed','Styles, intensity/debug settings and light snapshots are controlled, not live DMX/render parity','No editor integration/build, original writes, app launch or dependency install'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['status','smoke','cases','unique_geometry_rays','differences','coverage']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
