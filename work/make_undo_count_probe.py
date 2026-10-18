from pathlib import Path
s=Path('work/probe_native_scalar_reference_storage.cpp').read_text()
prefix=s[:s.index('static std::array<std::array<uint32_t,64>')]
redirect=s[s.index('static void redirect('):s.index('static std::array<uint32_t,8>events')]
mapping=s[s.index('int main('):s.index(' redirect(0x10359940')]
callbacks=r'''
static std::array<uint32_t,4>record{};static std::array<uint32_t,16>element{};static bool present=false;static unsigned context=0,queries=0;
static void* __fastcall lookup(void*,void*,uint32_t h,void*optional){if(h!=7||optional)throw std::runtime_error("record lookup");return present?record.data():nullptr;}
static void* __fastcall resolveTarget(void*,void*,uint32_t h){if(h!=7)throw std::runtime_error("resolve");++queries;return context==2?nullptr:element.data();}
'''
body=r'''
 redirect(0x103596d0,reinterpret_cast<void*>(&lookup));std::vector<uint32_t>framework(0xc19c/4+1);std::array<uint32_t,32>vt{};vt[0x48/4]=uint32_t(&resolveTarget);framework[0]=uint32_t(vt.data());using Count=void(__thiscall*)(void*,uint32_t,uint32_t);
 for(unsigned seed=0;seed<24576;++seed){context=seed/2048%3;present=(seed/6144%2)!=0;record={7,(seed%1024)<<20|0xc00abcdeu,(seed/12288%2)?0xfeu:0xffu,0xffffffffu};element[0x38/4]=context==0?0xffffffffu:0;framework[0xc198/4]=0xa5;queries=0;reinterpret_cast<Count>((seed/1024%2)?0x103599a0:0x10359850)(framework.data(),7,2);std::array<uint32_t,4>out{record[1],record[2],framework[0xc198/4],queries};oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));}
 std::cout<<"native_undo_target_count_cases=24576\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
'''
Path('work/probe_native_undo_target_count.cpp').write_text(prefix+redirect+callbacks+mapping+body)
