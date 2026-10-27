// Original VALUE comparison only: not an original store/draw transaction.
#define SFM64_NATIVE_GET_STRING
#define main preservedGetterReferenceMain
#include "probe_material_get_string.cpp"
#undef main
static void record(std::ostream&out,const std::array<uint8_t,40>&object){
 uint32_t flags=0,integer=0;std::array<uint32_t,4>values{};std::memcpy(&flags,object.data()+28,4);std::memcpy(&integer,object.data()+8,4);std::memcpy(values.data(),object.data()+12,16);put(out,flags&0x7fu);put(out,integer);for(auto v:values)put(out,v);
 std::string active;if((flags&15u)==1){uint32_t pointer;std::memcpy(&pointer,object.data()+4,4);need(pointer!=0,"native active string");active=reinterpret_cast<const char*>(uintptr_t(pointer));}text(out,active);
}
int main(int argc,char**argv){try{
 need(argc==4&&!std::ifstream(argv[3]).good(),"private image query fresh-output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"native scope value IO");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);AddVectoredExceptionHandler(1,&fault);GetFixture fixture(argv[1]);unsigned oldControl=0,control=0;_controlfp_s(&oldControl,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);auto mx=_mm_getcsr();_mm_setcsr(0x1f80);auto count=read(in);need(count&&count<4096,"query count");put(out,count);unsigned allocations=0,releases=0;
 for(unsigned i=0;i<count;++i){auto alpha=text(in);auto bits=read(in);need(!alpha.empty()&&alpha.size()<=128,"alpha scope");fixture.begin(true);std::array<uint8_t,40>object{};using Ctor=void*(__thiscall*)(void*,void*,const char*,const char*);reinterpret_cast<Ctor>(0x10017780)(object.data(),nullptr,nullptr,alpha.c_str());record(out,object);
  using Get=float(__thiscall*)(void*);float before=reinterpret_cast<Get>(0x10017c20)(object.data());uint32_t backup;std::memcpy(&backup,&before,4);
  using Set=void(__thiscall*)(void*,uint32_t);reinterpret_cast<Set>(0x10019350)(object.data(),bits);record(out,object);reinterpret_cast<Set>(0x10019350)(object.data(),backup);record(out,object);
  using Clear=void(__thiscall*)(void*);reinterpret_cast<Clear>(0x1001a0a0)(object.data());need(fixture.allocations==fixture.releases,"native source string lifetime");allocations+=fixture.allocations;releases+=fixture.releases;fixture.image.verify();
 }
 need(in.peek()==std::char_traits<char>::eof(),"query tail");out.flush();need(bool(out),"native output complete");_mm_setcsr(mx);_controlfp_s(&control,oldControl,_MCW_RC|_MCW_PC);std::cout<<"PASS native_scope_value_cases="<<count<<" allocations="<<allocations<<" releases="<<releases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
