from pathlib import Path
s=Path('work/probe_native_scalar_reference_storage.cpp').read_text()
prefix=s[:s.index('static std::array<std::array<uint32_t,64>')]
redirect=s[s.index('static void redirect('):s.index('static std::array<uint32_t,8>events')]
mapping=s[s.index('int main('):s.index(' redirect(0x10359940')]
callbacks=r'''
#include "quaternion_write_fixture.h"
static std::array<uint32_t,32>output{};static unsigned cursor=0;static uint32_t*current=nullptr;static uint16_t*flags=nullptr;static bool allowed=false;
static void emit(uint32_t c,uint32_t arg){output[cursor++]=c;output[cursor++]=arg;output[cursor++]=quaternionWriteHash(current);output[cursor++]=*flags;}
static bool __fastcall permission(void*,void*){emit(1,0);if(allowed)*flags|=0x4080;return allowed;}
static bool __fastcall undo(void*,void*,void*){emit(2,0);return false;}
static void __fastcall notify(void*,void*,void*){emit(3,0);}
static void __fastcall dirty(void*,void*,uint32_t value){emit(4,value&255);}
'''
body=r'''
 redirect(0x103623b0,reinterpret_cast<void*>(&permission));redirect(0x1035eff0,reinterpret_cast<void*>(&dirty));std::array<uint32_t,100>vt{};vt[0xf8/4]=uint32_t(&undo);std::array<uint32_t,1>model{uint32_t(vt.data())};*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(model.data());std::vector<uint32_t>framework(0xc184/4+1);*reinterpret_cast<uint32_t*>(0x10e9a518)=uint32_t(framework.data());std::array<uint32_t,1>ownerVt{uint32_t(&notify)};using Write=void(__thiscall*)(void*,const uint32_t*);
 for(unsigned seed=0;seed<1024;++seed){auto f=quaternionWriteFixture(seed);auto values=f.old;current=values.data();allowed=f.allow;std::array<uint32_t,8>attribute{};std::array<uint32_t,24>owner{};owner[0]=uint32_t(ownerVt.data());owner[0x3c/4]=f.ownerEnabled?0x40000:0;attribute[1]=uint32_t(values.data());attribute[2]=uint32_t(owner.data());attribute[4]=13|(f.notifyFlag?0x100:0);flags=reinterpret_cast<uint16_t*>(attribute.data()+4);framework[0xc180/4]=0x80;output.fill(0);cursor=0;reinterpret_cast<Write>(0x1036d270)(attribute.data(),f.incoming.data());for(unsigned i=0;i<4;++i)output[24+i]=values[i];output[28]=*flags;output[29]=framework[0xc180/4];output[30]=cursor;oracle.write(reinterpret_cast<const char*>(output.data()),sizeof(output));}
 std::cout<<"native_quaternion_write_cases=1024\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
'''
Path('work/probe_native_quaternion_write.cpp').write_text(prefix+redirect+callbacks+mapping+body)
