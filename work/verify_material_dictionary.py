"""Original material dictionary/reference deletion; never controls live SFM."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,shutil,struct,subprocess,tempfile,time
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_material_keyvalues_parser import DUMP
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
from pe_image import Image

OUT=ROOT/'outputs/material-dictionary-20261026'
NEW=['probe_material_dictionary.cpp','build_material_dictionary.cmd','verify_material_dictionary.py']


def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    ptr=json.loads((ROOT/'outputs/material-object-lifetime-20261026/latest-verification.json').read_text());receipt=Path(ptr['receipt'])
    require(sha(receipt)==ptr['receipt_sha256'],'Prior lifetime receipt');prior=json.loads(receipt.read_text())
    for p,h in prior['sources'].items():require(sha(Path(p))==h,'Accepted source '+p)
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Protected original '+p)
    for p,h in prior['private_library_hashes'].items():require(sha(Path(p))==h,'Private original '+p)
    require(sha(LIB)==sha(PRIVATE)==PIN,'Material PE identity')
    replay=Path(ptr['replay']);require(sha(replay/'manifest.json')==ptr['replay_manifest_sha256'] and all(sha(replay/k)==h for k,h in json.loads((replay/'manifest.json').read_text()).items()),'Prior frozen source/evidence')
    sources=source_closure([Path(p) for p in prior['sources']]+[ROOT/'work'/n for n in NEW])
    image=Image(PRIVATE);slots={str(i):hex(image.word(0x100984cc+4*i)) for i in [76,106,107,137,138,142,145]}
    require(slots=={'76':'0x1000b790','106':'0x1000ede0','107':'0x1000e5d0','137':'0x10016460','138':'0x10016470','142':'0x1000b3f0','145':'0x1000bfd0'},'Actual material-system method identities')
    require(image.word(0x10095d74+51*4)==0x10005e10 and image.word(0x10095a9c+51*4)==0x100078a0,'Raw and forwarding deletion methods differ')
    _,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('actual_two_tree_dictionary_ctor',0x10012910,0x10012997),('primary_tree_purge',0x10012840,0x100128a4),('secondary_tree_purge',0x100128b0,0x10012910),
        ('dictionary_comparators_and_tree_primitives',0x10007c60,0x10009a12),('cache_insert',0x10009a20,0x10009abc),
        ('cache_remove_pointer_only',0x10009ac0,0x10009b4a),('cache_remove_then_delete',0x10009b80,0x10009b9b),
        ('system_insert_adapter',0x10016460,0x1001646f),('system_remove_delete_adapter',0x10016470,0x1001647f),
        ('primary_tree_find',0x10010480,0x10010576),('procedural_create_or_reuse',0x10011860,0x10011946),
        ('raw_delete_if_unreferenced',0x10005e10,0x10005e41),('deletion_guard_flag',0x10017540,0x1001754d),
        ('delete_helper_nested_context',0x100029b0,0x100029ed),
        ('original_system_acquire_boundary_not_executed',0x1000ede0,0x1000eebe),('original_system_release_boundary_not_executed',0x1000e5d0,0x1000e656)])
    started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_dictionary.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=120)
    (run/'build-driver.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Fresh dictionary reference build')
    exe=OUT/'dictionary_reference_x86.exe';buildlog=OUT/'reference-build.log';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
    require(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==0x14c and struct.unpack_from('<H',raw,pe+24)[0]==0x10b and b'asInvoker' in raw,'Fresh I386 reference architecture/manifest')
    require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',buildlog.read_text(),re.I),'Clean reference build log')
    imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I)
    require({s.lower() for s in modules}=={'kernel32.dll'},'Direct imports')
    mount=run/'empty-mount';mount.mkdir();libs=ROOT/'outputs/material-fallback-owned-20261026/verification-0mhr77s9/copied-libraries';runs=[]
    for repeat in range(2):
        result=run/('records-'+str(repeat)+'.txt');p=subprocess.run([str(exe),str(PRIVATE),str(libs),str(mount),str(result)],cwd=run,capture_output=True,text=True,timeout=60)
        (run/('reference-'+str(repeat)+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,'Dictionary execution '+str(repeat)+' exit='+str(p.returncode)+'; inspect log')
        counts=re.search(r'dictionary_rounds=(\d+) created=(\d+) removed=(\d+) duplicate_hits=(\d+) independent_checks=(\d+) allocations=(\d+) releases=(\d+)',p.stdout)
        require(counts and list(map(int,counts.groups()[:4]))==[4,1160,1160,20] and int(counts[5])>=10765 and counts[6]==counts[7],'Runtime operation/check/release accounting')
        rows=[line.split() for line in result.read_text().splitlines()];require(len(rows)==2324,'All operation records')
        for round in range(4):
            group=[row for row in rows if int(row[0])==round];insert=[row for row in group if row[1]=='I'];hits=[row for row in group if row[1]=='H'];procedural=[row for row in group if row[1]=='P'];erase=[row for row in group if row[1]=='D']
            require(len(insert)==len(erase)==288 and len(hits)==4 and len(procedural)==1,'All per-round operation groups')
            require([int(x[4]) for x in insert]==list(range(1,289)) and [int(x[4]) for x in erase]==list(range(287,-1,-1)),'Independent exact count transitions')
            require({int(x[3]) for x in insert}=={int(x[3]) for x in erase} and len({int(x[3]) for x in insert})==288,'Every constructed cached identity removed once')
            require(all(int(x[4])==288 for x in hits) and int(procedural[0][3])==288,'Duplicate sources do not replace membership')
        runs.append(dict(rounds=int(counts[1]),created=int(counts[2]),removed=int(counts[3]),duplicate_hits=int(counts[4]),checks=int(counts[5]),allocations=int(counts[6]),releases=int(counts[7]),records=len(rows),records_sha256=sha(result)))
    require(runs[0]==runs[1],'Repeatable records and allocation accounting')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());graph=set()
    def visit(p):
        k=p.relative_to(ROOT).as_posix()
        if k in graph:return
        graph.add(k)
        for name in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            q=p.parent/name
            if q.is_file():visit(q.resolve())
    visit((ROOT/'work/probe_vulkan_win32_surface_x64.cpp').resolve())
    require(graph==set(build['editor_source_dependencies']) and all(sha(ROOT/k)==h for k,h in build['editor_source_dependencies'].items()),'Whole editor graph unchanged')
    require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor executable unchanged')
    for p,h in sources.items():require(sha(Path(p))==h,'Source drift '+p)
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Original changed '+p)
    for p,h in prior['private_library_hashes'].items():require(sha(Path(p))==h,'Private original changed '+p)
    limits=['Original-reference proof only: owned x64 material-object/dictionary lifetime and editor integration remain absent',
        'Actual dictionary constructor, both original comparator identities, primary tree insert/find/traverse/erase/growth/cleanup, system137/138 adapters, real object construction/destruction/source ownership, raw slot51 deletion and procedural create-or-reuse execute with unchanged RX code',
        'Secondary tree is initialized and purged but no secondary-tree insertion/lookup/resource behavior is claimed',
        'External system106/107 context/lock tokens use explicit bounded nested callbacks, justified by original recursive lock counter; original TLS/hardware/GPU lock/context bodies are static evidence only. Current-binding invalidation/mode, heap/KV symbol/lock services remain controlled fixtures',
        'No queued forwarding teardown, real worker safety, pool growth/failure injection, initialized shader/texture/proxy/resource destruction, general FindMaterial behavior or renderer fidelity',
        'No original writes, live SFM access, GPU/Blender/editor build/run, or broader previous-test rerun; all accepted source/editor identities retained']
    result=dict(status='PASS_REFERENCE_ONLY',utc=datetime.now(timezone.utc).isoformat(),scope='Original material dictionary and reference-gated deletion',previous_goal_turn='progress',runs=runs,executables=[dict(path=str(exe),sha256=sha(exe),machine='I386 PE32 reference',direct_imports=modules)],build_logs={str(buildlog):sha(buildlog)},sources=sources,original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],static_ranges=ranges,system_slots=slots,prior_receipt=str(receipt),prior_receipt_sha256=sha(receipt),editor_dependencies_unchanged=len(graph),editor_sha256=build['executable_sha256'],limits=limits)
    dest=run/'results.json';dest.write_text(json.dumps(result,indent=2)+'\n');replay=OUT/('replay-'+run.name.removeprefix('verification-'));replay.mkdir()
    for p in [Path(p) for p in sources]+[exe,buildlog,dest]+list(run.glob('*.txt'))+list(run.glob('*.log')):
        q=replay/p.relative_to(ROOT);q.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,q)
    manifest={p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()};(replay/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    ptr=dict(receipt=str(dest),receipt_sha256=sha(dest),replay=str(replay),replay_manifest_sha256=sha(replay/'manifest.json'));(OUT/'latest-verification.json').write_text(json.dumps(ptr,indent=2)+'\n')
    print(json.dumps(dict(status=result['status'],receipt=str(dest),runs=runs)))


if __name__=='__main__':main()
