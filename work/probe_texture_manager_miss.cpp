// Private native integration: CShaderSystem -> real texture manager/cache/factory
// -> CTexture download gates -> real material-variable SetTexture/Clear.
// Symbol pool and file/GPU core are explicit boundaries; no live SFM access.
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <deque>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "material_construction_native_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
static_assert(sizeof(void*)==4,"private x86 investigation only; not editor runtime");
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t read(std::istream&i){uint32_t n;need(bool(i.read(reinterpret_cast<char*>(&n),4)),"word input");return n;}
static std::string text(std::istream&i){auto n=read(i);need(n&&n<=259,"text extent");std::string s(n,'\0');need(bool(i.read(s.data(),n)),"text input");for(unsigned char c:s)need(c&&c<128,"ASCII prefix");return s;}
static void text(std::ostream&o,const std::string&s){auto n=uint32_t(s.size());o.write(reinterpret_cast<const char*>(&n),4);o.write(s.data(),n);}
static uint32_t word(const void*p,size_t offset){uint32_t n;std::memcpy(&n,static_cast<const uint8_t*>(p)+offset,4);return n;}
struct Fixture:MaterialConstructionNativeFixture{
    struct Guarded{uint8_t*raw;uint32_t size;bool live;};std::vector<Guarded>owned;
    std::deque<std::string>symbols;std::vector<void*>textures;
    std::array<uintptr_t,148>apiTable{};uintptr_t api=0;
    std::array<uintptr_t,6>debugTable{};uintptr_t debug=0;
    uint32_t deviceRaw=0,debugRaw=0,coreCalls=0;std::string trace;
    static Fixture&self(){return *static_cast<Fixture*>(active);}
    void guards()const{for(const auto&b:owned)if(b.live)for(size_t i=0;i<16;++i)need(b.raw[i]==0x9b&&b.raw[16+b.size+i]==0x9b,"allocator guards");}
    static void* __fastcall allocate(void*,void*,uint32_t n){auto&f=self();need(n&&n<=8192,"bounded allocation");auto*raw=static_cast<uint8_t*>(std::malloc(size_t(n)+32));need(raw!=nullptr,"host allocation");std::memset(raw,0x9b,size_t(n)+32);std::memset(raw+16,0xa5,n);f.owned.push_back({raw,n,true});return raw+16;}
    static void __fastcall release(void*,void*,void*p){auto&f=self();f.guards();for(auto&b:f.owned)if(b.live&&p==b.raw+16){b.live=false;std::free(b.raw);return;}throw std::runtime_error("unknown allocator release");}
    static void* __fastcall reallocate(void*,void*,void*p,uint32_t n){auto&f=self();if(!p)return allocate(nullptr,nullptr,n);for(const auto&b:f.owned)if(b.live&&p==b.raw+16){auto count=b.size;auto*next=allocate(nullptr,nullptr,n);std::memcpy(next,p,std::min(count,n));release(nullptr,nullptr,p);return next;}throw std::runtime_error("unknown realloc");}
    static uint32_t __fastcall config(void*p,void*){auto&f=self();need(p==&f.service,"configuration receiver");f.trace+="C 0\n";return 0;}
    static uint32_t __fastcall device(void*p,void*){auto&f=self();need(p==&f.api,"device receiver");f.trace+="D "+std::to_string(f.deviceRaw)+"\n";return f.deviceRaw;}
    static uint32_t __fastcall beginQuery(void*p,void*){auto&f=self();need(p==&f.service,"begin receiver");f.trace+="B\n";return 0x8642abcd;}
    static void __fastcall end(void*p,void*,uint32_t token){auto&f=self();need(p==&f.service&&token==0x8642abcd,"end receiver/token");f.trace+="E\n";}
    static uint32_t __fastcall loadEnabled(void*p,void*){auto&f=self();need(p==&f.debug,"load gate receiver");f.trace+="M "+std::to_string(f.debugRaw)+"\n";return f.debugRaw;}
    static void boundary(unsigned slot,CONTEXT&c){auto&f=self();auto*stack=reinterpret_cast<const uint32_t*>(c.Esp);
        if(slot==0){const auto*name=reinterpret_cast<const char*>(stack[1]);need(name!=nullptr,"explicit symbol name");std::string s(name);need(s.size()<=259,"symbol extent");uint16_t id=0;
            for(;id<f.symbols.size()&&f.symbols[id]!=s;++id){}if(id==f.symbols.size()){need(f.symbols.size()<4096,"symbol bound");f.symbols.push_back(s);}std::memcpy(reinterpret_cast<void*>(c.Ecx),&id,2);PrivateX86BoundaryObserverFixture::returnCdecl(c,c.Ecx);c.Esp+=4;
        }else if(slot==1){uint16_t id;std::memcpy(&id,reinterpret_cast<const void*>(c.Ecx),2);need(id<f.symbols.size(),"symbol identity");PrivateX86BoundaryObserverFixture::returnCdecl(c,uint32_t(reinterpret_cast<uintptr_t>(f.symbols[id].c_str())));
        }else if(slot==2){need(c.Ecx!=0,"native texture receiver");++f.coreCalls;f.trace+="L BOUNDARY\n";PrivateX86BoundaryObserverFixture::returnCdecl(c);}
        else throw std::runtime_error("unexpected native boundary");
    }
    static void __cdecl unused(){}
    static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xc0000005){std::cerr<<"PRIVATE_FAULT eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(89);}return EXCEPTION_CONTINUE_SEARCH;}
    explicit Fixture(const char*path):MaterialConstructionNativeFixture(path,true){
        allocators[1]=reinterpret_cast<uintptr_t>(&allocate);allocators[3]=reinterpret_cast<uintptr_t>(&reallocate);allocators[5]=reinterpret_cast<uintptr_t>(&release);
        apiTable.fill(reinterpret_cast<uintptr_t>(&unexpected));apiTable[147]=reinterpret_cast<uintptr_t>(&device);api=reinterpret_cast<uintptr_t>(apiTable.data());word(0x11b098,reinterpret_cast<uintptr_t>(&api));
        debugTable.fill(reinterpret_cast<uintptr_t>(&unexpected));debugTable[5]=reinterpret_cast<uintptr_t>(&loadEnabled);debug=reinterpret_cast<uintptr_t>(debugTable.data());word(0x11b0a0,reinterpret_cast<uintptr_t>(&debug));
        services[106]=reinterpret_cast<uintptr_t>(&beginQuery);services[107]=reinterpret_cast<uintptr_t>(&end);services[142]=reinterpret_cast<uintptr_t>(&config);image.base[0x109a60]=0;image.base[0x109a61]=0;
    }
    std::string symbol(const void*texture,size_t offset){uint16_t id;std::memcpy(&id,static_cast<const uint8_t*>(texture)+offset,2);need(id<symbols.size(),"texture symbol");return symbols[id];}
    uint32_t identity(void*p){for(uint32_t i=0;i<textures.size();++i)if(p==textures[i])return i+1;textures.push_back(p);return uint32_t(textures.size());}
    void discard(){guards();for(auto&b:owned)if(b.live){b.live=false;std::free(b.raw);}owned.clear();symbols.clear();textures.clear();}
    ~Fixture(){discard();}
};
int main(int argc,char**argv){try{
    need(argc==4&&!std::ifstream(argv[3]).good(),"image queries fresh-output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"probe IO");
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto fault=AddVectoredExceptionHandler(0,&Fixture::fault);Fixture f(argv[1]);
    PrivateX86BoundaryObserverFixture observer({0x100738e0,0x10073930,0x100301d0,uint32_t(reinterpret_cast<uintptr_t>(&Fixture::unused))},Fixture::boundary);observer.configure(true);
    const auto sequences=read(in);need(sequences&&sequences<=4096,"sequence count");uint32_t requests=0,totalCoreCalls=0;
    for(uint32_t serial=0;serial<sequences;++serial){
        const auto count=read(in);need(count&&count<=256,"sequence extent");std::array<uint8_t,0xa4>manager{};
        using Construct=void*(__thiscall*)(void*);reinterpret_cast<Construct>(0x10041e00)(manager.data());f.word(0x101a84,reinterpret_cast<uintptr_t>(manager.data()));f.symbols.clear();f.textures.clear();f.coreCalls=0;f.begin(true);
        for(uint32_t step=0;step<count;++step){
            const auto flags=read(in);f.deviceRaw=read(in);f.debugRaw=read(in);const auto name=text(in),group=text(in);need(group.size()<=255,"group extent");
            std::array<uint8_t,40>value{};using String=void*(__thiscall*)(void*,void*,const char*,const char*);using Load=void(__thiscall*)(void*,void*,const char*,uint32_t);using Clear=void(__thiscall*)(void*);
            reinterpret_cast<String>(0x10017780)(value.data(),nullptr,nullptr,name.c_str());std::array<uintptr_t,2>shader{0x1009c9dc,0x1009c9b4};f.trace.clear();
            reinterpret_cast<Load>(0x10037a30)(shader.data(),value.data(),group.c_str(),flags);need((value[28]&15u)==3,"real native typed assignment");
            auto*texture=reinterpret_cast<void*>(word(value.data(),36));need(texture&&texture!=reinterpret_cast<void*>(UINT32_MAX),"ordinary native texture result");const auto id=f.identity(texture);
            using Get=void*(__thiscall*)(void*);need(reinterpret_cast<Get>(0x10018130)(value.data())==texture,"actual GetTextureValue identity");
            need(word(texture,0)==0x1009ad14,"real CTexture vtable");uint16_t entries;std::memcpy(&entries,manager.data()+0x16,2);
            using Query=uint32_t(__thiscall*)(void*);const auto alpha=reinterpret_cast<Query>(0x1002cdd0)(texture),error=reinterpret_cast<Query>(0x1002ce50)(texture);
            std::array<uint32_t,10>record{serial,step,id,entries,word(texture,0x14),word(texture,0x18),alpha,error,word(texture,0x1c),0};const auto retainedName=f.symbol(texture,0x10),retainedGroup=f.symbol(texture,0x12);
            const auto trace=f.trace;reinterpret_cast<Clear>(0x1001a0a0)(value.data());record[9]=word(texture,0x1c);need(record[8]==1&&record[9]==0,"real native retain/release balance");
            for(auto*t:f.textures)need(word(t,0x1c)==0,"other cache references unchanged");f.guards();f.image.verify();out.write(reinterpret_cast<const char*>(record.data()),sizeof(record));text(out,retainedName);text(out,retainedGroup);text(out,trace);++requests;
        }
        totalCoreCalls+=f.coreCalls;f.discard();
    }
    need(in.peek()==EOF&&in.eof(),"query framing");out.flush();need(bool(out),"trace output");observer.configure(false);f.image.verify();RemoveVectoredExceptionHandler(fault);
    std::cout<<"PASS sequences="<<sequences<<" requests="<<requests<<" file_gpu_core_boundaries="<<totalCoreCalls<<" cache_insertion=ORIGINAL native_texture_objects=1 real_file_loads=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
