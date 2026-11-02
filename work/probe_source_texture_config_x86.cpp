// Private offline configuration-to-sampler research. Original DLL bytes stay external.
#define main precedingSamplingOracleMain
#include "probe_source_texture_sampling_x86.cpp"
#undef main
#include <algorithm>
struct ConfigFixture:Fixture {
    static constexpr uint32_t slots[]={0x10105b7c,0x10105c84,0x10105e94,0x10108aac,0x10108b04,0x10108b5c,0x10108bb4,0x10108c0c,0x10108c64,0x10108cbc,0x10108d14,0x10108d6c,0x10108dc4,0x10108e74,0x10108ecc,0x10108f24,0x10108f7c,0x10108fd4,0x1010902c,0x10109084,0x101090dc,0x10109134,0x1010918c,0x101091e4,0x1010923c,0x10109294,0x101092ec,0x10109344,0x1010939c,0x101093f4,0x1010944c,0x101094a4,0x101094fc,0x10109554,0x101095ac,0x10109604,0x101096b4,0x1010970c,0x10109764};
    std::array<std::array<uint32_t,16>,39>roots{};
    std::array<uintptr_t,15>convarTable{};
    std::array<uintptr_t,61>dxTable{};uintptr_t dxObject=0;
    std::array<uintptr_t,47>longHardware{};
    std::array<uint32_t,0x226c/4>owner{};
    uint32_t setCalls=0,lastSet=0;inline static ConfigFixture*current=nullptr;
    static uint32_t __fastcall maximumDx(void*p,void*){need(p==&current->dxObject,"max DX receiver");return 95;}
    static uint32_t __fastcall minimumDx(void*p,void*){need(p==&current->dxObject,"min DX receiver");return 80;}
    static uint32_t __fastcall hdr(void*p,void*){need(p==&current->hardwareObject,"HDR receiver");return 0;}
    static void __fastcall setInt(void*p,void*,uint32_t value){need(p==current->root(0x10105c84).data(),"DX ConVar setter receiver");current->root(0x10105c84)[12]=value;++current->setCalls;current->lastSet=value;}
    std::array<uint32_t,16>&root(uint32_t address){for(size_t n=0;n<39;++n)if(slots[n]==address)return roots[n];throw std::runtime_error("unknown root slot");}
    explicit ConfigFixture(const char*path):Fixture(path,true){
        current=this;convarTable.fill(reinterpret_cast<uintptr_t>(&forbidden));convarTable[14]=reinterpret_cast<uintptr_t>(&setInt);
        dxTable.fill(reinterpret_cast<uintptr_t>(&forbidden));dxTable[24]=reinterpret_cast<uintptr_t>(&maximumDx);dxTable[60]=reinterpret_cast<uintptr_t>(&minimumDx);dxObject=reinterpret_cast<uintptr_t>(dxTable.data());put(0x1255e4,reinterpret_cast<uintptr_t>(&dxObject));
        longHardware.fill(reinterpret_cast<uintptr_t>(&forbidden));longHardware[5]=reinterpret_cast<uintptr_t>(&cap);longHardware[46]=reinterpret_cast<uintptr_t>(&hdr);hardwareObject=reinterpret_cast<uintptr_t>(longHardware.data());
        for(size_t n=0;n<39;++n){roots[n][0]=uint32_t(reinterpret_cast<uintptr_t>(convarTable.data()));put(slots[n]-0x10000000,uint32_t(reinterpret_cast<uintptr_t>(roots[n].data())));}
    }
};
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==4,"private x86 CPU research, never owned x64 runtime");
    need(argc==5&&!std::ifstream(argv[3]).good()&&!std::ifstream(argv[4]).good(),"image queries fresh-commands fresh-config-trace");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream commands(argv[3],std::ios::binary),configs(argv[4],std::ios::binary);need(bool(in)&&bool(commands)&&bool(configs),"config IO");ConfigFixture f(argv[1]);const auto count=read(in);need(count&&count<=65536,"config case count");write(commands,count);write(configs,count);
    for(uint32_t serial=0;serial<count;++serial){std::array<uint32_t,11>input{};for(auto&v:input)v=read(in);need(input[4]<=8&&input[6]<=255&&input[10]<=1,"config query domain");for(auto&r:f.roots)for(size_t n=1;n<r.size();++n)r[n]=0;
        f.root(0x10108b04)[12]=input[1];f.root(0x10108bb4)[12]=input[7];f.root(0x10108c0c)[12]=input[8];f.root(0x10109554)[12]=input[9];auto rootsBefore=f.roots;
        f.owner.fill(0);f.owner[0x2268/4]=serial&1;f.put(0x12559c,input[10]);f.setCalls=f.lastSet=0;f.capability=input[2];f.events.clear();
        std::array<uint8_t,0x84>guarded;guarded.fill(0xcd);auto*config=guarded.data()+0x10;
        using ReadConfig=void(__thiscall*)(void*,void*);reinterpret_cast<ReadConfig>(0x1000cc80)(f.owner.data(),config);
        for(size_t i=0;i<0x10;++i)need(guarded[i]==0xcd&&guarded[0x74+i]==0xcd,"configuration outer guard");
        using Set=void(__thiscall*)(void*,uint32_t);reinterpret_cast<Set>(0x1000bbb0)(f.owner.data(),17);
        uint32_t force=17;if(input[10]){std::memcpy(&force,config+0x28,4);reinterpret_cast<Set>(0x1000bbb0)(f.owner.data(),force);need(f.setCalls==1&&f.lastSet==80,"controlled DX clamp side effect");}
        else {for(auto b:guarded)need(b==0xcd,"disabled configuration reader leaves target unchanged");need(f.setCalls==0,"disabled config has no DX side effect");}
        using Get=void*(__cdecl*)();auto*published=static_cast<uint8_t*>(reinterpret_cast<Get>(0x1000b640)());need(published==f.image.base+0xfc7c0,"original configuration singleton getter");uint32_t actualForce;std::memcpy(&actualForce,published+0x28,4);need(actualForce==force,"setter publishes exact config field used by texture routine");
        rootsBefore[1][12]=input[10]?80u:0u;need(rootsBefore==f.roots,"only controlled DX ConVar changes");
        write(configs,serial);for(auto v:input)write(configs,v);configs.write(reinterpret_cast<const char*>(config),0x64);write(configs,f.setCalls);write(configs,f.lastSet);write(configs,actualForce);
        std::array<uint32_t,7>q{};std::copy_n(input.begin(),7,q.begin());q[1]=force;if(!input[10])q[3]=0;
        std::array<uint8_t,0x68>texture{};std::array<uint32_t,8>handles{};for(uint32_t i=0;i<8;++i)handles[i]=0x2000+i;auto put=[&](size_t at,uint32_t v){std::memcpy(texture.data()+at,&v,4);};put(0x14,q[0]);put(0x18,q[3]);const auto frames=uint16_t(q[4]);std::memcpy(texture.data()+0x32,&frames,2);put(0x40,uint32_t(reinterpret_cast<uintptr_t>(handles.data())));const auto before=texture;
        using Dispatch=void(__thiscall*)(void*);reinterpret_cast<Dispatch>(0x1002ce70)(texture.data());need(texture==before,"CTexture metadata unchanged");
        write(commands,serial);for(auto v:q)write(commands,v);write(commands,uint32_t(f.events.size()));for(const auto&e:f.events)for(auto v:e)write(commands,v);
    }
    need(in.peek()==EOF&&in.eof(),"complete config query framing");commands.flush();configs.flush();need(bool(commands)&&bool(configs),"config trace flushed");f.image.verify();std::cout<<"PASS original_config_cases="<<count<<" complete_config_reader=1 actual_force_setter=1 material_commands=1 original_process=ABSENT GPU=NOT_USED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
