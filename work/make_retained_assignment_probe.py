from pathlib import Path
s=Path('work/probe_native_scalar_reference_storage.cpp').read_text()
prefix=s[:s.index('static std::array<std::array<uint32_t,64>')]
redirect=s[s.index('static void redirect('):s.index('static std::array<uint32_t,8>events')]
mapping=s[s.index('int main('):s.index(' redirect(0x10359940')]
callbacks=r'''
static std::array<std::array<uint32_t,4>,5>records{};static std::array<std::array<uint32_t,16>,4>elements{};static std::array<uint32_t,64>output{};static unsigned cursor=0;static uint32_t*storage=nullptr;static bool rejectClass=false;
static void emit(uint32_t code,uint32_t h){output[cursor++]=code;output[cursor++]=h;output[cursor++]=*storage;}
static void* __fastcall lookup(void*,void*,uint32_t h,void*optional){if(optional)throw std::runtime_error("optional");emit(1,h);return h<5?records[h].data():nullptr;}
static void* __fastcall resolveTarget(void*,void*,uint32_t h){emit(2,h);return h<4?elements[h].data():nullptr;}
static bool __fastcall match(void*self,void*,uint32_t symbol){if(symbol!=42)throw std::runtime_error("class symbol");uint32_t h=static_cast<uint32_t*>(self)[1];emit(3,h);return !(rejectClass&&(h&1));}
'''
body=r'''
 redirect(0x103596d0,reinterpret_cast<void*>(&lookup));std::array<uint32_t,32>vt{};vt[0x48/4]=uint32_t(&resolveTarget);std::array<uint32_t,1>model{uint32_t(vt.data())};*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(model.data());std::vector<uint32_t>framework(0xc19c/4+1);framework[0]=uint32_t(vt.data());*reinterpret_cast<uint32_t*>(0x10e9a518)=uint32_t(framework.data());*reinterpret_cast<uint32_t*>(0x10e9a684)=42;std::array<uint32_t,5>classVt{};classVt[4]=uint32_t(&match);using Assign=void(__thiscall*)(void*,uint32_t);
 for(unsigned seed=0;seed<784;++seed){unsigned old=seed%7,incoming=seed/7%7;uint32_t value=old==6?0xffffffffu:old;storage=&value;rejectClass=(seed/196%2)!=0;output.fill(0);cursor=0;framework[0xc198/4]=0xa5;const uint32_t counts[]={0,1,1023,17};for(unsigned h=0;h<5;++h)records[h]={h,0xc0000456u|(counts[seed/49%4]<<20),0xfe,0xffffffffu};for(unsigned h=0;h<4;++h){elements[h].fill(0);elements[h][0]=uint32_t(classVt.data());elements[h][1]=h;elements[h][0x38/4]=(seed/392%2)?0:0xffffffffu;}
 reinterpret_cast<Assign>(0x1025bf80)(&value,incoming==6?0xffffffffu:incoming);for(unsigned h=0;h<5;++h){output[48+h*2]=records[h][1];output[49+h*2]=records[h][2];}output[58]=value;output[59]=framework[0xc198/4];output[60]=cursor;oracle.write(reinterpret_cast<const char*>(output.data()),sizeof(output));}
 std::cout<<"native_retained_assignment_cases=784\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
'''
Path('work/probe_native_retained_assignment.cpp').write_text(prefix+redirect+callbacks+mapping+body)
