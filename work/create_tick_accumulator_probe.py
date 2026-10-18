from pathlib import Path
s=Path('work/probe_material_animation_arithmetic.cpp').read_text()
s=s.replace('material_animation_frame_candidate.h','engine_tick_accumulator_candidate.h')
a=s.index('__declspec(naked)');b=s.index('#endif',a)
s=s[:a]+'''__declspec(naked) void executeAccumulator(void*,uint32_t,uint32_t*){
 __asm {push ebp
 push ebx
 sub esp,16
 mov ebp,esp
 add ebp,8
 movd xmm0,[esp+32]
 call dword ptr [esp+28]
 mov ecx,[esp+36]
 mov [ecx],ebx
 movss [ecx+4],xmm0
 add esp,16
 pop ebx
 pop ebp
 ret}
}
'''+s[b:]
a=s.index(' std::ifstream input(argv[1]');b=s.index('#else',a)
s=s[:a]+''' std::ifstream input(argv[1],std::ios::binary);unsigned cases=0;std::array<uint32_t,6>r{};
 while(input.read(reinterpret_cast<char*>(r.data()),sizeof(r))){int32_t current;std::memcpy(&current,&r[3],4);auto v=engineTickAccumulatorCandidate(r[0],0,r[1],true,r[2]!=0,current);if(uint32_t(v.ticks)!=r[4]||v.remainderWord!=r[5])throw std::runtime_error("accumulator mismatch case "+std::to_string(cases));++cases;}
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");std::cout<<"x64_tick_accumulator_exact=65536\\n";
'''+s[b:]
s=s.replace(r'game\tf\bin\client.dll',r'game\bin\engine.dll')
s=s.replace('0x1000f1b0-0x1000f16a','0x1018248e-0x1018243a').replace('0x1000f16a','0x1018243a')
s=s.replace('const uint8_t tail[]={0x8b,0xc3,0x5b,0xc3};','const uint8_t tail[]={0xc3};')
a=s.index(' std::ofstream output');b=s.index(' output.flush()',a)
s=s[:a]+''' auto writable=[](uintptr_t address){DWORD old;if(!VirtualProtect(reinterpret_cast<void*>(address),4,PAGE_READWRITE,&old))throw std::runtime_error("private data protection");};
 for(uintptr_t address:{uintptr_t(0x104083f4),uintptr_t(0x106285cc),uintptr_t(0x104024e0),uintptr_t(0x106460f4)})writable(address);
 std::array<uint32_t,16>convar{};*reinterpret_cast<uintptr_t*>(0x106285cc)=reinterpret_cast<uintptr_t>(convar.data());
 std::ofstream output(argv[1],std::ios::binary);uint32_t seed=0x98765432;const uint32_t bounds[]={0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc01234,0x7f801234};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,6>r{};for(unsigned j=0;j<2;++j){seed=seed*1664525u+1013904223u;r[j]=i<64?bounds[(i+j)%8]:seed;}r[2]=i&1;seed=seed*1664525u+1013904223u;r[3]=seed;
 convar[12]=r[2];*reinterpret_cast<uint32_t*>(0x104083f4)=r[1];*reinterpret_cast<uint32_t*>(0x104024e0)=r[3];uint32_t out[2]{};executeAccumulator(copied,r[0],out);r[4]=out[0];r[5]=out[1];if(convar[12]!=r[2]||*reinterpret_cast<uint32_t*>(0x104083f4)!=r[1]||*reinterpret_cast<uint32_t*>(0x104024e0)!=r[3])throw std::runtime_error("input mutation");output.write(reinterpret_cast<const char*>(r.data()),sizeof(r));}
'''+s[b:]
s=s.replace('original_x86_material_frames=65536','original_x86_tick_accumulator=65536')
Path('work/probe_tick_accumulator.cpp').write_text(s)
build=Path('work/build_material_animation_arithmetic.cmd').read_text().replace('probe_material_animation_arithmetic','probe_tick_accumulator')
Path('work/build_tick_accumulator_probe.cmd').write_text(build)
v=Path('work/verify_material_animation_arithmetic.py').read_text().replace(r'game\tf\bin\client.dll',r'game\bin\engine.dll').replace('91dd1062227b095f1338312aa7268121894970c3e0a5b0d314e42c77f2aba73b','c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f').replace('material-arithmetic-','tick-accumulator-').replace('probe_material_animation_arithmetic','probe_tick_accumulator').replace('original_x86_material_frames','original_x86_tick_accumulator').replace('x64_material_frames_exact','x64_tick_accumulator_exact').replace('65536*7*4','65536*6*4').replace('sfm64-material-animation-arithmetic-validation','sfm64-tick-accumulator-native-validation')
a=v.index("limits='");b=v.index("')",a)
v=v[:a]+"limits='Original contiguous engine1018243a..1018248e with return adapter; raw accumulator/interval, alternate alignment and signed current tick. Private mapped data explicitly supplied; no imports or initialization. Elapsed addition, full scheduler, FP exception modes and playback integration remain unverified."+v[b:]
Path('work/verify_tick_accumulator_native.py').write_text(v)
