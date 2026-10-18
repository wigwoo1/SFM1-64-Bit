from pathlib import Path
s=Path('work/probe_time_snap.cpp').read_text().replace('recovered_time_snap_candidate.h','recovered_float_rate_candidate.h')
a=s.index(' std::ifstream input(argv[1]');b=s.index('#else',a)
s=s[:a]+''' std::ifstream input(argv[1],std::ios::binary);unsigned cases=0;std::array<uint32_t,2>r{};
 while(input.read(reinterpret_cast<char*>(r.data()),sizeof(r))){float fps;std::memcpy(&fps,&r[0],4);if(recoveredFloatRateCandidate(fps)!=r[1])throw std::runtime_error("ifm rate mismatch case "+std::to_string(cases));++cases;}
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");std::cout<<"x64_ifm_rate_exact=65536\\n";
'''+s[b:]
a=s.index(' using Routine=');b=s.index(' output.flush()',a)
s=s[:a]+''' using Routine=uint32_t*(__thiscall*)(uint32_t*,float);auto routine=reinterpret_cast<Routine>(0x1097b900);
 std::ofstream output(argv[1],std::ios::binary);uint32_t seed=0x97b900;
 const float bounds[]={0,24,25,29.97f,59.94f,23.976f,65535,0.001f};
 for(unsigned i=0;i<65536;++i){seed=seed*1664525u+1013904223u;float fps=i<8?bounds[i]:float(seed%4194240u)/64.0f;
 std::array<uint32_t,2>r{};std::memcpy(&r[0],&fps,4);uint32_t guarded[]={0x12345678,0,0x87654321};auto returned=routine(guarded+1,fps);
 if(returned!=guarded+1||guarded[0]!=0x12345678||guarded[2]!=0x87654321)throw std::runtime_error("rate ownership");r[1]=guarded[1];output.write(reinterpret_cast<const char*>(r.data()),sizeof(r));}
'''+s[b:]
s=s.replace('original_x86_time_snap','original_x86_ifm_rate')
Path('work/probe_ifm_rate.cpp').write_text(s)
Path('work/build_ifm_rate_probe.cmd').write_text(Path('work/build_time_snap_probe.cmd').read_text().replace('probe_time_snap','probe_ifm_rate'))
v=Path('work/verify_time_snap_native.py').read_text().replace('probe_time_snap','probe_ifm_rate').replace('original_x86_time_snap','original_x86_ifm_rate').replace('x64_time_snap_exact','x64_ifm_rate_exact').replace('65536*3*4','65536*2*4').replace("'time-snap-'","'ifm-rate-'").replace('sfm64-time-snap-native-validation','sfm64-ifm-rate-native-validation')
a=v.index("limits='");b=v.index("')",a)
v=v[:a]+"limits='Original full ifm1097b900 constructor, private mapping without initialization/imports. Finite nonnegative rates0..65535, default nearest x87/MXCSR controls. Invalid rates, other rounding controls and live getter lifecycle unverified."+v[b:]
Path('work/verify_ifm_rate_native.py').write_text(v)
