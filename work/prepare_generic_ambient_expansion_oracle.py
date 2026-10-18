from pathlib import Path
s=Path('work/probe_generic_shader_combo.cpp').read_text().replace('generic_shader_combo_candidate.h','generic_ambient_expansion_candidate.h').replace('stdshader_dx9.dll','shaderapidx9.dll')
a=s.index(' unsigned cases=0;');b=s.index('#else',a)
s=s[:a]+''' unsigned cases=0;std::array<uint32_t,43> record{};
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  std::array<uint32_t,18> rgb{};std::copy_n(record.begin(),18,rgb.begin());
  auto expanded=genericAmbientExpansionCandidate(rgb);
  if(!std::equal(expanded.begin(),expanded.end(),record.begin()+19))throw std::runtime_error("ambient expansion mismatch");
  ++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 std::cout<<"x64_native_ambient_expansions_exact="<<cases<<"\\n";
'''+s[b:]
a=s.index(' // No DLL entrypoint');b=s.index('#endif',a)
s=s[:a]+''' // Only original 10012140 executes. No imports, entrypoint or original initialization.
 using Expand=void(__thiscall*)(void*,void*,int32_t,const void*);
 auto expand=reinterpret_cast<Expand>(0x10012140);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");
 uint32_t seed=0x129abb42;
 const uint32_t boundary[]={0,0x80000000u,1,0x807fffffu,0x00800000u,0x3f800000u,0x7f7fffffu,0x7f800000u,0xff800000u,0x7f800001u,0xffbfffffu,0x7fc00001u};
 for(unsigned i=0;i<65536;++i){
  std::array<uint32_t,43> record{};std::array<uint32_t,18> rgb{};
  for(unsigned word=0;word<18;++word){seed=seed*1664525u+1013904223u;rgb[word]=i<216?boundary[(i+word)%12]:seed;record[word]=rgb[word];}
  seed=seed*1664525u+1013904223u;record[18]=seed;
  std::array<uint32_t,26> guarded{};guarded.fill(0xa5d49e71u);auto before=rgb;
  int32_t count;std::memcpy(&count,&record[18],4);
  expand(nullptr,guarded.data()+1,count,rgb.data());
  if(rgb!=before||guarded.front()!=0xa5d49e71u||guarded.back()!=0xa5d49e71u)throw std::runtime_error("expansion bounds or source mutation");
  std::copy_n(guarded.begin()+1,24,record.begin()+19);
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");
 std::cout<<"original_x86_ambient_expansions=65536\\n";
'''+s[b:]
Path('work/probe_generic_ambient_expansion.cpp').write_text(s)
Path('work/build_generic_ambient_expansion.cmd').write_text(Path('work/build_generic_shader_combo.cmd').read_text().replace('probe_generic_shader_combo','probe_generic_ambient_expansion'))
