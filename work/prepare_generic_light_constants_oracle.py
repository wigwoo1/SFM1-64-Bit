from pathlib import Path
s=Path('work/probe_generic_ambient_expansion.cpp').read_text().replace('generic_ambient_expansion_candidate.h','generic_light_constants_candidate.h')
a=s.index(' unsigned cases=0;');b=s.index('#else',a)
s=s[:a]+''' unsigned cases=0;std::array<uint32_t,363> record{};
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  int32_t count;std::memcpy(&count,&record[0],4);
  std::array<uint32_t,88> lights{};std::copy_n(record.begin()+1,88,lights.begin());
  std::array<uint32_t,137> destination{};std::copy_n(record.begin()+89,137,destination.begin());
  genericLightConstantsCandidate(destination,count,lights);
  if(!std::equal(destination.begin(),destination.end(),record.begin()+226))throw std::runtime_error("light constants mismatch");
  ++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 std::array<uint32_t,137> d{};d.fill(7);auto before=d;std::array<uint32_t,88> lights{};
 try{genericLightConstantsCandidate(d,5,lights);throw std::runtime_error("missing capacity guard");}catch(const std::invalid_argument&){}
 if(d!=before)throw std::runtime_error("capacity failure mutation");
 std::cout<<"x64_native_light_constants_exact="<<cases<<"\\n";
'''+s[b:]
a=s.index(' // Only original');b=s.index('#endif',a)
s=s[:a]+''' // Only original leaf10012210 executes; no DLL initialization or imports.
 using Pack=void(__thiscall*)(void*,void*,int32_t,const void*);
 auto pack=reinterpret_cast<Pack>(0x10012210);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");
 uint32_t seed=0xabc49e71;
 const int32_t counts[]={INT32_MIN,-1,0,1,2,3,4};
 const uint32_t boundary[]={0,0x80000000u,1,0x807fffffu,0x00800000u,0x3f800000u,0x7f7fffffu,0x7f800000u,0xff800000u,0x7f800001u,0xffbfffffu,0x7fc00001u};
 for(unsigned i=0;i<65536;++i){
  std::array<uint32_t,363> record{};int32_t count=counts[i%7];std::memcpy(&record[0],&count,4);
  std::array<uint32_t,110> payload{};std::memcpy(&payload[21],&count,4);
  for(unsigned word=0;word<88;++word){seed=seed*1664525u+1013904223u;payload[22+word]=i<216?boundary[(i+word)%12]:seed;}
  for(unsigned light=0;light<4;++light)payload[22+light*22]=(i+light)%5;
  std::copy_n(payload.begin()+22,88,record.begin()+1);auto payloadBefore=payload;
  std::array<uint32_t,139> guarded{};guarded.front()=guarded.back()=0xb794d1e5u;
  for(unsigned word=0;word<137;++word){seed=seed*1664525u+1013904223u;guarded[1+word]=seed;record[89+word]=seed;}
  pack(nullptr,guarded.data()+1,123,payload.data());
  if(payload!=payloadBefore||guarded.front()!=0xb794d1e5u||guarded.back()!=0xb794d1e5u)throw std::runtime_error("light constants bounds or source mutation");
  std::copy_n(guarded.begin()+1,137,record.begin()+226);
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");
 std::cout<<"original_x86_light_constants=65536\\n";
'''+s[b:]
Path('work/probe_generic_light_constants.cpp').write_text(s)
Path('work/build_generic_light_constants.cmd').write_text(Path('work/build_generic_shader_combo.cmd').read_text().replace('probe_generic_shader_combo','probe_generic_light_constants'))
v=Path('work/verify_generic_ambient_expansion.py').read_text().replace('ambient_expansion','light_constants').replace('ambient-expansion','light-constants').replace('ambient_expansions','light_constants').replace('65536*43*4','65536*363*4').replace('Original expansion only, masked x87 exceptions. Full upload dispatch and GPU rendering remain unverified. Count argument is varied but unused by recovered routine.','Original light packing only, signed counts through four lights, masked x87 exceptions. Destination tails preserved. Capacity guard is owned policy. Full upload and GPU rendering remain unverified.')
Path('work/verify_generic_light_constants.py').write_text(v.replace('light_constantss','light_constants'))

