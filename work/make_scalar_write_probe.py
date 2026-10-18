from pathlib import Path
s=Path('work/probe_native_scalar_reference_storage.cpp').read_text()
prefix=s[:s.index('static std::array<std::array<uint32_t,64>')]
redirect=s[s.index('static void redirect('):s.index('static std::array<uint32_t,8>events')]
mapping=s[s.index('int main('):s.index(' redirect(0x10359940')]
callbacks=r'''
static std::array<uint32_t,16>events{};static unsigned count=0;static uint32_t*current=nullptr;static bool allowed=false;
static void event(uint32_t code,uint32_t h){events[count++]=code;events[count++]=h;events[count++]=*current;}
static bool __fastcall permission(void*,void*){event(1,0);return allowed;}
static bool __fastcall undo(void*,void*,void*){event(2,0);return false;}
static void __fastcall removeTarget(void*,void*,uint32_t h,void*){event(3,h);}
static void __fastcall addTarget(void*,void*,uint32_t h,void*){event(4,h);}
static void __fastcall dirty(void*,void*,uint32_t value){event(5,value&255);}
'''
body=r'''
 redirect(0x103623B0,reinterpret_cast<void*>(&permission));redirect(0x10359940,reinterpret_cast<void*>(&removeTarget));redirect(0x103597E0,reinterpret_cast<void*>(&addTarget));redirect(0x1035EFF0,reinterpret_cast<void*>(&dirty));
 std::array<uint32_t,100>vtable{};vtable[0xf8/4]=uint32_t(&undo);std::array<uint32_t,1>model{uint32_t(vtable.data())};*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(model.data());std::vector<uint32_t>framework(0xc184/4+1);*reinterpret_cast<uint32_t*>(0x10e9a518)=uint32_t(framework.data());
 using Write=void(__thiscall*)(void*,const uint32_t*);auto write=reinterpret_cast<Write>(0x10366f90);
 unsigned cases=0;for(unsigned old=0;old<17;++old)for(unsigned incoming=0;incoming<17;++incoming)for(unsigned allow=0;allow<2;++allow){std::array<uint32_t,8>attribute{};std::array<uint32_t,3>storage{old==16?0xffffffffu:old,0xffffffffu,0};attribute[1]=uint32_t(storage.data());attribute[4]=1;current=storage.data();uint32_t newValue=incoming==16?0xffffffffu:incoming;allowed=allow!=0;events.fill(0);count=0;framework[0xc180/4]=0;write(attribute.data(),&newValue);std::array<uint32_t,19>out{};std::copy(events.begin(),events.end(),out.begin());out[16]=storage[0];out[17]=count;out[18]=framework[0xc180/4];oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}
 std::cout<<"native_scalar_write_cases="<<cases<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
'''
Path('work/probe_native_scalar_write.cpp').write_text(prefix+redirect+callbacks+mapping+body)
