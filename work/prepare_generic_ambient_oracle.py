from pathlib import Path
s=Path('work/probe_generic_shader_combo.cpp').read_text().replace('generic_shader_combo_candidate.h','generic_ambient_state_candidate.h').replace('stdshader_dx9.dll','shaderapidx9.dll')
a=s.index(' unsigned cases=0;');b=s.index('#else',a)
s=s[:a]+''' unsigned cases=0;std::array<uint32_t,86> record;
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  GenericAmbientStateCandidate state;std::copy_n(record.begin(),24,state.source.begin());std::copy_n(record.begin()+24,18,state.packed.begin());state.control=static_cast<uint8_t>(record[42]);
  std::array<uint32_t,24> supplied;std::copy_n(record.begin()+43,24,supplied.begin());state.set(supplied);
  if(state.source!=supplied||!std::equal(state.packed.begin(),state.packed.end(),record.begin()+67)||state.control!=record[85])throw std::runtime_error("ambient mismatch");
  auto packed=state.packed;auto control=state.control;if(state.set(supplied)||state.packed!=packed||state.control!=control)throw std::runtime_error("ambient repeat");++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 std::cout<<"x64_native_ambient_states_exact="<<cases<<'\\n';
'''+s[b:]
a=s.index(' // No DLL entrypoint');b=s.index('#endif',a)
s=s[:a]+''' // Original setter only, synthetic state, masked x87 exceptions.
 using Set=void(__thiscall*)(void*,const void*);auto set=reinterpret_cast<Set>(0x10010c40);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");
 uint32_t seed=0x194782ab;const uint32_t boundaries[]={0,0x80000000u,1,0x007fffffu,0x00800000u,0x7f800000u,0xff800000u,0x7f800001u,0xff800001u,0x7fc12345u,0xffffffffu};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,86> record{};
  for(unsigned j=0;j<67;++j){seed=seed*1664525u+1013904223u;record[j]=seed;}
  record[42]&=255;
  if(i<264)for(unsigned j=0;j<24;++j)record[43+j]=boundaries[(i+j)%11];
  if(i%4==0)std::copy_n(record.begin(),24,record.begin()+43);
  std::array<uint8_t,0x1800> object{};std::memcpy(object.data()+0x11b8,record.data(),96);std::memcpy(object.data()+0x13dc,record.data()+24,72);object[0x11b6]=static_cast<uint8_t>(record[42]);
  set(object.data(),record.data()+43);
  if(std::memcmp(object.data()+0x11b8,record.data()+43,96))throw std::runtime_error("source copy");
  std::memcpy(record.data()+67,object.data()+0x13dc,72);record[85]=object[0x11b6];
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_ambient_states=65536\\n";
'''+s[b:]
Path('work/probe_generic_ambient.cpp').write_text(s)
Path('work/build_generic_ambient.cmd').write_text(Path('work/build_generic_shader_combo.cmd').read_text().replace('probe_generic_shader_combo','probe_generic_ambient'))

