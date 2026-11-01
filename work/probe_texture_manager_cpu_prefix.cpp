// Execute the original CPU file reader through its natural return, including
// real IVTFTexture conversion. No original process or GPU service is used.
#define main precedingTextureManagerMain
#include "probe_texture_manager_miss.cpp"
#undef main
struct PostFixture:Fixture{
    // Full image Unserialize frees its initially null image buffer. Keep the
    // preceding fixture's strict checks for every non-null allocation identity.
    static void __fastcall releaseImage(void*p,void*q,void*value){if(value)Fixture::release(p,q,value);}
    std::array<uintptr_t,34>hardwareTable{};uintptr_t hardware=0;
    uint32_t requested=0,picmip=0,maxWidth=0,maxHeight=0,fullMips=0,clamp=0;
    static uint32_t __fastcall hardwareWidth(void*p,void*){auto&f=self();need(p==&f.hardware,"hardware receiver");f.trace+="HW W\n";return f.maxWidth;}
    static uint32_t __fastcall hardwareHeight(void*p,void*){auto&f=self();need(p==&f.hardware,"hardware receiver");f.trace+="HW H\n";return f.maxHeight;}
    static uint32_t __fastcall hardwareDepth(void*p,void*){auto&f=self();need(p==&f.hardware,"hardware receiver");f.trace+="HW D\n";return 1;}
    std::array<uintptr_t,87>files{};std::array<uintptr_t,5>positions{};
    std::array<uintptr_t,2>filesystem{};std::vector<uint8_t>scratch,bytes;
    std::string initialName,textureName;uint32_t mode=0,opens=0,reads=0,closes=0,seeks=0,position=0,accepted=0,stop=0;
    bool open=false;void*expectedOutput=nullptr;void*parsed=nullptr;
    static PostFixture&self(){return *static_cast<PostFixture*>(active);}
    static const char* __fastcall pathID(void*p,void*,void*out){auto&f=self();need(p==&f.service&&out==f.expectedOutput,"path-ID boundary");f.trace+="P GAME\n";return "GAME";}
    static void* __fastcall openFile(void*p,void*,const char*name,const char*options,uint32_t flags,const char*path){
        auto&f=self();need(p==f.filesystem.data()&&name&&options&&path&&std::string(options)=="rb"&&flags==0&&std::string(path)=="GAME","read-only open");++f.opens;need(f.opens<=2,"open retry bound");f.trace+="O "+std::string(name)+" rb 0 GAME\n";
        const bool exists=f.mode==1||(f.mode==2&&f.opens==2);if(!exists)return nullptr;need(!f.open,"single fixture file");f.open=true;f.position=0;return &f;
    }
    static uint32_t __fastcall readFile(void*p,void*,void*dest,uint32_t capacity,uint32_t wanted,void*handle){
        auto&f=self();need(p==f.filesystem.data()&&handle==&f&&f.open&&dest==f.scratch.data()&&capacity<=f.scratch.size()&&wanted<=capacity,"bounded file read");auto count=std::min(size_t(wanted),f.bytes.size()-std::min(size_t(f.position),f.bytes.size()));std::memcpy(dest,f.bytes.data()+std::min(size_t(f.position),f.bytes.size()),count);f.position+=uint32_t(count);++f.reads;f.trace+="R "+std::to_string(capacity)+" "+std::to_string(wanted)+" "+std::to_string(count)+"\n";return uint32_t(count);
    }
    static uint32_t __fastcall constraints(void*p,void*,void*handle,void*,void*a,void*b){auto&f=self();need(p==f.filesystem.data()&&handle==&f&&f.open&&!a&&!b,"IO constraint boundary");f.trace+="A 0\n";return 0;}
    static void __fastcall seek(void*p,void*,void*handle,uint32_t offset,uint32_t origin){auto&f=self();need(p==f.filesystem.data()+1&&handle==&f&&f.open&&origin==0,"seek boundary");f.position=offset;++f.seeks;f.trace+="S "+std::to_string(offset)+"\n";}
    static void __fastcall close(void*p,void*,void*handle){auto&f=self();need(p==f.filesystem.data()+1&&handle==&f&&f.open,"close boundary");f.open=false;++f.closes;f.trace+="X\n";}
    static void __cdecl warning(const char*format,...){auto&f=self();need(format!=nullptr,"warning format");f.trace+="W\n";}
    static void boundary(unsigned slot,CONTEXT&c){auto&f=self();
        if(slot==2){need(c.Ecx!=0,"symbol receiver");PrivateX86BoundaryObserverFixture::returnCdecl(c,uint32_t(reinterpret_cast<uintptr_t>(f.textureName.c_str())));return;}
        need(slot==3,"override boundary");auto*s=reinterpret_cast<uint32_t*>(c.Esp);need(c.Ecx==0x1011accc||c.Ecx==0x1011ac04,"override lookup receiver");need(std::string(reinterpret_cast<const char*>(s[2]))==f.textureName,"override name");*reinterpret_cast<uint16_t*>(s[1])=0xffff;f.trace+="OVERRIDE MISS\n";PrivateX86BoundaryObserverFixture::returnCdecl(c,s[1]);c.Esp+=8;
    }
    explicit PostFixture(const char*path):Fixture(path),scratch(0x200000,0){
        allocators[5]=reinterpret_cast<uintptr_t>(&releaseImage);
        hardwareTable.fill(reinterpret_cast<uintptr_t>(&unexpected));hardwareTable[6]=reinterpret_cast<uintptr_t>(&hardwareWidth);hardwareTable[7]=reinterpret_cast<uintptr_t>(&hardwareHeight);hardwareTable[33]=reinterpret_cast<uintptr_t>(&hardwareDepth);hardware=reinterpret_cast<uintptr_t>(hardwareTable.data());word(0x1059f4,reinterpret_cast<uintptr_t>(&hardware));
        files.fill(reinterpret_cast<uintptr_t>(&unexpected));positions.fill(reinterpret_cast<uintptr_t>(&unexpected));files[71]=reinterpret_cast<uintptr_t>(&openFile);files[72]=reinterpret_cast<uintptr_t>(&readFile);files[85]=reinterpret_cast<uintptr_t>(&constraints);positions[3]=reinterpret_cast<uintptr_t>(&close);positions[4]=reinterpret_cast<uintptr_t>(&seek);
        filesystem={reinterpret_cast<uintptr_t>(files.data()),reinterpret_cast<uintptr_t>(positions.data())};word(0x1255c4,reinterpret_cast<uintptr_t>(filesystem.data()));services[143]=reinterpret_cast<uintptr_t>(&pathID);
        word(0x11aaa0,reinterpret_cast<uintptr_t>(scratch.data()));word(0x11aaa4,uint32_t(scratch.size()));word(0x951e4,reinterpret_cast<uintptr_t>(&warning));word(0x951b8,reinterpret_cast<uintptr_t>(&warning));
    }
};
struct FinalFixture:PostFixture{
    std::array<uintptr_t,35>finalHardware{};
    std::vector<void*>scratchStack;
    uint32_t selected=0,hdrRaw=0,scratchAllocations=0,scratchReleases=0,nearestCalls=0,hdrCalls=0;
    static FinalFixture&self(){return *static_cast<FinalFixture*>(active);}
    static uint32_t __fastcall nearest(void*p,void*,uint32_t request,uint32_t flag){
        auto&f=self();need(p==&f.api&&flag==1,"nearest-format receiver/flag");++f.nearestCalls;
        f.trace+="FORMAT "+std::to_string(request)+" 1 "+std::to_string(f.selected)+"\n";return f.selected;
    }
    static uint32_t __fastcall hdr(void*p,void*){auto&f=self();need(p==&f.hardware,"HDR receiver");++f.hdrCalls;f.trace+="HDR "+std::to_string(f.hdrRaw)+"\n";return f.hdrRaw;}
    // tier0 MemAllocScratch(size) and MemFreeScratch() are a LIFO pair.
    // The free import has no pointer argument. Both use guarded owned storage.
    static void* __cdecl scratchAllocate(uint32_t n){auto&f=self();need(f.scratchStack.size()<16,"scratch nesting");auto*p=Fixture::allocate(nullptr,nullptr,n);f.scratchStack.push_back(p);++f.scratchAllocations;f.trace+="SCRATCH ALLOC "+std::to_string(n)+"\n";return p;}
    static void __cdecl scratchRelease(){auto&f=self();need(!f.scratchStack.empty(),"scratch balance");auto*p=f.scratchStack.back();Fixture::release(nullptr,nullptr,p);f.scratchStack.pop_back();++f.scratchReleases;f.trace+="SCRATCH FREE\n";}
    static void boundary(unsigned slot,CONTEXT&c){need(slot>=2,"unexpected native error/core boundary");PostFixture::boundary(slot,c);}
    explicit FinalFixture(const char*p):PostFixture(p){
        std::copy(hardwareTable.begin(),hardwareTable.end(),finalHardware.begin());finalHardware[34]=reinterpret_cast<uintptr_t>(&hdr);hardware=reinterpret_cast<uintptr_t>(finalHardware.data());
        apiTable[109]=reinterpret_cast<uintptr_t>(&nearest);
        word(0x951a0,reinterpret_cast<uintptr_t>(&scratchAllocate));word(0x95138,reinterpret_cast<uintptr_t>(&scratchRelease));
        image.base[0xfc805]=0;image.base[0xfc806]=0;
    }
    bool bounded(const uint8_t*p,size_t n)const{for(const auto&b:owned)if(b.live){auto start=reinterpret_cast<uintptr_t>(b.raw+16),pos=reinterpret_cast<uintptr_t>(p);if(pos>=start&&pos-start<=b.size&&n<=b.size-(pos-start))return true;}return false;}
};
static uint32_t prefixSp=0,prefixIp=0;
__declspec(naked) static void __cdecl shaderPrefix(uint32_t,void*,void*,const char*,uint32_t){
    __asm {
        push ebp
        push ebx
        push esi
        push edi
        mov prefixSp,esp
        mov eax,offset finished
        mov prefixIp,eax
        mov eax,[esp+20]
        mov ecx,[esp+24]
        push dword ptr [esp+36]
        push dword ptr [esp+36]
        push dword ptr [esp+36]
        call eax
    finished:
        pop edi
        pop esi
        pop ebx
        pop ebp
        ret
    }
}
struct CoreFixture:FinalFixture{
    void*preparedTexture=nullptr;bool stopped=false;
    static CoreFixture&self(){return *static_cast<CoreFixture*>(active);}
    static const char* __fastcall prefixPathID(void*p,void*,void*out){auto&f=self();need(p==&f.service&&out&&::word(out,0)==0,"path-ID receiver/output");f.expectedOutput=out;f.trace+="P GAME\n";return "GAME";}
    static void boundary(unsigned slot,CONTEXT&c){auto&f=self();
        if(slot<2){Fixture::boundary(slot,c);return;}
        if(slot==2){PostFixture::boundary(3,c);return;}
        need(!f.stopped&&c.Eip==0x100302d9,"single measured pre-GPU boundary");f.stopped=true;f.preparedTexture=reinterpret_cast<void*>(c.Esi);f.parsed=reinterpret_cast<void*>(c.Ebx);
        need(f.preparedTexture&&f.parsed&&::word(f.parsed,0)==0x100dfa34&&!f.open&&f.scratchStack.empty(),"CPU preparation completed");f.trace+="STOP BEFORE GPU ALLOCATION\n";c.Esp=prefixSp;c.Eip=prefixIp;
    }
    explicit CoreFixture(const char*p):FinalFixture(p){services[143]=reinterpret_cast<uintptr_t>(&prefixPathID);}
};
int main(int argc,char**argv){try{
    need(argc==4&&!std::ifstream(argv[3]).good(),"image queries fresh-output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"probe IO");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    auto fault=AddVectoredExceptionHandler(0,&Fixture::fault);CoreFixture f(argv[1]);PrivateX86BoundaryObserverFixture observer({0x100738e0,0x10073930,0x100733b0,0x100302d9},CoreFixture::boundary);observer.configure(true);
    const auto count=read(in);need(count&&count<=8192,"case count");uint32_t stopped=0;
    for(uint32_t serial=0;serial<count;++serial){std::array<uint32_t,12>q{};for(auto&n:q)n=read(in);f.initialName=text(in);f.textureName=text(in);const auto source=text(in);
        f.requested=q[0];f.picmip=q[1];f.maxWidth=q[2];f.maxHeight=q[3];f.fullMips=q[4];f.clamp=q[5];f.mode=q[6];f.selected=q[7];f.debugRaw=q[8];f.hdrRaw=q[9];f.deviceRaw=q[11];need(!f.clamp&&f.mode==1&&!(f.requested&0x4c00u),"fresh ordinary texture subset");
        std::ifstream file(source,std::ios::binary);need(bool(file),"snapshot input");f.bytes.assign(std::istreambuf_iterator<char>(file),{});f.opens=f.reads=f.closes=f.position=0;f.open=false;f.stopped=false;f.preparedTexture=f.parsed=nullptr;f.scratchAllocations=f.scratchReleases=f.nearestCalls=f.hdrCalls=0;need(f.scratchStack.empty(),"previous scratch retired");f.begin(true);
        std::array<uint8_t,0xa4>manager{};using Construct=void*(__thiscall*)(void*);reinterpret_cast<Construct>(0x10041e00)(manager.data());f.word(0x101a84,reinterpret_cast<uintptr_t>(manager.data()));f.word(0xfc7ec,f.picmip);f.image.base[0x11aa98]=uint8_t(f.fullMips);f.image.base[0xfc7fa]=uint8_t(q[10]);f.word(0x11aa9c,0);
        std::array<uint8_t,40>value{};using String=void*(__thiscall*)(void*,void*,const char*,const char*);reinterpret_cast<String>(0x10017780)(value.data(),nullptr,nullptr,f.textureName.c_str());std::array<uintptr_t,2>shader{0x1009c9dc,0x1009c9b4};f.trace.clear();
        shaderPrefix(0x10037a30,shader.data(),value.data(),"Prefix group",f.requested);auto*texture=f.stopped?f.preparedTexture:reinterpret_cast<void*>(word(value.data(),36));need(texture&&word(texture,0)==0x1009ad14,"real native texture");
        uint16_t cache;std::memcpy(&cache,manager.data()+0x16,2);const auto half=[&](size_t at){uint16_t n;std::memcpy(&n,static_cast<const uint8_t*>(texture)+at,2);return uint32_t(n);};using Get=uint32_t(__thiscall*)(void*);
        const auto alpha=reinterpret_cast<Get>(0x1002cdd0)(texture),error=reinterpret_cast<Get>(0x1002ce50)(texture);bool zero=true;if(word(texture,0x40)){auto*p=reinterpret_cast<const uint32_t*>(word(texture,0x40));for(uint32_t i=0;i<half(0x32);++i)zero&=p[i]==0;}
        std::vector<uint32_t>record{serial,uint32_t(f.stopped),cache,uint32_t(value[28]&15u),word(texture,0x1c),0,word(texture,0x14),word(texture,0x18),alpha,error,word(texture,0x20),half(0x24),half(0x26),half(0x2a),half(0x2c),half(0x30),half(0x32),half(0x3e)};
        if(f.stopped){for(auto method:{0x10078cf0u,0x10078d10u,0x1002e490u,0x10073fe0u,0x10012f50u,0x10078d00u})record.push_back(reinterpret_cast<Get>(method)(f.parsed));++stopped;}else record.resize(24,0);
        record.insert(record.end(),{f.opens,f.reads,f.closes,f.scratchAllocations,f.scratchReleases,f.nearestCalls,f.hdrCalls,uint32_t(zero)});
        std::string pixels;if(f.stopped){using Size=uint32_t(__thiscall*)(void*,uint32_t);using Data=const uint8_t*(__thiscall*)(void*,uint32_t,uint32_t,uint32_t);for(uint32_t mip=0;mip<record[22];++mip){const auto n=reinterpret_cast<Size>(0x10078e90)(f.parsed,mip);need(n&&n<=8192,"mip size");for(uint32_t frame=0;frame<record[23];++frame){const auto*p=reinterpret_cast<Data>(0x1007aac0)(f.parsed,frame,0,mip);need(f.bounded(p,n),"native CPU pixel bounds");pixels.append(reinterpret_cast<const char*>(&n),4);pixels.append(reinterpret_cast<const char*>(p),n);}}}
        const auto name=f.symbol(texture,0x10),group=f.symbol(texture,0x12);using Clear=void(__thiscall*)(void*);reinterpret_cast<Clear>(0x1001a0a0)(value.data());record[5]=word(texture,0x1c);f.guards();f.image.verify();out.write(reinterpret_cast<const char*>(record.data()),std::streamsize(record.size()*4));text(out,name);text(out,group);text(out,f.trace);text(out,pixels);f.discard();
    }
    need(in.peek()==EOF&&in.eof(),"query framing");out.flush();need(bool(out),"output write");observer.configure(false);f.image.verify();RemoveVectoredExceptionHandler(fault);std::cout<<"PASS manager_CPU_prefix_cases="<<count<<" stopped_before_GPU="<<stopped<<" normal_return="<<count-stopped<<" gpu_calls=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
