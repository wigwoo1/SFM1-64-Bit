from pathlib import Path
s=Path('work/probe_generic_shader_combo.cpp').read_text()
a=s.index(' unsigned cases=0;');b=s.index('#else',a)
s=s[:a]+''' unsigned cases=0;std::array<uint32_t,3> record;
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  int32_t count;std::memcpy(&count,&record[0],4);
  if(genericHasDynamicLightCandidate(count,static_cast<uint8_t>(record[1]))!=bool(record[2]))throw std::runtime_error("light predicate mismatch");++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 // Signed light-count bits must remain intact in NUM_LIGHTS, even when outside
 // the as-yet-unrecovered legal rendering domain.
 for(int32_t count : {INT32_MIN,-1,0,1,INT32_MAX})for(int32_t bones : {-1,0,1})for(uint8_t flow : {uint8_t(0),uint8_t(1),uint8_t(255)}){
  GenericVs20DynamicInputsCandidate inputs{1,count,255,bones,flow,2};
  auto fields=genericVs20DynamicFieldsCandidate(inputs);
  if(fields[0]!=1||fields[1]!=1||fields[2]!=uint32_t(bones>0)||fields[3]!=0||fields[4]!=(flow?0:static_cast<uint32_t>(count))||fields[5]!=1)throw std::runtime_error("scoped dynamic fields");
 }
 std::cout<<"x64_native_light_predicates_exact="<<cases<<" scoped_field_cases=45\\n";
'''+s[b:]
a=s.index(' // No DLL entrypoint');b=s.index('#endif',a)
s=s[:a]+''' // Original pure predicate only; no interface calls or DLL initialization.
 using Predicate=uint32_t(__thiscall*)(const void*);
 auto predicate=reinterpret_cast<Predicate>(0x10018160);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");
 uint32_t seed=0x194782ab;const int32_t boundaries[]={INT32_MIN,-1,0,1,INT32_MAX};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,3> record{};
  seed=seed*1664525u+1013904223u;record[0]=seed;
  if(i<1280)std::memcpy(&record[0],&boundaries[i%5],4);
  record[1]=i<1280?i/5:i&255u;
  std::array<uint8_t,8> state{};std::memcpy(state.data(),&record[0],4);state[4]=static_cast<uint8_t>(record[1]);
  auto before=state;record[2]=predicate(state.data());if(state!=before)throw std::runtime_error("predicate mutation");
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_light_predicates=65536\\n";
'''+s[b:]
Path('work/probe_generic_light_predicate.cpp').write_text(s)
Path('work/build_generic_light_predicate.cmd').write_text(Path('work/build_generic_shader_combo.cmd').read_text().replace('probe_generic_shader_combo','probe_generic_light_predicate'))
