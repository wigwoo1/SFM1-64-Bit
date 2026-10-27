// Reference-only registry investigation. No owned runtime behavior is changed.
#define SFM64_NATIVE_OBJECT_PARAMETERS
#define SFM64_OBJECT_PARAMETER_MAIN registryPriorParameterMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#include "material_equals_client_native_fixture.h"
#include <map>
#include <cctype>
struct RegistryFixture:MaterialEqualsClientNativeFixture {
 std::map<void*,uint32_t>sizes;unsigned reallocations=0;
 static RegistryFixture&current(){return static_cast<RegistryFixture&>(self());}
 static void* __fastcall alloc(void*,void*,uint32_t n){auto&f=current();gate(n&&n<=65536,"bounded registry allocation");auto*p=std::malloc(n);gate(p!=nullptr,"host allocation");f.blocks.insert(p);f.sizes.emplace(p,n);++f.allocations;return p;}
 static void* __fastcall resize(void*,void*,void*p,uint32_t n){auto&f=current();gate(n&&n<=65536&&f.sizes.count(p)==1,"checked registry realloc");auto*q=std::malloc(n);gate(q!=nullptr,"host realloc allocation");std::memcpy(q,p,std::min(n,f.sizes.at(p)));f.sizes.erase(p);gate(f.blocks.erase(p)==1,"realloc tracked allocation");std::free(p);++f.releases;f.blocks.insert(q);f.sizes.emplace(q,n);++f.allocations;++f.reallocations;return q;}
 static void __fastcall freeBlock(void*,void*,void*p){auto&f=current();if(!p)return;gate(f.sizes.erase(p)==1&&f.blocks.erase(p)==1,"checked registry release");std::free(p);++f.releases;}
 explicit RegistryFixture(const char*path):MaterialEqualsClientNativeFixture(path){
  heapMethods[1]=reinterpret_cast<uintptr_t>(&alloc);heapMethods[3]=reinterpret_cast<uintptr_t>(&resize);heapMethods[5]=reinterpret_cast<uintptr_t>(&freeBlock);
  // Reproduce the singleton's data initialization and call its real symbol
  // constructor. Skip only CRT atexit installation; call teardown explicitly.
  std::memset(image.base+0x6fff08,0,0x50);word(0x6fff08,0x104ffe6c);word(0x6fff58,1);
  using Construct=void*(__thiscall*)(void*,int,int,bool);
  reinterpret_cast<Construct>(0x103ccba0)(image.base+0x6fff20,0,32,true);image.verify();
 }
 uint32_t factory(const char*name){uint16_t symbol=0xffff;using Find=uint16_t*(__thiscall*)(void*,uint16_t*,const char*);auto*p=reinterpret_cast<Find>(0x103cc950)(image.base+0x6fff20,&symbol,name);gate(p==&symbol,"native symbol output identity");if(symbol==0xffff)return 0;uint32_t count=0,table=0;std::memcpy(&count,image.base+0x6fff18,4);std::memcpy(&table,image.base+0x6fff0c,4);gate(symbol<count,"native symbol index bounded by factory vector");return reinterpret_cast<uint32_t*>(table)[symbol];}
 void*create(const char*name,bool provider){if(provider){using Create=void*(__thiscall*)(void*,const char*);return reinterpret_cast<Create>(0x1009a100)(nullptr,name);}using Create=void*(__thiscall*)(void*,const char*);return reinterpret_cast<Create>(0x10136290)(image.base+0x6fff08,name);}
 void finish(){using Destroy=void*(__thiscall*)(void*,uint32_t);reinterpret_cast<Destroy>(0x101362d0)(image.base+0x6fff08,0);image.verify();gate(sizes.empty()&&blocks.empty()&&allocations==releases,"complete native registry cleanup");}
};
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==4,"isolated original x86 reference only");gate(argc==3,"client-copy registrations.tsv");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
 std::ifstream input(argv[2]);gate(bool(input),"registration input");struct Entry{uint32_t callback,factory;std::string name;};std::vector<Entry>entries;Entry e{};while(input>>std::hex>>e.callback>>e.factory>>e.name)entries.push_back(e);gate(input.eof()&&entries.size()==74,"complete authenticated registration list");
 RegistryFixture f(argv[1]);gate(!f.factory("invis")&&!f.factory("Equals"),"initial registry empty");unsigned checks=2;
 for(const auto&entry:entries){using Register=void(__cdecl*)();reinterpret_cast<Register>(entry.callback)();gate(f.factory(entry.name.c_str())==entry.factory,"native registration retained exact factory");++checks;f.image.verify();std::cout<<"REGISTER\t"<<entry.name<<'\t'<<std::hex<<entry.factory<<std::dec<<'\n';}
 for(const auto&entry:entries)for(unsigned mode=0;mode<3;++mode){auto name=entry.name;for(size_t i=0;i<name.size();++i)name[i]=char(mode==0?std::tolower(static_cast<unsigned char>(name[i])):mode==1?std::toupper(static_cast<unsigned char>(name[i])):(i%2?std::tolower(static_cast<unsigned char>(name[i])):std::toupper(static_cast<unsigned char>(name[i]))));gate(f.factory(name.c_str())==entry.factory,"real symbol table case aliases");++checks;}
 for(const auto*name:{"invis","INVIS","Invis","AnimatedWeaponSheen","animatedweaponsheen","AnimatedWeaponSheen extra","_invis","invis_","weapon_invis_extra",""}){
  gate(!f.factory(name),"missing native factory");for(bool provider:{false,true}){gate(f.create(name,provider)==nullptr,"actual native Create returns null");++checks;}std::cout<<"MISSING\t"<<name<<"\tDIRECT_AND_PROVIDER_NULL\n";
 }
 gate(!f.factory(nullptr)&&!f.create(nullptr,false)&&!f.create(nullptr,true),"null query native refusal");++checks;
 for(const auto*name:{"Equals","Multiply","YellowLevel","ModelGlowColor"})for(bool provider:{false,true}){auto*p=f.create(name,provider);gate(p!=nullptr,"known native constructor via actual Create");using Construct=void*(__cdecl*)();auto*q=reinterpret_cast<Construct>(f.factory(name))();gate(q&&*static_cast<uint32_t*>(q)==*static_cast<uint32_t*>(p),"factory-created vtable identity");f.destroy(q);f.destroy(p);++checks;}
 for(const auto*name:{"spy_invis","vm_invis","weapon_invis"}){gate(f.factory(name)!=0,"three distinct registered invis-related names");++checks;}
 f.finish();std::cout<<"PASS registered="<<entries.size()<<" checks="<<checks<<" allocations="<<f.allocations<<" releases="<<f.releases<<" reallocations="<<f.reallocations<<" code=UNCHANGED_RX imports=NOT_RESOLVED SFM=NOT_OPENED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
