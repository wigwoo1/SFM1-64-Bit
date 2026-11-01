// Native CPU image read, metadata and mip selection. No conversion/GPU calls.
// Overrides are explicit misses; hardware limits/configuration are supplied.
#define main precedingTextureManagerMain
#include "probe_texture_manager_miss.cpp"
#undef main
static uint32_t prefixSp=0,prefixIp=0;
__declspec(naked) static void __cdecl prefix(uint32_t,void*,char*,void*){
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
        push dword ptr [esp+32]
        push dword ptr [esp+32]
        call eax
    finished:
        pop edi
        pop esi
        pop ebx
        pop ebp
        ret
    }
}
struct ImageFixture:Fixture{
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
    static ImageFixture&self(){return *static_cast<ImageFixture*>(active);}
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
        if(slot==3){auto*s=reinterpret_cast<uint32_t*>(c.Esp);need(c.Ecx==0x1011accc||c.Ecx==0x1011ac04,"override lookup receiver");need(std::string(reinterpret_cast<const char*>(s[2]))==f.textureName,"override name");*reinterpret_cast<uint16_t*>(s[1])=0xffff;f.trace+="OVERRIDE MISS\n";PrivateX86BoundaryObserverFixture::returnCdecl(c,s[1]);c.Esp+=8;return;}
        if(slot>=2||f.stop){std::cerr<<"PREFIX_STOP slot="<<slot<<" stop="<<f.stop<<" eip="<<std::hex<<c.Eip<<" resume="<<prefixIp<<" sp="<<prefixSp<<'\n';throw std::runtime_error("single prefix stop");}f.stop=slot+1;f.accepted=slot==0;f.parsed=reinterpret_cast<void*>(c.Esi);
        f.trace+=slot==0?"IMAGE ACCEPTED\n":"ERROR DISPATCH\n";c.Esp=prefixSp;c.Eip=prefixIp;
    }
    explicit ImageFixture(const char*path):Fixture(path),scratch(0x200000,0){
        allocators[5]=reinterpret_cast<uintptr_t>(&releaseImage);
        hardwareTable.fill(reinterpret_cast<uintptr_t>(&unexpected));hardwareTable[6]=reinterpret_cast<uintptr_t>(&hardwareWidth);hardwareTable[7]=reinterpret_cast<uintptr_t>(&hardwareHeight);hardwareTable[33]=reinterpret_cast<uintptr_t>(&hardwareDepth);hardware=reinterpret_cast<uintptr_t>(hardwareTable.data());word(0x1059f4,reinterpret_cast<uintptr_t>(&hardware));
        files.fill(reinterpret_cast<uintptr_t>(&unexpected));positions.fill(reinterpret_cast<uintptr_t>(&unexpected));files[71]=reinterpret_cast<uintptr_t>(&openFile);files[72]=reinterpret_cast<uintptr_t>(&readFile);files[85]=reinterpret_cast<uintptr_t>(&constraints);positions[3]=reinterpret_cast<uintptr_t>(&close);positions[4]=reinterpret_cast<uintptr_t>(&seek);
        filesystem={reinterpret_cast<uintptr_t>(files.data()),reinterpret_cast<uintptr_t>(positions.data())};word(0x1255c4,reinterpret_cast<uintptr_t>(filesystem.data()));services[143]=reinterpret_cast<uintptr_t>(&pathID);
        word(0x11aaa0,reinterpret_cast<uintptr_t>(scratch.data()));word(0x11aaa4,uint32_t(scratch.size()));word(0x951e4,reinterpret_cast<uintptr_t>(&warning));word(0x951b8,reinterpret_cast<uintptr_t>(&warning));
    }
};
int main(int argc,char**argv){try{
    need(argc==4&&!std::ifstream(argv[3]).good(),"image query fresh-output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"probe IO");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    auto fault=AddVectoredExceptionHandler(0,&Fixture::fault);ImageFixture f(argv[1]);PrivateX86BoundaryObserverFixture observer({0x10030097,0x1002ef10,0x10073930,0x100733b0},ImageFixture::boundary);observer.configure(true);
    const auto count=read(in);uint32_t successes=0;need(count&&count<=4096,"case count");
    for(uint32_t serial=0;serial<count;++serial){
        f.requested=read(in);f.picmip=read(in);f.maxWidth=read(in);f.maxHeight=read(in);f.fullMips=read(in);f.clamp=read(in);need(f.picmip<=3&&f.maxWidth>=4&&f.maxHeight>=4&&f.fullMips<=1&&f.clamp<=32,"explicit image configuration");
        f.mode=read(in);need(f.mode<=2,"file availability mode");f.initialName=text(in);f.textureName=text(in);const auto source=text(in);
        // Source paths are test-driver-supplied read-only snapshots, never the
        // native logical filename passed to OpenEx. No write interface is exposed.
        std::ifstream sourceFile(source,std::ios::binary);need(bool(sourceFile),"source snapshot");f.bytes.assign(std::istreambuf_iterator<char>(sourceFile),{});need(f.bytes.size()<=0x200000,"snapshot extent");
        f.opens=f.reads=f.closes=f.seeks=f.position=f.accepted=f.stop=0;f.open=false;f.parsed=nullptr;f.trace.clear();std::fill(f.scratch.begin(),f.scratch.end(),uint8_t(0));f.word(0x11aa9c,0);f.begin(true);f.word(0xfc7ec,f.picmip);f.image.base[0x11aa98]=uint8_t(f.fullMips);
        std::array<uint8_t,0x68>texture{};using Construct=void*(__thiscall*)(void*);reinterpret_cast<Construct>(0x1002fb00)(texture.data());std::memcpy(texture.data()+0x14,&f.requested,4);const auto clamp16=uint16_t(f.clamp);std::memcpy(texture.data()+0x3a,&clamp16,2);std::array<char,260>filename{};std::memcpy(filename.data(),f.initialName.data(),f.initialName.size());uint32_t callbackOutput=0;f.expectedOutput=&callbackOutput;
        prefix(0x1002fdb0,texture.data(),filename.data(),&callbackOutput);need(f.stop!=0,"prefix boundary reached");f.guards();f.image.verify();
        const auto half=[&](size_t at){uint16_t n;std::memcpy(&n,texture.data()+at,2);return uint32_t(n);};
        bool zeroHandles=true;if(word(texture.data(),0x40)){auto*h=reinterpret_cast<const uint32_t*>(word(texture.data(),0x40));for(uint32_t i=0;i<half(0x32);++i)zeroHandles&=h[i]==0;}
        std::vector<uint32_t>record{serial,f.accepted,f.opens,f.reads,f.closes,word(texture.data(),0x14),word(texture.data(),0x18),word(texture.data(),0x20),half(0x24),half(0x26),half(0x28),half(0x2a),half(0x2c),half(0x2e),half(0x30),half(0x32),half(0x3e),word(texture.data(),4),word(texture.data(),8),word(texture.data(),12),uint32_t(zeroHandles)};
        std::vector<uint8_t>pixels;
        if(f.accepted){need(f.parsed!=nullptr&&word(f.parsed,0)==0x100dfa34,"original VTF object");const uint32_t methods[7]={0x1002e490,0x10073fe0,0x1002ce40,0x10078cf0,0x10078d00,0x10078d10,0x10012f50};using Get=uint32_t(__thiscall*)(void*);for(auto method:methods)record.push_back(reinterpret_cast<Get>(method)(f.parsed));++successes;
            using Size=uint32_t(__thiscall*)(void*,uint32_t);using Data=const uint8_t*(__thiscall*)(void*,uint32_t,uint32_t,uint32_t);
            for(uint32_t mip=0;mip<record[27];++mip){const auto n=reinterpret_cast<Size>(0x10078e90)(f.parsed,mip);need(n&&n<=8192,"mip extent");for(uint32_t frame=0;frame<record[25];++frame){const auto*data=reinterpret_cast<Data>(0x1007aac0)(f.parsed,frame,0,mip);bool bounded=false;for(const auto&block:f.owned)if(block.live){const auto start=reinterpret_cast<uintptr_t>(block.raw+16),pos=reinterpret_cast<uintptr_t>(data);if(pos>=start&&pos-start<=block.size&&n<=block.size-(pos-start))bounded=true;}need(bounded,"owned native image bounds");pixels.insert(pixels.end(),reinterpret_cast<const uint8_t*>(&n),reinterpret_cast<const uint8_t*>(&n)+4);pixels.insert(pixels.end(),data,data+n);}}
        }else record.resize(28,0);
        f.guards();f.image.verify();out.write(reinterpret_cast<const char*>(record.data()),std::streamsize(record.size()*4));text(out,filename.data());text(out,f.trace);text(out,std::string(pixels.begin(),pixels.end()));f.discard();
    }
    need(in.peek()==EOF&&in.eof(),"complete queries");out.flush();need(bool(out),"trace write");observer.configure(false);f.image.verify();RemoveVectoredExceptionHandler(fault);
    std::cout<<"PASS file_image_cases="<<count<<" accepted="<<successes<<" native_VTF_image_parser=1 full_image_read=1 gpu_calls=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
