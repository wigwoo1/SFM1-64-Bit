from pathlib import Path
s=Path('work/probe_drift_history.cpp').read_text().replace('clock_drift_history_candidate.h','clock_drift_mean_candidate.h')
at=s.index('int main');s=s[:at]+'''#ifndef _WIN64
__declspec(naked) uint32_t executeMean(void*){__asm{
 mov ecx,[esp+4]
 mov edx,10143820h
 call edx
 fmul dword ptr ds:[104083f4h]
 sub esp,4
 fstp dword ptr [esp]
 pop eax
 ret
}}
#endif
'''+s[at:]
a=s.index(' std::ifstream input(argv[1]');b=s.index('#else',a)
s=s[:a]+''' std::ifstream input(argv[1],std::ios::binary);unsigned cases=0;std::array<uint32_t,18>r{};
 while(input.read(reinterpret_cast<char*>(r.data()),sizeof(r))){std::array<uint32_t,16>v;std::copy_n(r.begin(),16,v.begin());if(clockDriftMeanIntervalCandidate(v,r[16])!=r[17])throw std::runtime_error("drift mean mismatch case "+std::to_string(cases));++cases;}
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");std::cout<<"x64_drift_mean_exact=65536\\n";
'''+s[b:]
a=s.index(' using Routine=');b=s.index(' output.flush()',a)
s=s[:a]+''' std::ofstream output(argv[1],std::ios::binary);uint32_t seed=0x98765432;const uint32_t bounds[]={0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc01234,0x7f801234};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,18>r{};for(unsigned j=0;j<17;++j){seed=seed*1664525u+1013904223u;r[j]=i<64?bounds[(i+j)%8]:seed;}
 *reinterpret_cast<uint32_t*>(0x104083f4)=r[16];auto before=r;r[17]=executeMean(r.data());if(!std::equal(r.begin(),r.begin()+17,before.begin())||*reinterpret_cast<uint32_t*>(0x104083f4)!=r[16])throw std::runtime_error("mean input mutation");output.write(reinterpret_cast<const char*>(r.data()),sizeof(r));}
'''+s[b:]
s=s.replace('original_x86_drift_history','original_x86_drift_mean')
Path('work/probe_drift_mean.cpp').write_text(s)
build=Path('work/build_drift_history_probe.cmd').read_text().replace('probe_drift_history','probe_drift_mean')
pos=build.rindex('cl /nologo')
build=build[:pos]+'ml64 /nologo /c /Fowork\\clock_drift_mean_x64.obj work\\clock_drift_mean_x64.asm\nif errorlevel 1 exit /b 1\n'+build[pos:]
build=build.replace('work\\probe_drift_mean.cpp /Fowork\\probe_drift_mean_x64','work\\probe_drift_mean.cpp work\\clock_drift_mean_x64.obj /Fowork\\probe_drift_mean_x64')
Path('work/build_drift_mean_probe.cmd').write_text(build)
v=Path('work/verify_drift_history_native.py').read_text().replace('probe_drift_history','probe_drift_mean').replace('original_x86_drift_history','original_x86_drift_mean').replace('x64_drift_history_exact','x64_drift_mean_exact').replace('65536*34*4','65536*18*4').replace("'drift-history-'","'drift-mean-'").replace('sfm64-drift-history-native-validation','sfm64-drift-mean-native-validation')
a=v.index("limits='");b=v.index("')",a)
v=v[:a]+"limits='Original engine10143820 mean plus x87 interval multiplication and final float store. Raw sixteen history words/interval, default masked FP controls; inputs preserved, no imports or initialization. Full correction and playback integration remain unverified."+v[b:]
Path('work/verify_drift_mean_native.py').write_text(v)
