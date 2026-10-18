from pathlib import Path
s=Path('work/probe_time_snap.cpp').read_text().replace('recovered_time_snap_candidate.h','anchored_document_time_candidate.h')
s=s.replace('#include "anchored_document_time_candidate.h"','#include "anchored_document_time_candidate.h"\n#include "monotonic_engine_time_candidate.h"')
at=s.index('int main');s=s[:at]+'''#ifndef _WIN64
static bool gateValue;static float timeValue;
static bool __fastcall gateOracle(void*,void*){return gateValue;}
static float __fastcall timeOracle(void*,void*){return timeValue;}
#endif
'''+s[at:]
a=s.index(' std::ifstream input(argv[1]');b=s.index('#else',a)
s=s[:a]+''' std::ifstream input(argv[1],std::ios::binary);unsigned cases=0;std::array<uint32_t,14>r{};
 while(input.read(reinterpret_cast<char*>(r.data()),sizeof(r))){AnchoredDocumentTimeState state(r[0]);state.saved684=int32_t(r[1]);state.mode47c=int32_t(r[2]);for(unsigned j=0;j<4;++j)state.modes480[j]=int32_t(r[3+j]);state.flag69c=(r[7]&1)!=0;state.flag228=(r[7]&2)!=0;state.flag2a8=(r[7]&4)!=0;state.flag6c0=false;
 uint32_t retained=r[12];const auto converted=monotonicEngineTicksCandidate(r[11],retained);if(uint32_t(converted)!=r[9]||retained!=r[13])throw std::runtime_error("engine converter mismatch case "+std::to_string(cases));
 auto actual=anchoredDocumentTimeCandidate(state,[&]{return r[8]!=0;},[&]{return converted;},[]()->std::optional<float>{throw std::runtime_error("unexpected rate lookup");});if(uint32_t(actual)!=r[10])throw std::runtime_error("document time mismatch case "+std::to_string(cases));++cases;}
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");std::cout<<"x64_document_time_exact=65536\\n";
'''+s[b:]
a=s.index(' using Routine=');b=s.index(' output.flush()',a)
s=s[:a]+''' std::array<uintptr_t,48>table{};table[29]=reinterpret_cast<uintptr_t>(&gateOracle);table[47]=reinterpret_cast<uintptr_t>(&timeOracle);uintptr_t object=reinterpret_cast<uintptr_t>(table.data());
 for(uintptr_t address:{uintptr_t(0x10fd4398),uintptr_t(0x10f321d0)}){DWORD prior;if(!VirtualProtect(reinterpret_cast<void*>(address),4,PAGE_READWRITE,&prior))throw std::runtime_error("private clock state protection");}
 *reinterpret_cast<uintptr_t*>(0x10fd4398)=reinterpret_cast<uintptr_t>(&object);
 using Getter=int32_t*(__thiscall*)(void*,int32_t*);auto getter=reinterpret_cast<Getter>(0x1029a950);
 using Clock=int32_t*(__cdecl*)(int32_t*);auto clock=reinterpret_cast<Clock>(0x10275640);
 std::ofstream output(argv[1],std::ios::binary);uint32_t seed=0x29a950;const uint32_t floatBounds[]={0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc01234,0x7f801234};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,14>r{};for(auto&word:r){seed=seed*1664525u+1013904223u;word=seed;}if(i%3==0)r[0]=0x80000000u;r[2]=(i>>1)%3==0?0:r[2];for(unsigned j=0;j<4;++j)r[3+j]=((i>>(j+2))&1)?2:1;r[7]=i%8;r[8]=i&1;if(i<64){r[11]=floatBounds[i%8];r[12]=floatBounds[(i/8)%8];}
 std::array<uint32_t,0x700/4>owner{};owner[0x430/4]=r[0];owner[0x684/4]=r[1];owner[0x47c/4]=r[2];for(unsigned j=0;j<4;++j)owner[0x480/4+j]=r[3+j];auto bytes=reinterpret_cast<uint8_t*>(owner.data());bytes[0x69c]=uint8_t(r[7]&1);bytes[0x228]=uint8_t(r[7]&2);bytes[0x2a8]=uint8_t(r[7]&4);
 gateValue=r[8]!=0;std::memcpy(&timeValue,&r[11],4);*reinterpret_cast<uint32_t*>(0x10f321d0)=r[12];const auto original=owner;
 int32_t result=0;auto returned=getter(owner.data(),&result);if(returned!=&result||owner!=original)throw std::runtime_error("getter ownership");std::memcpy(&r[10],&result,4);
 // Capture the actual original time conversion as an explicit callback result.
 *reinterpret_cast<uint32_t*>(0x10f321d0)=r[12];int32_t converted=0;clock(&converted);std::memcpy(&r[9],&converted,4);r[13]=*reinterpret_cast<uint32_t*>(0x10f321d0);
 output.write(reinterpret_cast<const char*>(r.data()),sizeof(r));}
'''+s[b:]
s=s.replace('original_x86_time_snap','original_x86_document_time')
s=s.replace('> owner(allocation','> imageOwner(allocation')
Path('work/probe_document_time.cpp').write_text(s)
Path('work/build_document_time_probe.cmd').write_text(Path('work/build_time_snap_probe.cmd').read_text().replace('probe_time_snap','probe_document_time'))
v=Path('work/verify_time_snap_native.py').read_text().replace('probe_time_snap','probe_document_time').replace('original_x86_time_snap','original_x86_document_time').replace('x64_time_snap_exact','x64_document_time_exact').replace('65536*3*4','65536*14*4').replace("'time-snap-'","'document-time-'").replace('sfm64-time-snap-native-validation','sfm64-document-time-native-validation')
a=v.index("limits='");b=v.index("')",a)
v=v[:a]+"limits='Original full ifm1029a950 and10275640 compared with owned x64 monotonic conversion, controlled engine gate/current float/retained float and owner fields. Flag6c0=false excludes settings/snapping. Default masked FP; real owner lifecycle and live playback unfinished."+v[b:]
Path('work/verify_document_time_native.py').write_text(v)

