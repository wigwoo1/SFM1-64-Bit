"""Reverify corrected info mapping without replacing historical evidence."""
from pathlib import Path
from datetime import datetime,timezone
import contextlib,difflib,hashlib,io,json,re,struct,subprocess,sys,tempfile,time,zlib
import verify_vertexlit_shared_init as legacy
from verify_material_texture_file_resolver import closure

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/vertexlit-shared-mapping-correction-20261028'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def sha(p):
    with Path(p).open('rb') as f:
        return hashlib.file_digest(f,'sha256').hexdigest()

def need(b,s):
    if not b:
        raise RuntimeError(s)

def main():
    OUT.mkdir(exist_ok=True)
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    print('Verification directory: '+str(run),flush=True)
    parent=ROOT/'outputs/concrete-vertexlit-draw-reference-20261028/verification-cjh3tc3p/results.json'
    need(sha(parent)=='7279b38f63b05b3b4402d19e93399e97b6fd17894e4387f915dab467e0a9780a','accepted concrete original-built mapping receipt')
    prior=json.loads(parent.read_text())
    historical=ROOT/'outputs/vertexlit-shared-init-20261025/verification-lpr0qmmc/results.json'
    old=json.loads(historical.read_text())
    protected={**prior['protected'],**prior['source'],str(parent):sha(parent),str(historical):sha(historical)}
    for e in prior['executables'].values():
        protected[e['path']]=e['sha256']
    for name in ['info.bin','queries.bin','native-records.bin.zlib','parameter-links.json']:
        p=historical.parent/name;protected[str(p)]=sha(p)
    protected[old['editor']['path']]=old['editor']['sha256']
    for e in old['executables']:
        protected[e['path']]=e['sha256']
    preflight=json.loads((OUT/'preflight.json').read_text())
    need(preflight['parent_sha256']==sha(parent),'pre-change parent')
    changed={}
    path=ROOT/'work/verify_vertexlit_shared_init.py'
    before=OUT/'verify_vertexlit_shared_init.py.before'
    need(sha(before)==preflight['before'][path.name]==protected[str(path)],'preserved original verifier')
    expected=before.read_text().replace(r'(?:\+([0-9A-F]+)h)?',r'(?:\+([0-9A-F]+)h?)?')
    expected=expected.replace("    require(mapping[0]==6 and mapping[0x64//4]==9,'Base/flashlight literal mapping')", "    require(mapping[1:3]==[14,15],'COMPRESS/STRETCH stores with offsets4/8 must be mapped')\n    require(mapping[0]==6 and mapping[0x64//4]==9,'Base/flashlight literal mapping')")
    need(path.read_text()==expected and before.read_text()!=expected,'only two parser expressions plus explicit mapping assertion changed')
    changed[str(path)]=dict(before=str(before),before_sha256=sha(before),after_sha256=sha(path),diff=''.join(difflib.unified_diff(before.read_text().splitlines(True),path.read_text().splitlines(True))))
    del protected[str(path)];protected[str(before)]=sha(before)
    doc=ROOT/'work/CONCRETE_VERTEXLIT_DRAW_REFERENCE.md';saved=OUT/(doc.name+'.before')
    need(sha(saved)==protected[str(doc)],'preserved prior scope notes')
    changed[str(doc)]=dict(before=str(saved),before_sha256=sha(saved),after_sha256=sha(doc),diff=''.join(difflib.unified_diff(saved.read_text().splitlines(True),doc.read_text().splitlines(True))))
    del protected[str(doc)];protected[str(saved)]=sha(saved)
    need(all(sha(p)==h for p,h in protected.items()),'all unaffected source and historical artifacts unchanged before')
    originals=dict(prior['original_inputs'])
    need(all(sha(p)==h for p,h in originals.items()),'original/private inputs unchanged before')
    sources=closure([Path(__file__),ROOT/'work/build_vertexlit_shared_mapping_correction.cmd',ROOT/'work/probe_vertexlit_shared_init.cpp',path,doc])
    for module in tuple(sys.modules.values()):
        p=getattr(module,'__file__',None)
        if p and Path(p).resolve().parent==ROOT/'work' and Path(p).suffix=='.py':
            sources[str(Path(p).resolve())]=sha(p)
    started=time.time()
    build=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_vertexlit_shared_mapping_correction.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180)
    (run/'build-driver.log').write_text(build.stdout+build.stderr)
    need(build.returncode==0,'fresh isolated native/owned shared initializer builds')
    executables={}
    for tag,machine,magic in [('native',0x14c,0x10b),('owned',0x8664,0x20b)]:
        exe=OUT/('vertexlit_shared_'+tag+('_x86.exe' if tag=='native' else '_x64.exe'))
        raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic and b'asInvoker' in raw,'fresh architecture/manifest '+tag)
        log=(OUT/(tag+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log)
        need(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log,re.I),'clean compile '+tag)
        inspected=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,timeout=30)
        (run/(tag+'-imports.txt')).write_text(inspected.stdout)
        imports=sorted(set(re.findall(r'^\s+([^\s]+\.dll)\s*$',inspected.stdout,re.I|re.M)))
        need(inspected.returncode==0 and imports==['KERNEL32.dll'],'system-only imports '+tag)
        executables[tag]=dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=imports)
    # Run the entire corrected old verifier unchanged apart from its mapping fix.
    # Only its module output directory is redirected; all original component,
    # registry, analytical, callback, warning and archived-editor checks remain.
    previous_dirs=set(OUT.glob('verification-*'));legacy.OUT=OUT
    captured=io.StringIO()
    with contextlib.redirect_stdout(captured):
        legacy.main()
    (run/'complete-corrected-initializer.log').write_text(captured.getvalue())
    new_dirs=set(OUT.glob('verification-*'))-previous_dirs
    need(len(new_dirs)==1,'one isolated full initializer receipt')
    corrected_dir=new_dirs.pop();corrected_receipt=corrected_dir/'results.json'
    corrected=json.loads(corrected_receipt.read_text())
    need(corrected['status']=='PASS' and corrected['cases']==old['cases']==2661,'complete2661-case revalidation')
    current_mapping=list(struct.unpack('<110I',(corrected_dir/'info.bin').read_bytes()))
    old_mapping=list(struct.unpack('<110I',(historical.parent/'info.bin').read_bytes()))
    diffs=[dict(word=i,old=a,corrected=b) for i,(a,b) in enumerate(zip(old_mapping,current_mapping)) if a!=b]
    need(diffs==[dict(word=1,old=0xffffffff,corrected=14),dict(word=2,old=0xffffffff,corrected=15)],'exact two-field mapping correction')
    mapping_bytes=(parent.parent/'mapping.bin').read_bytes();count=struct.unpack_from('<I',mapping_bytes)[0];at=4+8*count
    need(struct.unpack_from('<I',mapping_bytes,at)[0]==110,'native concrete builder mapping extent')
    need(current_mapping==list(struct.unpack_from('<110I',mapping_bytes,at+4))==prior['info_records'][0],'all110 corrected words match native-constructed concrete records')
    need(corrected['mapped_info_fields']==84 and old['mapped_info_fields']==82,'registered fields82 ->84')
    need(corrected['query_sha256']==old['query_sha256']==sha(corrected_dir/'queries.bin'),'identical complete initializer input corpus')
    baseline=zlib.decompress((historical.parent/'native-records.bin.zlib').read_bytes())
    current=zlib.decompress((corrected_dir/'native-records.bin.zlib').read_bytes())
    need(hashlib.sha256(baseline).hexdigest()==old['output_sha256'] and hashlib.sha256(current).hexdigest()==corrected['output_sha256'],'historical/corrected full output evidence')
    runs=[]
    for profile,mapping,expected_bytes in [('historical',historical.parent/'info.bin',baseline),('corrected',corrected_dir/'info.bin',current)]:
        for tag in ['native','owned']:
            target=run/(profile+'-'+tag+'.bin')
            result=subprocess.run([executables[tag]['path'],str(legacy.PRIVATE),str(mapping),str(corrected_dir/'queries.bin'),str(target)],cwd=run,capture_output=True,text=True,timeout=120)
            (run/(target.stem+'.log')).write_text(result.stdout+result.stderr)
            need(result.returncode==0 and target.read_bytes()==expected_bytes,'fresh complete '+profile+' '+tag+' trace')
            runs.append(dict(profile=profile,tag=tag,output_sha256=sha(target),stdout=result.stdout.strip()))
            need(target.resolve().parent==run.resolve(),'disposable trace path')
            target.unlink()  # Only newly created duplicate workspace traces.
    same=current==baseline
    impact=dict(identical_full_trace=same,bytes=len(current),historical_bytes=len(baseline),
                corrected_sha256=corrected['output_sha256'],historical_sha256=old['output_sha256'],
                conclusion='No output change across this2661-case initializer corpus' if same else 'Initializer output changed; corrected native/owned parity established')
    need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in protected.items()),'all current source and unaffected historical artifacts unchanged after')
    need(all(sha(p)==h for p,h in originals.items()),'original/private inputs unchanged after')
    need(all(sha(p)==h for p,h in prior['editor_dependencies'].items()) and sha(prior['editor'])==prior['editor_sha256'],'313 current editor dependencies and exact executable unchanged')
    outcome=dict(status='VERIFIED corrected shared initializer mapping and complete revalidation',utc=datetime.now(timezone.utc).isoformat(),
        previous_goal_turn='progress',parent=str(parent),parent_sha256=sha(parent),source=sources,protected=protected,
        original_inputs=originals,executables=executables,changed_files=changed,build_warnings=0,
        historical_receipt=str(historical),historical_receipt_sha256=sha(historical),
        corrected_initializer_receipt=str(corrected_receipt),corrected_initializer_receipt_sha256=sha(corrected_receipt),
        mapping_differences=diffs,registered_mapped_fields=84,coverage=dict(cases=2661,analytical_checks=10,schema_refusals=1,
        generic_cases=corrected['generic_cases'],phong_cases=corrected['phong_cases'],callback_events=corrected['callback_events']),
        runs=runs,impact=impact,editor=prior['editor'],editor_sha256=prior['editor_sha256'],editor_dependencies=prior['editor_dependencies'],editor_rebuilt=False,
        findings=[
          'Corrected two offset grammar expressions and added explicit indices14/15 assertion in the existing initializer verifier. Complete corrected mapping matches all110 native-built concrete shader info words. Only COMPRESS/STRETCH slots differ from the preserved historical fixture.',
          'Entire original2661-case initializer suite rerun with fresh x86/x64 executables in isolated output. All analytical, schema, dispatch, hardware-query, callback, warning, original/private identity and archived-editor checks retained. Native initialization executes unchanged original bodies with explicit controlled services.',
          'Both fresh executables additionally replay historical and corrected mappings on identical queries. The impact record compares every output byte, including states, routes and callback trace. Findings apply to this initializer corpus, not arbitrary material/rendering behavior.',
          'Historical verifier snapshot, receipts, fixtures and binaries preserved; existing initializer/runtime helper sources, current313 editor dependencies and editor binary unchanged. No SFM process access, original writes, dependencies or publication.',
          'Actual render-helper bodies, GPU shader output, full material lifecycle, cross-library draw composition and overall SFM fidelity remain unverified.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(outcome,indent=2)+'\n')
    (OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt)),indent=2)+'\n')
    print(json.dumps(dict(status=outcome['status'],receipt=str(receipt),coverage=outcome['coverage'],impact=impact,editor_rebuilt=False),indent=2),flush=True)

if __name__=='__main__':
    main()
