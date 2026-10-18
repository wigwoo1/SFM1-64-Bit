from pathlib import Path
s=Path('work/probe_tick_accumulator.cpp').read_text().replace('engine_tick_accumulator_candidate.h','clock_drift_history_candidate.h')
a=s.index('#ifndef _WIN64');b=s.index('int main',a);s=s[:a]+s[b:]
a=s.index(' std::ifstream input(argv[1]');b=s.index('#else',a)
s=s[:a]+''' std::ifstream input(argv[1],std::ios::binary);unsigned cases=0;std::array<uint32_t,34>r{};
 while(input.read(reinterpret_cast<char*>(r.data()),sizeof(r))){std::array<uint32_t,16>v;std::copy_n(r.begin(),16,v.begin());clockDriftHistoryCandidate(v,r[16],r[17]);if(!std::equal(v.begin(),v.end(),r.begin()+18))throw std::runtime_error("drift history mismatch case "+std::to_string(cases));++cases;}
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");std::cout<<"x64_drift_history_exact=65536\\n";
'''+s[b:]
a=s.index(' const size_t bytes=');b=s.index(' output.flush()',a)
s=s[:a]+''' DWORD dataOld;if(!VirtualProtect(reinterpret_cast<void*>(0x104083f4),4,PAGE_READWRITE,&dataOld))throw std::runtime_error("private interval protection");
 using Routine=void(__thiscall*)(void*,float);auto routine=reinterpret_cast<Routine>(0x10143870);
 std::ofstream output(argv[1],std::ios::binary);uint32_t seed=0x98765432;const uint32_t bounds[]={0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc01234,0x7f801234};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,34>r{};for(unsigned j=0;j<18;++j){seed=seed*1664525u+1013904223u;r[j]=i<64?bounds[(i+j)%8]:seed;}
 std::array<uint32_t,18>guarded{};guarded[0]=0x12345678;guarded[17]=0x87654321;std::copy_n(r.begin(),16,guarded.begin()+1);
 *reinterpret_cast<uint32_t*>(0x104083f4)=r[17];float correction;std::memcpy(&correction,&r[16],4);routine(guarded.data()+1,correction);
 if(guarded[0]!=0x12345678||guarded[17]!=0x87654321||*reinterpret_cast<uint32_t*>(0x104083f4)!=r[17])throw std::runtime_error("bounds or interval mutation");
 std::copy_n(guarded.begin()+1,16,r.begin()+18);output.write(reinterpret_cast<const char*>(r.data()),sizeof(r));}
'''+s[b:]
s=s.replace('original_x86_tick_accumulator=65536','original_x86_drift_history=65536')
Path('work/probe_drift_history.cpp').write_text(s)
Path('work/build_drift_history_probe.cmd').write_text(Path('work/build_tick_accumulator_probe.cmd').read_text().replace('probe_tick_accumulator','probe_drift_history'))
v=Path('work/verify_tick_accumulator_native.py').read_text().replace('probe_tick_accumulator','probe_drift_history').replace('original_x86_tick_accumulator','original_x86_drift_history').replace('x64_tick_accumulator_exact','x64_drift_history_exact').replace('65536*6*4','65536*34*4').replace("'tick-accumulator-'","'drift-history-'").replace('sfm64-tick-accumulator-native-validation','sfm64-drift-history-native-validation')
a=v.index("limits='");b=v.index("')",a)
v=v[:a]+"limits='Original full engine10143870 updates sixteen history words with raw correction/interval. Default masked FP environment, bounded supplied history and guard words; no imports or initialization. Full clock state and playback integration remain unverified."+v[b:]
Path('work/verify_drift_history_native.py').write_text(v)
