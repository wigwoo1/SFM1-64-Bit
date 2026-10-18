from pathlib import Path
s=Path('work/test_vertex_constant_runs_native.cpp').read_text()
a=s.index('static void invoke(');b=s.index('\n#endif',a)
s=s[:a]+s[b:]
needle='FlushInstructionCache(GetCurrentProcess(),runCode,4096);'
extra='''
 std::array<unsigned char,sizeof(vertexConstantWrapperBytes)> wrapperBytes{};dll.seekg(vertexConstantWrapperFileOffset);dll.read(reinterpret_cast<char*>(wrapperBytes.data()),wrapperBytes.size());if(!dll||std::memcmp(wrapperBytes.data(),vertexConstantWrapperBytes,wrapperBytes.size()))return 8;
 auto* wrapperCode=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!wrapperCode)return 9;std::memcpy(wrapperCode,wrapperBytes.data(),wrapperBytes.size());for(auto& call:vertexConstantWrapperCalls){const uint32_t delta=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(runCode)-reinterpret_cast<uintptr_t>(wrapperCode+call[0]+5));std::memcpy(wrapperCode+call[0]+1,&delta,4);}if(!VirtualProtect(wrapperCode,4096,PAGE_EXECUTE_READ,&protection))return 10;FlushInstructionCache(GetCurrentProcess(),wrapperCode,4096);auto invoke=reinterpret_cast<void(__cdecl*)(void*,void*,void*,uint32_t)>(wrapperCode);
'''
assert needle in s;s=s.replace(needle,needle+extra)
s=s.replace('native-vertex-constant-runs.bin','native-vertex-constant-wrapper.bin')
s=s.replace('for(unsigned mask=0;mask<256;++mask)','for(unsigned mask:{0u,1u,85u,255u})')
s=s.replace('std::array<float,40>','std::array<float,6144>').replace('(i+1)*4','(i+1024)*4').replace('[5]','[4097]')
s=s.replace('std::array<uint32_t,128>','std::array<uint32_t,4096>').replace('remaining=127','remaining=4095')
s=s.replace('cache.data(),force,1,8)','cache.data(),force)').replace('studioVertexConstantRunsCandidate(1,8','studioVertexConstantRunsCandidate(1024,512')
s=s.replace('std::array<uint32_t,320>','std::array<uint32_t,19000>')
s=s.replace('source.data(),160','source.data(),24576').replace('record.data()+43','record.data()+6147').replace('cached.data(),160','cached.data(),24576')
s=s.replace('record.begin()+83','record.begin()+12291').replace('events.size()>109','events.size()>2613').replace('record.begin()+211','record.begin()+16387')
s=s.replace('VirtualFree(runCode,0,MEM_RELEASE);','VirtualFree(runCode,0,MEM_RELEASE);VirtualFree(wrapperCode,0,MEM_RELEASE);')
s=s.replace('vertex_constant_runs_original_cases','vertex_constant_wrapper_original_cases')
Path('work/test_vertex_constant_wrapper_native.cpp').write_text(s)
build=Path('work/build_vertex_constant_runs_native.cmd').read_text().replace('vertex_constant_runs_native','vertex_constant_wrapper_native')
Path('work/build_vertex_constant_wrapper_native.cmd').write_text(build)
print('created_full_range_wrapper_oracle=1')
