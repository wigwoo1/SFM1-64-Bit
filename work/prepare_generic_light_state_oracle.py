from pathlib import Path
s=Path('work/probe_generic_shader_combo.cpp').read_text().replace('generic_shader_combo_candidate.h','generic_light_state_candidate.h').replace('stdshader_dx9.dll','shaderapidx9.dll')
a=s.index(' unsigned cases=0;');b=s.index('#else',a)
s=s[:a]+''' std::array<uint32_t,18> baseline;
 input.read(reinterpret_cast<char*>(baseline.data()),sizeof(baseline));if(!input)throw std::runtime_error("baseline");
 unsigned cases=0;std::array<uint32_t,99> record;
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  if(record[0]>4)throw std::runtime_error("record count");std::vector<GenericLightRecordCandidate> records;
  for(unsigned j=0;j<record[0];++j){auto p=record.data()+1+j*24;GenericLightRecordCandidate r;r.present=p[0]!=0;r.secondFlagWord=p[1];std::copy_n(p+2,18,r.ambientWords.begin());std::memcpy(&r.lightCount,p+23,4);records.push_back(r);}
  RetainedGenericLightStateCandidate retained;retained.replace(records,baseline);
  auto result=retained.query();auto repeated=retained.query();
  if(retained.aggregations()!=1||repeated.lightCount!=result.lightCount||repeated.firstFlag!=result.firstFlag||repeated.secondFlag!=result.secondFlag)throw std::runtime_error("retained light state");
  retained.invalidate();retained.query();if(retained.aggregations()!=2)throw std::runtime_error("light invalidation");
  retained.replace({},baseline);auto empty=retained.query();if(empty.lightCount||empty.firstFlag||empty.secondFlag||retained.aggregations()!=3)throw std::runtime_error("replacement invalidation");
  uint32_t count;std::memcpy(&count,&result.lightCount,4);
  if(count!=record[97]||uint32_t(result.firstFlag)|(uint32_t(result.secondFlag)<<8) != record[98])throw std::runtime_error("light state mismatch");++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 std::cout<<"x64_native_light_states_exact="<<cases<<'\\n';
'''+s[b:]
a=s.index(' // No DLL entrypoint');b=s.index('#endif',a)
s=s[:a]+''' // Original aggregation over synthetic records; static baseline is read from
 // the private image. No DLL runtime or imports are initialized.
 using Aggregate=void(__thiscall*)(void*);auto aggregate=reinterpret_cast<Aggregate>(0x10014160);
 std::array<uint32_t,18> baseline;std::memcpy(baseline.data(),reinterpret_cast<void*>(0x10180158),sizeof(baseline));
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");output.write(reinterpret_cast<const char*>(baseline.data()),sizeof(baseline));
 uint32_t seed=0x194782ab;
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,99> record{};record[0]=i%5;
  std::array<uint8_t,0x3000> object{};std::array<std::array<uint32_t,19>,4> nativeRecords{};
  for(unsigned j=0;j<4;++j){auto p=record.data()+1+j*24;seed=seed*1664525u+1013904223u;p[0]=(seed>>22)&1u;p[1]=(seed>>23)&1u;std::copy(baseline.begin(),baseline.end(),p+2);
   if((seed>>24)&1u)p[2+(i+j)%18]^=1u<<((i/18+j)%32);
   p[23]=i<1280?std::array<uint32_t,5>{0,1,0xffffffffu,0x80000000u,0x7fffffffu}[(i/5+j)%5]:seed;
   nativeRecords[j][6]=p[0]?reinterpret_cast<uint32_t>(p+2):0;nativeRecords[j][11]=p[1];
  }
  std::memcpy(object.data()+0x2eb8,&record[0],4);auto pointer=reinterpret_cast<uint32_t>(nativeRecords.data());std::memcpy(object.data()+0x2ebc,&pointer,4);
  auto before=record;aggregate(object.data());if(record!=before)throw std::runtime_error("record mutation");
  std::memcpy(&record[97],object.data()+0x1864,4);uint16_t flags;std::memcpy(&flags,object.data()+0x1868,2);record[98]=flags;
  using Query=void(__thiscall*)(void*,void*);auto query=reinterpret_cast<Query>(0x10014260);
  object[0x186c]=0;std::array<uint8_t,8> cold{},warm{},fresh{};
  query(object.data()+0x2d0,cold.data());if(object[0x186c]!=1)throw std::runtime_error("native cache valid");
  uint32_t zero=0;std::memcpy(object.data()+0x2eb8,&zero,4);
  query(object.data()+0x2d0,warm.data());if(warm!=cold)throw std::runtime_error("native warm cache");
  object[0x186c]=0;query(object.data()+0x2d0,fresh.data());if(fresh!=std::array<uint8_t,8>{})throw std::runtime_error("native invalidation");
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_light_states=65536\\n";
'''+s[b:]
# Explicit parentheses preserve comparison priority relative to bitwise OR.
s=s.replace('uint32_t(result.firstFlag)|(uint32_t(result.secondFlag)<<8) != record[98]','(uint32_t(result.firstFlag)|(uint32_t(result.secondFlag)<<8)) != record[98]')
Path('work/probe_generic_light_state.cpp').write_text(s)
Path('work/build_generic_light_state.cmd').write_text(Path('work/build_generic_shader_combo.cmd').read_text().replace('probe_generic_shader_combo','probe_generic_light_state'))
