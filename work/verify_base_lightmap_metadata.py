"""Bounded metadata-only admission, owned GPU equality and original world inventory."""
from pathlib import Path
from datetime import datetime,timezone
import json,os,re,struct,tempfile,shutil
from verify_vtf_lod_preparation import ROOT,VS,sha,need,put,compile,command
import verify_scene_base_lightmap as scenes
import verify_base_lightmap_inventory as inventory
OUT=ROOT/'outputs/material-metadata-contract-20261101'
MODIFIED=['source_bsp_base_lightmap_candidate.h','probe_base_lightmap_inventory_x64.cpp','verify_base_lightmap_inventory.py','probe_d3d9_scene_base_lightmap_x64.cpp']
NEW=['test_base_lightmap_metadata.cpp',Path(__file__).name]
def state():
    b=json.loads((OUT/'before.json').read_text());need(sha(b['parent']['receipt'])==b['parent']['receipt_sha256'],'parent identity');sources=dict(b['source_sha256']);transitions={}
    for n in MODIFIED:
        p=str(ROOT/'work'/n);need(sha(OUT/'baseline'/n)==sources[p],'old source preserved '+n);transitions[p]={'before':sources[p],'after':sha(p)};sources[p]=sha(p)
    for n in NEW:sources[str(ROOT/'work'/n)]=sha(ROOT/'work'/n)
    for group in [sources,b['protected_sha256']]:need(all(sha(p)==h for p,h in group.items()),'current identities')
    return b,sources,transitions
