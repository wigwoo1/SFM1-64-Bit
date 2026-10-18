from pathlib import Path
s=Path('work/probe_drift_history.cpp').read_text().replace('clock_drift_history_candidate.h','clock_drift_rate_candidate.h')
at=s.index('int main');s=s[:at]+'''#ifndef _WIN64
__declspec(naked) uint32_t executeRate(uint32_t){__asm{
 movd xmm0,[esp+4]
 mov edx,10143790h
 call edx
 movd eax,xmm0
 ret
}}
#endif
'''+s[at:]
a=s.index(' std::ifstream input(argv[1]');b=s.index('#else',a)
s=s[:a]+''' std::ifstream input(argv[1],std::ios::binary);unsigned cases=0;std::array<uint32_t,5>r{};
 while(input.read(reinterpret_cast<char*>(r.data()),sizeof(r))){if(clockDriftRateCandidate(r[0],r[1],r[2],r[3])!=r[4])throw std::runtime_error("drift rate mismatch case "+std::to_string(cases));++cases;}
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");std::cout<<"x64_drift_rate_exact=65536\\n";
'''+s[b:]
a=s.index(' DWORD dataOld;');b=s.index(' output.flush()',a)
s=s[:a]+''' std::array<uint32_t,16>low{},high{},amount{};
 for(uintptr_t address:{uintptr_t(0x1062948c),uintptr_t(0x1062964c),uintptr_t(0x106296a4)}){DWORD prior;if(!VirtualProtect(reinterpret_cast<void*>(address),4,PAGE_READWRITE,&prior))throw std::runtime_error("private configuration protection");}
 *reinterpret_cast<uintptr_t*>(0x1062948c)=reinterpret_cast<uintptr_t>(low.data());*reinterpret_cast<uintptr_t*>(0x1062964c)=reinterpret_cast<uintptr_t>(high.data());*reinterpret_cast<uintptr_t*>(0x106296a4)=reinterpret_cast<uintptr_t>(amount.data());
 std::ofstream output(argv[1],std::ios::binary);uint32_t seed=0x98765432;const uint32_t bounds[]={0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc01234,0x7f801234};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,5>r{};for(unsigned j=0;j<4;++j){seed=seed*1664525u+1013904223u;r[j]=i<64?bounds[(i+j)%8]:seed;}if((i&3)==0)r[2]=r[1];
 low[11]=r[1];high[11]=r[2];amount[11]=r[3];auto beforeLow=low,beforeHigh=high,beforeAmount=amount;r[4]=executeRate(r[0]);if(low!=beforeLow||high!=beforeHigh||amount!=beforeAmount)throw std::runtime_error("configuration mutation");output.write(reinterpret_cast<const char*>(r.data()),sizeof(r));}
'''+s[b:]
s=s.replace('original_x86_drift_history','original_x86_drift_rate')
Path('work/probe_drift_rate.cpp').write_text(s)
Path('work/build_drift_rate_probe.cmd').write_text(Path('work/build_drift_history_probe.cmd').read_text().replace('probe_drift_history','probe_drift_rate'))
v=Path('work/verify_drift_history_native.py').read_text().replace('probe_drift_history','probe_drift_rate').replace('original_x86_drift_history','original_x86_drift_rate').replace('x64_drift_history_exact','x64_drift_rate_exact').replace('65536*34*4','65536*5*4').replace("'drift-history-'","'drift-rate-'").replace('sfm64-drift-history-native-validation','sfm64-drift-rate-native-validation')
a=v.index("limits='");b=v.index("')",a)
v=v[:a]+"limits='Original full engine10143790 called with XMM0 input and three explicitly supplied private convar records. Raw input/min/max/amount words, equality forced in one quarter of cases; configuration preservation verified. Default masked FP environment; no imports or initialization. Full correction and playback integration remain unverified."+v[b:]
Path('work/verify_drift_rate_native.py').write_text(v)
