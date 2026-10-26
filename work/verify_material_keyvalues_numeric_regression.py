"""Recompile with build_material_keyvalues_numeric_regression.cmd first.

Replay accepted queries against pinned x86 oracles and new x64 binaries; keep
the accepted binaries and receipts intact. No editor or live SFM access.
"""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,struct,subprocess,tempfile
from verify_material_keyvalues_numeric import ROOT,OUT,sha,need,invoke,DUMP

def main():
    run=Path(tempfile.mkdtemp(prefix='regression-',dir=OUT));build=OUT/'regression-build'
    private=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
    original=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/bin/materialsystem.dll')
    pin='c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8'
    need(sha(private)==sha(original)==pin,'Pinned reference library')
    accepted=[('value_construction','material-value-construction-20261025/verification-0othnx98'),('vector_literal','material-vector-literal-20261025/verification-saxzt8pd'),('matrix_scan','material-matrix-scan-20261025/verification-qtomolk1'),('typed_dispatch','material-typed-dispatch-20261026/verification-mzxxgbl6')]
    results=[]
    for name,folder in accepted:
        prior=ROOT/'outputs'/folder;receipt=json.loads((prior/'results.json').read_text());query=prior/'queries.bin'
        need(receipt['status']=='PASS' and sha(query)==receipt['query_sha256'],'Accepted query '+name)
        native=receipt['executables'][0];need(sha(native['path'])==native['sha256'],'Pinned native executable '+name)
        exe=build/(name+'_owned_x64.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        need(struct.unpack_from('<H',raw,pe+4)[0]==0x8664 and struct.unpack_from('<H',raw,pe+24)[0]==0x20b and b'asInvoker' in raw,'Owned architecture/manifest '+name)
        log=build/(name+'-build.log');need(not re.search(r'\b(warning|error)\b',log.read_text(),re.I),'Owned clean build '+name)
        if name=='typed_dispatch':need(not re.search(r'\b(warning|error)\b',(build/'asm-build.log').read_text(),re.I),'Owned clean assembler')
        no=run/(name+'-native.bin');oo=run/(name+'-owned.bin');absent=run/'no-original-engine.dll';need(not absent.exists(),'Owned engine absence')
        invoke(Path(native['path']),[private,query,no],run,name+'-native')
        invoke(exe,[absent,query,oo],run,name+'-owned')
        need(no.read_bytes()==oo.read_bytes(),'Exact regression comparison '+name)
        imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(name+'-imports.txt')).write_text(imports)
        modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);need(set(m.lower() for m in modules)=={'kernel32.dll'},'Owned imports '+name)
        tests=invoke(exe,['--test'],run,name+'-analytical') if name in ['value_construction','typed_dispatch'] else 'Not provided by probe'
        results.append(dict(name=name,cases=receipt['cases'],query_sha256=sha(query),output_sha256=sha(oo),native=native,owned=dict(path=str(exe),sha256=sha(exe),build_log_sha256=sha(log),machine='AMD64 PE32+',direct_imports=modules),analytical=tests))
    need(sha(private)==sha(original)==pin,'Original library changed')
    summary=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),cases=sum(r['cases'] for r in results),results=results,library_sha256=pin,limits=['Pinned preexisting native executables replayed; x64 probes freshly compiled in isolated directory','Exact accepted query outputs; no editor, renderer or workflow equivalence claim'])
    (run/'results.json').write_text(json.dumps(summary,indent=2)+'\n')
    (OUT/'latest-regression.json').write_text(json.dumps(dict(path=str(run),receipt_sha256=sha(run/'results.json')),indent=2)+'\n')
    print(json.dumps(dict(status='PASS',cases=summary['cases'],groups=[dict(name=r['name'],cases=r['cases']) for r in results],receipt=str(run/'results.json')),indent=2))
if __name__=='__main__':main()
