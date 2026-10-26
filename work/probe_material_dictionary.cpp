#define main acceptedMaterialLifetimeMain
#include "probe_material_object_lifetime.cpp"
#undef main
#include <random>
#include <set>

// Actual original two-tree dictionary constructor, comparator, insertion,
// lookup, traversal, erasure, growth and cleanup. The system object contains
// the real dictionary subobject at +0x188; no cache methods are substituted.
// External context/token/mode/unbind and host heap/KV services remain fixtures.
struct MaterialDictionaryFixture:MaterialObjectLifetimeFixture{
    static constexpr unsigned capacity=292;
    static constexpr size_t rawStride=0xa8+32,stateStride=0x2c0+32;
    std::array<uint8_t,0x1c8+32>systemStorage{};
    std::vector<uint8_t>rawSlots,stateSlots;
    std::vector<uint32_t>tokens;
    unsigned maxDepth=0,unbindings=0,flaggedUnbindings=0;
    std::vector<unsigned>destroyed;
    static MaterialDictionaryFixture&self(){return *static_cast<MaterialDictionaryFixture*>(active);}
    static uint32_t __fastcall nestedAcquire(void*object,void*){
        auto&f=self();if(object!=f.system()||f.tokens.size()>=4)ExitProcess(220);
        const auto token=0x12340000u+uint32_t(f.tokens.size());f.tokens.push_back(token);f.maxDepth=std::max(f.maxDepth,unsigned(f.tokens.size()));return token;
    }
    static void __fastcall nestedRelease(void*object,void*,uint32_t token){auto&f=self();if(object!=f.system()||f.tokens.empty()||f.tokens.back()!=token)ExitProcess(221);f.tokens.pop_back();}
    static void __fastcall observeUnbind(void*object,void*,void*raw){auto&f=self();if(object!=f.system())ExitProcess(222);const auto id=f.id(raw);f.destroyed.push_back(id);++f.unbindings;if(f.image.base[0x109a60])++f.flaggedUnbindings;}
    void*system(){return systemStorage.data()+16;}
    uint8_t*dictionary(){return static_cast<uint8_t*>(system())+0x188;}
    uint8_t*rawAt(unsigned id){if(id>=capacity)throw std::runtime_error("Material slot bound");return rawSlots.data()+id*rawStride+16;}
    uint8_t*stateAt(unsigned id){return stateSlots.data()+id*stateStride+16;}
    unsigned id(const void*p){for(unsigned i=0;i<capacity;++i)if(p==rawAt(i))return i;throw std::runtime_error("Unknown original material pointer");}
    static uint16_t half(const void*p,size_t offset){uint16_t n;std::memcpy(&n,static_cast<const uint8_t*>(p)+offset,2);return n;}
    explicit MaterialDictionaryFixture(const char*p,const char*d,const char*m):MaterialObjectLifetimeFixture(p,d,m),rawSlots(capacity*rawStride),stateSlots(capacity*stateStride){
        lifetimeMethods[106]=reinterpret_cast<uintptr_t>(&nestedAcquire);lifetimeMethods[107]=reinterpret_cast<uintptr_t>(&nestedRelease);
        lifetimeMethods[137]=0x10016460;lifetimeMethods[138]=0x10016470;lifetimeMethods[145]=reinterpret_cast<uintptr_t>(&observeUnbind);
        systemStorage.fill(0xa5);const auto table=reinterpret_cast<uintptr_t>(lifetimeMethods.data());std::memcpy(system(),&table,4);word(0xfc7b0,reinterpret_cast<uintptr_t>(system()));
    }
    void initialize(){
        MaterialObjectLifetimeFixture::prepare(0);tokens.clear();maxDepth=unbindings=flaggedUnbindings=warnings=0;destroyed.clear();
        rawSlots.assign(rawSlots.size(),0xa5);stateSlots.assign(stateSlots.size(),0xa5);
        for(unsigned i=0;i<capacity;++i){const uintptr_t nextRaw=i+1<capacity?reinterpret_cast<uintptr_t>(rawAt(i+1)):0,nextState=i+1<capacity?reinterpret_cast<uintptr_t>(stateAt(i+1)):0;std::memcpy(rawAt(i),&nextRaw,4);std::memcpy(stateAt(i),&nextState,4);}
        word(0x1058d4,reinterpret_cast<uintptr_t>(rawAt(0)));word(0x105874,reinterpret_cast<uintptr_t>(stateAt(0)));
        using Ctor=void*(__thiscall*)(void*);if(reinterpret_cast<Ctor>(0x10012910)(dictionary())!=dictionary())throw std::runtime_error("Original dictionary ctor result");
        if(read(dictionary())!=0x10007c60||read(dictionary(),0x1c)!=0x10007c80||half(dictionary(),0x10)!=0xffff||half(dictionary(),0x12)!=0||read(dictionary(),8)!=256)throw std::runtime_error("Original empty dictionary initialization");
    }
    std::vector<unsigned>enumerate(){
        using First=uint16_t(__thiscall*)(void*);using Next=uint16_t(__thiscall*)(void*,uint16_t);std::vector<unsigned>ids;std::set<uint16_t>seen;
        for(auto i=reinterpret_cast<First>(0x100088b0)(dictionary());i!=0xffff;i=reinterpret_cast<Next>(0x10008980)(dictionary(),i)){
            if(i>=read(dictionary(),8)||!seen.insert(i).second)throw std::runtime_error("Original traversal bound/cycle");
            auto*p=reinterpret_cast<const uint8_t*>(read(dictionary(),4))+size_t(i)*16;auto raw=reinterpret_cast<void*>(read(p,8));
            if(half(p,12)!=half(raw,0x18)||p[14]!=((read(raw,0x20)>>4)&1))throw std::runtime_error("Dictionary payload/source classification");ids.push_back(id(raw));
        }
        return ids;
    }
    unsigned find(const std::string&name,bool source){
        uint16_t symbol=0xffff;using Intern=void*(__thiscall*)(void*,const char*);reinterpret_cast<Intern>(0x100738e0)(&symbol,name.c_str());
        std::array<uint8_t,8>key{};std::memcpy(key.data()+4,&symbol,2);key[6]=source?1:0;
        using Find=uint16_t(__thiscall*)(void*,void*);auto i=reinterpret_cast<Find>(0x10010480)(dictionary(),key.data());
        if(i==0xffff)return capacity;if(i>=read(dictionary(),8))throw std::runtime_error("Original find result bound");return id(reinterpret_cast<void*>(read(reinterpret_cast<const uint8_t*>(read(dictionary(),4))+size_t(i)*16,8)));
    }
    void removeOnly(void*p){using Remove=void(__thiscall*)(void*,void*);reinterpret_cast<Remove>(0x10009ac0)(dictionary(),p);}
    void deleteIfUnreferenced(void*p){using Ref=void(__thiscall*)(void*);auto table=*static_cast<uintptr_t**>(p);if(table[51]!=0x10005e10)throw std::runtime_error("Real delete-if-unreferenced slot51");reinterpret_cast<Ref>(table[51])(p);}
    void cleanupDictionary(){
        if(half(dictionary(),0x12)||!enumerate().empty())throw std::runtime_error("Dictionary still owns materials");
        using Clear=void(__thiscall*)(void*);reinterpret_cast<Clear>(0x10012840)(dictionary());reinterpret_cast<Clear>(0x100128b0)(dictionary()+0x1c);
        if(read(dictionary(),4)||read(dictionary(),0x20)||!tokens.empty()||read(image.base+0x1058c8,0x10)||read(image.base+0x105868,0x10))throw std::runtime_error("Original dictionary/pool cleanup");
    }
    void cacheGuards(){
        for(unsigned i=0;i<16;++i)if(systemStorage[i]!=0xa5||systemStorage[0x1c8+16+i]!=0xa5)throw std::runtime_error("System/dictionary guard");
        for(unsigned i=0;i<capacity;++i)for(size_t j=0;j<16;++j)if(rawSlots[i*rawStride+j]!=0xa5||rawSlots[i*rawStride+16+0xa8+j]!=0xa5||stateSlots[i*stateStride+j]!=0xa5||stateSlots[i*stateStride+16+0x2c0+j]!=0xa5)throw std::runtime_error("Material/state slot guard");
    }
};

