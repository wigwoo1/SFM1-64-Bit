"""Native typed material predicate; authored VMT requests kept separate.

Does not initialize native materials/services or label a live shader selected.
"""
from datetime import datetime,timezone
import hashlib
import itertools
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from verify_ifm_draw_packet import check_ranges
from verify_shader_parameter_registry import decode,Wire

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/shader-skin-material-predicate-20261025'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
SHADER=GAME/'bin/stdshader_dx9.dll'
SHADER_SHA='fb09e2ddc8a99aff4ef31e3fddfdb1ca84d89379336abdab4e76e94adfa7e6f8'
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def require(value,message):
    if not value:raise RuntimeError(message)


def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));before={str(SHADER):sha(SHADER)};require(before[str(SHADER)]==SHADER_SHA,'Shader pin')
    image,evidence=check_ranges(SHADER,'stdshader_dx9-disassembly.txt',[
        ('complete_typed_material_predicate',0x10054420,0x10054487),
        ('draw_predicate_hardware_route_prefix',0x1005a820,0x1005a86f),
        ('mat_phong_registration',0x10085da0,0x10085dc2),
        ('info_bumpmap_index',0x10052f9f,0x10052fa8),
        ('info_diffuse_warp_index',0x1005305e,0x1005306a),
        ('info_phong_and_mask_indices',0x1005308e,0x100530a6),
        ('descriptor_and_following_index_construction',0x10053a50,0x10053a7f),
        ('phong_parameter_initializer',0x10085380,0x1008539e),
        ('warp_parameter_initializer',0x100852e0,0x100852fe),
        ('mask_parameter_initializer',0x100853a0,0x100853be),
        ('bump_parameter_initializer',0x100850c0,0x100850de),
        ('phong_vs20_shadow_selection',0x1003873e,0x1003875c),
        ('phong_vs30_shadow_selection',0x10038b8d,0x10038baf),
        ('phong_ps20b_selection_argument',0x10038ace,0x10038ae6)])
    require(image.string(0x1009ce3c)=='mat_phong','CVar name')
    program_names=[image.string(a) for a in [0x100966b4,0x100964dc,0x100964fc]]
    require(program_names==['phong_vs20','phong_vs30','phong_ps20b'],'Installed Phong program names')
    metadata_dir=ROOT/'outputs/material-effective-parameters-20261017/verification-l11z2le6';metadata_receipt=json.loads((metadata_dir/'results.json').read_text())
    require(metadata_receipt['original_hashes'][str(SHADER)]==SHADER_SHA,'Prior registry binary identity')
    raw=(metadata_dir/'registry-0.bin').read_bytes();require(hashlib.sha256(raw).hexdigest()==metadata_receipt['output_sha256'],'Registry output pin')
    metadata=decode(raw);require(metadata==json.loads((metadata_dir/'shader-metadata.json').read_text()),'Derived metadata identity')
    vertexlit=next(r for r in metadata if r['name']=='VertexLitGeneric')
    # The global index immediately follows each CRT-initialized 20-byte
    # parameter descriptor; native info helper copies that exact index field.
    fields=[('$phong',48,0xa8,0x100caad8,0x10085380),('$lightwarptexture',43,0x8c,0x100ca9b8,0x100852e0),('$basemapalphaphongmask',49,0xac,0x100cae20,0x100853a0),('$bumpmap',26,0x3c,0x100caee0,0x100850c0)]
    field_links=[]
    for name,index,offset,global_index,initializer in fields:
        param=vertexlit['parameters'][index];require(param['name'].lower()==name and image.base+param['rva']+20==global_index,'Installed parameter link')
        require(image.string(image.word(initializer+15)).lower()==name and image.word(initializer+20)==image.base+param['rva'],'Initializer name/descriptor link')
        field_links.append(dict(name=name,parameter_index=index,info_offset=hex(offset),global_index=hex(global_index),metadata_descriptor_rva=hex(param['rva']),initializer=hex(initializer)))
    binaries=[]
    for tag,name,machine in [('native','skin_material_native_x86',0x14c),('owned','skin_material_owned_x64',0x8664),('test','skin_material_test_x64',0x8664),('audit','skin_material_requests_x64',0x8664)]:
        log=(OUT/(tag+'-build.log')).read_text();require('warning ' not in log.lower() and 'error ' not in log.lower(),'Build diagnostics '+tag);path=OUT/(name+'.exe');b=path.read_bytes();pe=struct.unpack_from('<I',b,60)[0]
        require(b[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==(0x10b if machine==0x14c else 0x20b),'Executable architecture');binaries.append(dict(path=str(path),sha256=sha(path),machine=hex(machine)))
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,name+' '+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout
    test=execute('skin_material_test_x64',[],'test');require('checks=273 typed_effective_inputs_only=1' in test,'Analytical and full type-byte coverage')
    rows=[]
    for flags,global_phong,phong,low,high,mask,bump in itertools.product(range(16),[-1,0,1],[-1,0,1],range(16),[0,0x10,0x80,0xf0],[0,1,2],range(2)):
        rows.append(struct.pack('<6I',flags,global_phong&0xffffffff,phong&0xffffffff,low|high,mask,bump))
    rows.extend(struct.pack('<6I',15,1,1,typ,0,0) for typ in range(256))
    rng=random.Random(2026102511)
    for _ in range(4096):rows.append(struct.pack('<6I',rng.randrange(16),rng.getrandbits(32),rng.getrandbits(32),rng.randrange(256),rng.getrandbits(32),rng.randrange(2)))
    query=run/'queries.bin';query.write_bytes(b''.join(rows));outputs=[]
    for tag in ['native_x86','owned_x64']:
        path=run/(tag+'.bin');execute('skin_material_'+tag,[SHADER,query,path],tag);outputs.append(path.read_bytes())
    require(outputs[0]==outputs[1] and len(outputs[0])==len(rows)*8,'Predicate decision/defined-query trace mismatch')
    decisions=[struct.unpack_from('<2I',outputs[0],i*8) for i in range(len(rows))];require(all(a<=1 and b<=1 for a,b in decisions),'Predicate trace domain')
    materials=[]
    for i,relative in enumerate(['tf_movies/materials/models/player/pyro/pyro_blue.vmt','usermod/materials/models/player/pyro/pyro_head.vmt','tf/materials/models/weapons/c_items/c_flamethrower_blue.vmt']):
        path=GAME/relative;before[str(path)]=sha(path);output=run/('authored-'+str(i)+'.bin');execute('skin_material_requests_x64',[path,output],'authored-'+str(i));w=Wire(output.read_bytes());shader=w.text();values={}
        for _ in range(4):
            key=w.text();values[key]=[w.text() for _ in range(w.number())]
        w.end();require(shader.lower()=='vertexlitgeneric' and values['$phong']==['1'],'Benchmark authored request changed')
        materials.append(dict(path=str(path),sha256=before[str(path)],shader=shader,authored=values,effective_typed_values='UNVERIFIED',active_draw_family='UNVERIFIED'))
    for path,digest in before.items():require(sha(path)==digest,'Original changed')
    tracked=['material_var_defined_candidate.h','shader_skin_material_predicate_candidate.h','probe_shader_skin_material_predicate.cpp','test_shader_skin_material_predicate.cpp','audit_skin_material_requests.cpp','build_shader_skin_material_predicate.cmd','verify_shader_skin_material_predicate.py']
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),predicate_cases=len(rows),exact_decision_and_trace_bytes=len(outputs[0]),wants_skin_cases=sum(a for a,_ in decisions),bump_query_cases=sum(b for _,b in decisions),analytical_checks=273,
        installed_parameter_links=field_links,installed_program_names=program_names,authored_materials=materials,executables=binaries,static_ranges=evidence,sources_sha256={name:sha(ROOT/'work'/name) for name in tracked},originals_sha256=before,originals_unchanged=True,
        metadata_receipt=str(metadata_dir/'results.json'),metadata_output_sha256=metadata_receipt['output_sha256'],computer_use=False,sfm_process_used=False,gpu_used=False,editor_changed=False,
        limitations='Native eligibility predicate exact with explicit effective typed inputs and fixture mat_phong/bump IsDefined. Historical IsTexture label corrected: slot17 accepts every type except low-nibble UNDEFINED6. Warp direct type3 gate remains distinct. Caller hardware>=92 gate byte-checked, not full draw execution. Current VMT authored requests verified separately; effective loaded material values, active family/VS20/VS30/combo, native lifecycle, GPU output and editor integration remain unverified. Raw VMT text alone does not prove effective typed state.')
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(status='PASS',receipt=str(run/'results.json'),predicate_cases=len(rows),exact_decision_and_trace_bytes=len(outputs[0]),materials=materials)))


if __name__=='__main__':main()
