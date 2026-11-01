// Whole original upload with a real parsed/converted VTF object. Command service
// observations only: no original process, actual GPU upload or manager commit.
#include "texture_file_final_native_fixture.h"
static void append(std::string&s,uint32_t n){s.append(reinterpret_cast<const char*>(&n),4);}
struct UploadFixture:FinalFixture{
    std::vector<uint32_t>handles;
    uint32_t selects=0,uploads=0,frames=0;std::string events;
    static UploadFixture&self(){return *static_cast<UploadFixture*>(active);}
    static void __fastcall select(void*p,void*,uint32_t handle){auto&f=self();need(p==&f.api&&f.selects<f.frames&&handle==f.handles[f.selects+1],"select receiver/frame identity");append(f.events,1);append(f.events,f.selects);append(f.events,handle);++f.selects;}
    static void __fastcall upload(void*p,void*,uint32_t mip,uint32_t face,uint32_t destination,uint32_t slice,uint32_t w,uint32_t h,uint32_t source,uint32_t zero,const uint8_t*data){
        auto&f=self();need(p==&f.api&&f.selects&&face==0&&slice==0&&zero==0&&w&&h,"ordinary upload receiver/constant fields");
        need(source==0||source==3||source==12||source==13||source==15,"measured source format");const uint32_t size=(source==13||source==15)?((w+3)/4)*((h+3)/4)*(source==15?16u:8u):w*h*(source==3?3u:4u);
        using Data=const uint8_t*(__thiscall*)(void*,uint32_t,uint32_t,uint32_t);const auto*expected=reinterpret_cast<Data>(0x1007aac0)(f.parsed,f.selects-1,0,mip);need(data==expected&&f.bounded(data,size),"real VTF pixel identity/bounds");
        for(auto n:{2u,f.selects-1,mip,face,destination,slice,w,h,source,zero,size})append(f.events,n);f.events.append(reinterpret_cast<const char*>(data),size);++f.uploads;
    }
    explicit UploadFixture(const char*p):FinalFixture(p){apiTable[117]=reinterpret_cast<uintptr_t>(&select);apiTable[118]=reinterpret_cast<uintptr_t>(&upload);}
};
int main(int argc,char**argv){try{
    need(argc==4&&!std::ifstream(argv[3]).good(),"image queries fresh-output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"IO");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto fault=AddVectoredExceptionHandler(0,&Fixture::fault);UploadFixture f(argv[1]);PrivateX86BoundaryObserverFixture observer({0x1002ef10,0x100301d0,0x10073930,0x100733b0},FinalFixture::boundary);observer.configure(true);
    const auto count=read(in);need(count&&count<=8192,"cases");
    for(uint32_t serial=0;serial<count;++serial){std::array<uint32_t,17>q{};for(auto&n:q)n=read(in);f.initialName=text(in);f.textureName=text(in);const auto source=text(in);f.requested=q[0];f.picmip=q[1];f.maxWidth=q[2];f.maxHeight=q[3];f.fullMips=q[4];f.clamp=q[5];f.mode=q[6];f.selected=q[7];f.debugRaw=q[8];f.hdrRaw=q[9];need(f.mode==1&&q[12]<=1&&q[16]<=2,"ordinary fixture configuration");
        std::ifstream file(source,std::ios::binary);need(bool(file),"snapshot");f.bytes.assign(std::istreambuf_iterator<char>(file),{});need(f.bytes.size()<=0x200000,"snapshot extent");f.opens=f.reads=f.closes=f.seeks=f.position=0;f.open=false;f.parsed=nullptr;f.trace.clear();f.selects=f.uploads=0;f.events.clear();f.scratchAllocations=f.scratchReleases=0;need(f.scratchStack.empty(),"scratch retired");f.begin(true);f.word(0x11aa9c,0);f.word(0xfc7ec,f.picmip);f.image.base[0x11aa98]=uint8_t(f.fullMips);f.image.base[0xfc7fa]=uint8_t(q[10]);
        std::array<uint8_t,0x68>texture{};using Construct=void*(__thiscall*)(void*);reinterpret_cast<Construct>(0x1002fb00)(texture.data());std::memcpy(texture.data()+0x14,&f.requested,4);const auto clamp=uint16_t(f.clamp);std::memcpy(texture.data()+0x3a,&clamp,2);std::array<char,260>filename{};need(f.initialName.size()<filename.size(),"filename extent");std::memcpy(filename.data(),f.initialName.data(),f.initialName.size());uint32_t callback=0;f.expectedOutput=&callback;using Load=void*(__thiscall*)(void*,char*,void*);f.parsed=reinterpret_cast<Load>(0x1002fdb0)(texture.data(),filename.data(),&callback);need(f.parsed&&::word(f.parsed,0)==0x100dfa34&&!f.open&&f.scratchStack.empty(),"real CPU preparation complete");
        const auto half=[&](size_t at){uint16_t n;std::memcpy(&n,texture.data()+at,2);return uint32_t(n);};f.frames=half(0x32);need(f.frames&&f.frames<=8,"frame extent");f.handles.assign(f.frames+2,0x9b9b9b9b);for(uint32_t i=0;i<f.frames;++i)f.handles[i+1]=q[16]==0||(q[16]==2&&i==0)?0x4000u+i:0;
        const auto handlePointer=uint32_t(reinterpret_cast<uintptr_t>(f.handles.data()+1));std::memcpy(texture.data()+0x40,&handlePointer,4);std::memcpy(texture.data()+0x18,&q[15],4);const auto before=texture;const auto beforeHandles=f.handles;
        const uint32_t mips=q[13]==0xffffffffu?half(0x30):q[13],target=q[14]==0xffffffffu?::word(texture.data(),0x20):q[14];need(mips<=half(0x30)&&target<64,"selected mip/target domain");using Get=uint32_t(__thiscall*)(void*);const auto format=reinterpret_cast<Get>(0x10078cf0)(f.parsed);std::vector<std::vector<uint8_t>>ownedBefore;for(const auto&b:f.owned)ownedBefore.emplace_back(b.live?std::vector<uint8_t>(b.raw+16,b.raw+16+b.size):std::vector<uint8_t>());
        using Upload=void(__thiscall*)(void*,uint32_t,uint32_t,uint32_t,uint32_t,void*,uint32_t);reinterpret_cast<Upload>(0x1002d1d0)(texture.data(),q[11],q[12],0,mips,f.parsed,target);
        need(texture==before&&f.handles==beforeHandles&&f.selects==f.frames,"upload preserves CTexture/handles and selects all frames");need(f.owned.size()==ownedBefore.size(),"no upload allocation");for(size_t i=0;i<f.owned.size();++i){const auto&b=f.owned[i];need(b.live==!ownedBefore[i].empty()&&(!b.live||std::memcmp(b.raw+16,ownedBefore[i].data(),b.size)==0),"native VTF/owned storage unchanged");}
        const std::array<uint32_t,7>record{serial,f.frames,mips,format,target,f.selects,f.uploads};out.write(reinterpret_cast<const char*>(record.data()),sizeof(record));text(out,f.events);f.guards();f.image.verify();f.discard();
    }
    need(in.peek()==EOF&&in.eof(),"query framing");out.flush();need(bool(out),"trace write");observer.configure(false);f.image.verify();RemoveVectoredExceptionHandler(fault);std::cout<<"PASS native_upload_cases="<<count<<" real_VTF=1 controlled_GPU_commands=1 real_GPU_uploads=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
