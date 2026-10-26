"""Original CMaterial construction/source/refcount/empty-state destruction gate.

Reference only; no live SFM, GPU, original writes, or owned lifetime API added.
Cache insertion and current-binding invalidation remain observed fixture edges.
"""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, json, re, shutil, struct, subprocess, tempfile, time
from verify_material_numeric_value import ROOT, LIB, PRIVATE, PIN, require, sha
from verify_material_keyvalues_parser import DUMP
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
from pe_image import Image

OUT=ROOT/'outputs/material-object-lifetime-20261026'
NEW=['probe_material_object_lifetime.cpp','build_material_object_lifetime.cmd','verify_material_object_lifetime.py']


def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    pointer=json.loads((ROOT/'outputs/unlit-material-integration-20261026/latest-verification.json').read_text())
    receipt=Path(pointer['receipt']);require(sha(receipt)==pointer['receipt_sha256'],'Prior whole-caller receipt')
    prior=json.loads(receipt.read_text())
    for p,h in prior['sources'].items():require(sha(Path(p))==h,'Accepted source '+p)
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Protected original '+p)
    for p,h in prior['private_library_hashes'].items():require(sha(Path(p))==h,'Private library '+p)
    require(sha(LIB)==sha(PRIVATE)==PIN,'Original material library')
    sources=source_closure([Path(p) for p in prior['sources']]+[ROOT/'work'/n for n in NEW])
    image=Image(PRIVATE)
    tables={hex(t):{str(i):hex(image.word(t+4*i)) for i in [12,13,69,70,81,93,94,100]} for t in [0x10095d74,0x10095a9c]}
    require(image.word(0x100984cc+76*4)==0x1000b790,'Real VMaterialSystem080 slot76')
    require(image.word(0x10095d74+100*4)==0x10006d20,'Actual raw scalar deleting destructor')
    require(image.word(0x10095f08)==0x3e4ccccd,'Original constructor three-component default literal is 0.2')
    imports={hex(a):image.string(image.base+image.word(a)+2) for a in [0x10095174,0x10095178,0x1009517c,0x10095180,0x10095184,0x100951e4]}
    require(imports['0x10095184']=='??0CThreadSpinRWLock@@QAE@XZ' and imports['0x100951e4']=='Warning','Exact external lock/warning ABI')
    _,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('CMaterial_ctor',0x10006650,0x10006784),('CMaterial_dtor',0x100067c0,0x10006839),
        ('raw_factory',0x10006cc0,0x10006d1a),('scalar_deleter',0x10006d20,0x10006d43),
        ('public_CreateMaterial',0x1000b790,0x1000b7d6),('allocate_empty_shader_state',0x10002a80,0x10002b00),
        ('free_shader_state',0x10002b00,0x10002bba),('uncache',0x10005d10,0x10005db2),
        ('raw_increment',0x10005dc0,0x10005dce),('raw_decrement',0x1002dd90,0x1002dd9c),
        ('forward_increment',0x100075a0,0x100075aa),('forward_decrement',0x10007820,0x1000789b),
        ('raw_to_forward',0x100067b0,0x100067b7),('forward_to_raw',0x100043d0,0x100043d4),
        ('fresh_flags_bit2_predicate',0x10002be0,0x10002beb),('fresh_flags_bit1_predicate',0x10002bf0,0x10002bfa),
        ('symbol_global_init',0x10073840,0x100738d6),('symbol_global_cleanup',0x10094c20,0x10094c56),
        ('symbol_intern',0x100738e0,0x10073926),('symbol_text',0x10073930,0x1007394a),
        ('separate_delete_if_unreferenced',0x10005e10,0x10005e41),('delete_material_helper',0x100029b0,0x100029ed),
        ('pool_allocate',0x10070390,0x100703de),('pool_free',0x100702b0,0x100702c9)])
    _,name_ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('constructor_strip_extension_preserve_slashes',0x1006e7e0,0x1006e871)])
    ranges+=name_ranges
    started=time.time();build=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_object_lifetime.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=120)
    (run/'build-driver.log').write_text(build.stdout+build.stderr);require(build.returncode==0,'Fresh reference compile')
    exe=OUT/'lifetime_reference_x86.exe';log=OUT/'reference-build.log';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
    require(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==0x14c and struct.unpack_from('<H',raw,pe+24)[0]==0x10b and b'asInvoker' in raw,'Fresh x86 reference architecture/manifest')
    require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Clean compile log')
    imported=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout
    modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imported,re.M|re.I);require({m.lower() for m in modules}=={'kernel32.dll'},'Reference direct imports')
    mount=run/'empty-mount';mount.mkdir();libs=ROOT/'outputs/material-fallback-owned-20261026/verification-0mhr77s9/copied-libraries'
    results=[]
    for repeat in range(2):
        output=run/('records-'+str(repeat)+'.txt')
        proc=subprocess.run([str(exe),str(PRIVATE),str(libs),str(mount),str(output)],cwd=run,capture_output=True,text=True,timeout=30)
        (run/('reference-'+str(repeat)+'.log')).write_text(proc.stdout+proc.stderr)
        require(proc.returncode==0,'Original lifetime execution '+str(repeat)+' exit='+str(proc.returncode)+'; inspect log')
        counts=re.search(r'material_lifetime_cases=(\d+) independent_checks=(\d+) allocations=(\d+) releases=(\d+)',proc.stdout)
        require(counts and int(counts[1])==80 and int(counts[2])>=1800 and counts[3]==counts[4],'Case/assertion/allocation accounting')
        rows=[list(map(int,l.split())) for l in output.read_text().splitlines()];require(len(rows)==80,'Complete record count')
        expected=[]
        for mode in range(4):
            for sourced in range(2):
                for api in range(2):
                    for _ in range(5):expected.append([len(expected),mode,sourced,api,0x1600 if mode&2 else 0x580 if mode&1 else 0x2c0,api,1])
        require(rows==expected,'Independent mode/source/factory/state/cache/destructor coverage')
        results.append(dict(cases=int(counts[1]),checks=int(counts[2]),allocations=int(counts[3]),releases=int(counts[4]),output_sha256=sha(output)))
    require(results[0]==results[1],'Repeatable original results/accounting')
    info=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());graph=set()
    def visit(p):
        k=p.relative_to(ROOT).as_posix()
        if k in graph:return
        graph.add(k)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            q=p.parent/inc
            if q.is_file():visit(q.resolve())
    visit((ROOT/'work/probe_vulkan_win32_surface_x64.cpp').resolve())
    require(graph==set(info['editor_source_dependencies']) and all(sha(ROOT/k)==v for k,v in info['editor_source_dependencies'].items()),'Entire editor source graph unchanged')
    require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==info['executable_sha256'],'Editor executable unchanged')
    for p,h in sources.items():require(sha(Path(p))==h,'Source changed during verification '+p)
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Protected original changed '+p)
    for p,h in prior['private_library_hashes'].items():require(sha(Path(p))==h,'Private original changed '+p)
    limits=['Original reference only: no owned x64 material-object factory/lifetime implementation or editor integration added',
        'Normal original constructor, pool hot paths, symbols, source ownership, raw refs, forwarding identity/increment/null-queue decrement and empty-state destructor execute; pool growth/allocator failure/resource initialization not tested',
        'Cache slot137 and current-binding invalidation slot145 are observed service callbacks, not original cache/render-context implementations; duplicate-name lookup/cache erase/delete-if-unreferenced remain unverified dynamically',
        'Original queued forwarding decrement is statically verified but dynamically tested only with a null call queue; no worker-thread safety or queued teardown claim',
        'Controlled single-thread allocator, KeyValues symbol services and locks/token/mode/context callbacks retained; original imported symbol-lock constructor resolved from private tier0 export',
        'No GPU, live SFM, Blender, original writes, full-editor build/run or broader test rerun; accepted prior sources and editor graph/executable identities preserved']
    result=dict(status='PASS_REFERENCE_ONLY',utc=datetime.now(timezone.utc).isoformat(),scope='Original material-object source/refcount/empty-state lifecycle',previous_goal_turn='progress',runs=results,executables=[dict(path=str(exe),sha256=sha(exe),machine='I386 PE32 reference',direct_imports=modules)],build_logs={str(log):sha(log)},sources=sources,original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],static_ranges=ranges,actual_vtables=tables,imports=imports,prior_receipt=str(receipt),prior_receipt_sha256=sha(receipt),editor_dependencies_unchanged=len(graph),editor_sha256=info['executable_sha256'],limits=limits)
    dest=run/'results.json';dest.write_text(json.dumps(result,indent=2)+'\n');replay=OUT/('replay-'+run.name.removeprefix('verification-'));replay.mkdir()
    for p in [Path(p) for p in sources]+[exe,log,dest]+list(run.glob('*.txt'))+list(run.glob('*.log')):
        q=replay/p.relative_to(ROOT);q.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,q)
    manifest={p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()};(replay/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    pointer=dict(receipt=str(dest),receipt_sha256=sha(dest),replay=str(replay),replay_manifest_sha256=sha(replay/'manifest.json'));(OUT/'latest-verification.json').write_text(json.dumps(pointer,indent=2)+'\n')
    print(json.dumps(dict(status=result['status'],receipt=str(dest),runs=results)))


if __name__=='__main__':main()
