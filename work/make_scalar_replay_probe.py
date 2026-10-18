from pathlib import Path
s=Path('work/probe_native_scalar_reference_storage.cpp').read_text()
prefix=s[:s.index('static std::array<std::array<uint32_t,64>')]
redirect=s[s.index('static void redirect('):s.index('static std::array<uint32_t,8>events')]
mapping=s[s.index('int main('):s.index(' redirect(0x10359940')]
callbacks=r'''
static std::array<uint32_t,16>output{};static unsigned count=0;static bool ownerPresent=false,attributePresent=false;static std::array<uint32_t,8>attribute{};
static void emit(uint32_t c,uint32_t arg){output[count++]=c;output[count++]=arg;}
static void* __fastcall resolveOwner(void*,void*,uint32_t h){emit(1,h);return ownerPresent?reinterpret_cast<void*>(1):nullptr;}
static void* __fastcall findAttribute(void*,void*,uint32_t name){emit(2,name==0x10a8a4f0?0xffffffffu:name);return attributePresent?attribute.data():nullptr;}
static void __fastcall writeAttribute(void*self,void*,const uint32_t*value){if(self!=attribute.data())throw std::runtime_error("writer attribute");emit(3,*value);}
'''
body=r'''
 redirect(0x1035dd50,reinterpret_cast<void*>(&findAttribute));redirect(0x10366f90,reinterpret_cast<void*>(&writeAttribute));std::array<uint32_t,32>vt{};vt[0x48/4]=uint32_t(&resolveOwner);std::array<uint32_t,1>model{uint32_t(vt.data())};*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(model.data());using Replay=void(__thiscall*)(void*);
 for(unsigned seed=0;seed<512;++seed){ownerPresent=(seed&1)!=0;attributePresent=(seed&2)!=0;attribute.fill(0);attribute[4]=(seed&4)?0x21:1;std::array<uint32_t,9>record{};record[5]=(seed&8)?0xffffffffu:42;record[6]=seed/16;record[7]=(seed&16)?0xffffffffu:seed+100;record[8]=(seed&32)?0xffffffffu:seed+200;output.fill(0);count=0;reinterpret_cast<Replay>((seed&64)?0x1036c190:0x1036c1f0)(record.data());output[15]=count;oracle.write(reinterpret_cast<const char*>(output.data()),sizeof(output));}
 std::cout<<"native_scalar_replay_cases=512\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
'''
Path('work/probe_native_scalar_replay.cpp').write_text(prefix+redirect+callbacks+mapping+body)
