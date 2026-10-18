from pathlib import Path
root=Path(__file__).resolve().parent
source=(root/'probe_generic_shader_combo.cpp').read_text().replace('generic_shader_combo_candidate.h','dynamic_shader_commands_candidate.h').replace('stdshader_dx9.dll','shaderapidx9.dll').replace('#include <algorithm>','#include <algorithm>\n#include <array>')
start=source.index(' unsigned cases=0;')
end=source.index('#else',start)
source=source[:start]+r''' unsigned cases=0;std::array<uint32_t,21> record;
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  auto state=dynamicShaderCommandsCandidate({record[0],record[1]},record.data()+2,7);
  if(state.vertex!=record[9]||state.pixel!=record[10])throw std::runtime_error("original dispatcher state mismatch");++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 for(auto bad:std::vector<std::vector<uint32_t>>{{},{12},{11,1},{0,1},{1,0,0},{17,0}}){bool failed=false;
  try{dynamicShaderCommandsCandidate({123,456},bad.data(),bad.size());}catch(const std::runtime_error&){failed=true;}
  if(!failed)throw std::runtime_error("invalid dynamic stream accepted");}
 std::cout<<"x64_native_dynamic_streams_exact="<<cases<<" invalid_rejected=6\n";
''' +source[end:]
needle=' DWORD oldProtect;'
assert source.count(needle)==1
source=source.replace(needle,r''' std::array<uint8_t,64> manager{},object{};auto managerPointer=manager.data();
 std::memcpy(reinterpret_cast<void*>(0x1016e20c),&managerPointer,4);
 DWORD oldProtect;''')
start=source.index(' // No DLL entrypoint/import initialization:')
end=source.index('#endif',start)
source=source[:start]+r''' // Original dispatcher, restricted to audited commands11/12/0. Manager
 // global points only to owned synthetic storage; no DLL entrypoint runs.
 using Execute=void(__thiscall*)(void*,const uint32_t*);
 auto execute=reinterpret_cast<Execute>(0x10016ac0);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");
 uint32_t seed=0x194782ab;
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,21> record{};
  auto next=[&](){seed=seed*1664525u+1013904223u;return seed;};
  record[0]=next();record[1]=next();
  for(unsigned j=0;j<3;++j){record[2+j*2]=((i>>j)&1)?11:12;record[3+j*2]=next();}
  record[8]=0;manager.fill(0xa5);std::memcpy(manager.data()+4,&record[0],4);std::memcpy(manager.data()+8,&record[1],4);
  auto before=manager;execute(object.data(),record.data()+2);
  for(size_t j=0;j<manager.size();++j)if((j<4||j>=12)&&manager[j]!=before[j])throw std::runtime_error("native command unrelated storage changed");
  std::memcpy(&record[9],manager.data()+4,4);std::memcpy(&record[10],manager.data()+8,4);
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_dynamic_streams=65536\n";
''' +source[end:]
(root/'probe_dynamic_shader_commands.cpp').write_text(source)
build=(root/'build_generic_shader_combo.cmd').read_text().replace('probe_generic_shader_combo','probe_dynamic_shader_commands')
(root/'build_dynamic_shader_commands.cmd').write_text(build)
verify=(root/'verify_generic_shader_combo.py').read_text().replace('stdshader_dx9.dll','shaderapidx9.dll').replace('sfm64-generic-shader-combo-packing-recovery.json','sfm64-native-dynamic-shader-command-recovery.json').replace("recovery['sha256']","recovery['binaries'][str(source)]").replace('generic-combo-','dynamic-commands-').replace('probe_generic_shader_combo','probe_dynamic_shader_commands').replace('sfm64-generic-shader-combo-validation.json','sfm64-dynamic-shader-command-validation.json').replace("'static_word_count':13,'dynamic_word_count':6,","'stream_words':7,'supported_opcodes':[0,11,12],").replace('basis vectors, all-bits basis, low-bit fields and deterministic full uint32 overflow inputs','three selection commands, all eight stage orders, deterministic initial state/operands, unrelated manager storage preservation').replace('Executes only original audited leaf arithmetic','Executes original dispatcher restricted to audited selection commands with a synthetic manager global').replace('No full material draw, valid domain recovery, field ownership, command consumer or renderer parity is proved.','No other opcodes, pointer/nested streams, material draw or GPU binding is proved. Owned parser rejects unsupported/truncated/trailing streams before returning changed state.')
(root/'verify_dynamic_shader_commands.py').write_text(verify)
