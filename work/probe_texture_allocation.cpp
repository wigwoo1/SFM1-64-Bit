// Original CTexture allocation command/state oracle. Supplied IDs are command
// service observations, not GPU resources or evidence of real upload.
#define main precedingTextureManagerMain
#include "probe_texture_manager_miss.cpp"
#undef main
struct AllocationFixture:Fixture{
    std::array<uintptr_t,211>fullApi{};std::array<uint32_t,10>handles{};
    uint32_t frames=0,mode=0,creates=0,depths=0;std::string events;
    static AllocationFixture&self(){return *static_cast<AllocationFixture*>(active);}
    static void __fastcall create(void*p,void*,uint32_t*out,uint32_t count,uint32_t w,uint32_t h,uint32_t d,uint32_t format,uint32_t mips,uint32_t copies,uint32_t flags,const char*name,const char*group){
        auto&f=self();need(p==&f.api&&out==f.handles.data()+1&&count<=f.frames&&name&&group,"create command identity/extent");++f.creates;f.events+="CREATE";for(auto n:{count,w,h,d,format,mips,copies,flags})f.events+=" "+std::to_string(n);f.events+="\nNAME "+std::string(name)+"\nGROUP "+std::string(group)+"\n";
        for(uint32_t i=0;i<count;++i)if(f.mode==1||(f.mode==2&&i==0))out[i]=0x4000u+i;
    }
    static uint32_t __fastcall createDepth(void*p,void*,uint32_t format,uint32_t w,uint32_t h,const char*name,uint32_t mode){auto&f=self();need(p==&f.api&&name&&mode<=1,"depth command identity");++f.depths;f.events+="DEPTH "+std::to_string(format)+" "+std::to_string(w)+" "+std::to_string(h)+" "+std::to_string(mode)+"\nNAME "+std::string(name)+"\n";return f.mode==1?0x5000u:0u;}
    explicit AllocationFixture(const char*p):Fixture(p){fullApi.fill(reinterpret_cast<uintptr_t>(&unexpected));std::copy(apiTable.begin(),apiTable.end(),fullApi.begin());fullApi[210]=reinterpret_cast<uintptr_t>(&create);fullApi[114]=reinterpret_cast<uintptr_t>(&createDepth);api=reinterpret_cast<uintptr_t>(fullApi.data());}
};
int main(int argc,char**argv){try{
    need(argc==4&&!std::ifstream(argv[3]).good(),"image queries fresh-output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"probe IO");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto fault=AddVectoredExceptionHandler(0,&Fixture::fault);AllocationFixture f(argv[1]);
    PrivateX86BoundaryObserverFixture observer({0x100738e0,0x10073930,uint32_t(reinterpret_cast<uintptr_t>(&Fixture::unused)),uint32_t(reinterpret_cast<uintptr_t>(&Fixture::unused))+1},Fixture::boundary);observer.configure(true);
    const auto count=read(in);need(count&&count<=16384,"case count");
    for(uint32_t serial=0;serial<count;++serial){std::array<uint32_t,11>q{};for(auto&n:q)n=read(in);const auto name=text(in),group=text(in);need(name.size()<80&&q[3]&&q[4]&&q[5]&&q[6]&&q[7]&&q[7]<=8&&q[10]<=2&&(!(q[0]&0x10000u)||q[7]>=2),"allocation input extent");
        f.begin(true);f.deviceRaw=q[9];f.frames=q[7];f.mode=q[10];f.creates=f.depths=0;f.events.clear();f.trace.clear();f.handles.fill(0);f.handles[0]=f.handles[q[7]+1]=0x9b9b9b9b;
        using Factory=void*(__cdecl*)(const char*,const char*);auto*texture=static_cast<uint8_t*>(reinterpret_cast<Factory>(0x10030460)(name.c_str(),group.c_str()));need(texture!=nullptr,"texture factory");
        const auto put=[&](size_t at,uint32_t n){std::memcpy(texture+at,&n,4);};const auto half=[&](size_t at,uint32_t n){need(n<=65535,"half input");const auto v=uint16_t(n);std::memcpy(texture+at,&v,2);};
        put(0x14,q[0]);put(0x18,q[1]);put(0x20,q[2]);half(0x2a,q[3]);half(0x2c,q[4]);half(0x2e,q[5]);half(0x30,q[6]);half(0x32,q[7]);put(0x4c,q[8]);put(0x40,uint32_t(reinterpret_cast<uintptr_t>(f.handles.data()+1)));std::array<uint8_t,0x68>before{};std::memcpy(before.data(),texture,before.size());
        using Allocate=uint32_t(__thiscall*)(void*);const auto result=reinterpret_cast<Allocate>(0x1002db40)(texture)&255u;auto expected=before;const auto status=::word(texture,0x18);std::memcpy(expected.data()+0x18,&status,4);need(std::memcmp(texture,expected.data(),expected.size())==0,"allocation metadata guard");need(f.handles[0]==0x9b9b9b9b&&f.handles[q[7]+1]==0x9b9b9b9b,"handle output guards");
        const std::array<uint32_t,5>record{serial,result,status,f.creates,f.depths};out.write(reinterpret_cast<const char*>(record.data()),sizeof(record));out.write(reinterpret_cast<const char*>(f.handles.data()+1),q[7]*4);text(out,f.trace+f.events);f.guards();f.image.verify();f.discard();
    }
    need(in.peek()==EOF&&in.eof(),"query framing");out.flush();need(bool(out),"trace write");observer.configure(false);f.image.verify();RemoveVectoredExceptionHandler(fault);std::cout<<"PASS native_allocation_cases="<<count<<" controlled_handle_ids=1 real_GPU_allocations=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
