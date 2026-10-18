"""Compare owned file->metadata->pose->support binding with actual original calls.

Original search supplies only direction and visit-cache input. This is not a
complete owned GJK or TraceBox implementation.
"""
import collections,hashlib,json,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/phy-point-support-binding-20261015'
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def take(data,at):
    require(at+4<=len(data),'Missing frame length');n=struct.unpack_from('<I',data,at)[0];require(at+4+n<=len(data),'Truncated frame');return data[at+4:at+4+n],at+4+n
def main():
    prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Prior reference status');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libraries=run/'libraries';libraries.mkdir()
    for e in prior['libraries']:
        source=Path(e['source']);require(sha(source)==e['sha256'],'Original library changed');shutil.copyfile(source,libraries/source.name)
    executables=[]
    for name,machine in [('binding_native_x86',0x14c),('binding_owned_x64',0x8664),('binding_safety_x64',0x8664)]:
        p=OUT/(name+'.exe');b=p.read_bytes();pe=struct.unpack_from('<I',b,60)[0];require(struct.unpack_from('<H',b,pe+4)[0]==machine and b'asInvoker' in b,'Architecture/manifest');executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    models=[];modes=collections.Counter();total=0
    for index,m in enumerate(prior['models']):
        case=run/('%03d'%index);case.mkdir();queries=Path(m['path'])/'queries.bin';old=Path(m['path'])/'native-traces.bin';require(sha(Path(m['phy']))==m['phy_sha256'] and sha(queries)==m['query_sha256'] and sha(old)==m['trace_sha256'],'Prior asset/query/trace changed');trace=case/'traces.bin';capture=case/'captures.bin';owned=case/'owned.bin'
        p=subprocess.run([str(OUT/'binding_native_x86.exe'),str(libraries),m['phy'],str(queries),str(trace),str(capture)],capture_output=True,text=True,timeout=45,cwd=case);(case/'native.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Native capture failed '+str(case)+' '+p.stdout+p.stderr);require(trace.read_bytes()==old.read_bytes(),'Observation changed original trace')
        p=subprocess.run([str(OUT/'binding_owned_x64.exe'),m['phy'],str(queries),str(capture),str(owned)],capture_output=True,text=True,timeout=45,cwd=case);(case/'owned.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Owned binding failed '+str(case)+' '+p.stdout+p.stderr)
        native=capture.read_bytes();rebuilt=owned.read_bytes();at=bt=0;calls=0
        while at<len(native):
            require(at+8<=len(native),'Truncated query/ledge');query,ledge=struct.unpack_from('<II',native,at);at+=8;require(query<m['queries'],'Query index');originalInput,at=take(native,at);originalOutput,at=take(native,at);ownedInput,bt=take(rebuilt,bt);ownedOutput,bt=take(rebuilt,bt)
            if originalInput!=ownedInput or originalOutput!=ownedOutput:
                mismatch=dict(query=query,ledge=ledge,call=calls,mode=struct.unpack_from('<I',originalInput)[0],original_input=originalInput.hex(),owned_input=ownedInput.hex(),original_output=originalOutput.hex(),owned_output=ownedOutput.hex());(case/'difference.json').write_text(json.dumps(mismatch,indent=2)+'\n');raise RuntimeError('Binding mismatch '+str(case/'difference.json'))
            modes[struct.unpack_from('<I',ownedInput)[0]]+=1;calls+=1
        require(bt==len(rebuilt),'Trailing owned output');total+=calls;models.append(dict(model=m['model'],path=str(case),queries=m['queries'],support_calls=calls,captures_sha256=sha(capture),owned_sha256=sha(owned),trace_sha256=sha(trace)));print('PASS bound',index+1,'calls='+str(calls),flush=True)
    require(total>0,'No support calls');safety=subprocess.run([str(OUT/'binding_safety_x64.exe')],capture_output=True,text=True,timeout=15);require(safety.returncode==0,'Safety '+safety.stdout+safety.stderr)
    for e in prior['libraries']:require(sha(Path(e['source']))==e['sha256'] and sha(libraries/Path(e['source']).name)==e['sha256'],'Original/copy DLL changed')
    for m in prior['models']:
        for field in ['phy','mdl']:require(sha(Path(m[field]))==m[field+'_sha256'],'Original asset changed')
    from verify_ifm_draw_packet import check_ranges
    image,evidence=check_ranges(Path(prior['libraries'][1]['source']),'vphysics-disassembly.txt',[('complete_original_point_shape_constructor',0x1002a120,0x1002a331),('observed_ledge_binding_call',0x1002a375,0x1002a37a)])
    require(image.word(0x100c53e4)==0xbf800000 and image.word(0x100ee00c)==struct.unpack('<I',struct.pack('<f',39.370079040527344))[0],'Point transform constants')
    sources=['source_phy_point_support_binding_candidate.h','probe_native_phy_point_support.cpp','probe_owned_phy_point_support.cpp','test_phy_point_support_binding.cpp','build_phy_point_support_binding.cmd','verify_phy_point_support_binding.py','source_phy_compact_geometry_candidate.h','source_physics_support_metadata_candidate.h','source_physics_ray_preparation_candidate.h','source_physics_support_preparation_candidate.h','source_physics_support_candidate.h','physics_support_test_fixture.h','native_physics_support_fixture.h','native_vphysics_library_fixture.h','x64_fsin_cos.asm','verify_ifm_draw_packet.py']
    result=dict(status='PASS',models=models,cases=total,differences=0,modes=dict(modes),safety=safety.stdout+safety.stderr,original_trace_cases=prior['totals']['traces'],all_original_trace_bytes_unchanged=True,executables=executables,libraries=prior['libraries'],current_library_copies=str(libraries),prior_receipt=str(PRIOR),prior_receipt_sha256=sha(PRIOR),source_sha256={name:sha(ROOT/'work'/name) for name in sources},limits=['Owned context and result byte-equal to actual original support calls: owned PHY extraction, metadata generation, pose preparation, packed preparation/edge-walk and selection composed','Only search direction and visit-cache input copied from original capture; full iterative GJK, cache lifetime and hit reporting remain unimplemented','Native library is an isolated x86 test oracle only; owned AMD64 program loads no original DLL','Only point rays/zero extents and available benchmark loose PHY files tested; hull rays, other formats and missing archive assets unverified','Original full traces checked unchanged by observation, not generated by owned tracer','No editor integration/build, original writes, renderer changes or dependency installation'])
    result['static_evidence']=evidence
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({key:result[key] for key in ['status','cases','differences','modes','safety','original_trace_cases']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
