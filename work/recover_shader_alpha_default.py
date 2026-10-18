from pathlib import Path
from pe_image import Image
import struct,json,hashlib
path=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll');p=Image(path)
def verify(address,hexbytes):
 b=bytes.fromhex(hexbytes);assert p.b[p.offset(address):p.offset(address)+len(b)]==b,(hex(address),hexbytes)
 return {'address':hex(address),'bytes':b.hex()}
assert p.string(0x1016e0ec)=='.?AVCShaderShadowDX8@@'
assert p.word(0x1016ac78+12)==0x1016e0e4
assert p.word(0x100febc0)==0x1016ac78
assert p.word(0x100febc4)==0x1002caa0
assert p.word(0x100febc4+13*4)==0x1002ce60
assert p.string(0x100fec70)=='AlphaFunc: invalid param\n'
records=[verify(0x1002cb25,'f3 0f 10 05 50 ec 0f 10'),verify(0x1002cb2d,'8b 50 34'),verify(0x1002cb30,'51 f3 0f 11 04 24'),verify(0x1002cb36,'6a 06 8b ce ff d2'),verify(0x1002cea8,'f3 0f 10 45 0c'),verify(0x1002cead,'f3 0f 59 05 60 c7 0f 10'),verify(0x1002ceb5,'89 41 20'),verify(0x1002ceb8,'f3 0f 2c c0'),verify(0x1002cebc,'89 41 24'),verify(0x1002cec3,'68 70 ec 0f 10')]
reference=struct.unpack_from('<f',p.b,p.offset(0x100fec50))[0];scale=struct.unpack_from('<f',p.b,p.offset(0x100fc760))[0]
assert scale==255.0 and p.b[p.offset(0x100fec50):p.offset(0x100fec50)+4]==bytes.fromhex('3333333f')
case=p.word(0x1002ced8+6*4);assert case==0x1002ce9c;records.append(verify(case,'b8 07 00 00 00'))
records.append(verify(0x1002ce63,'8b 45 08 83 f8 07 77 58'))
for index in range(8):
 target=p.word(0x1002ced8+index*4);records.append(verify(target,'b8 '+struct.pack('<I',index+1).hex()))
assert p.word(0x100febc4+12*4)==0x1002ce40
records.append(verify(0x1002ce43,'8a 45 08 c0 e0 06 32 41 6c 24 40 30 41 6c'))
records.append(verify(0x1002cadd,'8b 50 30 57 8b ce ff d2'))
records.append(verify(0x1002d5a8,'c0 e8 06 24 01 74 0e 8b 56 20 89 56 50 8b 56 24 89 56 54 eb 25'))
records.append(verify(0x1002d5bd,'ba 07 00 00 00 89 56 50 c7 46 54 00 00 00 00'))
records.append(verify(0x1002d5cc,'84 c9 74 12 83 7e 38 05 75 0c 83 7e 3c 06 75 06 89 56 50 89 7e 54'))
records.append(verify(0x1002d54c,'8a 46 6c 8a c8 c0 e9 05 80 e1 01'))
records.append(verify(0x1002d557,'ba 02 00 00 00 74 17'))
records.append(verify(0x1002d570,'8d 7a ff eb 0e'))
records.append(verify(0x1002d514,'8b 4e 04 8b 01 8b 50 04 57 ff d2 85 c0 7e 29'))
records.append(verify(0x1002d523,'bb a8 ff ff ff 8d 7e 58 2b de 8d 49 00'))
records.append(verify(0x1002d530,'8a 47 30 32 07 47 24 01 30 47 ff'))
records.append(verify(0x1002d53b,'8b 4e 04 8b 11 8b 42 04 ff d0 8d 0c 3b 3b c8 7c e4'))
texture_slots=[slot for slot in range(64) if p.word(0x100febc4+slot*4)==0x1002d1e0];assert len(texture_slots)==1
records.append(verify(0x1002d1e6,'8b 4e 04 8b 01 8b 50 04 ff d2 8b 4d 08 3b c8 7d 18'))
records.append(verify(0x1002d1f7,'8a 84 31 88 00 00 00 32 45 0c 24 01 30 84 31 88 00 00 00'))
blend_mapping=[1,2,9,10,5,6,7,8,11,3,4]
for enum,mapped in enumerate(blend_mapping):
 target=p.word(0x1002cf84+enum*4);records.append(verify(target,'b8 '+struct.pack('<I',mapped).hex()))
