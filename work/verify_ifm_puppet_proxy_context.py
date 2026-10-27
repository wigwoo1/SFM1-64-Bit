"""Measure original IFM puppet interfaces and trace weapon limit provenance.

No live SFM, editor changes, original writes or fabricated production context.
Original IFM and client run separately at their common preferred image base.
The resulting composition is a prerequisite, not a combined native draw test.
"""
import hashlib,json,re,shutil,struct,subprocess,tempfile,time
from datetime import datetime,timezone
from pathlib import Path
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
from verify_session_illumination_dag import IFM,IFM_SHA

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/ifm-puppet-proxy-context-20261027'
PARENT=ROOT/'outputs/material-weapon-invis-contract-20261027/latest-verification.json'
NEW=['probe_ifm_puppet_proxy_context.cpp','build_ifm_puppet_proxy_context.cmd','verify_ifm_puppet_proxy_context.py']
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def need(ok,message):
    if not ok:raise RuntimeError(message)

def main():
    OUT.mkdir(exist_ok=True)
    pointer=json.loads(PARENT.read_text());parent=Path(pointer['receipt'])
    need(sha(parent)==pointer['receipt_sha256']=='cf9fb27d3a36c48b3f72dfd58feafca1968f835f93337b4d00fb8c8161cd1edb','Accepted weapon receipt')
    prior=json.loads(parent.read_text());frozen=Path(pointer['replay'])
    need(sha(frozen/'manifest.json')==pointer['replay_manifest_sha256'],'Accepted frozen manifest')
    need(all(sha(frozen/n)==h for n,h in json.loads((frozen/'manifest.json').read_text()).items()),'All previous frozen files')
    protected=[prior[k] for k in ['sources','original_hashes','private_library_hashes','benchmark_material_hashes','fixture_hashes']]
    for group in protected:need(all(sha(n)==h for n,h in group.items()),'Protected inputs before')
    need(sha(IFM)==IFM_SHA,'Pinned IFM input')
    previous_exes=prior['executables']+prior['reused_executables']
    need(all(sha(e['path'])==e['sha256'] for e in previous_exes),'All 38 prior executable identities')
    sources=source_closure([Path(n) for n in prior['sources']]+[ROOT/'work'/n for n in NEW]+[ROOT/'work'/n for n in ['ifm_model_draw_packet_candidate.h','session_render_scene_candidate.h','scene_material_inputs_candidate.h','scene_material_draw_inputs_candidate.h']])
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
    ifm,ifm_ranges=check_ranges(IFM,'ifm-disassembly.txt',[
        ('puppet_constructor_vtable_prefix_static',0x10243220,0x10243241),
        ('renderable_to_unknown',0x10242660,0x10242664),
        ('unknown_to_renderable',0x10242670,0x1024267b),
        ('unknown_to_base_entity',0x1022b000,0x1022b003),
        ('draw_packet_renderable_assignment_static',0x10243a8a,0x10243aa9),
        ('puppet_allocation_and_construction_static',0x102abdb0,0x102abdd5)])
    client=next(Path(n) for n in prior['private_library_hashes'] if Path(n).name.lower()=='client.dll')
    ci,client_ranges=check_ranges(client,'../outputs/material-proxy-evaluation-20261027/client-disassembly-private.txt',[
        ('limit_static_initializer',0x1049d8f0,0x1049d915),
        ('limit_constructor_wrapper',0x103c6fd0,0x103c7037),
        ('convar_create_parent_default_cache_and_registration',0x103c6ce0,0x103c6e54),
        ('limit_destructor_thunk',0x104ac100,0x104ac10a),
        ('cloak_value_getter',0x10226450,0x102264fb)])
    need(ifm.word(0x10adb904)==0x10242660 and ifm.word(0x10adb9b8+5*4)==0x10242670 and ifm.word(0x10adb9b8+7*4)==0x1022b000,'Actual puppet interface table slots')
    rtti=[]
    for table in [0x10adb9b8,0x10adb904]:
        locator=ifm.word(table-4);descriptor=ifm.word(locator+12);name=ifm.string(descriptor+8)
        need(name=='.?AVCDmePuppet@@','Actual RTTI class identity')
        rtti.append(dict(vtable=hex(table),locator=hex(locator),descriptor=hex(descriptor),name=name))
    need(ci.string(0x105362c4)=='tf_teammate_max_invis' and ci.string(0x1051e284)=='0.95','Exact declared configuration name/default')
    # Recover operands from checked instruction records rather than assuming
    # that an isolated float/string occurrence identifies a configuration.
    instructions={x['address']:x for rg in client_ranges for x in rg['instructions']}
    operand=lambda a:struct.unpack('<I',bytes.fromhex(instructions[a]['bytes'])[-4:])[0]
    need(operand(0x1049d8f0)==0x4002 and operand(0x1049d8f5)==0x1051e284 and operand(0x1049d8fa)==0x105362c4 and operand(0x1049d8ff)==0x1079d508,'Initializer flags/default/name/receiver operands')
    need(instructions[0x103c6ce9]['instruction']=='mov         dword ptr [esi+1Ch],esi','Constructor stores self-parent')
    need(0x1079d508+0x1c==0x1079d524,'Getter cell is parent field, not a separate global configuration pointer')
    need(instructions[0x103c6dc2]['instruction']=='fst         dword ptr [esi+2Ch]','Create initializes float cache')
    configuration=dict(name='tf_teammate_max_invis',declared_default='0.95',declared_default_as_f32=hex(struct.unpack('<I',struct.pack('<f',.95))[0]),flags_raw=hex(0x4002),object_address=hex(0x1079d508),parent_cell_address=hex(0x1079d524),parent_offset=hex(0x1c),float_cache_offset=hex(0x2c),declaration_and_layout='VERIFIED static original instructions/data',active_runtime_value='UNVERIFIED; declaration is not an observed runtime setting',registration_aliases_and_overrides='UNVERIFIED')
    (run/'configuration-provenance.json').write_text(json.dumps(configuration,indent=2)+'\n')
    start=time.time();built=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_ifm_puppet_proxy_context.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180)
    (run/'build-driver.log').write_text(built.stdout+built.stderr);need(built.returncode==0,'Fresh IFM interface probe build')
    exe=OUT/'puppet_context_reference_x86.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
    need(exe.stat().st_mtime>=start-2 and raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==0x14c and struct.unpack_from('<H',raw,pe+24)[0]==0x10b and b'asInvoker' in raw,'Fresh x86 reference-only executable')
    imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout
    modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.I|re.M)
    need(modules==['KERNEL32.dll'],'Only recorded direct system imports')
    log=OUT/'build.log';need(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Clean compiler/linker log')
    def execute(executable,args,label):
        result=subprocess.run([str(executable),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=120)
        (run/(label+'.log')).write_text(result.stdout+result.stderr)
        need(result.returncode==0,label+' failed; inspect retained log')
        return result.stdout
    output=run/'ifm-interfaces.bin';trace=execute(exe,[IFM,output],'ifm-interfaces')
    expected=b''.join(struct.pack('<5I',i,1,1,1,1) for i in range(32))
    need(output.read_bytes()==expected and trace=='PASS puppet_interface_cases=32 base_entity_null=32 unchanged_storage=32\n','All actual original IFM interface observations')
    native=Path(prior['executables'][0]['path']);owned=Path(prior['executables'][1]['path'])
    query=parent.parent/'query.bin';old_native=run/'weapon-native.bin';old_owned=run/'weapon-owned.bin';services=run/'weapon-services.bin'
    execute(native,[client,query,old_native,services],'weapon-native')
    execute(owned,[run/'absent-original.dll',query,old_owned],'weapon-owned')
    need(sha(old_native)==sha(old_owned)==prior['output_sha256'] and sha(services)==prior['services_sha256'],'Complete preceding 15664-case dependency/value replay')
    rows=list(struct.iter_unpack('<11I',query.read_bytes()))
    # These existing controlled client cases use the now-measured IFM result:
    # found output, renderable present, null resolved entity => no writes.
    selected=[i for i,q in enumerate(rows) if q[0]&3==3 and not q[0]&4]
    need(selected,'Null-entity cases present in the complete retained corpus')
    at=0;states=[];blob=old_native.read_bytes()
    while at<len(blob):
        h=struct.unpack_from('<5I',blob,at);at+=20
        size=struct.unpack_from('<I',blob,at)[0];at+=4+size
        need(struct.unpack_from('<I',blob,at)[0]==1,'One output variable');at+=4
        v=struct.unpack_from('<28I',blob,at);at+=112+v[10]+v[11];states.append((h,v))
    need(at==len(blob) and len(states)==15664,'Complete value replay framing')
    for i in selected:
        h,v=states[i];q=rows[i]
        need(h== (i,1,1,0,0) and v[1]==q[1] and v[3]==q[10],'Puppet-compatible null-entity path preserves material value')
    (run/'null-entity-case-indices.json').write_text(json.dumps(selected)+'\n')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());graph=set()
    def visit(name):
        if name in graph:return
        graph.add(name);p=ROOT/name
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            child=(p.parent/inc).relative_to(ROOT).as_posix()
            if (ROOT/child).exists():visit(child)
    visit('work/probe_vulkan_win32_surface_x64.cpp')
    need(graph==set(build['editor_source_dependencies']) and len(graph)==310 and all(sha(ROOT/n)==h for n,h in build['editor_source_dependencies'].items()),'Exact editor graph unchanged')
    need(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256']==prior['editor_sha256'],'Editor executable unchanged')
    for group in [sources,*protected]:need(all(sha(n)==h for n,h in group.items()),'Final protected identities')
    need(sha(IFM)==IFM_SHA and all(sha(e['path'])==e['sha256'] for e in previous_exes),'Original IFM and previous binaries unchanged')
    need(not (run/'absent-original.dll').exists(),'Owned replay absent-original argument')
    result=dict(status='PASS',goal_status='ACTIVE / INCOMPLETE',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',scope='Original CDmePuppet renderable/unknown/base-entity interface measurements plus exact static configuration provenance and separate client/x64 value replay; not combined original draw execution',configuration=configuration,puppet_rtti=rtti,ifm_interface_cases=32,interface_output_sha256=sha(output),weapon_regression_cases=15664,null_entity_preservation_cases=len(selected),prior_output_sha256=prior['output_sha256'],sources=sources,new_sources=NEW,prior_sources_unchanged=len(prior['sources']),original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],benchmark_material_hashes=prior['benchmark_material_hashes'],fixture_hashes=prior['fixture_hashes'],ifm_path=str(IFM),ifm_sha256=IFM_SHA,executables=[dict(path=str(exe),sha256=sha(exe),machine='0x14c',direct_imports=modules)],reused_executables=previous_exes,tested_owned_executable=prior['executables'][1],build_log_sha256=sha(log),prior_receipt=str(parent),prior_receipt_sha256=sha(parent),static_ifm_ranges=ifm_ranges,static_client_ranges=client_ranges,editor_dependencies_unchanged=310,editor_sha256=prior['editor_sha256'],editor_rebuilt=False,editor_runtime_test='NOT RUN',limits=['Only the verified CDmePuppet vtable path establishes null entity; do not assume every Source renderable has no entity','The IFM child uses a controlled storage shell with actual vtables and executes actual accessor methods, not the full constructor/DllMain/draw pipeline','IFM and client execute in separate children because their preferred bases coincide; this is a composition prerequisite, not a combined engine/render test','The 0.95 setting is a declared default, not an observed active value; registration aliases, updates and override lifetimes remain unverified','No new owned context API, live weapon registration, editor/renderer integration or full pilot acceptance; existing generic renderable handle is not proof of puppet provenance','Originals read-only, no live SFM control, no executable instruction patches'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n')
    replay=OUT/('replay-'+run.name.removeprefix('verification-'));replay.mkdir()
    included=[Path(n) for n in sources]+[exe,log,receipt]+list(run.glob('*.log'))+list(run.glob('*.json'))+[output]
    for p in included:
        dest=replay/p.relative_to(ROOT);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,dest)
    manifest={p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()}
    (replay/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    (OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt),replay=str(replay),replay_manifest_sha256=sha(replay/'manifest.json')),indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ['status','ifm_interface_cases','weapon_regression_cases','null_entity_preservation_cases','editor_rebuilt']}),flush=True)

if __name__=='__main__':main()
