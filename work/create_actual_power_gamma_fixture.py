from pathlib import Path
s=Path('work/test_gamma_startup_compare.cpp').read_text()
s=s.replace('#include "gamma_startup_native_init.h"','#include "gamma_startup_native_init.h"\n#include "actual_power_native_init.h"')
s=s.replace('static unsigned floatCalls,doubleCalls;','''static unsigned floatCalls,doubleCalls;
#if !defined(_WIN64)
static float(__vectorcall* actualFloat)(float,float);
static double(__vectorcall* actualDouble)(double,double);
#endif''')
s=s.replace('return x*y;}','''
#if !defined(_WIN64)
 return actualFloat(x,y);
#else
 return std::pow(x,y);
#endif
}''',1)
s=s.replace('return x*y;}','''
#if !defined(_WIN64)
 return actualDouble(x,y);
#else
 return std::pow(x,y);
#endif
}''',1)
needle='auto code=static_cast<unsigned char*>'
pos=s.index(needle)
s=s[:pos]+r'''auto power=static_cast<unsigned char*>(VirtualAlloc(nullptr,65536,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(power);
 std::ifstream source(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 const auto load=[&](unsigned offset,const unsigned char* expected,unsigned size,unsigned fileOffset){source.seekg(fileOffset);source.read(reinterpret_cast<char*>(power+offset),size);check(bool(source)&&!std::memcmp(power+offset,expected,size));};
 load(0,powerBytes0,sizeof(powerBytes0),powerFileOffset0);load(4096,powerBytes1,sizeof(powerBytes1),powerFileOffset1);load(16384,powerData,sizeof(powerData),powerDataFileOffset);
 for(const auto& f:powerFixups0){unsigned value=static_cast<unsigned>(reinterpret_cast<uintptr_t>(power+16384+f[1]-0x10153a00));std::memcpy(power+f[0],&value,4);}
 for(const auto& f:powerFixups1){unsigned value=static_cast<unsigned>(reinterpret_cast<uintptr_t>(power+16384+f[1]-0x10153a00));std::memcpy(power+4096+f[0],&value,4);}
 DWORD previous;check(VirtualProtect(power,65536,PAGE_EXECUTE_READ,&previous)&&FlushInstructionCache(GetCurrentProcess(),power,65536));
 actualDouble=reinterpret_cast<double(__vectorcall*)(double,double)>(power);actualFloat=reinterpret_cast<float(__vectorcall*)(float,float)>(power+4096);
 '''+s[pos:]
s=s.replace('native-gamma-startup.bin','native-actual-power-gamma-startup.bin')
s=s.replace('check(VirtualFree(code,0,MEM_RELEASE));','check(VirtualFree(code,0,MEM_RELEASE)&&VirtualFree(power,0,MEM_RELEASE));')
s=s.replace('unsigned cases=0;','unsigned cases=0;\n#if defined(_WIN64)\n unsigned mismatchedCases=0;size_t differingBytes=0;\n#endif')
start=s.index(' if(trace!=original)')
end=s.index('\n#endif',start)
s=s[:start]+''' if(trace!=original){++mismatchedCases;for(size_t i=0;i<trace.size();++i)if(trace[i]!=original[i])++differingBytes;}
 // Compare every argument independently before assessing output parity.
 constexpr size_t argumentBytes=1536*8+5632*16;
 check(!std::memcmp(trace.data(),original.data(),argumentBytes));
'''+s[end:]
s=s.replace(' std::printf("gamma_startup_%zu_bit_controlled_power_cases=%u passed\\n",sizeof(void*)*8,cases);','''
#if defined(_WIN64)
 std::printf("actual_power_gamma_%zu_bit_cases=%u argument_match=passed mismatched_table_cases=%u differing_bytes=%zu\\n",sizeof(void*)*8,cases,mismatchedCases,differingBytes);
#else
 std::printf("actual_power_gamma_%zu_bit_cases=%u original_tables_recorded=passed\\n",sizeof(void*)*8,cases);
#endif''')
Path('work/test_actual_power_gamma_startup.cpp').write_text(s)
for suffix in ['native','compare']:
 b=Path('work/build_gamma_startup_'+suffix+'.cmd').read_text().replace('test_gamma_startup_compare.cpp','test_actual_power_gamma_startup.cpp').replace('gamma_startup_native','actual_power_gamma_startup_native').replace('gamma_startup_compare','actual_power_gamma_startup_compare')
 Path('work/build_actual_power_gamma_startup_'+suffix+'.cmd').write_text(b)

