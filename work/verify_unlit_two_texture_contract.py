"""Read-only extraction of installed UnlitTwoTexture programs and evidence."""
from pathlib import Path
import ctypes
from ctypes import wintypes
import hashlib
import json
import re
import struct
import tempfile
from vcs6_shader_cache_candidate import Vcs6ShaderCache
from pe_image import Image

root=Path(__file__).resolve().parent.parent
output=root/'outputs/unlit-two-texture-contract-20261014'
output.mkdir(exist_ok=True)
directory=Path(tempfile.mkdtemp(prefix='native-',dir=output))
installed=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\shaders\fxc')
compiler=ctypes.WinDLL('d3dcompiler_47.dll')
disassemble=compiler.D3DDisassemble
disassemble.argtypes=[ctypes.c_void_p,ctypes.c_size_t,wintypes.UINT,ctypes.c_char_p,ctypes.POINTER(ctypes.c_void_p)]
disassemble.restype=ctypes.c_long
records=[];contracts=[];unique=set()
for name in ('unlittwotexture_vs20.vcs','unlittwotexture_ps20.vcs','unlittwotexture_ps20b.vcs'):
 path=installed/name;source=path.read_bytes();cache=Vcs6ShaderCache(source)
 programs=[]
 for static,payload in sorted(cache.payloads.items()):
  for dynamic,shader in sorted(payload.items()):
   memory=ctypes.create_string_buffer(shader);blob=ctypes.c_void_p()
   result=disassemble(memory,len(shader),0,None,ctypes.byref(blob))
   if result<0 or not blob.value:raise RuntimeError('D3DDisassemble rejected '+name)
   table=ctypes.cast(blob,ctypes.POINTER(ctypes.POINTER(ctypes.c_void_p))).contents
   get_buffer=ctypes.WINFUNCTYPE(ctypes.c_void_p,ctypes.c_void_p)(table[3])
   get_size=ctypes.WINFUNCTYPE(ctypes.c_size_t,ctypes.c_void_p)(table[4])
   release=ctypes.WINFUNCTYPE(wintypes.ULONG,ctypes.c_void_p)(table[2])
   try:assembly=ctypes.string_at(get_buffer(blob),get_size(blob)).rstrip(b'\0').decode('utf-8')
   finally:release(blob)
   stem=f'{Path(name).stem}-s{static}-d{dynamic}'
   (directory/(stem+'.asm')).write_text(assembly)
   (directory/(stem+'.bin')).write_bytes(shader)
   unique.add(hashlib.sha256(shader).hexdigest())
   instructions=[re.sub(r'\s+',' ',line.strip()) for line in assembly.splitlines()
                 if line.strip() and not line.lstrip().startswith('//')]
   if name=='unlittwotexture_vs20.vcs':
    # Skinning variants move the declared UV input from v1 to v3.
    uv=[match.group(1) for line in instructions
        if (match:=re.fullmatch(r'dcl_texcoord (v\d+)',line))]
    if len(uv)!=1:raise RuntimeError('native UV input declaration changed: '+stem)
    required=[f'dp4 oT0.x, {uv[0]}, c48',f'dp4 oT0.y, {uv[0]}, c49',
              f'dp4 oT1.x, {uv[0]}, c50',f'dp4 oT1.y, {uv[0]}, c51']
    if not all(line in instructions for line in required):
     raise RuntimeError('native independent UV transform contract changed: '+stem)
   if name.startswith('unlittwotexture_ps') and static==0 and dynamic==0:
    required=['def c0, 1, 0, 0, 0','texld r0, t0, s0','texld r1, t1, s1',
              'mul r0.xyz, r0, r1','mul r0.xyz, r0, c1','mov r0.w, c0.x','mov oC0, r0']
    if not all(line in instructions for line in required):
     raise RuntimeError('native color product/alpha contract changed: '+stem)
    contracts.append(dict(program=stem,independent_samplers=[0,1],rgb_product_and_c1_modulation=True,
                          output_alpha=1,light_scale_register='c30',fog_and_final_output='variant-specific'))
   programs.append(dict(static=static,dynamic=dynamic,bytecode_sha256=hashlib.sha256(shader).hexdigest(),
                        disassembly=stem+'.asm'))
 if path.read_bytes()!=source:raise RuntimeError('installed shader changed during read')
 records.append(dict(path=str(path),bytes=len(source),sha256=hashlib.sha256(source).hexdigest(),
  total_combos=cache.total_combos,dynamic_combos=cache.dynamic_combos,
  static_aliases=cache.aliases,programs=programs))
pilot=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\materials\models\effects\pyro')
textures=[]
for name in ('pilotlight.vtf','pilotlight_motion.vtf'):
 data=(pilot/name).read_bytes()
 if data[:4]!=b'VTF\0' or len(data)<64:raise RuntimeError('pilot VTF header')
 textures.append(dict(path=str(pilot/name),sha256=hashlib.sha256(data).hexdigest(),
                      dimensions=struct.unpack_from('<HH',data,16),flags=hex(struct.unpack_from('<I',data,20)[0]),
                      frames=struct.unpack_from('<H',data,24)[0]))
if len(contracts)!=2:raise RuntimeError('normal pixel shader contract selections missing')
host_path=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll')
host=Image(host_path)
if host.string(0x1009ac20)!='UnlitTwoTexture_DX9':raise RuntimeError('host shader name anchor changed')
name_anchor=struct.pack('<I',0x1009ac20)
if host.b[host.offset(0x1004ccc1):host.offset(0x1004ccc1)+4]!=name_anchor or host.word(0x100bbf84)!=0x1009ac20:
 raise RuntimeError('host shader name references changed')
if host_path.read_bytes()!=host.b:raise RuntimeError('installed host shader changed during read')
report=dict(caches=records,checked_pixel_contracts=contracts,independent_vertex_transform_records=
            len(records[0]['programs']),unique_disassembled_programs=len(unique),pilot_textures=textures,
            host_shader_name_anchors=dict(path=str(host_path),sha256=hashlib.sha256(host.b).hexdigest(),
              string_address='0x1009ac20',code_reference='0x1004ccc1',data_reference='0x100bbf84',
              interpretation='name references only; object/vtable draw/blend/depth dispatch unresolved'),
            installation_unchanged=True,native_sfm_process_used=False,
            scope='compiled shader contracts; native host blend/depth state and GPU execution not yet verified')
(directory/'evidence.json').write_text(json.dumps(report,indent=2))
print('UnlitTwoTexture native programs extracted: '+str(sum(len(r['programs']) for r in records)))
print('Evidence: '+str(directory))
