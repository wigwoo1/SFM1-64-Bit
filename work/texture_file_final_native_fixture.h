#pragma once
// Exact owned service fixture prefix from probe_texture_file_final.cpp.
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
