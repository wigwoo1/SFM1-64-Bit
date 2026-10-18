from pathlib import Path
s=Path('work/probe_generic_shader_combo.cpp').read_text()
s=s.replace('int main(','''#ifndef _WIN64
static int32_t levels[2],dynamicQuery;static unsigned queries;
static int32_t __fastcall hardwareQuery(void*,void*) { return levels[queries++]; }
static int32_t __fastcall apiQuery(void*,void*) { return dynamicQuery; }
#endif
int main(''')
a=s.index(' unsigned cases=0;');b=s.index('#else',a)
s=s[:a]+''' unsigned cases=0;std::array<uint32_t,23> record;
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  int32_t first,second,query;std::memcpy(&first,&record[0],4);std::memcpy(&second,&record[2],4);std::memcpy(&query,&record[3],4);
  auto stat=genericVs20StaticInitialCandidate(first,record[1],second);
  auto dyn=genericVs20DynamicInitialCandidate(query);
  if(!std::equal(stat.begin(),stat.end(),record.begin()+4)||!std::equal(dyn.begin(),dyn.end(),record.begin()+17))throw std::runtime_error("initializer mismatch");++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 std::cout<<"x64_native_initializer_pairs_exact="<<cases<<'\\n';
'''+s[b:]
a=s.index(' DWORD oldProtect;')
s=s[:a]+''' void* hardwareTable[18]{};hardwareTable[17]=reinterpret_cast<void*>(&hardwareQuery);void** hardware=hardwareTable;
 void* apiTable[3]{};apiTable[2]=reinterpret_cast<void*>(&apiQuery);void** api=apiTable;
 *reinterpret_cast<void***>(static_cast<uint8_t*>(allocation)+0xdb9d8)=reinterpret_cast<void**>(&hardware);
'''+s[a:]
a=s.index(' // No DLL entrypoint');b=s.index('#endif',a)
s=s[:a]+''' // Audited constructors execute privately with synthetic interface callbacks.
 // No entrypoint, imports, original interface implementation or GPU is initialized.
 using StaticInit=void*(__thiscall*)(uint32_t*,void*,void*);
 using DynamicInit=void*(__thiscall*)(uint32_t*,void*);
 auto stat=reinterpret_cast<StaticInit>(0x10053af0);auto dyn=reinterpret_cast<DynamicInit>(0x10053bc0);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");
 uint32_t seed=0x194782ab;const int32_t boundaries[]={INT32_MIN,-1,0,91,92,93,INT32_MAX};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,23> record{};
  for(unsigned j=0;j<4;++j){seed=seed*1664525u+1013904223u;record[j]=seed;}
  if(i<196){levels[0]=boundaries[i%7];levels[1]=boundaries[(i/7)%7];record[1]=(i/49)&1?0x80000000u:0;dynamicQuery=(i/98)?2:1;}
  else {std::memcpy(&levels[0],&record[0],4);std::memcpy(&levels[1],&record[2],4);std::memcpy(&dynamicQuery,&record[3],4);}
  std::memcpy(&record[0],&levels[0],4);std::memcpy(&record[2],&levels[1],4);std::memcpy(&record[3],&dynamicQuery,4);
  uint32_t material[3]={0,0,record[1]};void* params[]={material};queries=0;
  std::fill(record.begin()+4,record.end(),0xdeadbeefu);
  if(stat(record.data()+4,nullptr,params)!=record.data()+4||queries!=2)throw std::runtime_error("static constructor contract");
  if(dyn(record.data()+17,&api)!=record.data()+17)throw std::runtime_error("dynamic constructor contract");
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_initializer_pairs=65536\\n";
'''+s[b:]
Path('work/probe_generic_shader_initializers.cpp').write_text(s)
build=Path('work/build_generic_shader_combo.cmd').read_text().replace('probe_generic_shader_combo','probe_generic_shader_initializers')
Path('work/build_generic_shader_initializers.cmd').write_text(build)