def main():
    before,sources,transitions=state();parent=json.loads(Path(before['parent']['receipt']).read_text());native_ref=json.loads((OUT/'latest-reference.json').read_text());need(sha(native_ref['receipt'])==native_ref['receipt_sha256'],'native metadata invariance receipt')
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print(run,flush=True);(OUT/'active-verification.txt').write_text(str(run));exes={};warnings={};logs=[]
    def build(name,source,libs=''):
        exe,w=compile(run,name,source,libs);exes[name]={'path':str(exe),'sha256':sha(exe)};warnings[name]=w;return exe
    unit=build('metadata-unit','test_base_lightmap_metadata.cpp');decoder=build('decoder-unit','test_source_bsp_base_lightmap.cpp');need(not warnings['metadata-unit'] and not warnings['decoder-unit'],'CPU no new warnings')
    for repeat in range(2):
        logs.append(command([unit],f'metadata-unit-{repeat}',run));logs.append(command([decoder],f'decoder-unit-{repeat}',run))
    gpu=build('scene-gpu','probe_d3d9_scene_base_lightmap_x64.cpp','d3d9.lib d3dcompiler.lib user32.lib');old=ROOT/'outputs/scene-base-lightmap-20261101/r-i6gb_ua0';old_result=json.loads((old/'results.json').read_text());need([list(w) for w in warnings['scene-gpu']]==old_result['warnings']['scene-gpu'],'same five inherited GPU warnings')
    plain=scenes.build_cases(run);rows=[];mapping=[]
    variants=['plain','keywords','surface','both']
    for source_serial,row in enumerate(plain):
        for variant in variants:
            root=Path(row['root'])
            if variant!='plain':
                root=run/(f'metadata-{source_serial}-'+variant);shutil.copytree(row['root'],root)
                extra=(' %KEYWORDS "tf, owned labels"' if variant in ['keywords','both'] else '')+(' $surfaceprop unresolved_owned_surface' if variant in ['surface','both'] else '')
                for mat in ('a','b'):
                    p=root/'materials/owned'/(mat+'.vmt');text=p.read_text();at=text.rfind('}');p.write_text(text[:at]+extra+' '+text[at:])
            q=dict(row,root=str(root));rows.append(q);mapping.append({'source':source_serial,'variant':variant})
    put(run/'case-map.json',mapping);query=run/'query.bin';scenes.query(rows,query);pixels=[];metadata=[]
    expected_pixels=(old/'g0/pixels.bin').read_bytes();expected_metadata=(old/'g0/metadata.bin').read_bytes();need(len(expected_pixels)==24*16388 and len(expected_metadata)==24*368,'frozen scene framing')
    for repeat in range(2):
        out=run/f'gpu-{repeat}';logs.append(command([gpu,query,out],f'gpu-{repeat}',run));pixels.append((out/'pixels.bin').read_bytes());metadata.append((out/'metadata.bin').read_bytes())
        need(len(pixels[-1])==96*16388 and len(metadata[-1])==96*368,'complete96-case GPU outputs')
        for serial,entry in enumerate(mapping):
            source=entry['source'];a=pixels[-1][serial*16388:(serial+1)*16388];b=metadata[-1][serial*368:(serial+1)*368]
            need(struct.unpack_from('<I',a)[0]==serial and struct.unpack_from('<I',b)[0]==serial,'serial framing')
            need(a[4:]==expected_pixels[source*16388+4:(source+1)*16388],'metadata does not change GPU pixels')
            need(b[4:]==expected_metadata[source*368+4:(source+1)*368],'metadata does not change geometry/UV/draw plan')
    need(pixels[0]==pixels[1] and metadata[0]==metadata[1],'exact96-case repeats')
    # Default owner never enables the optional base-lightmap admission; its full
    # preceding730-case images/samplers must remain byte-identical.
    indexed=build('indexed-gpu','probe_d3d9_prepared_scene_x64.cpp','d3d9.lib d3dcompiler.lib user32.lib');old_index_ref=parent['indexed_gpu_receipt'];need(sha(old_index_ref['receipt'])==old_index_ref['receipt_sha256'],'old indexed receipt');old_index=json.loads(Path(old_index_ref['receipt']).read_text());previous=Path(old_index['executable']['path']).parent
    for repeat in range(2):
        out=run/f'indexed-{repeat}';logs.append(command([indexed,previous/'packets.bin',out,'owned'],f'indexed-{repeat}',run))
        for filename in ['pixels.bin','samplers.bin']:need((out/filename).read_bytes()==(previous/'actual-0'/filename).read_bytes(),'default730 unchanged '+filename)
    inv=build('inventory','probe_base_lightmap_inventory_x64.cpp');invunit=build('inventory-unit','test_base_lightmap_inventory.cpp');fixtures=inventory.owned_cases(run)
    for f in fixtures:
        if f['name'] in ['metadata_all','lod_a']:f['admitted']=3
    for repeat in range(2):logs.append(command([invunit,fixtures[0]['root']],f'inventory-unit-{repeat}',run))
    for f in fixtures:
        out=run/('checked-'+f['name']);command([inv,f['root']/'maps/owned.bsp',f['root'],'owned',out,'owned'],'inventory-'+f['name'],run);j=inventory.reconcile(out/'inventory.json',f['root']/'maps/owned.bsp');need(j['summary']['combined_admitted_faces']==f['admitted'] and j['summary']['triangles']==f['triangles'],'owned inventory '+f['name'])
    expectation=json.loads((OUT/'predeclared-benchmark.json').read_text());inputs=parent['benchmark_input_sha256'];need(all(sha(p)==h for p,h in inputs.items()),'original assets before');bench=[]
    for repeat in range(2):
        out=run/f'benchmark-{repeat}';command([inv,inventory.MAP,inventory.CONTENT,'mtt_soldier',out,'mounted'],f'benchmark-{repeat}',run);j=inventory.reconcile(out/'inventory.json',inventory.MAP);admitted=[f['id'] for f in j['faces'] if not f['combined_reasons']];need(admitted==expectation['source_face_ids'],'predeclared exact admitted source faces');need(j['summary']['combined_admitted_faces']==1528 and j['summary']['combined_admitted_triangles']==4535 and j['summary']['materials_admitted']==11,'predeclared totals');bench.append(j)
    need((run/'benchmark-0/inventory.json').read_bytes()==(run/'benchmark-1/inventory.json').read_bytes(),'exact benchmark repeats');need(all(sha(p)==h for p,h in inputs.items()),'original assets after');state()
    result=dict(status='VERIFIED bounded normalized lightmap metadata admission; full Source HDR/visual parity remains unverified',time_utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='PROGRESS',source_sha256=sources,source_transitions=transitions,protected_sha256=before['protected_sha256'],parent=before['parent'],native_reference=native_ref,executables=exes,compiler_warnings=warnings,gpu_cases=96,gpu_pixels_per_run=96*1024,gpu_repetitions=2,old_indexed_cases=730,owned_inventory_cases=20,benchmark_summary=bench[0]['summary'],benchmark_reasons=bench[0]['overlapping_combined_face_reasons'],benchmark_input_sha256=inputs,benchmark_inventory={'path':str(run/'benchmark-0/inventory.json'),'sha256':sha(run/'benchmark-0/inventory.json')},admitted_source_faces=expectation['source_face_ids'],logs=logs,limits=['Only plain scalar %keywords/$surfaceprop once each are exempted from this normalized pass shader switches; authored syntax retained.','Native collector36 cases show declared parameter invariance under controlled gate words, not native GPU or live tool configuration equivalence.','Other compile/tool/physics/shader features, nested/conditional/duplicate metadata and native lightmap/HDR behavior are not broadened.'])
    receipt=run/'results.json';put(receipt,result);put(OUT/'latest-verification.json',{'receipt':str(receipt),'receipt_sha256':sha(receipt)});print(json.dumps({k:result[k] for k in ['status','gpu_cases','old_indexed_cases','benchmark_summary']},indent=2))
if __name__=='__main__':main()
