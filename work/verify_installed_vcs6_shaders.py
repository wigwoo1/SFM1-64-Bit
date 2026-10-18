from pathlib import Path
import json,hashlib,struct,ctypes,uuid,lzma
from ctypes import wintypes
from vcs6_shader_cache_candidate import Vcs6ShaderCache
root=Path(__file__).resolve().parent.parent;installed=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\shaders\fxc');selector_checks=0;results=[];directory=root/'outputs'/('vcs6-disassembly-'+uuid.uuid4().hex);directory.mkdir();compiler=ctypes.WinDLL('d3dcompiler_47.dll');disassemble=compiler.D3DDisassemble;disassemble.argtypes=[ctypes.c_void_p,ctypes.c_size_t,wintypes.UINT,ctypes.c_char_p,ctypes.POINTER(ctypes.c_void_p)];disassemble.restype=ctypes.c_long;disassembled={}
def validate_shader(shader):
 digest=hashlib.sha256(shader).hexdigest()
 if digest in disassembled:return
 memory=ctypes.create_string_buffer(shader);blob=ctypes.c_void_p();result=disassemble(memory,len(shader),0,None,ctypes.byref(blob))
 if result<0 or not blob.value:raise ValueError('D3DDisassemble rejected bytecode: '+hex(result&0xffffffff))
 table=ctypes.cast(blob,ctypes.POINTER(ctypes.POINTER(ctypes.c_void_p))).contents
 get_buffer=ctypes.WINFUNCTYPE(ctypes.c_void_p,ctypes.c_void_p)(table[3]);get_size=ctypes.WINFUNCTYPE(ctypes.c_size_t,ctypes.c_void_p)(table[4]);release=ctypes.WINFUNCTYPE(wintypes.ULONG,ctypes.c_void_p)(table[2])
 try:text=ctypes.string_at(get_buffer(blob),get_size(blob)).rstrip(b'\0').decode('utf-8');(directory/(digest+'.asm')).write_text(text);disassembled[digest]=True
 finally:release(blob)

for path in sorted(installed.glob('unlitgeneric*.vcs')):
 data=path.read_bytes();cache=Vcs6ShaderCache(data);count=sum(len(records) for records in cache.payloads.values());aliases=0
 for records in cache.payloads.values():
  for shader in records.values():validate_shader(shader)
 for static in list(cache.payloads)+list(cache.aliases):
  for dynamic,shader in cache.payloads[cache.aliases.get(static,static)].items():
   for offset in range(cache.dynamic_combos):assert cache.shader_for_selector(static*cache.dynamic_combos+offset,dynamic)==shader;selector_checks+=1
 for key,target in cache.aliases.items():
  for dynamic,shader in cache.payloads[target].items():assert cache.shader(key,dynamic)==shader;aliases+=1
 results.append({'file':str(path),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'total_combos':cache.total_combos,'dynamic_combos':cache.dynamic_combos,'static_payloads':len(cache.payloads),'static_aliases':len(cache.aliases),'decoded_shader_records':count,'alias_shader_checks':aliases,'shader_versions':sorted(set(hex(struct.unpack_from('<I',shader)[0]) for records in cache.payloads.values() for shader in records.values()))})
 assert path.read_bytes()==data
assert results
fixture=(installed/'unlitgeneric_vs20.vcs').read_bytes();invalids=[fixture[:length] for length in [0,4,27,39,len(fixture)-1]]
for offset,value in [(0,7),(8,0),(20,0),(32,0),(28,0xffffffff),(100,0xffffffff),(108,0xffffffff),(168,0x80000001),(172,0),(176,0xffffffff),(180,0xffffffff)]:
 malformed=bytearray(fixture);struct.pack_into('<I',malformed,offset,value);invalids.append(bytes(malformed))
for malformed in invalids:
 try:Vcs6ShaderCache(malformed)
 except (ValueError,lzma.LZMAError):pass
 else:raise AssertionError('malformed cache accepted')
report={'D3DDisassemble_unique_programs':len(disassembled),'malformed_cache_rejections':len(invalids),'selector_argument_lookup_checks':selector_checks,'disassembly_directory':str(directory),'caches':results,'decoded_shader_records':sum(item['decoded_shader_records'] for item in results),'installation_unchanged':True,'limits':'Observed VCS6 LZMA layout and bytecode envelopes. Native cache loader/combo parity, GPU shaders and other cache versions/encodings remain unverified.'};(root/'outputs/sfm64-installed-vcs6-shader-cache-validation.json').write_text(json.dumps(report,indent=2)+'\n');print('VCS6_caches='+str(len(results))+' decoded_shader_records='+str(report['decoded_shader_records']))