records.append(verify(0x1002cfcd,'89 7e 08 5f 89 46 0c'))
records.append(verify(0x1002cffd,'89 7e 14 5f 89 46 18'))
blend_slots={hex(address):[slot for slot in range(64) if p.word(0x100febc4+slot*4)==address] for address in [0x1002cfb0,0x1002cfe0]};assert all(len(slots)==1 for slots in blend_slots.values())
operation_slots={hex(address):[slot for slot in range(64) if p.word(0x100febc4+slot*4)==address] for address in [0x1002d010,0x1002d090]};assert all(len(slots)==1 for slots in operation_slots.values())
for table,offset in [(0x1002d074,0x10),(0x1002d0f4,0x1c)]:
 for enum in range(5):
  target=p.word(table+enum*4);records.append(verify(target,'b8 '+struct.pack('<I',enum+1).hex()+' 89 46 '+bytes([offset]).hex()))
for address,code in [(0x1002ce03,'8a 45 08 c0 e0 05 32 41 6c 24 20 30 41 6c'),(0x1002ce23,'8a 41 6d 32 45 08 24 01 30 41 6d'),(0x1002d183,'8a 45 08 02 c0 02 c0 02 c0 32 41 6d 24 08 30 41 6d')]:records.append(verify(address,code))
control_slots={hex(address):[slot for slot in range(64) if p.word(0x100febc4+slot*4)==address] for address in [0x1002ce00,0x1002ce20,0x1002d180]};assert all(len(slots)==1 for slots in control_slots.values())
write_control_slots={hex(address):[slot for slot in range(64) if p.word(0x100febc4+slot*4)==address] for address in [0x1002cd30,0x1002cd50,0x1002cd70,0x1002cd90,0x1002cde0]};assert all(len(slots)==1 for slots in write_control_slots.values())
for address,code in [(0x1002cd33,'8a 45 08 02 c0 32 41 6c 24 02 30 41 6c'),(0x1002cd53,'33 c0 38 45 08 0f 95 c0 89 41 2c'),(0x1002cd73,'8a 45 08 02 c0 02 c0 32 41 6c 24 0c 30 41 6c'),(0x1002cd93,'80 7d 08 00 74 08 83 49 30 07'),(0x1002cde3,'80 7d 08 00 74 08 83 49 30 08')]:records.append(verify(address,code))
assert p.word(0x100febc4+4)==0x1002cc10
records.append(verify(0x1002cc13,'8b 45 08 56 57 8b f9 83 f8 07 0f 87 cf 00 00 00 ff 24 85 10 cd 02 10'))
for address in [0x1002cc38,0x1002cc6a,0x1002cc8e,0x1002ccc0]:
 records.append(verify(address,'8b 0d 58 3b 18 10 8b 01 8b 50 20 ff d2 0f b6 70 3d f7 de 1b f6'))