#ifndef SFM64_MATERIAL_DICTIONARY_FIXTURE_ONLY
int main(int argc,char**argv){try{
    need(argc==5,"material libraries workspace-mount fresh-output");need(!std::ifstream(argv[4]).good(),"Fresh output required");
    MaterialDictionaryFixture f(argv[1],argv[2],argv[3]);auto observer=AddVectoredExceptionHandler(1,&lifetimeFault);need(observer!=nullptr,"Fault observer");std::ofstream out(argv[4]);need(bool(out),"Record output");
    unsigned checks=0,created=0,removed=0,duplicateHits=0;auto check=[&](bool b,const char*s){need(b,s);++checks;};
    using Create=void*(__thiscall*)(void*,const char*,void*);using Procedural=void*(__thiscall*)(void*,const char*,const char*,void*);using Ref=void(__thiscall*)(void*);using Delete=void(__thiscall*)(void*,void*);
    for(unsigned round=0;round<4;++round){
        currentCase=round;f.initialize();std::map<unsigned,std::pair<std::string,bool>>live;
        auto source=[&](){SourceMaterialPatchNodeCandidate n;n.key="UnlitGeneric";SourceMaterialPatchNodeCandidate v;v.key="$basetexture";v.kind=1;v.text="fixture/source";n.children.push_back(v);return f.build(n);};
        auto audit=[&](){auto ids=f.enumerate();std::set<unsigned>a(ids.begin(),ids.end()),b;for(const auto&e:live)b.insert(e.first);check(a==b&&ids.size()==live.size()&&f.half(f.dictionary(),0x12)==live.size(),"Exact original cache membership");check(f.tokens.empty(),"External token stack balanced");f.cacheGuards();};
        for(unsigned i=0;i<288;++i){
            const auto normalized="fixture/"+std::to_string(i%37);const auto input=(i%3?"Fixture/":"FIXTURE/")+std::to_string(i%37)+(i%3==0?".VMT":"");const bool supplied=i%2!=0;void*tree=supplied?source():nullptr;
            auto*handle=static_cast<uint8_t*>(reinterpret_cast<Create>(0x1000b790)(f.system(),input.c_str(),tree));auto*raw=handle-0x80;const auto id=f.id(raw);
            check(!live.count(id)&&f.read(raw)==0x10095d74&&f.read(raw,0x1c)==1&&f.read(raw,0x7c)==reinterpret_cast<uintptr_t>(tree),"Public create produces independent adopted object");live[id]={normalized,supplied};++created;audit();
            const auto found=f.find(normalized,supplied);check(live.count(found)&&live.at(found)==std::make_pair(normalized,supplied),"Actual dictionary find same category/name");
            out<<round<<" I "<<i<<' '<<id<<' '<<live.size()<<' '<<found<<'\n';
        }
        check(f.read(f.dictionary(),8)>256,"Actual original dictionary storage grew");
        for(unsigned n=0;n<37;++n)for(bool category:{false,true}){const auto name="fixture/"+std::to_string(n);auto id=f.find(name,category);check(live.count(id)&&live.at(id)==std::make_pair(name,category),"Both source categories coexist");}
        check(f.find("fixture/absent",true)==f.capacity&&f.find("fixture/absent",false)==f.capacity,"Missing dictionary lookup");
        // The separate original procedural creation helper searches category1,
        // reuses an existing entry, and destroys the unused supplied tree.
        for(unsigned n=0;n<4;++n){void*extra=source();const auto before=f.releases;auto*forward=static_cast<uint8_t*>(reinterpret_cast<Procedural>(0x10011860)(f.system(),("FiXtUrE/"+std::to_string(n)).c_str(),"Other textures",extra));const auto id=f.id(forward-0x80);
            check(live.count(id)&&live.at(id)==std::make_pair("fixture/"+std::to_string(n),true)&&f.read(forward-0x80,0x1c)==1,"Procedural existing hit returns original identity/count");bool sourceLive=false;for(const auto&b:f.blocks)if(b.p==extra&&b.live)sourceLive=true;check(!sourceLive&&f.releases==before+3,"Duplicate procedural source destroyed exactly");++duplicateHits;audit();out<<round<<" H "<<n<<' '<<id<<' '<<live.size()<<'\n';}
        void*ownedSource=source();auto*procedural=static_cast<uint8_t*>(reinterpret_cast<Procedural>(0x10011860)(f.system(),"Fixture/Procedural","Creation group",ownedSource))-0x80;const auto proceduralId=f.id(procedural);live[proceduralId]={"fixture/procedural",true};++created;
        check(f.read(procedural,0x1c)==0&&f.read(procedural,0x7c)==reinterpret_cast<uintptr_t>(ownedSource),"Procedural missing entry adopts source without adding reference");audit();
        void*unusedSource=source();const auto beforeUnused=f.releases;auto*reused=static_cast<uint8_t*>(reinterpret_cast<Procedural>(0x10011860)(f.system(),"FIXTURE\\PROCEDURAL","Ignored group",unusedSource))-0x80;
        check(reused==procedural&&f.read(procedural,0x1c)==0&&f.read(procedural,0x7c)==reinterpret_cast<uintptr_t>(ownedSource)&&f.releases==beforeUnused+3,"Procedural normalized-slash hit keeps old source/refcount");
        using Group=const char*(__thiscall*)(void*);check(std::string(reinterpret_cast<Group>(0x100041a0)(procedural))=="Creation group","Existing procedural group is not replaced");++duplicateHits;
        f.deleteIfUnreferenced(procedural);live.erase(proceduralId);++removed;audit();out<<round<<" P "<<proceduralId<<' '<<live.size()<<'\n';
        // Absent-pointer erasure and null remove/delete helper are safe only
        // on this initialized dictionary. Do not double-delete stale objects.
        using RawCreate=void*(__cdecl*)(const char*,const char*,void*);void*orphan=reinterpret_cast<RawCreate>(0x10006cc0)("fixture/orphan","Other textures",source());const auto orphanId=f.id(orphan);const auto before=f.destroyed.size();f.removeOnly(orphan);check(f.destroyed.size()==before&&f.read(orphan)==0x10095d74,"Absent erase does not destroy");audit();
        reinterpret_cast<Delete>(0x10009b80)(f.dictionary(),orphan);check(f.destroyed.size()==before+1&&f.destroyed.back()==orphanId,"Absent remove-and-delete still destroys argument");++created;++removed;
        reinterpret_cast<Delete>(0x10009b80)(f.dictionary(),nullptr);audit();
        std::vector<unsigned>order;for(const auto&e:live)order.push_back(e.first);if(round==1)std::reverse(order.begin(),order.end());if(round>=2){std::mt19937 random(6400+round);std::shuffle(order.begin(),order.end(),random);}
        unsigned step=0,expectedWarnings=0;
        for(const auto id:order){auto*raw=f.rawAt(id);const auto key=live.at(id);const auto old=f.destroyed.size();f.deleteIfUnreferenced(raw);check(f.destroyed.size()==old&&f.read(raw,0x1c)==1&&f.image.base[0x109a60]==0,"Positive count refuses automatic deletion");
            reinterpret_cast<Ref>(0x1002dd90)(raw);if(round==3&&step%11==0){reinterpret_cast<Ref>(0x1002dd90)(raw);++expectedWarnings;}
            f.deleteIfUnreferenced(raw);check(f.destroyed.size()==old+1&&f.destroyed.back()==id&&f.image.base[0x109a60]==0,"Slot51 removes exact material then restores deletion flag");live.erase(id);++removed;audit();
            const auto found=f.find(key.first,key.second);bool remains=false;for(const auto&e:live)if(e.second==key)remains=true;check(remains?(live.count(found)&&live.at(found)==key):found==f.capacity,"Lookup tracks duplicate erasure");
            out<<round<<" D "<<step++<<' '<<id<<' '<<live.size()<<' '<<found<<'\n';
        }
        check(f.flaggedUnbindings==289&&f.unbindings==290&&f.maxDepth==2&&f.warnings==expectedWarnings,"Nested token/deletion flag/negative reference diagnostics");f.cleanupDictionary();f.cacheGuards();f.verifyCode();
    }
    f.finishLifetime();out.flush();check(bool(out),"Complete records");RemoveVectoredExceptionHandler(observer);
    std::cout<<"PASS_REFERENCE_ONLY dictionary_rounds=4 created="<<created<<" removed="<<removed<<" duplicate_hits="<<duplicateHits<<" independent_checks="<<checks<<" allocations="<<f.allocations<<" releases="<<f.releases<<"\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
#endif
