// Research only: offline original CPU command routines, no DllMain/import load.
// Separate processes transport normalized commands between the two PE images.
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#include "native_private_world_image_fixture.h"
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t read(std::istream&f){uint32_t n;need(bool(f.read(reinterpret_cast<char*>(&n),4)),"word");return n;}
static void write(std::ostream&f,uint32_t n){f.write(reinterpret_cast<const char*>(&n),4);}
using Event=std::array<uint32_t,3>;
struct Fixture {
    NativePrivateWorldImageFixture image;
    std::array<uintptr_t,118>api{};std::array<uintptr_t,6>hardware{};
    uintptr_t apiObject=0,hardwareObject=0;uint32_t capability=0;
    std::vector<Event>events;inline static Fixture*active=nullptr;
    static void __fastcall forbidden(void*,void*){ExitProcess(91);}
    static void __fastcall select(void*p,void*,uint32_t id){need(p==&active->apiObject,"select receiver");active->events.push_back({0,id,0});}
    static void __fastcall wrap(void*p,void*,uint32_t component,uint32_t mode){need(p==&active->apiObject,"wrap receiver");active->events.push_back({1,component,mode});}
    static void __fastcall minimum(void*p,void*,uint32_t mode){need(p==&active->apiObject,"minimum receiver");active->events.push_back({2,mode,0});}
    static void __fastcall maximum(void*p,void*,uint32_t mode){need(p==&active->apiObject,"maximum receiver");active->events.push_back({3,mode,0});}
    static uint32_t __fastcall cap(void*p,void*){need(p==&active->hardwareObject,"capability receiver");active->events.push_back({4,active->capability,0});return active->capability;}
    void put(size_t at,uintptr_t value){std::memcpy(image.base+at,&value,4);}
    explicit Fixture(const char*path,bool material):image(path,0x100000){
        need(active==nullptr,"single native image");active=this;
        if(material){api.fill(reinterpret_cast<uintptr_t>(&forbidden));hardware.fill(reinterpret_cast<uintptr_t>(&forbidden));api[79]=reinterpret_cast<uintptr_t>(&minimum);api[80]=reinterpret_cast<uintptr_t>(&maximum);api[81]=reinterpret_cast<uintptr_t>(&wrap);api[117]=reinterpret_cast<uintptr_t>(&select);hardware[5]=reinterpret_cast<uintptr_t>(&cap);apiObject=reinterpret_cast<uintptr_t>(api.data());hardwareObject=reinterpret_cast<uintptr_t>(hardware.data());put(0x11b098,reinterpret_cast<uintptr_t>(&apiObject));put(0x1059f4,reinterpret_cast<uintptr_t>(&hardwareObject));}
        image.seal();
    }
    ~Fixture(){image.verify();VirtualFree(image.base,0,MEM_RELEASE);active=nullptr;}
};
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==4,"private research oracle is x86, never the editor");
    need(argc==5&&!std::ifstream(argv[4]).good(),"material|shader image queries fresh-output");
    const std::string mode=argv[1];need(mode=="material"||mode=="shader","mode");std::ifstream in(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);need(bool(in)&&bool(out),"IO");Fixture f(argv[2],mode=="material");const auto count=read(in);need(count&&count<=65536,"case count");write(out,count);
    for(uint32_t serial=0;serial<count;++serial){
        std::array<uint32_t,7>q{};
        if(mode=="material")for(auto&v:q)v=read(in);
        else {need(read(in)==serial,"transport serial");for(auto&v:q)v=read(in);}
        need(q[4]<=8&&q[6]<=255,"frame/mip domain");f.capability=q[2];f.events.clear();
        write(out,serial);for(auto v:q)write(out,v);
        if(mode=="material"){
            std::array<uint8_t,0x68>texture{};std::array<uint32_t,8>handles{};for(uint32_t i=0;i<8;++i)handles[i]=0x2000+i;
            const auto put=[&](size_t at,uint32_t v){std::memcpy(texture.data()+at,&v,4);};put(0x14,q[0]);put(0x18,q[3]);const auto frames=uint16_t(q[4]);std::memcpy(texture.data()+0x32,&frames,2);put(0x40,uint32_t(reinterpret_cast<uintptr_t>(handles.data())));f.put(0xfc7e8,q[1]);const auto before=texture;
            using Set=void(__thiscall*)(void*);reinterpret_cast<Set>(0x1002ce70)(texture.data());need(texture==before,"CTexture metadata unchanged");write(out,uint32_t(f.events.size()));for(const auto&e:f.events)for(auto v:e)write(out,v);
        }else{
            const auto n=read(in);need(n<=80,"transport event bound");std::vector<Event>events(n);for(auto&e:events)for(auto&v:e)v=read(in);
            std::array<uint8_t,0x2c50>object{};std::array<uint8_t,0x5ac>hardware{};std::array<std::array<uint8_t,0x20>,8>records{};
            for(auto&r:records){r.fill(0xcd);r[0x18]=uint8_t(q[6]);}
            hardware[0x5a8]=uint8_t(q[5]);f.put(0x16d130,uint32_t(reinterpret_cast<uintptr_t>(hardware.data())));
            uint32_t selected=UINT32_MAX,selections=0;
            for(const auto&e:events){
                if(e[0]==0){need(e[1]>=0x2000&&e[1]<0x2000+q[4],"frame ID transport");selected=e[1]-0x2000;++selections;const auto ptr=uint32_t(reinterpret_cast<uintptr_t>(records[selected].data()));std::memcpy(object.data()+0x2c48,&ptr,4);}
                else if(e[0]==1){need(selected<q[4]&&e[1]<=2&&e[2]<=2,"wrap domain");using Set=void(__thiscall*)(void*,uint32_t,uint32_t);reinterpret_cast<Set>(0x1001e150)(object.data(),e[1],e[2]);}
                else if(e[0]==2||e[0]==3){need(selected<q[4]&&(e[1]==0||e[1]==1||e[1]==3||e[1]==5||e[1]==6),"filter domain");using Set=void(__thiscall*)(void*,uint32_t);reinterpret_cast<Set>(e[0]==2?0x1001e1e0:0x1001e2d0)(object.data(),e[1]);}
                else need(e[0]==4&&e[1]==q[2],"capability transport");
            }
            write(out,selections);for(uint32_t i=0;i<selections;++i){write(out,0x2000+i);for(uint32_t k=0;k<6;++k){uint32_t v;std::memcpy(&v,records[i].data()+k*4,4);write(out,v);}need(records[i][0x18]==uint8_t(q[6]),"mip byte preserved");for(size_t k=0x19;k<0x20;++k)need(records[i][k]==0xcd,"record tail guard");}
        }
    }
    need(in.peek()==EOF&&in.eof(),"complete transport input");out.flush();need(bool(out),"trace output");f.image.verify();std::cout<<"PASS original_sampling_"<<mode<<"_cases="<<count<<" private_x86=1 DllMain=NOT_CALLED imports=NOT_LOADED GPU=NOT_USED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
