from pathlib import Path
s=Path('work/probe_generic_shader_combo.cpp').read_text()
a=s.index(' unsigned cases=0;');b=s.index('#else',a)
s=s[:a]+''' unsigned cases=0;std::array<uint32_t,50> record;
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  std::array<uint32_t,24> fields;std::copy_n(record.begin(),24,fields.begin());
  if(genericPs20bStaticComboCandidate(fields)!=record[24])throw std::runtime_error("pixel static mismatch");
  auto initial=genericPs20bStaticInitialCandidate(record[25]);
  if(!std::equal(initial.begin(),initial.end(),record.begin()+26))throw std::runtime_error("pixel initializer mismatch");++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 std::cout<<"x64_native_pixel_static_combos_exact="<<cases<<'\\n';
'''+s[b:]
a=s.index(' // No DLL entrypoint');b=s.index('#endif',a)
s=s[:a]+''' // Original audited pure arithmetic packer; no external calls or init.
 using Pack=uint32_t(__thiscall*)(const uint32_t*);auto pack=reinterpret_cast<Pack>(0x10053e10);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");uint32_t seed=0x194782ab;
 using Init=void*(__thiscall*)(uint32_t*,void*,void*);auto init=reinterpret_cast<Init>(0x10053da0);
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,50> record{};
  for(unsigned j=0;j<24;++j){seed=seed*1664525u+1013904223u;record[j]=i<48?(i/24?UINT32_MAX:1u)*(i%24==j):i<8192?(seed>>23)&1u:seed;}
  auto before=record;record[24]=pack(record.data());if(!std::equal(record.begin(),record.begin()+24,before.begin()))throw std::runtime_error("native input mutation");
  seed=seed*1664525u+1013904223u;record[25]=seed;uint32_t material[3]={0,0,seed};void* params[]={material};
  std::fill(record.begin()+26,record.end(),0xdeadbeefu);
  if(init(record.data()+26,nullptr,params)!=record.data()+26)throw std::runtime_error("pixel initializer return");
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_pixel_static_combos=65536\\n";
'''+s[b:]
Path('work/probe_generic_pixel_static.cpp').write_text(s)
Path('work/build_generic_pixel_static.cmd').write_text(Path('work/build_generic_shader_combo.cmd').read_text().replace('probe_generic_shader_combo','probe_generic_pixel_static'))
