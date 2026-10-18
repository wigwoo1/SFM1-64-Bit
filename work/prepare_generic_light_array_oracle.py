from pathlib import Path
s=Path('work/probe_generic_shader_combo.cpp').read_text().replace('generic_shader_combo_candidate.h','generic_light_array_candidate.h').replace('stdshader_dx9.dll','shaderapidx9.dll')
s=s.replace('int main(','''#ifndef _WIN64
static unsigned flushes;static void __fastcall fakeFlush(void*,void*) {++flushes;}
#endif
int main(''')
a=s.index(' unsigned cases=0;');b=s.index('#else',a)
s=s[:a]+''' unsigned cases=0;std::array<uint32_t,270> record;
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  GenericLightArrayCandidate state;std::copy_n(record.begin()+3,88,state.words.begin());state.control=static_cast<uint8_t>(record[2]);
  std::array<uint32_t,88> supplied;std::copy_n(record.begin()+91,88,supplied.begin());int32_t requested,configured;std::memcpy(&requested,&record[0],4);std::memcpy(&configured,&record[1],4);unsigned flushCount=0;
  state.set(requested,configured,supplied,[&]{++flushCount;});uint32_t countBits;std::memcpy(&countBits,&state.count,4);
  if(!std::equal(state.words.begin(),state.words.end(),record.begin()+179)||countBits!=record[267]||state.control!=record[268]||flushCount!=record[269])throw std::runtime_error("light array mismatch");
  unsigned expected=state.count!=0?2:1;state.clear([&]{++flushCount;});if(state.count||flushCount!=expected)throw std::runtime_error("owned clear");++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 std::cout<<"x64_native_light_arrays_exact="<<cases<<'\\n';
'''+s[b:]
a=s.index(' DWORD oldProtect;')
s=s[:a]+''' std::array<uint8_t,0x548> config{};*reinterpret_cast<void**>(static_cast<uint8_t*>(allocation)+0x16d130)=config.data();
'''+s[a:]
a=s.index(' // No DLL entrypoint');b=s.index('#endif',a)
s=s[:a]+''' // Synthetic flush callback/config, original memcpy and setter code only.
 using Set=void(__thiscall*)(void*,int32_t,const void*);auto set=reinterpret_cast<Set>(0x10010bb0);
 using Clear=void(__thiscall*)(void*);auto clear=reinterpret_cast<Clear>(0x10010c10);
 std::array<void*,85> table{};table[84]=reinterpret_cast<void*>(&fakeFlush);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");uint32_t seed=0x194782ab;
 const int32_t boundaries[]={INT32_MIN,-1,0,1,3,4,5,INT32_MAX};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,270> record{};for(unsigned j=0;j<179;++j){seed=seed*1664525u+1013904223u;record[j]=seed;}record[2]&=255;
  if(i<16384){std::memcpy(&record[0],&boundaries[i%8],4);std::memcpy(&record[1],&boundaries[(i/8)%8],4);record[2]=(i/64)&255;}
  std::memcpy(config.data()+0x544,&record[1],4);std::array<uint8_t,0x1800> object{};auto tp=table.data();std::memcpy(object.data(),&tp,4);object[0x11b6]=static_cast<uint8_t>(record[2]);std::memcpy(object.data()+0x1434,record.data()+3,352);
  int32_t requested;std::memcpy(&requested,&record[0],4);flushes=0;set(object.data(),requested,record.data()+91);
  std::memcpy(record.data()+179,object.data()+0x1434,352);std::memcpy(&record[267],object.data()+0x1430,4);record[268]=object[0x11b6];record[269]=flushes;
  clear(object.data());uint32_t cleared;std::memcpy(&cleared,object.data()+0x1430,4);if(cleared||flushes!=(record[267]?2u:1u))throw std::runtime_error("native clear");
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_light_arrays=65536\\n";
'''+s[b:]
Path('work/probe_generic_light_array.cpp').write_text(s)
Path('work/build_generic_light_array.cmd').write_text(Path('work/build_generic_shader_combo.cmd').read_text().replace('probe_generic_shader_combo','probe_generic_light_array'))
