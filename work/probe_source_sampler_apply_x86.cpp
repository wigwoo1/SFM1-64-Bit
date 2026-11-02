// Offline interception of original complete pixel BindTexture + sampler submission.
// Fake COM endpoints record arguments only; no original process or GPU is used.
#define main precedingSamplingOracleMain
#include "probe_source_texture_sampling_x86.cpp"
#undef main
#include <algorithm>
using DeviceEvent=std::array<uint32_t,4>;
struct ApplyFixture:Fixture {
    std::array<uintptr_t,70>deviceTable{};uintptr_t deviceObject=0;
    std::array<uintptr_t,37>objectTable{};
    std::array<uintptr_t,9>configTable{};uintptr_t configObject=0;
    std::array<uint8_t,0x3100>object{};std::array<uint8_t,100>config{};
    std::array<uint32_t,16>budget{};
    std::array<uint32_t,258>queue{};std::vector<DeviceEvent>calls;bool deactivated=false,recording=false;uint32_t getters=0;
    inline static ApplyFixture*current=nullptr;
    static uint32_t __stdcall setTexture(void*p,uint32_t stage,void*texture){need(p==&current->deviceObject,"device texture receiver");const auto id=uint32_t(reinterpret_cast<uintptr_t>(texture));need(!id||id==0x4000,"controlled texture token");current->calls.push_back({0,stage,id,0});return 0;}
    static uint32_t __stdcall setSampler(void*p,uint32_t stage,uint32_t state,uint32_t value){need(p==&current->deviceObject,"device sampler receiver");current->calls.push_back({1,stage,state,value});return 0;}
    static uint32_t __fastcall isDeactivated(void*p,void*){need(p==current->object.data(),"deactivation receiver");return current->deactivated;}
    static void*__fastcall getConfig(void*p,void*){need(p==&current->configObject,"config provider receiver");++current->getters;return current->config.data();}
    void field(size_t at,uint32_t value){std::memcpy(object.data()+at,&value,4);}
    explicit ApplyFixture(const char*path):Fixture(path,false){current=this;deviceTable.fill(reinterpret_cast<uintptr_t>(&forbidden));deviceTable[65]=reinterpret_cast<uintptr_t>(&setTexture);deviceTable[69]=reinterpret_cast<uintptr_t>(&setSampler);deviceObject=reinterpret_cast<uintptr_t>(deviceTable.data());objectTable.fill(reinterpret_cast<uintptr_t>(&forbidden));objectTable[36]=reinterpret_cast<uintptr_t>(&isDeactivated);configTable.fill(reinterpret_cast<uintptr_t>(&forbidden));configTable[8]=reinterpret_cast<uintptr_t>(&getConfig);configObject=reinterpret_cast<uintptr_t>(configTable.data());put(0x183b58,reinterpret_cast<uintptr_t>(&configObject));budget[12]=0xffffffff;put(0x180104,reinterpret_cast<uintptr_t>(budget.data()));}
    void reset(uint32_t stage,bool enabled){object.fill(0);field(0,uint32_t(reinterpret_cast<uintptr_t>(objectTable.data())));field(0x648,uint32_t(reinterpret_cast<uintptr_t>(&deviceObject)));field(0x2f14,0x1234);std::fill_n(object.data()+0x19f8+36*stage,36,uint8_t(0xcd));object[0x19f8+36*stage+32]=uint8_t(enabled);queue.fill(0xcdcdcdcd);field(0x658,recording?1u:0u);field(0x660,uint32_t(reinterpret_cast<uintptr_t>(queue.data()+1)));field(0x664,256);calls.clear();getters=0;}
};
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==4,"private x86 research only");need(argc==5&&!std::ifstream(argv[4]).good(),"shader-image config-trace sampler-record-trace fresh-output");std::ifstream configs(argv[2],std::ios::binary),records(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);need(bool(configs)&&bool(records)&&bool(out),"apply IO");ApplyFixture f(argv[1]);uint32_t count=read(configs);need(count&&count==read(records)&&count<=65536,"apply cases");write(out,count);uint32_t bindings=0,events=0;
    for(uint32_t serial=0;serial<count;++serial){need(read(configs)==serial,"config serial");std::array<uint32_t,11>input{};for(auto&v:input)v=read(configs);need(bool(configs.read(reinterpret_cast<char*>(f.config.data()),100)),"configuration bytes");for(unsigned i=0;i<3;++i)(void)read(configs);
        need(read(records)==serial,"sampler serial");std::array<uint32_t,7>q{};for(auto&v:q)v=read(records);const auto n=read(records);need(n<=8,"sampler record count");write(out,serial);write(out,n);
        for(uint32_t frame=0;frame<n;++frame){const auto id=read(records);std::array<uint32_t,6>states{};for(auto&v:states)v=read(records);write(out,id);const auto savedConfig=f.config;
            for(uint32_t variant=0;variant<6;++variant){uint32_t stage=(serial+frame)%16;bool enabled=variant%3!=1;f.deactivated=variant%3==2;f.recording=variant>=3;f.config=savedConfig;f.reset(stage,enabled);
                std::array<uint8_t,0x70>guarded{};guarded.fill(0xa7);auto*texture=guarded.data()+16;std::fill_n(texture,0x50,uint8_t(0));std::memcpy(texture,states.data(),24);texture[0x18]=uint8_t(q[6]);texture[0x1a]=1;uint32_t token=0x4000,epoch=0x1234;std::memcpy(texture+0x48,&token,4);std::memcpy(texture+0x34,&epoch,4);const auto before=guarded;
                using Bind=void(__thiscall*)(void*,uint32_t,void*,uint32_t);auto bind=reinterpret_cast<Bind>(0x1001d1f0);
                for(uint32_t phase=0;phase<4;++phase){if(phase==2){f.config[0x3c]=!savedConfig[0x3c];f.config[0x48]=!savedConfig[0x48];}f.calls.clear();f.getters=0;uint32_t start=0;std::memcpy(&start,f.object.data()+0x660,4);bind(f.object.data(),stage,texture,phase==1?0:1);if(f.recording){need(f.calls.empty(),"queued mode must not call device endpoints");uint32_t end=0;std::memcpy(&end,f.object.data()+0x660,4);auto*at=reinterpret_cast<const uint32_t*>(start);auto*finish=reinterpret_cast<const uint32_t*>(end);need(at>=f.queue.data()+1&&finish>=at&&finish<=f.queue.data()+257&&f.queue.front()==0xcdcdcdcd&&f.queue.back()==0xcdcdcdcd,"command recording capacity/guards");while(at<finish){auto type=*at++;if(type==2){need(finish-at>=2,"queued texture framing");auto stageValue=*at++,textureValue=*at++;f.calls.push_back({0,stageValue,textureValue,0});}else{need(type==17&&finish-at>=3,"queued sampler framing");auto stageValue=*at++,stateValue=*at++,value=*at++;f.calls.push_back({1,stageValue,stateValue,value});}}}write(out,variant);write(out,phase);write(out,f.getters);write(out,uint32_t(f.calls.size()));for(const auto&e:f.calls)for(auto v:e)write(out,v);events+=uint32_t(f.calls.size());++bindings;}
                // Bind statistics may change only record peak/use counts at+38/+3c.
                for(size_t i=0;i<guarded.size();++i)if(i<16+0x38||i>=16+0x40)need(guarded[i]==before[i],"texture record/outer guards preserved");
            }
            f.config=savedConfig;
        }
    }
    need(configs.peek()==EOF&&records.peek()==EOF,"complete apply input framing");out.flush();need(bool(out),"device trace flushed");f.image.verify();std::cout<<"PASS original_sampler_apply_cases="<<count<<" actual_BindTexture_calls="<<bindings<<" device_events="<<events<<" GPU=NOT_USED original_process=ABSENT private_x86=1\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
