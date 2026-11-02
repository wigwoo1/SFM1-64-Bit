// Owned research harness. Executes copied original CPU routines offline only.
// No original instruction arrays, DllMain, import loading, live SFM or GPU use.
#define main precedingSourceSamplingMain
#include "probe_source_texture_sampling_x86.cpp"
#undef main
#include <algorithm>
struct AnisotropyFixture:Fixture {
    std::array<uintptr_t,70>deviceTable{};uintptr_t deviceObject=0;
    std::array<uintptr_t,37>objectTable{};
    std::array<uintptr_t,19>providerTable{};uintptr_t providerObject=0;
    std::array<uint8_t,0x3120>guarded{};std::array<uint8_t,0x5ac>caps{};
    std::array<uint32_t,258>queue{};std::vector<std::array<uint32_t,3>>submissions;
    uint32_t note=0,notes=0,gates=0,gateIndex=0,deviceResult=0;
    inline static AnisotropyFixture*current=nullptr;
    uint8_t*base(){return guarded.data()+16;}
    void field(size_t at,uint32_t value){std::memcpy(base()+at,&value,4);}
    uint32_t field(size_t at){uint32_t value;std::memcpy(&value,base()+at,4);return value;}
    static void __fastcall noteLevel(void*p,void*,uint32_t value){need(p==&current->providerObject,"controlled Note receiver");++current->notes;current->note=value;}
    static uint32_t __fastcall isDeactivated(void*p,void*){need(p==current->base(),"actual adjusted base receiver");auto n=current->gateIndex++;return current->gates==1||(current->gates==2&&(n&1));}
    static uint32_t __stdcall setSampler(void*p,uint32_t stage,uint32_t state,uint32_t value){need(p==&current->deviceObject&&state==10,"actual state10 endpoint");current->submissions.push_back({stage,state,value});return current->deviceResult;}
    explicit AnisotropyFixture(const char*path):Fixture(path,false){
        current=this;deviceTable.fill(reinterpret_cast<uintptr_t>(&forbidden));deviceTable[69]=reinterpret_cast<uintptr_t>(&setSampler);deviceObject=reinterpret_cast<uintptr_t>(deviceTable.data());objectTable.fill(reinterpret_cast<uintptr_t>(&forbidden));objectTable[36]=reinterpret_cast<uintptr_t>(&isDeactivated);providerTable.fill(reinterpret_cast<uintptr_t>(&forbidden));providerTable[18]=reinterpret_cast<uintptr_t>(&noteLevel);providerObject=reinterpret_cast<uintptr_t>(providerTable.data());put(0x183b58,reinterpret_cast<uintptr_t>(&providerObject));put(0x16d130,reinterpret_cast<uintptr_t>(caps.data()));
    }
    void reset(const std::array<uint32_t,6>&q){
        guarded.fill(0xa7);std::fill_n(base(),0x3100,uint8_t(0xcd));field(0,uint32_t(reinterpret_cast<uintptr_t>(objectTable.data())));field(0x2d0,0x100fcedc);field(0x648,uint32_t(reinterpret_cast<uintptr_t>(&deviceObject)));field(0x658,q[3]);queue.fill(0xcdcdcdcd);field(0x660,uint32_t(reinterpret_cast<uintptr_t>(queue.data()+1)));field(0x664,256);caps.fill(0x9b);uint32_t table=0x100fa6b4;std::memcpy(caps.data(),&table,4);std::memcpy(caps.data()+0x508,&q[2],4);std::memcpy(caps.data()+0x510,&q[1],4);gates=q[4];deviceResult=q[5];note=notes=gateIndex=0;submissions.clear();
    }
};
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==4,"private x86 research only");need(argc==5&&!std::ifstream(argv[4]).good(),"material|shader original-module queries fresh-output");const std::string mode=argv[1];need(mode=="material"||mode=="shader","mode");std::ifstream in(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);need(bool(in)&&bool(out),"anisotropy IO");const auto count=read(in);need(count&&count<=65536,"bounded query count");write(out,count);
    if(mode=="material"){
        Fixture f(argv[2],false);using Get=void*(__thiscall*)(void*);using Set=void(__thiscall*)(void*,uint32_t);auto*table=reinterpret_cast<uintptr_t*>(0x10098424);need(table[8]==0x1000b640&&table[18]==0x1000bbb0,"original provider vtable");uintptr_t provider=reinterpret_cast<uintptr_t>(table);auto*config=static_cast<uint8_t*>(reinterpret_cast<Get>(table[8])(&provider));need(config==reinterpret_cast<uint8_t*>(0x100fc7c0),"original singleton");
        for(uint32_t serial=0;serial<count;++serial){std::array<uint32_t,6>q{};for(auto&v:q)v=read(in);std::fill_n(config-16,132,uint8_t(0xa7));const std::vector<uint8_t>before(config-16,config+116);reinterpret_cast<Set>(table[18])(&provider,q[0]);uint32_t published;std::memcpy(&published,config+0x28,4);for(size_t i=0;i<before.size();++i)if(i<16+0x28||i>=16+0x2c)need((config-16)[i]==before[i],"raw publisher configuration guards");need(published==q[0]&&reinterpret_cast<Get>(table[8])(&provider)==config,"raw publication alias");write(out,serial);for(auto v:q)write(out,v);write(out,published);}
        f.image.verify();
    }else{
        AnisotropyFixture f(argv[2]);using Get=uint32_t(__thiscall*)(void*);using Set=void(__thiscall*)(void*,uint32_t);auto*api=reinterpret_cast<uintptr_t*>(0x100fcedc);auto*hw=reinterpret_cast<uintptr_t*>(0x100fa6b4);need(api[152]==0x10010920&&hw[5]==0x10003400,"actual setter/getter table entries");
        for(uint32_t serial=0;serial<count;++serial){need(read(in)==serial,"published serial");std::array<uint32_t,6>q{};for(auto&v:q)v=read(in);auto published=read(in);need(published==q[0]&&int32_t(q[2])<=16&&q[3]<=1&&q[4]<=2,"published input domain");f.reset(q);const auto before=f.guarded;const auto capsBefore=f.caps;auto cap=reinterpret_cast<Get>(hw[5])(f.caps.data());need(cap==q[1],"actual original hardware getter");reinterpret_cast<Set>(api[152])(f.base()+0x2d0,published);need(f.notes==1&&f.note==published&&f.gateIndex==uint32_t(std::max(0,int32_t(q[2]))),"raw Note callback and per-stage gates");
            if(q[3]){need(f.submissions.empty(),"queued mode excludes direct COM");auto*end=reinterpret_cast<const uint32_t*>(f.field(0x660));auto*at=f.queue.data()+1;need(end>=at&&end<=f.queue.data()+257,"adequate recorded capacity");while(at<end){need(end-at>=4&&at[0]==17&&at[2]==10,"actual queue framing");f.submissions.push_back({at[1],at[2],at[3]});at+=4;}need(f.field(0x664)==256-uint32_t(end-(f.queue.data()+1)),"queue capacity consumption");}else need(f.field(0x660)==uint32_t(reinterpret_cast<uintptr_t>(f.queue.data()+1))&&f.field(0x664)==256,"direct path recording untouched");
            need(f.queue.front()==0xcdcdcdcd&&f.queue.back()==0xcdcdcdcd&&f.caps==capsBefore,"queue/capability guards");for(size_t i=0;i<f.guarded.size();++i){size_t at=i>=16?i-16:SIZE_MAX;bool cache=false;for(int32_t stage=0;stage<std::max(0,int32_t(q[2]));++stage)cache|=at>=0x1a14+size_t(stage)*36&&at<0x1a18+size_t(stage)*36;bool recording=q[3]&&at>=0x660&&at<0x668;if(!cache&&!recording)need(f.guarded[i]==before[i],"base/interface/stage-neighbor and outer guards");}
            write(out,serial);for(auto v:q)write(out,v);write(out,published);write(out,cap);write(out,f.note);write(out,f.gateIndex);write(out,uint32_t(f.submissions.size()));for(const auto&e:f.submissions)for(auto v:e)write(out,v);write(out,uint32_t(std::max(0,int32_t(q[2]))));for(int32_t stage=0;stage<std::max(0,int32_t(q[2]));++stage)write(out,f.field(0x1a14+size_t(stage)*36));
        }f.image.verify();
    }
    need(in.peek()==EOF,"complete anisotropy query framing");out.flush();need(bool(out),"anisotropy trace output");std::cout<<"PASS original_anisotropy_"<<mode<<"_cases="<<count<<" private_x86=1 DllMain=NOT_CALLED imports=NOT_LOADED GPU=NOT_USED original_process=ABSENT\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