records.append(verify(0x1002cc4d,'83 e6 03 83 c6 02'))
records.append(verify(0x1002cc7f,'83 e6 03 83 c6 04'))
records.append(verify(0x1002cca3,'83 e6 fd 83 c6 05'))
records.append(verify(0x1002ccd5,'83 e6 fd 83 c6 07'))
misc_slots={hex(address):[slot for slot in range(64) if p.word(0x100febc4+slot*4)==address] for address in [0x1002d110,0x1002d160,0x1002d4d0,0x1002d4f0]};assert all(len(slots)==1 for slots in misc_slots.values())
for address,code in [(0x1002d113,'83 7d 08 01 74 40'),(0x1002d139,'b8 03 00 00 00 89 41 34'),(0x1002d145,'b8 02 00 00 00 89 41 34'),(0x1002d151,'b8 01 00 00 00 89 41 34'),(0x1002d163,'8a 45 08 c0 e0 04 32 41 6c 24 10 30 41 6c'),(0x1002d4d3,'8a 51 6c 32 55 0c 8b 45 08 80 e2 01 30 51 6c 89 41 68'),(0x1002d4f3,'8a 45 08 02 c0 02 c0 32 41 6d 24 04 30 41 6d')]:records.append(verify(address,code))
for address,code in [(0x1002d1a6,'8b 4e 04 8b 01 8b 50 68 ff d2 84 c0 74 15'),(0x1002d1b4,'8a 46 6c 8a 4d 08 24 7f c0 e1 07 0a c1 88 46 6c'),(0x1002d1c9,'80 66 6c 7f'),(0x1002d286,'8b 4e 04 8b 01 8b 50 68 ff d2 84 c0 75 0d'),(0x1002d294,'8b 45 08 80 64 30 58 fd'),(0x1002d2b2,'8a 45 0c 02 c0 32 44 31 58 24 02 30 44 31 58')]:records.append(verify(address,code))
assert p.word(0x100febc4+19*4)==0x1002d1a0 and p.word(0x100febc4+20*4)==0x1002d280
assert p.word(0x100febc4+17*4)==0x1002d3e0 and p.word(0x100febc4+18*4)==0x1002d450
assert p.string(0x100fee9c)=='vs-file %s vs-index %d' and p.string(0x100feeb4)=='ps-file %s ps-index %d'
for address,code in [(0x1002d416,'8b 0d 0c e2 16 10 8b 11 8b 52 0c'),(0x1002d433,'89 5e 78 89 46 70'),(0x1002d486,'8b 0d 0c e2 16 10 8b 11 8b 52 18'),(0x1002d4a3,'89 5e 7c 89 46 74')]:records.append(verify(address,code))
product=struct.unpack('<f',struct.pack('<f',reference*scale))[0];encoded=int(product);assert encoded==178
report={'binary':str(path),'sha256':hashlib.sha256(p.b).hexdigest(),'class_rtti':'CShaderShadowDX8','vtable':hex(0x100febc4),'default_state_function':hex(0x1002caa0),'alpha_func_slot':13,'alpha_func':hex(0x1002ce60),'default_reference_float32':reference,'reference_scale':scale,'default_encoded_reference':encoded,'comparison_argument':6,'mapped_comparison':7,'comparison_interpretation':'SDK ShaderAlphaFunc enum GEQUAL maps to D3D comparison 7; native method mapping verified','enable_method':hex(0x1002ce40),'enable_flag_offset':'this+0x6c','enable_mask':64,'default_enables_alpha_test':False,'aggregate_alpha_function':hex(0x1002d510),'shader_selection_methods':{'shadow_slots':[17,18],'manager_global':'0x1016e20c','manager_slots':[3,6],'formatter':'0x100d75c0','label_buffer_bytes':500,'handle_fields':['0x70','0x74'],'combo_fields':['0x78','0x7c'],'execution_verified':False},'hardware_gated_controls':{'slots':[19,20],'capability_query_slot':26,'slot19_field_mask':'0x6c/0x80','slot20_field_mask':'0x58+stage/0x2','unsupported_behavior':'clear owned bit; slot20 skips hardware-count query'},'raster_misc_methods':{'addresses_and_slots':misc_slots,'interpretation':'field effects verified; complete renderer semantic names not assumed'},'depth_comparison':{'method':'0x1002cc10','slot':1,'field':'0x28','configuration_global':'0x10183b58','configuration_query_slot':8,'configuration_byte':'0x3d','zero_flag_mapping':[1,2,3,4,5,6,7,8],'nonzero_flag_mapping':[1,5,3,7,2,6,4,8]},'write_control_setters':{'methods_and_vtable_slots':write_control_slots,'argument_rules':'depth enable uses bit0; packed raster uses bits0..1; depth/RGB/alpha writes normalize any nonzero byte'},'blend_control_setters':{'methods_and_vtable_slots':control_slots,'primary_mask':32,'separate_mask':1,'coverage_mask':8,'argument_bit':0},'blend_operation_setters':{'mapping':[1,2,3,4,5],'methods_and_vtable_slots':operation_slots,'primary_field':'0x10','separate_field':'0x1c'},'blend_factor_setters':{'mapping':blend_mapping,'methods_and_vtable_slots':blend_slots,'primary_fields':['0x8','0xc'],'separate_fields':['0x14','0x18'],'invalid_enum':'native warns then maps to 2; owned helper rejects before mutation'},'texture_enable_setter':{'address':'0x1002d1e0','vtable_slot':texture_slots[0],'stage_flags_offset':'0x88','mask':1,'native_bound_check':'signed stage < hardware count; negative stages not safe and not exercised','invalid_stage_logging':'not reproduced'},'aggregate_texture_stage_loop':{'hardware_query_vtable_slot':1,'source_offset':'0x88','destination_offset':'0x58','copied_mask':1,'hardware_query_frequency':'initial and after each stage','native_maximum_stages':'not inferred'},'aggregate_output_fields':['this+0x50 comparison','this+0x54 encoded reference'],'disabled_test_fallback':'comparison7/reference0; standard alpha blend source5/destination6 selects reference1','verified_code':records,'limits':'Offline binary evidence for shadow default and AlphaFunc conversion. Full shader call path, native rendered threshold behavior and Vulkan parity not yet verified.'}
Path('outputs/sfm64-native-alpha-default-recovery.json').write_text(json.dumps(report,indent=2)+'\n');print('native_alpha_default_float='+str(reference)+' encoded_reference='+str(encoded)+' verified_spans='+str(len(records)))
