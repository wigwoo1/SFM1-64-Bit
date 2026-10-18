from pathlib import Path
s=Path('work/probe_drift_correction.cpp').read_text()
s=s.replace('clock_drift_correction_candidate.h','recovered_time_snap_candidate.h')
a=s.index('#ifndef _WIN64');b=s.index('int main',a);s=s[:a]+s[b:]
a=s.index(' std::ifstream input(argv[1]');b=s.index('#else',a)
s=s[:a]+''' std::ifstream input(argv[1],std::ios::binary);unsigned cases=0;std::array<uint32_t,3>r{};
 while(input.read(reinterpret_cast<char*>(r.data()),sizeof(r))){int32_t ticks,expected;std::memcpy(&ticks,&r[0],4);std::memcpy(&expected,&r[2],4);if(recoveredTimeSnapCandidate(ticks,r[1])!=expected)throw std::runtime_error("time snap mismatch case "+std::to_string(cases));++cases;}
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");std::cout<<"x64_time_snap_exact=65536\\n";
'''+s[b:]
s=s.replace('game\\bin\\engine.dll','game\\bin\\tools\\ifm.dll')
a=s.index(' std::array<uint32_t,16>low');b=s.index(' output.flush()',a)
s=s[:a]+''' using Routine=int32_t*(__thiscall*)(int32_t*,int32_t*,uint32_t,uint32_t);auto routine=reinterpret_cast<Routine>(0x1097b020);
 std::ofstream output(argv[1],std::ios::binary);uint32_t seed=0x97b020;
 const uint32_t rates[]={24u|(10000u<<16),25u|(10000u<<16),30u|(10010u<<16),60u|(10010u<<16),1u|(65535u<<16),65535u|(1u<<16)};
 const uint32_t bounds[]={0,1,0xffffffffu,0x7fffffffu,0x80000000u,200,0xffffff38u};
 for(unsigned i=0;i<65536;++i){seed=seed*1664525u+1013904223u;std::array<uint32_t,3>r{};r[0]=i<42?bounds[i%7]:seed;
 seed=seed*1664525u+1013904223u;r[1]=i%2?rates[(i/2)%6]:((seed&65535u)|((seed>>16)<<16));if(!(r[1]&65535u))r[1]|=1;if(!(r[1]>>16))r[1]|=1u<<16;
 int32_t ticks,result;std::memcpy(&ticks,&r[0],4);const int32_t original=ticks;auto returned=routine(&ticks,&result,r[1],0);
 if(returned!=&result||ticks!=original)throw std::runtime_error("snap ownership");std::memcpy(&r[2],&result,4);output.write(reinterpret_cast<const char*>(r.data()),sizeof(r));}
'''+s[b:]
s=s.replace('original_x86_drift_correction','original_x86_time_snap')
Path('work/probe_time_snap.cpp').write_text(s)
build=Path('work/build_drift_correction_probe.cmd').read_text().replace('probe_drift_correction','probe_time_snap')
build=build.replace('ml64 /nologo /c /Fowork\\clock_drift_mean_x64.obj work\\clock_drift_mean_x64.asm','rem no assembly required').replace(' work\\clock_drift_mean_x64.obj','')
Path('work/build_time_snap_probe.cmd').write_text(build)
v=Path('work/verify_drift_correction_native.py').read_text().replace('game\\bin\\engine.dll','game\\bin\\tools\\ifm.dll').replace('c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f','8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4').replace('probe_drift_correction','probe_time_snap').replace('original_x86_drift_correction','original_x86_time_snap').replace('x64_drift_correction_exact','x64_time_snap_exact').replace('65536*41*4','65536*3*4').replace("'drift-correction-'","'time-snap-'").replace('sfm64-drift-correction-native-validation','sfm64-time-snap-native-validation')
a=v.index("limits='");b=v.index("')",a)
v=v[:a]+"limits='Original full ifm1097b020 mode0, private mapping without initialization/imports. Signed raw int32 ticks, nonzero uint16 packed rates, default FP controls. Other rounding modes, float rate constructor and live getter lifecycle unverified."+v[b:]
Path('work/verify_time_snap_native.py').write_text(v)
