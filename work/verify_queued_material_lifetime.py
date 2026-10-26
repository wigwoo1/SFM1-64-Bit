"""Actual heap material functors through original enqueue/FIFO/drain/cleanup.

Private reference only. Expected allocation-failure children abort at the
original unsafe null dereference; those are not successful-cleanup cases.
"""
from pathlib import Path
from datetime import datetime,timezone
import difflib,hashlib,json,re,shutil,struct,subprocess,tempfile,time
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_material_keyvalues_parser import DUMP
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
from pe_image import Image

OUT=ROOT/'outputs/queued-material-lifetime-20261026'
NEW=['probe_queued_material_lifetime.cpp','build_queued_material_lifetime.cmd','verify_queued_material_lifetime.py']


def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    ptr=json.loads((OUT/'preflight.json').read_text())['prior'];receipt=Path(ptr['receipt']);require(sha(receipt)==ptr['receipt_sha256'],'Prior dictionary receipt');prior=json.loads(receipt.read_text())
    changed=ROOT/'work/probe_material_dictionary.cpp';before=OUT/'before/work/probe_material_dictionary.cpp';require(sha(before)==prior['sources'][str(changed)],'Exact preceding fixture source')
    old=before.read_text();current=changed.read_text();expected=old.replace('int main(int argc,char**argv){try{','#ifndef SFM64_MATERIAL_DICTIONARY_FIXTURE_ONLY\nint main(int argc,char**argv){try{',1)+'#endif\n'
    require(current==expected,'Only the exact optional main guard changed')
    delta=dict(before=sha(before),after=sha(changed),diff=''.join(difflib.unified_diff(old.splitlines(True),current.splitlines(True),fromfile='before',tofile='after')));(run/'source-change.json').write_text(json.dumps(delta,indent=2)+'\n')
    for p,h in prior['sources'].items():
        if p!=str(changed):require(sha(Path(p))==h,'Unrelated accepted source '+p)
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Protected original '+p)
    for p,h in prior['private_library_hashes'].items():require(sha(Path(p))==h,'Private original '+p)
    require(sha(LIB)==sha(PRIVATE)==PIN,'Original material image identity')
    replay=Path(ptr['replay']);require(sha(replay/'manifest.json')==ptr['replay_manifest_sha256'] and all(sha(replay/k)==h for k,h in json.loads((replay/'manifest.json').read_text()).items()),'Prior dictionary frozen evidence')
    sources=source_closure([Path(p) for p in prior['sources']]+[ROOT/'work'/n for n in NEW])
    image=Image(PRIVATE);require([image.word(0x10096228+i*4) for i in range(3)]==[0x10007790,0x100077a0,0x1000f020],'Actual atomic heap functor methods')
    require([image.word(0x1009621c+i*4) for i in range(2)]==[0x1000f010,0x10020c60] and image.word(0x10096854)==0x1000f710,'Actual delete policy/external enqueue vtables')
    _,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('forward_decrement',0x10007820,0x1000789b),('forward_delete_if_unreferenced',0x100078a0,0x1000791e),
        ('heap_functor_atomic_addref',0x10007790,0x1000779e),('heap_functor_atomic_release',0x100077a0,0x100077cc),
        ('functor_secondary_delete_adjust',0x1000f010,0x1000f018),('functor_invoke',0x1000f020,0x1000f02a),('heap_delete_policy',0x10020c60,0x10020c63),
        ('heap_scalar_delete',0x10032200,0x1003221c),('heap_free',0x10032320,0x10032337),('heap_allocate',0x10032340,0x10032357),
        ('deferred_raw_decrement',0x100218a0,0x100218a5),('deferred_raw_delete_check',0x10021850,0x10021858),
        ('actual_external_enqueue',0x1000f710,0x1000f73d),('actual_queue_append',0x10005320,0x10005398),('actual_fifo_invoke_release_drain',0x100219a0,0x100219dd),
        ('actual_queue_buffer_construct',0x10073950,0x10073973),('actual_queue_buffer_initialize',0x100739c0,0x10073ac1),
        ('actual_queue_buffer_reset',0x10073980,0x100739c0),('actual_queue_buffer_release',0x10073ad0,0x10073b1d),('actual_queue_buffer_release_guard',0x10073c70,0x10073c7c)])
    started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_queued_material_lifetime.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=120)
    (run/'build-driver.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Fresh affected reference builds')
    exes=[];logs={}
    for tag in ['queued_lifetime','dictionary']:
        exe=OUT/(tag+'_reference_x86.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==0x14c and struct.unpack_from('<H',raw,pe+24)[0]==0x10b and b'asInvoker' in raw,'Fresh architecture/manifest '+tag)
        text=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',text,re.M|re.I);require({s.lower() for s in modules}=={'kernel32.dll'},'Direct imports '+tag)
        exes.append(dict(path=str(exe),sha256=sha(exe),machine='I386 PE32 reference',direct_imports=modules))
    for name in ['reference-build.log','dictionary-build.log']:
        p=OUT/name;require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',p.read_text(),re.I),'Clean compile '+name);logs[str(p)]=sha(p)
    mount=run/'empty-mount';mount.mkdir();libs=ROOT/'outputs/material-fallback-owned-20261026/verification-0mhr77s9/copied-libraries'
    def execute(exe,output,label,extra=(),code=0):
        p=subprocess.run([str(exe),str(PRIVATE),str(libs),str(mount),str(output),*map(str,extra)],cwd=run,capture_output=True,text=True,timeout=60)
        (run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==code,label+' exit='+str(p.returncode)+'; inspect log');return p.stdout+p.stderr
    runs=[];exe=OUT/'queued_lifetime_reference_x86.exe'
    for repeat in range(2):
        record=run/('queued-'+str(repeat)+'.txt');text=execute(exe,record,'queued-'+str(repeat));counts=re.search(r'queued_lifetime_cases=(\d+) functors=(\d+) retained_functors=(\d+) independent_checks=(\d+) allocations=(\d+) releases=(\d+)',text)
        require(counts and list(map(int,counts.groups()[:3]))==[33,365,8] and int(counts[4])>=654 and counts[5]==counts[6],'Exact queued coverage/cleanup')
        rows=[line.split() for line in record.read_text().splitlines()];expected=[]
        for scenario in range(4):
            for sourced in range(2):
                for refs in [1,2,3,7]:expected.append([str(len(expected)),str(scenario),str(sourced),str(refs),'1',str(0 if scenario==2 else refs+(2 if scenario==1 else 1))])
        expected.append(['32','B','64','255']);require(rows==expected,'Independent complete scenario/count/FIFO coverage')
        runs.append(dict(cases=int(counts[1]),functors=int(counts[2]),retained_functors=int(counts[3]),checks=int(counts[4]),allocations=int(counts[5]),releases=int(counts[6]),records_sha256=sha(record)))
    require(runs[0]==runs[1],'Repeatable queue reference results/accounting')
    failures=[]
    for slot in [13,51]:
        record=run/('failure-'+str(slot)+'.txt');text=execute(exe,record,'failure-'+str(slot),[slot],238)
        require(text.strip()==f'EXPECTED_UNSAFE_NULL_ENQUEUE slot={slot} failed_allocations=1 eip=1000f717 address=0','Exact original unsafe allocation-failure observation')
        failures.append(dict(slot=slot,expected_exit=238,failed_functor_allocations=1,eip='0x1000f717',address=0,cleanup='NOT EXECUTED; disposable expected-fault child terminated by observer'))
    regression=run/'dictionary-regression.txt';text=execute(OUT/'dictionary_reference_x86.exe',regression,'dictionary-regression')
    require(sha(regression)==prior['runs'][0]['records_sha256'],'All2324 dictionary operation records unchanged')
    require('created=1160 removed=1160 duplicate_hits=20 independent_checks=10793 allocations=1830 releases=1830' in text,'Dictionary regression counts preserved')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());graph=set()
    def visit(p):
        k=p.relative_to(ROOT).as_posix()
        if k in graph:return
        graph.add(k)
        for name in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            q=p.parent/name
            if q.is_file():visit(q.resolve())
    visit((ROOT/'work/probe_vulkan_win32_surface_x64.cpp').resolve())
    require(graph==set(build['editor_source_dependencies']) and all(sha(ROOT/k)==h for k,h in build['editor_source_dependencies'].items()),'Entire editor graph unchanged')
    require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor unchanged')
    for p,h in sources.items():require(sha(Path(p))==h,'Source drift '+p)
    for p,h in prior['original_hashes'].items():require(sha(Path(p))==h,'Original drift '+p)
    for p,h in prior['private_library_hashes'].items():require(sha(Path(p))==h,'Private original drift '+p)
    limits=['Original-reference only; no owned x64 lifetime/cache/functor implementation or editor integration added',
        'Actual original forwarding heap functor construction, atomic refcounts/invoke/delete, external enqueue, linked append, FIFO drain, Windows queue-buffer initialization/reset/release and actual material/dictionary cleanup execute with unchanged RX instructions',
        'Enqueue does not add a raw material reference. Fixtures keep target alive through valid ordered operations; no automatic strong-target ownership, arbitrary cancellation, stale-target use or worker-thread safety is claimed',
        'Material context lookup, current queue selection, nested system106/107 tokens, mode/binding invalidation and heap/KV symbol/lock services remain controlled; surrounding render-context object is not normally initialized',
        'Both allocation-failure children intentionally terminate at original unconditional null dereference in enqueue. No successful cleanup, native recovery or safe failure policy is inferred; original queue requires a nonnull functor',
        'No initialized textures/proxies/resources, general material rendering, original writes, live SFM access, GPU, Blender or editor build/run; broader earlier tests not rerun except affected dictionary regression']
    result=dict(status='PASS_REFERENCE_ONLY',utc=datetime.now(timezone.utc).isoformat(),scope='Original queued material-object heap functor lifetime',previous_goal_turn='progress',runs=runs,expected_unsafe_failures=failures,dictionary_regression=dict(records=2324,sha256=sha(regression),log=text),executables=exes,build_logs=logs,source_change=delta,sources=sources,original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],static_ranges=ranges,prior_receipt=str(receipt),prior_receipt_sha256=sha(receipt),editor_dependencies_unchanged=len(graph),editor_sha256=build['executable_sha256'],limits=limits)
    dest=run/'results.json';dest.write_text(json.dumps(result,indent=2)+'\n');replay=OUT/('replay-'+run.name.removeprefix('verification-'));replay.mkdir()
    for p in [Path(p) for p in sources]+[Path(e['path']) for e in exes]+[Path(p) for p in logs]+[dest,before]+list(run.glob('*.txt'))+list(run.glob('*.log'))+[run/'source-change.json']:
        q=replay/p.relative_to(ROOT);q.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,q)
    manifest={p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()};(replay/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    ptr=dict(receipt=str(dest),receipt_sha256=sha(dest),replay=str(replay),replay_manifest_sha256=sha(replay/'manifest.json'));(OUT/'latest-verification.json').write_text(json.dumps(ptr,indent=2)+'\n')
    print(json.dumps(dict(status=result['status'],receipt=str(dest),runs=runs,expected_unsafe_failures=failures)))


if __name__=='__main__':main()
