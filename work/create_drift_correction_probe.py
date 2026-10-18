from pathlib import Path
s=Path('work/probe_drift_history.cpp').read_text().replace('clock_drift_history_candidate.h','clock_drift_correction_candidate.h')
at=s.index('int main');s=s[:at]+'''#ifndef _WIN64
static bool demoResult;
static bool __fastcall demoOracle(void*,void*){return demoResult;}
#endif
'''+s[at:]
a=s.index(' std::ifstream input(argv[1]');b=s.index('#else',a)
s=s[:a]+''' std::ifstream input(argv[1],std::ios::binary);unsigned cases=0;std::array<uint32_t,41>r{};
 while(input.read(reinterpret_cast<char*>(r.data()),sizeof(r))){std::array<uint32_t,16>v;std::copy_n(r.begin(),16,v.begin());auto duration=clockDriftCorrectionCandidate(v,r[16],r[17],r[18],r[19],r[20],r[21]!=0,r[22]!=0,r[23]!=0);if(duration!=r[40]||!std::equal(v.begin(),v.end(),r.begin()+24))throw std::runtime_error("drift correction mismatch case "+std::to_string(cases));++cases;}
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");std::cout<<"x64_drift_correction_exact=65536\\n";
'''+s[b:]
a=s.index(' DWORD dataOld;');b=s.index(' output.flush()',a)
s=s[:a]+''' std::array<uint32_t,16>low{},high{},amount{},enabled{},debug{};
 std::array<uintptr_t,7>vtable{};vtable[6]=reinterpret_cast<uintptr_t>(&demoOracle);uintptr_t object=reinterpret_cast<uintptr_t>(vtable.data());
 for(uintptr_t address:{uintptr_t(0x1062948c),uintptr_t(0x1062964c),uintptr_t(0x106296a4),uintptr_t(0x1062953c),uintptr_t(0x106295f4),uintptr_t(0x104b60c0),uintptr_t(0x1060d4ec),uintptr_t(0x104083f4),uintptr_t(0x10643ed8)}){DWORD prior;if(!VirtualProtect(reinterpret_cast<void*>(address),4,PAGE_READWRITE,&prior))throw std::runtime_error("private state protection");}
 *reinterpret_cast<uintptr_t*>(0x1062948c)=reinterpret_cast<uintptr_t>(low.data());*reinterpret_cast<uintptr_t*>(0x1062964c)=reinterpret_cast<uintptr_t>(high.data());*reinterpret_cast<uintptr_t*>(0x106296a4)=reinterpret_cast<uintptr_t>(amount.data());*reinterpret_cast<uintptr_t*>(0x1062953c)=reinterpret_cast<uintptr_t>(enabled.data());*reinterpret_cast<uintptr_t*>(0x106295f4)=reinterpret_cast<uintptr_t>(debug.data());*reinterpret_cast<uintptr_t*>(0x104b60c0)=reinterpret_cast<uintptr_t>(&object);*reinterpret_cast<uint32_t*>(0x1060d4ec)=0;
 using Routine=float(__thiscall*)(void*,float);auto routine=reinterpret_cast<Routine>(0x10143c40);
 std::ofstream output(argv[1],std::ios::binary);uint32_t seed=0x98765432;const uint32_t bounds[]={0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc01234,0x7f801234};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,41>r{};for(unsigned j=0;j<21;++j){seed=seed*1664525u+1013904223u;r[j]=i<64?bounds[(i+j)%8]:seed;}r[21]=i&1;r[22]=(i>>1)&1;r[23]=(i>>2)&1;
 low[11]=r[18];high[11]=r[19];amount[11]=r[20];enabled[12]=r[21];demoResult=r[22]!=0;*reinterpret_cast<uint32_t*>(0x104083f4)=r[17];*reinterpret_cast<uint8_t*>(0x10643ed8)=uint8_t(r[23]);
 std::array<uint32_t,18>guarded{};guarded[0]=0x12345678;guarded[17]=0x87654321;std::copy_n(r.begin(),16,guarded.begin()+1);float duration;std::memcpy(&duration,&r[16],4);float result=routine(guarded.data()+1,duration);std::memcpy(&r[40],&result,4);
 if(guarded[0]!=0x12345678||guarded[17]!=0x87654321)throw std::runtime_error("history bounds");std::copy_n(guarded.begin()+1,16,r.begin()+24);output.write(reinterpret_cast<const char*>(r.data()),sizeof(r));}
'''+s[b:]
s=s.replace('original_x86_drift_history','original_x86_drift_correction')
Path('work/probe_drift_correction.cpp').write_text(s)
Path('work/build_drift_correction_probe.cmd').write_text(Path('work/build_drift_mean_probe.cmd').read_text().replace('probe_drift_mean','probe_drift_correction'))
v=Path('work/verify_drift_history_native.py').read_text().replace('probe_drift_history','probe_drift_correction').replace('original_x86_drift_history','original_x86_drift_correction').replace('x64_drift_history_exact','x64_drift_correction_exact').replace('65536*34*4','65536*41*4').replace("'drift-history-'","'drift-correction-'").replace('sfm64-drift-history-native-validation','sfm64-drift-correction-native-validation')
a=v.index("limits='");b=v.index("')",a)
v=v[:a]+"limits='Original full engine10143c40 with supplied private configuration, state below2, logging disabled, controlled demo predicate. Raw history/duration/interval/min/max/amount; enabled/demo/thread branch combinations. No imports or initialization. State>=2 interface gate, logging, FP modes and playback integration unverified."+v[b:]
Path('work/verify_drift_correction_native.py').write_text(v)
