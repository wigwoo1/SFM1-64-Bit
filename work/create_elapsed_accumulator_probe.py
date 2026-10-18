from pathlib import Path
s=Path('work/probe_tick_accumulator.cpp').read_text().replace('std::array<uint32_t,6>r','std::array<uint32_t,8>r')
s=s.replace('Candidate(r[0],0,r[1],true,r[2]!=0,current)','Candidate(r[0],r[4],r[1],r[5]!=0,r[2]!=0,current)').replace('!=r[4]||v.remainderWord!=r[5]','!=r[6]||v.remainderWord!=r[7]')
point=' auto writable='
a=s.index(point)
prefix=''' const size_t prefixBytes=0x10182430-0x10182418;
 auto combined=VirtualAlloc(nullptr,prefixBytes+bytes+1,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!combined)throw std::runtime_error("combined stub allocation");
 std::unique_ptr<void,void(*)(void*)> combinedOwner(combined,[](void*p){VirtualFree(p,0,MEM_RELEASE);});
 std::memcpy(combined,reinterpret_cast<void*>(0x10182418),prefixBytes);
 std::memcpy(static_cast<uint8_t*>(combined)+prefixBytes,copied,bytes+1);
 if(!VirtualProtect(combined,prefixBytes+bytes+1,PAGE_EXECUTE_READ,&oldProtect))throw std::runtime_error("combined protection");FlushInstructionCache(GetCurrentProcess(),combined,prefixBytes+bytes+1);
'''
s=s[:a]+prefix+s[a:]
s=s.replace('uintptr_t(0x106460f4)})','uintptr_t(0x106460f4),uintptr_t(0x10644ea4)})')
s=s.replace('r[3]=seed;','r[3]=seed;seed=seed*1664525u+1013904223u;r[4]=i<64?bounds[(i+2)%8]:seed;r[5]=(i>>1)&1;')
s=s.replace('executeAccumulator(copied,r[0],out);r[4]=out[0];r[5]=out[1];','*reinterpret_cast<uint32_t*>(0x106460f4)=r[0];*reinterpret_cast<uint32_t*>(0x10644ea4)=r[4];executeAccumulator(r[5]?copied:combined,r[0],out);r[6]=out[0];r[7]=out[1];')
s=s.replace('if(convar[12]!=r[2]','if(*reinterpret_cast<uint32_t*>(0x10644ea4)!=r[4]||convar[12]!=r[2]')
Path('work/probe_elapsed_accumulator.cpp').write_text(s)
Path('work/build_elapsed_accumulator_probe.cmd').write_text(Path('work/build_tick_accumulator_probe.cmd').read_text().replace('probe_tick_accumulator','probe_elapsed_accumulator'))
v=Path('work/verify_tick_accumulator_native.py').read_text().replace('probe_tick_accumulator','probe_elapsed_accumulator').replace('65536*6*4','65536*8*4').replace("'tick-accumulator-'","'elapsed-accumulator-'").replace('sfm64-tick-accumulator-native-validation','sfm64-elapsed-accumulator-native-validation').replace('Original contiguous engine1018243a..1018248e','Original engine10182418..10182430 elapsed-add prefix plus1018243a..1018248e').replace('raw accumulator/interval','raw accumulator/elapsed/interval and retain branch').replace('Elapsed addition, full scheduler','Full scheduler')
Path('work/verify_elapsed_accumulator_native.py').write_text(v)
