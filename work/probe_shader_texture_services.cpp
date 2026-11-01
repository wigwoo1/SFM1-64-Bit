// Original CShaderSystem texture methods + real CMaterialVar state transitions,
// compared with an owned x64 CPU value policy. Manager/resources are test services.
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#ifndef _WIN64
#include "material_construction_native_fixture.h"
#else
#include "material_shader_texture_init_candidate.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t read(std::istream&i){uint32_t n=0;need(bool(i.read(reinterpret_cast<char*>(&n),4)),"input word");return n;}
static std::string text(std::istream&i){auto n=read(i);need(n<=4095,"text bound");std::string s(n,'\0');need(bool(i.read(s.data(),n)),"text input");return s;}
static void text(std::ostream&o,const std::string&s){auto n=uint32_t(s.size());o.write(reinterpret_cast<const char*>(&n),4);o.write(s.data(),n);}
struct Query{uint32_t serial,kind,type,metadata,flags,flags2,config,hdr,suppress,result,groupPresent;};
static_assert(sizeof(Query)==44,"query ABI");
static std::string findEvent(uint32_t flags,const std::optional<std::string>&group,const std::string&name){return "F "+std::to_string(flags)+" "+(group?"G:"+*group:"NULL")+" "+name+"\n";}
#ifndef _WIN64
struct Fixture:MaterialConstructionNativeFixture{
    struct Resource{uintptr_t table;uint32_t id,refs;};
    std::array<uintptr_t,25>resourceTable{};std::array<Resource,3>resources{};
    std::array<uintptr_t,17>managerTable{};uintptr_t manager=0;
    std::array<uintptr_t,35>hardwareTable{};uintptr_t hardware=0;
    std::array<uintptr_t,6>debugTable{};uintptr_t debug=0;
    Query q{};std::string trace;
    static Fixture&self(){return *static_cast<Fixture*>(active);}
    static Resource&resource(void*p){auto&f=self();for(auto&r:f.resources)if(p==&r)return r;throw std::runtime_error("resource identity");}
    static void __fastcall retain(void*p,void*){++resource(p).refs;}
    static void __fastcall release(void*p,void*){auto&r=resource(p);need(r.refs>1,"retained resource release");--r.refs;}
    static void* __fastcall find(void*p,void*,const char*name,const char*group,uint32_t flags){auto&f=self();need(p==&f.manager&&name,"manager find receiver");f.trace+=findEvent(flags,group?std::optional<std::string>(group):std::nullopt,name);return f.q.result?&f.resources[f.q.result-1]:nullptr;}
    static void* __fastcall error(void*p,void*){auto&f=self();need(p==&f.manager,"error receiver");f.trace+="E\n";return &f.resources[1];}
    static uint32_t __fastcall config(void*p,void*){auto&f=self();need(p==&f.service,"config receiver");f.trace+="C "+std::to_string(f.q.config)+"\n";return f.q.config;}
    static uint32_t __fastcall hdr(void*p,void*){auto&f=self();need(p==&f.hardware,"HDR receiver");f.trace+="H "+std::to_string(f.q.hdr)+"\n";return f.q.hdr;}
    static uint32_t __fastcall suppress(void*p,void*){auto&f=self();need(p==&f.debug,"debug receiver");f.trace+="S "+std::to_string(f.q.suppress)+"\n";return f.q.suppress;}
    static void __cdecl warning(const char*format,const char*name){auto&f=self();need(format==reinterpret_cast<const char*>(0x1009c8bc)&&name,"warning format");f.trace+="W "+std::string(name)+"\n";}
    explicit Fixture(const char*path):MaterialConstructionNativeFixture(path,true){
        resourceTable.fill(reinterpret_cast<uintptr_t>(&unexpected));resourceTable[10]=reinterpret_cast<uintptr_t>(&retain);resourceTable[11]=reinterpret_cast<uintptr_t>(&release);
        for(uint32_t i=0;i<3;++i)resources[i]={reinterpret_cast<uintptr_t>(resourceTable.data()),i+1,1};
        managerTable.fill(reinterpret_cast<uintptr_t>(&unexpected));managerTable[7]=reinterpret_cast<uintptr_t>(&find);managerTable[16]=reinterpret_cast<uintptr_t>(&error);manager=reinterpret_cast<uintptr_t>(managerTable.data());word(0x101a84,reinterpret_cast<uintptr_t>(&manager));
        services[142]=reinterpret_cast<uintptr_t>(&config);
        hardwareTable.fill(reinterpret_cast<uintptr_t>(&unexpected));hardwareTable[34]=reinterpret_cast<uintptr_t>(&hdr);hardware=reinterpret_cast<uintptr_t>(hardwareTable.data());word(0x1059f4,reinterpret_cast<uintptr_t>(&hardware));
        debugTable.fill(reinterpret_cast<uintptr_t>(&unexpected));debugTable[5]=reinterpret_cast<uintptr_t>(&suppress);debug=reinterpret_cast<uintptr_t>(debugTable.data());word(0x11b0a0,reinterpret_cast<uintptr_t>(&debug));word(0x951e4,reinterpret_cast<uintptr_t>(&warning));
        image.base[0x109a60]=0;image.base[0x109a61]=0;
    }
    uint32_t token(uint32_t p){if(p==0xffffffffu)return p;for(auto&r:resources)if(p==reinterpret_cast<uintptr_t>(&r))return r.id;throw std::runtime_error("unrecognized texture pointer");}
};
static uint32_t word(const void*p,size_t at){uint32_t n;std::memcpy(&n,static_cast<const uint8_t*>(p)+at,4);return n;}
#else
using Ref=MaterialTextureReferenceCandidate;
static std::vector<uint8_t>vtf(){std::vector<uint8_t>b(96,0);auto w=[&](size_t o,uint32_t v){std::memcpy(b.data()+o,&v,4);};auto h=[&](size_t o,uint16_t v){std::memcpy(b.data()+o,&v,2);};w(0,0x00465456);w(4,7);w(8,2);w(12,80);h(16,2);h(18,2);h(24,1);w(52,0);b[56]=1;h(63,1);return b;}
static MaterialShaderTextureStateCandidate state(uint32_t type,const std::string&name){
    MaterialShaderTextureStateCandidate s;auto kind=type==1?MaterialValueConstructionKindCandidate::String:type==0?MaterialValueConstructionKindCandidate::Float:type==4?MaterialValueConstructionKindCandidate::Integer:type==2?MaterialValueConstructionKindCandidate::Vector:MaterialValueConstructionKindCandidate::Undefined;
    s.value.created=materialValueConstructCandidate(s.value.value,kind,{0x3f800000,0,0,0},type==2?4u:0u,type==1?name:"");need(s.value.created,"owned constructor");return s;
}
static int safety(){
    auto resource=Ref::ordinary(MaterialTextureResourceCandidate::fromVtf("fixture/real-owned-bytes",vtf()));
    MaterialShaderTextureServicesCandidate calls;
    calls.configMask=[]()->std::optional<uint32_t>{return 0;};calls.hdrType=[]()->std::optional<uint32_t>{return 0;};calls.suppressWarning=[]()->std::optional<uint32_t>{return 0;};calls.warning=[](const auto&){};
    calls.errorTexture=[&]()->std::optional<Ref>{return resource;};
    calls.find=[&](const auto&)->std::optional<MaterialShaderTextureLookupCandidate>{return MaterialShaderTextureLookupCandidate{resource};};
    unsigned checks=0;
    const auto check=[&](bool b){need(b,"owned shader texture safety");++checks;};
    auto original=state(1,"fixture/source");original.flags2=0x76543210;
    auto output=state(1,"preserve/output");output.flags2=0x10203040;
    const auto unchanged=[&](){return output.value.value.text=="preserve/output"&&output.flags2==0x10203040&&!output.value.texture.valid()&&original.value.value.text=="fixture/source"&&!original.value.texture.valid();};
    for(unsigned mode=0;mode<10;++mode){auto broken=calls;auto kind=MaterialShaderTextureKindCandidate::Normal;
        if(mode==0)broken.configMask={};if(mode==1)broken.configMask=[]()->std::optional<uint32_t>{return {};};
        if(mode==2)broken.find={};if(mode==3)broken.find=[](const auto&)->std::optional<MaterialShaderTextureLookupCandidate>{return {};};
        if(mode>=4)broken.find=[](const auto&)->std::optional<MaterialShaderTextureLookupCandidate>{return MaterialShaderTextureLookupCandidate{};};
        if(mode==4)broken.suppressWarning={};if(mode==5)broken.warning={};if(mode==6)broken.errorTexture={};
        if(mode==7)broken.errorTexture=[]()->std::optional<Ref>{return Ref::environment();};
        if(mode==8)broken.errorTexture=[]()->std::optional<Ref>{throw std::bad_alloc();};
        if(mode==9){kind=MaterialShaderTextureKindCandidate::Cube;broken.hdrType={};}
        check(!materialShaderTextureInitCandidate(original,output,kind,0,std::nullopt,broken)&&unchanged());
    }
    for(auto name:{std::string(),std::string(260,'x'),std::string("bad\0name",8),std::string("\xff",1)}){auto bad=state(1,"fixture/source");bad.value.value.text=name;check(!materialShaderTextureInitCandidate(bad,output,MaterialShaderTextureKindCandidate::Normal,0,std::nullopt,calls)&&unchanged());}
    auto unavailable=calls;unavailable.find=[](const auto&)->std::optional<MaterialShaderTextureLookupCandidate>{return MaterialShaderTextureLookupCandidate{Ref{}};};check(!materialShaderTextureInitCandidate(original,output,MaterialShaderTextureKindCandidate::Normal,0,std::nullopt,unavailable)&&unchanged());
    unavailable=calls;unavailable.find=[](const auto&)->std::optional<MaterialShaderTextureLookupCandidate>{throw std::runtime_error("service");};check(!materialShaderTextureInitCandidate(original,output,MaterialShaderTextureKindCandidate::Normal,0,std::nullopt,unavailable)&&unchanged());
    auto alias=original;check(materialShaderTextureInitCandidate(alias,alias,MaterialShaderTextureKindCandidate::Normal,0,std::nullopt,calls)&&materialTypedTextureValidCandidate(alias.value));
    auto frozen=calls;unsigned finds=0;frozen.configMask=[&]()->std::optional<uint32_t>{frozen.find={};return 0;};frozen.find=[&](const auto&)->std::optional<MaterialShaderTextureLookupCandidate>{++finds;return MaterialShaderTextureLookupCandidate{resource};};check(materialShaderTextureInitCandidate(original,output,MaterialShaderTextureKindCandidate::Normal,0,std::nullopt,frozen)&&finds==1);
    auto retained=output;output={};check(retained.value.texture.resource()->bytes()==vtf());retained={};alias={};check(resource.resource().use_count()==1);
    std::cout<<"PASS owned_safety_checks="<<checks<<" real_cpu_texture_bytes=1 editor=NOT_CONNECTED\n";return 0;
}
#endif
int main(int argc,char**argv){try{
#ifdef _WIN64
    static_assert(sizeof(void*)==8,"owned x64");if(argc==2&&std::string(argv[1])=="--test")return safety();
#else
    static_assert(sizeof(void*)==4,"private oracle");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
#endif
    need(argc==4&&!std::ifstream(argv[3]).good(),"image query fresh-output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"probe IO");
#ifndef _WIN64
    Fixture f(argv[1]);
#else
    (void)argv[1];std::array<Ref,3>resources;
    for(size_t i=0;i<3;++i)resources[i]=Ref::ordinary(MaterialTextureResourceCandidate::fromVtf("fixture/resource"+std::to_string(i+1),vtf()));
#endif
    Query q{};uint32_t cases=0;
    while(in.read(reinterpret_cast<char*>(&q),sizeof(q))){
        auto name=text(in),group=text(in);need(q.serial==cases&&q.kind<3&&q.result<=3&&q.groupPresent<2&&q.metadata<=240&&!(q.metadata&15),"query domain");need(q.type==0||q.type==1||q.type==2||q.type==3||q.type==4||q.type==6,"input type");need(!name.empty()&&name.size()<=259&&name.find('\0')==std::string::npos&&group.size()<=255&&group.find('\0')==std::string::npos,"text admission");for(unsigned char c:name)need(c<128,"ASCII name");
        std::array<uint32_t,12>record{};record[0]=cases;std::string trace;
#ifndef _WIN64
        f.begin(true);f.q=q;for(auto&r:f.resources)need(r.refs==1,"previous resource balance");
        std::array<uint8_t,40>object{},flags2{};
        using Empty=void*(__thiscall*)(void*,void*,const char*);using Scalar=void*(__thiscall*)(void*,void*,const char*,uint32_t);using String=void*(__thiscall*)(void*,void*,const char*,const char*);using Vector=void*(__thiscall*)(void*,void*,const char*,const void*,int);using Set=void(__thiscall*)(void*,void*);using Clear=void(__thiscall*)(void*);
        const std::array<uint32_t,4>vector{0x3f800000,0,0,0};
        if(q.type==1)reinterpret_cast<String>(0x10017780)(object.data(),nullptr,nullptr,name.c_str());
        else if(q.type==0||q.type==4)reinterpret_cast<Scalar>(q.type==0?0x10017910:0x100179b0)(object.data(),nullptr,nullptr,0x3f800000);
        else if(q.type==2)reinterpret_cast<Vector>(0x10017850)(object.data(),nullptr,nullptr,vector.data(),4);
        else{reinterpret_cast<Empty>(0x10017a50)(object.data(),nullptr,nullptr);if(q.type==3)reinterpret_cast<Set>(0x10019710)(object.data(),&f.resources[2]);}
        reinterpret_cast<Scalar>(0x100179b0)(flags2.data(),nullptr,nullptr,q.flags2);object[28]=uint8_t(q.metadata|q.type);
        std::array<void*,3>params{nullptr,nullptr,flags2.data()};
        std::array<uintptr_t,2>shader{0x1009c9dc,0x1009c9b4};f.trace.clear();
        using Normal=void(__thiscall*)(void*,void*,const char*,uint32_t);using Bump=void(__thiscall*)(void*,void*,const char*);using Cube=void(__thiscall*)(void*,void**,void*,uint32_t);
        if(q.kind==0)reinterpret_cast<Normal>(0x10037a30)(shader.data(),object.data(),q.groupPresent?group.c_str():nullptr,q.flags);
        else if(q.kind==1)reinterpret_cast<Bump>(0x10037b20)(shader.data(),object.data(),q.groupPresent?group.c_str():nullptr);
        else reinterpret_cast<Cube>(0x10037ba0)(shader.data(),params.data(),object.data(),q.flags);
        record[1]=object[28];record[2]=word(object.data(),8);for(size_t i=0;i<4;++i)record[3+i]=word(object.data(),12+i*4);
        record[7]=f.token(word(object.data(),36));record[8]=word(flags2.data(),8);for(size_t i=0;i<3;++i)record[9+i]=f.resources[i].refs;trace=f.trace;
        reinterpret_cast<Clear>(0x1001a0a0)(object.data());reinterpret_cast<Clear>(0x1001a0a0)(flags2.data());
        for(auto&r:f.resources)need(r.refs==1,"final reference balance");need(f.allocations==f.releases,"final native allocation balance");f.image.verify();
#else
        auto value=state(q.type,name);if(q.type==3)need(materialTypedTextureSetCandidate(value.value,resources[2]),"owned retained seed");value.value.value.numeric.typeAndMetadata=(value.value.value.numeric.typeAndMetadata&~255u)|q.metadata|q.type;value.flags2=q.flags2;
        MaterialShaderTextureServicesCandidate calls;
        calls.configMask=[&]()->std::optional<uint32_t>{trace+="C "+std::to_string(q.config)+"\n";return q.config;};
        calls.hdrType=[&]()->std::optional<uint32_t>{trace+="H "+std::to_string(q.hdr)+"\n";return q.hdr;};
        calls.suppressWarning=[&]()->std::optional<uint32_t>{trace+="S "+std::to_string(q.suppress)+"\n";return q.suppress;};
        calls.warning=[&](const auto&n){trace+="W "+n+"\n";};
        calls.errorTexture=[&]()->std::optional<Ref>{trace+="E\n";return resources[1];};
        calls.find=[&](const MaterialShaderTextureRequestCandidate&r)->std::optional<MaterialShaderTextureLookupCandidate>{trace+=findEvent(r.flags,r.group,r.name);MaterialShaderTextureLookupCandidate result;if(q.result)result.texture=resources[q.result-1];return result;};
        need(materialShaderTextureInitCandidate(value,value,MaterialShaderTextureKindCandidate(q.kind),q.flags,q.groupPresent?std::optional<std::string>(group):std::nullopt,calls),"owned preparation");
        record[1]=value.value.value.numeric.typeAndMetadata&255u;record[2]=value.value.value.numeric.integer;for(size_t i=0;i<4;++i)record[3+i]=value.value.value.numeric.vector[i];
        if(value.value.texture.isEnvironment())record[7]=0xffffffff;else for(size_t i=0;i<3;++i)if(value.value.texture==resources[i])record[7]=uint32_t(i+1);
        need(record[7]!=0,"owned result identity");record[8]=value.flags2;for(size_t i=0;i<3;++i)record[9+i]=uint32_t(resources[i].resource().use_count());value={};
        for(auto&r:resources)need(r.resource().use_count()==1,"final owned reference balance");
#endif
        out.write(reinterpret_cast<const char*>(record.data()),sizeof(record));text(out,trace);++cases;
    }
    need(in.eof()&&!in.gcount()&&cases,"query framing");out.flush();need(bool(out),"trace write");std::cout<<"PASS shader_texture_cases="<<cases<<" manager=CONTROLLED real_file_loads=0 gpu_calls=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
