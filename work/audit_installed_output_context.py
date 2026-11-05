"""Read-only installed IFM output-context evidence. No original code execution."""
from pathlib import Path
import hashlib,json,re,struct,subprocess
from pe_image import Image
from audit_source_inline_numbering import DUMP,FILES,PIN
ROOT=Path(__file__).resolve().parent.parent
RANGES=[('session_render_settings_lookup',0x1028bee0,0x1028bf31),
 ('session_tone_getter',0x1028c4e0,0x1028c548),
 ('camera_tone_attribute_init',0x104ca28f,0x104ca2bf),
 ('camera_tone_getter',0x104ca720,0x104ca727),
 ('session_camera_scale_product',0x1029f55e,0x1029f5bb),
 ('first_target_setup_call',0x1029f612,0x1029f640),
 ('second_target_setup_call',0x1029f6c3,0x1029f705),
 ('target_mode_getter',0x102bc440,0x102bc447),
 ('target_setup_complete',0x102bf180,0x102bf220),
 ('target_pop_complete',0x102bf220,0x102bf272),
 ('tone_cvar_registration',0x10a69840,0x10a69865),
 ('tone_cvar_constructor',0x1097d0e0,0x1097d147),
 ('tone_float_setter_wrapper',0x1097c8e0,0x1097c8fc),
 ('post_material_parameter_block',0x1022b72c,0x1022b943)]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(x,s):
    if not x:raise RuntimeError(s)
def audit(directory):
    directory=Path(directory);directory.mkdir(exist_ok=False);im=Image(FILES['ifm']);need(sha(FILES['ifm'])==PIN['ifm'],'installed IFM identity')
    evidence=[];rowsByVa={}
    for label,a,b in RANGES:
        p=subprocess.run([str(DUMP),'/disasm:bytes',f'/range:0x{a:X},0x{b-1:X}',str(FILES['ifm'])],capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW)
        (directory/(label+'.txt')).write_text(p.stdout,encoding='utf-8');rows=[];pending=None
        for line in p.stdout.splitlines():
            m=re.match(r'\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2} )+)(.*)',line)
            if m:
                va=int(m[1],16);pending=None
                if a<=va<b:pending=dict(address=va,bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());rows.append(pending)
            elif pending and re.fullmatch(r'\s+(?:[0-9A-F]{2} ?)+',line):pending['bytes']+=bytes.fromhex(line).hex()
        cursor=a
        for row in rows:
            raw=bytes.fromhex(row['bytes']);need(row['address']==cursor,'instruction gap '+label);need(im.b[im.offset(cursor):im.offset(cursor)+len(raw)]==raw,'instruction identity '+label)
            rowsByVa[cursor]=row['instruction'];cursor+=len(raw)
        need(cursor==b,'incomplete instruction span '+label);evidence.append(dict(label=label,start=hex(a),end_exclusive=hex(b),instructions=rows))
    strings={hex(a):im.string(a) for a in [0x10a8a6c4,0x10ae44a8,0x10ae45d4,0x10aebb30,0x10ad8c08,0x10ad8170,0x10ad8060,0x10ad803c,0x10ad8048,0x10a95a68,0x10a927f0]}
    need(strings['0x10a8a6c4']=='settings' and strings['0x10ae44a8']=='renderSettings' and strings['0x10ae45d4']=='toneMapScale','attribute names')
    need(im.word(0x10a8a56c)==0x3f800000,'default scale one')
    expected={0x104ca2a0:'push        10AE45D4h',0x104ca2ba:'call        1036D040',
        0x104ca720:'fld         dword ptr [ecx+000000E0h]',
        0x1029f5a8:'call        104CA720',0x1029f5ad:'fmul        dword ptr [ebp+FFFFFEFCh]',
        0x1029f5b5:'fstp        dword ptr [ebp+FFFFFEFCh]',
        0x102bf1c2:'mov         ecx,10F32728h',0x102bf1cc:'call        1097C8E0',
        0x10a6984a:'push        10AEBB30h',0x10a6984f:'mov         ecx,10F32710h',
        0x1097d109:'mov         dword ptr [esi+18h],10DDA874h',
        0x102bf1dd:'mov         ecx,dword ptr [edi+00000440h]',0x102bf1f1:'mov         ecx,dword ptr [edi+00000490h]',
        0x102bf1fd:'mov         edx,dword ptr [eax+000001A8h]',0x1029f6ce:'cmp         eax,2',
        0x1022b73a:'push        10AD8C08h',0x1022b8c2:'push        10A95A68h',0x1022b8c7:'push        10AD8060h',
        0x1022b8d3:'push        10A927F0h',0x1022b8d8:'push        10AD803Ch'}
    for va,want in expected.items():need(rowsByVa[va]==want,'semantic instruction anchor '+hex(va))
    need(strings['0x10ad8c08']=='Engine_Post_dx9' and strings['0x10ad8060']=='$TOOLMODE' and strings['0x10ad803c']=='$TV_GAMMA' and strings['0x10a95a68']=='1' and strings['0x10a927f0']=='0','post material parameters')
    need(sha(FILES['ifm'])==PIN['ifm'],'original modified during audit')
    result=dict(status='VERIFIED_STATIC_CONTEXT; final target format/post transform UNVERIFIED',original=dict(path=str(FILES['ifm']),sha256=PIN['ifm'],user_intentionally_modified_installation=True),
        instruction_count=sum(len(v['instructions']) for v in evidence),strings=strings,default_scale_bits='0x3f800000',evidence=evidence,
        findings=['Session settings->renderSettings toneMapScale is read with fallback1.',
                  'Camera toneMapScale registration initializes its float payload at +0xe0 to1; getter loads that float.',
                  'The observed caller multiplies session/render-setting scale by active camera getter with x87 fmul and rounds to float storage.',
                  'Target setup receives the product and, in its first route, invokes a float setter wrapper on the registered mat_force_tonemap_scale object secondary interface at base+0x18.',
                  'Target setup chooses separate owner fields+0x440/+0x490 according to its final stage argument; target formats/allocations are not identified by this audit.',
                  'A separate material-construction block creates Engine_Post_dx9 with explicit TOOLMODE1 and TV_GAMMA0. This does not by itself prove the final draw or gamma curve.'],
        limits=['Static bytes/call arguments only; no original native execution, original process, CUA or target creation.',
                'IFM is the intentionally modified installed version, not claimed pristine Valve distribution.',
                'No full viewport call graph, live configuration, shader selection, render target format or final display transfer proved.',
                'Earlier exploratory ranges include unrelated objects with matching offsets; only the authenticated ranges above support these findings.'])
    (directory/'evidence.json').write_text(json.dumps(result,indent=2)+'\n');return result
if __name__=='__main__':
    parent=Path((ROOT/'outputs/native-output-context-current.txt').read_text().strip());r=audit(parent/'authenticated');print(json.dumps(dict(status=r['status'],instructions=r['instruction_count'],evidence=str(parent/'authenticated/evidence.json')),indent=2))
