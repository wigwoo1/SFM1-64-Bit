"""Verify installed slot17 and correct the material predicate's named contract.

Original files are read only. Private x86 oracle is verification-only. No SFM,
DLL loader, original import/service initialization, UI or GPU use.
"""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,json,re,struct,subprocess,tempfile
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/material-var-defined-20261025'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
LIB=GAME/'bin/materialsystem.dll'
PRIVATE=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
PIN='c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8'

def require(b,s):
    if not b:raise RuntimeError(s)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    print('Verification directory: '+str(run),flush=True)
    require(sha(LIB)==sha(PRIVATE)==PIN,'Installed/private material library identity')
    image,evidence=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('constructor_vtable_store',0x100176bb,0x100176c1),
        ('complete_material_IsDefined',0x10018450,0x1001845d)])
    table=0x10098f34;locator=image.word(table-4);descriptor=image.word(locator+12)
    require(image.word(locator)==0 and image.word(locator+4)==0 and
            image.string(descriptor+8)=='.?AVCMaterialVar@@','Installed vtable RTTI class identity')
    require(image.word(table+17*4)==0x10018450 and image.word(table+18*4)==0x1001a0a0,
            'Original slot17/18 identity')
    std=GAME/'bin/stdshader_dx9.dll'
    require(sha(std)=='fb09e2ddc8a99aff4ef31e3fddfdb1ca84d89379336abdab4e76e94adfa7e6f8','Stdshader pin')
    si,shared=check_ranges(std,'stdshader_dx9-disassembly.txt',[
        ('predicate_bump_virtual_slot17',0x1005446d,0x10054487),
        ('initialization_default_virtual_slot17',0x100544c3,0x100544e8),
        ('force_vertexfog_registration',0x10085e00,0x10085e22),
        ('shared_prefix_force_vertexfog_operand',0x10054746,0x10054770)])
    require(si.string(0x100945ec)=='mat_force_vertexfog','Shared prefix CVar name')
    exes=[]
    for tag,machine in [('native',0x14c),('owned',0x8664)]:
        name='material_defined_'+tag+('_x86.exe' if tag=='native' else '_x64.exe')
        path=OUT/name;raw=path.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine
                and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if tag=='native' else 0x20b),'Architecture '+tag)
        log=(OUT/(tag+'-build.log')).read_text();require(not re.search(r'\b(?:warning|error)\b',log,re.I),'Build diagnostic '+tag)
        exes.append(dict(path=str(path),sha256=sha(path),machine=hex(machine)))
    query=run/'types.bin';query.write_bytes(bytes(range(256)));outputs=[]
    for tag,exe in zip(['native','owned'],exes):
        target=run/(tag+'.bin');result=subprocess.run([exe['path'],str(PRIVATE),str(query),str(target)],
            cwd=ROOT,capture_output=True,text=True,timeout=60)
        (run/(tag+'.log')).write_text(result.stdout+result.stderr)
        require(result.returncode==0 and 'PASS material_defined_types=256' in result.stdout,'Type oracle '+tag)
        outputs.append(target.read_bytes())
    require(len(outputs[0])==1024 and outputs[0]==outputs[1],'Complete IsDefined native/owned output')
    values=struct.unpack('<256I',outputs[0]);require(values==tuple(int((t&15)!=6) for t in range(256)),'Independent type-byte expectation')
    # Rebuild is deliberately separate; run the freshly built affected suite.
    python=Path('C:/Users/wigwo/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe')
    regression=subprocess.run([str(python),str(ROOT/'work/verify_shader_skin_material_predicate.py')],
        cwd=ROOT,capture_output=True,text=True,timeout=120)
    (run/'predicate-regression.log').write_text(regression.stdout+regression.stderr)
    require(regression.returncode==0,'Predicate regression '+regression.stderr)
    summary=json.loads(regression.stdout.strip().splitlines()[-1]);predicate_receipt=Path(summary['receipt'])
    p=json.loads(predicate_receipt.read_text());require(p['status']=='PASS' and p['predicate_cases']==59648
        and p['analytical_checks']==273,'Updated predicate acceptance')
    old_dir=ROOT/'outputs/shader-skin-material-predicate-20261025/verification-mk5rusk1'
    same_queries=sha(old_dir/'queries.bin')==sha(predicate_receipt.parent/'queries.bin')
    same_native=sha(old_dir/'native_x86.bin')==sha(predicate_receipt.parent/'native_x86.bin')
    require(same_queries and same_native,'Contract correction changed existing numeric decisions/trace')
    for name,pin in p['originals_sha256'].items():require(sha(name)==pin,'Original changed '+name)
    require(sha(LIB)==sha(PRIVATE)==PIN,'Original/private material library changed')
    old_init=ROOT/'outputs/phong-material-init-20261025/completion-check.json'
    init=json.loads(old_init.read_text())
    for name,pin in init['sources_sha256'].items():require(sha(ROOT/'work'/name)==pin,'Existing initializer changed '+name)
    require(sha(init['editor']['path'])==init['editor']['sha256'],'Editor changed')
    tracked=['material_var_defined_candidate.h','probe_material_var_defined.cpp',
             'build_material_var_defined.cmd','verify_material_var_defined.py']
    r=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),native_type_cases=256,
        exact_type_bytes=1024,undefined_type_bytes=sum(v==0 for v in values),
        defined_nontexture_examples=[0,1,2,4],predicate_cases=p['predicate_cases'],
        predicate_exact_bytes=p['exact_decision_and_trace_bytes'],predicate_checks=273,
        previous_predicate_queries_and_outputs_unchanged=True,predicate_receipt=str(predicate_receipt),
        predicate_receipt_sha256=sha(predicate_receipt),vtable=dict(address=hex(table),
        rtti_locator=hex(locator),type_descriptor=hex(descriptor),class_name='CMaterialVar',
        slot17=hex(image.word(table+68)),name='IsDefined',rule='(typeByte & 15) != 6'),
        static_ranges=evidence+shared,static_instruction_bytes=sum(x['bytes'] for x in evidence+shared),
        shared_init_prerequisite=dict(cvar='mat_force_vertexfog',registration_object='0x100cb038',
            operand='0x100cb054',effect='If flags_defined bit31 is clear and CVar integer nonzero, set flags bit31',
            status='BYTE_CHECKED; full shared routine not yet executed'),
        executables=exes,sources_sha256={n:sha(ROOT/'work'/n) for n in tracked},
        predicate_sources_sha256=p['sources_sha256'],query_sha256=sha(query),
        native_output_sha256=sha(run/'native.bin'),owned_output_sha256=sha(run/'owned.bin'),
        original_materialsystem_sha256=PIN,originals_unchanged=True,prior_phong_init_unchanged=True,
        editor=init['editor'],computer_use=False,sfm_process_used=False,dll_loader_used=False,gpu_used=False,
        sdk_context_url='https://raw.githubusercontent.com/ValveSoftware/source-sdk-2013/master/src/public/materialsystem/imaterialvar.h',
        limitations='Corrects an inaccurate IsTexture name/contract, not existing boolean decision arithmetic. '
        'Native IsDefined method/RTTI/vtable and all256 bytes verified; complete eligibility predicate rerun. '
        'Does not establish full shared initialization, real typed benchmark construction, texture loading, '
        'active shader choice, original setter semantics, GPU shading or production integration.')
    (run/'results.json').write_text(json.dumps(r,indent=2)+'\n')
    print(json.dumps(dict(status='PASS',receipt=str(run/'results.json'),native_type_cases=256,
        predicate_cases=59648,predicate_checks=273,previous_numeric_results_unchanged=True)),flush=True)

if __name__=='__main__':main()
