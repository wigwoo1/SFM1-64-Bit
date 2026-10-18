from pathlib import Path
from pe_image import Image
import json,hashlib
root=Path(__file__).resolve().parent.parent;path=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll');p=Image(path);records=[]
def verify(address,code):
 value=bytes.fromhex(code);assert p.b[p.offset(address):p.offset(address)+len(value)]==value,hex(address);records.append({'address':hex(address),'bytes':value.hex()})
assert p.string(0x1016e29c)=='.?AVCShaderManager@@';assert p.word(0x1016adfc+12)==0x1016e294;assert p.word(0x100ff9b0)==0x1016adfc
assert p.word(0x100ff9b4+3*4)==0x10036380;assert p.word(0x100ff9b4+6*4)==0x10036460
verify(0x1003638f,'3b c7 75 0b 5f 83 c8 ff 5b 8b e5 5d c2 0c 00')
verify(0x1003646f,'3b c7 75 0b 5f 83 c8 ff 5b 8b e5 5d c2 0c 00')
verify(0x10036426,'89 4e 04');verify(0x10036506,'89 4e 04')
verify(0x10036446,'e8 95 f5 ff ff');verify(0x10036525,'e8 b6 f4 ff ff')
verify(0x10035ac2,'8d 78 04 89 43 20')
verify(0x10033897,'6a 1c 57 ff d2')
verify(0x1003389c,'8b 07 83 f8 04 74 34 7e 05 83 f8 06 7e 2d')
verify(0x10036028,'8b 46 04 8b 8d 70 fe ff ff 99 f7 79 0c')
verify(0x10036035,'8b 51 68 8b 49 74 52 51 50')
verify(0x10036044,'e8 77 d9 ff ff')
verify(0x10036057,'8b 4a 68 8b 44 c1 04')
verify(0x1003606f,'8b 41 54 8b 57 14')
verify(0x10036084,'e8 37 d9 ff ff')
verify(0x100339d8,'8d 04 37 99 2b c2 d1 f8 8b 4c c1 f8 3b d9 73 05')
verify(0x100339ed,'76 0f 8d 70 01')
report={'binary':str(path),'sha256':hashlib.sha256(p.b).hexdigest(),'manager_rtti':'CShaderManager','vtable':'0x100ff9b4','vertex_selection_method':'0x10036380','pixel_selection_method':'0x10036460','common_cache_loader':'0x100359e0','null_name_handle':'0xffffffff','named_selection_handle':'native record pointer, not a portable x64 pointer or shader bytecode','static_combo_record_field':'record+4','static_combo_normalization':'signed selector combo / cached dynamic count at cached+0xc','alias_search':'0x100339c0; sorted unsigned key/value pairs, stride8','alias_fields':['cached+0x68 table','cached+0x74 count'],'static_table_fields':['cached+0x54 table','header+0x14 count'],'verified_code':records,'limits':'Checked offline instruction evidence joins manager methods to combo normalization and alias/static lookup. Full loader execution, cache field population and GPU shader behavior remain unverified.'};(root/'outputs/sfm64-native-shader-cache-selection-recovery.json').write_text(json.dumps(report,indent=2)+'\n');print('native_shader_cache_selection_verified_spans='+str(len(records)))
