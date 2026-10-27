"""Owned per-model context snapshot with original registration/name evidence.

Not a full original CreatePuppets execution or material/renderer integration.
"""
import hashlib,json,random,re,shutil,struct,subprocess,tempfile,time
from datetime import datetime,timezone
from pathlib import Path
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
from verify_session_illumination_dag import IFM,IFM_SHA,SESSION,SESSION_SHA
ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/session-model-proxy-context-20261027'
NEW=['session_model_proxy_context_candidate.h','probe_session_model_proxy_context.cpp','build_session_model_proxy_context.cmd','verify_session_model_proxy_context.py']
CHANGED='work/session_render_scene_candidate.h'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def need(b,s):
    if not b:raise RuntimeError(s)

def main():
    baseline=json.loads((OUT/'before.local.json').read_text());pointer=baseline['parent'];parent=Path(pointer['receipt'])
    need(sha(parent)==pointer['receipt_sha256']=='820c7696785433ea06406c4a622c1b3307f6ce29395806a2127d66f11a35926b','Accepted routing receipt')
    prior=json.loads(parent.read_text());frozen=Path(pointer['replay']);need(sha(frozen/'manifest.json')==pointer['replay_manifest_sha256'],'Prior manifest')
    need(all(sha(frozen/n)==h for n,h in json.loads((frozen/'manifest.json').read_text()).items()),'Complete preceding frozen evidence')
    unchanged={n:h for n,h in baseline['sources'].items() if Path(n)!=ROOT/CHANGED};need(len(unchanged)==276 and all(sha(n)==h for n,h in unchanged.items()),'Unchanged preceding source scope')
    need(sha(OUT/'before'/CHANGED)==baseline['sources'][str(ROOT/CHANGED)],'Preserved previous scene header')
    need(sha(IFM)==IFM_SHA and sha(SESSION)==SESSION_SHA,'Read-only benchmark and IFM identities')
    protected=[prior[k] for k in ['original_hashes','private_library_hashes','benchmark_material_hashes','fixture_hashes']]
    for group in protected:need(all(sha(n)==h for n,h in group.items()),'Protected preflight')
    need(all(sha(e['path'])==e['sha256'] for e in baseline['executables']),'42 preceding executables unchanged')
    sources=source_closure([Path(n) for n in baseline['sources']]+[ROOT/'work'/n for n in NEW])
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
    image,ranges=check_ranges(IFM,'ifm-disassembly.txt',[
        ('game_model_name_registration_initializer',0x10a6e3b0,0x10a6e3c7),
        ('factory_registration_link_constructor',0x1038edb0,0x1038ede4),
        ('game_model_symbol_setter',0x105c4980,0x105c498f),
        ('game_sprite_symbol_setter',0x105c49e0,0x105c49ef),
        ('create_puppets_type_name_gate_and_construction',0x102abd2b,0x102abdd5),
        ('original_byte_string_compare',0x10975120,0x10975157),
        ('puppet_vtable_constructor_prefix',0x10243220,0x10243241),
        ('puppet_game_entity_null',0x1022b000,0x1022b003)])
    factory=image.word(0x10eb52d8);need(factory==0x10b94738 and image.word(factory+20)==0x105c4980,'Actual model factory setter')
    locator=image.word(factory-4);need(image.string(image.word(locator+12)+8)=='.?AV?$CDmElementFactory@VCDmeGameModel@@@@','Model factory RTTI')
    sprite=image.word(0x10eb52f8);loc=image.word(sprite-4);need(image.string(image.word(loc+12)+8)=='.?AV?$CDmElementFactory@VCDmeGameSprite@@@@' and image.word(sprite+20)==0x105c49e0,'Separate sprite identity')
    need(image.string(0x10b948cc)=='DmeGameModel' and image.string(0x10abd738)=='modelName' and image.string(0x10ac210c)=='?' and image.string(0x10adba88)=='effects','Exact name gates')
    started=time.time();b=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_session_model_proxy_context.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(b.stdout+b.stderr);need(b.returncode==0,'Fresh context builds')
    executables=[];buildlogs={};warnings={}
    for kind,machine,magic in [('reference_x86',0x14c,0x10b),('owned_x64',0x8664,0x20b)]:
        exe=OUT/('model_context_'+kind+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic and b'asInvoker' in raw,'Fresh architecture and privilege')
        log=OUT/('reference-build.log' if machine==0x14c else 'owned-build.log');text=log.read_text();need(not re.search(r'\berror\s+[A-Z]+\d+\s*:',text,re.I),'Build errors');warnings[kind]=re.findall(r'warning (C\d+): ([^\r\n]+)',text)
        need(all(code=='C4458' for code,_ in warnings[kind]),'Only known graph hierarchy warning');buildlogs[str(log)]=sha(log)
        imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);need(not any(x.lower() in ['ifm.dll','client.dll','engine.dll','studiorender.dll','materialsystem.dll'] for x in modules),'No original DLL import dependency')
        executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=modules))
    def execute(exe,args,label,timeout=120):
        p=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=timeout);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed; inspect retained log');return p.stdout
    names=[b'models/a.mdl',b'?',b'',b'*1',b'??',b' ?',b'? ',b'models/a.MDL',b'*bad',b'a*',b'models/'+b'a'*4089]
    rng=random.Random(2026102718)
    for _ in range(1024):names.append(bytes(rng.choice(b'abcXYZ012 /\\.*?_-') for _ in range(rng.randrange(65))))
    query=run/'query.bin';query.write_bytes(struct.pack('<I',len(names))+b''.join(struct.pack('<I',len(n))+n for n in names));known=sum(bool(n) and n!=b'?' and n[0]!=42 for n in names)
    expected=struct.pack('<I',len(names))+b''.join(struct.pack('<4I',i,int(bool(n) and n!=b'?' and n[0]!=42),int(bool(n) and n!=b'?' and n[0]!=42),0 if bool(n) and n!=b'?' and n[0]!=42 else 0xffffffff) for i,n in enumerate(names))
    need(query.read_bytes()[:4+sum(4+len(n) for n in names[:4])][4:]==(OUT/'explore-query.bin').read_bytes()[4:],'Original four-name prefix retained')
    for kind,library in [('reference_x86',IFM),('owned_x64',run/'absent-original.dll')]:
        output=run/(kind+'.bin');trace=execute(OUT/('model_context_'+kind+'.exe'),[library,query,output],kind)
        need(trace==f'PASS model_context_cases={len(names)} ordinary_puppet={known}\n' and output.read_bytes()==expected,'Complete independently expected admission/context trace')
    safety=execute(OUT/'model_context_owned_x64.exe',['--safety'],'safety');need(safety=='PASS model_context_safety_checks=20\n','Value ownership and malformed-source checks')
    benchmark=execute(OUT/'model_context_owned_x64.exe',['--benchmark',SESSION],'benchmark');match=re.search(r'PASS benchmark_context_frames=(\d+) model_snapshots=(\d+) unknown_snapshots=(\d+)',benchmark);need(match and int(match[1])==3 and int(match[2])>int(match[3])>0,'Actual benchmark ordinary/exceptional source snapshots')
    # Full editor build with unchanged settings into an isolated directory.
    # Only root/output locations differ from the authoritative build script.
    editor_dir=run/'editor';editor_dir.mkdir();script=ROOT/'work/build_sfm64_editor_x64.cmd';original=script.read_text();custom=original.replace('cd /d "%~dp0.."','cd /d "'+str(ROOT)+'"').replace('set "OUT=%CD%\\outputs\\sfm64-editor-x64"','set "OUT='+str(editor_dir)+'"');need(custom!=original and custom.count(str(editor_dir))==1,'Isolated build location substitutions')
    editor_script=run/'build-editor-isolated.cmd';editor_script.write_text(custom)
    built=subprocess.run(['cmd.exe','/d','/c',str(editor_script)],cwd=ROOT,capture_output=True,text=True,timeout=240);(run/'editor-build-driver.log').write_text(built.stdout+built.stderr);need(built.returncode==0,'Complete isolated editor build')
    editor=editor_dir/'sfm64_session_ui.exe';raw=editor.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(struct.unpack_from('<H',raw,pe+4)[0]==0x8664 and struct.unpack_from('<H',raw,pe+24)[0]==0x20b,'Full editor AMD64 PE32+')
    editor_warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(editor_dir/'build.log').read_text());need(sha(script)==baseline['preserved']['work/build_sfm64_editor_x64.cmd'],'Authoritative build script unchanged')
    need(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==baseline['editor_sha256'],'Previous editor executable preserved')
    # Reuse the accepted complete owned weapon value proof; no DLL can be read.
    owned=Path(prior['prior_owned_executable']['path']);context_parent=json.loads(Path(prior['prior_receipt']).read_text());weapon_receipt=Path(context_parent['prior_receipt']);weapon=json.loads(weapon_receipt.read_text());value_output=run/'weapon-owned.bin';execute(owned,[run/'absent-original.dll',weapon_receipt.parent/'query.bin',value_output],'prior-weapon');need(sha(value_output)==weapon['output_sha256'],'All15664 previous owned weapon cases')
    for group in [sources,*protected]:need(all(sha(n)==h for n,h in group.items()),'Final protected identity')
    need(all(sha(e['path'])==e['sha256'] for e in baseline['executables']) and not (run/'absent-original.dll').exists(),'Previous binary and absent-DLL identities')
    result=dict(status='PASS',goal_status='ACTIVE / INCOMPLETE',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',sources=sources,new_sources=NEW,changed_existing_source=CHANGED,previous_sources_unchanged=len(unchanged),previous_executables_unchanged=42,original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],benchmark_material_hashes=prior['benchmark_material_hashes'],fixture_hashes=prior['fixture_hashes'],executables=executables,reused_executables=baseline['executables'],build_logs=buildlogs,compiler_warnings=warnings,static_ifm_ranges=ranges,admission_cases=len(names),ordinary_cases=known,output_sha256=hashlib.sha256(expected).hexdigest(),query_sha256=sha(query),safety_checks=20,benchmark_frames=int(match[1]),benchmark_model_snapshots=int(match[2]),benchmark_unknown_snapshots=int(match[3]),benchmark_log_sha256=sha(run/'benchmark.log'),prior_owned_weapon_cases=15664,prior_owned_executable=prior['prior_owned_executable'],prior_owned_output_sha256=sha(value_output),prior_receipt=str(parent),prior_receipt_sha256=sha(parent),editor_build=dict(path=str(editor),sha256=sha(editor),architecture='AMD64 PE32+',warnings=editor_warnings,authoritative_script_sha256=sha(script),location_only_adaptation=str(editor_script),runtime_test='NOT RUN'),previous_editor_sha256=baseline['editor_sha256'],limits=['Native child runs complete type-registration initializer, symbol setter, string comparator and puppet null-entity accessor; complete CreatePuppets/puppet constructor and engine registration services are static-only/unexecuted','Owned source classification deliberately covers exact DmeGameModel ordinary model-name path only. Sprite, empty/question/inline names, effects, derived/unknown types and malformed strings stay unknown; this is not native rejection of those exceptional models','Snapshots own source model ID/GUID/asset, not game entities, pointers or live configuration; an old snapshot belongs to its captured frame, and callers must match its identity before later use','Context metadata now flows with real SessionRenderModelCandidate values, but no material proxy registration/evaluation/cache or transaction behavior changes; whole-material and fullpilot admission unchanged','Editor freshly compiled in isolated output with existing settings and actual source; editor runtime/GPU/visual equivalence not tested; published/default build preserved','Original files read-only; no live SFM or CUA; no original instruction patching or instruction arrays'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');replay=OUT/('replay-'+run.name.removeprefix('verification-'));replay.mkdir()
    included=[Path(n) for n in sources]+[Path(e['path']) for e in executables]+[Path(n) for n in buildlogs]+[receipt,query,editor,editor_script,editor_dir/'build.log']+list(run.glob('*.bin'))+list(run.glob('*.log'))
    for p in included:
        d=replay/p.relative_to(ROOT);d.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,d)
    manifest={p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()};(replay/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt),replay=str(replay),replay_manifest_sha256=sha(replay/'manifest.json')),indent=2)+'\n')
    print(json.dumps(dict(status='PASS',cases=len(names),known=known,safety_checks=20,benchmark_frames=int(match[1]),benchmark_models=int(match[2]),benchmark_unknown=int(match[3]),editor_sha256=sha(editor),editor_warnings=len(editor_warnings))),flush=True)

if __name__=='__main__':main()
