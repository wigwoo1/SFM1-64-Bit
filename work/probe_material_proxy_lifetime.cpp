// Test services exercise the actual original material caller. They are NOT
// implementations of Sine/Equals/TextureScroll or any other built-in proxy.
#define SFM64_UNLIT_TWO_TEXTURE_PARAMETERS
#define SFM64_LIGHTMAPPED_PARAMETERS
#define SFM64_OBJECT_PARAMETER_MAIN acceptedProxyParameterRecorderMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#include "material_proxy_lifetime_candidate.h"
#include <iomanip>

using ProxyNode=SourceMaterialPatchNodeCandidate;
static ProxyNode proxyInput(std::istream&in,unsigned depth=0){
    gate(depth<16,"Proxy query depth");ProxyNode n;n.kind=word(in);n.bits=word(in);n.key=text(in);n.text=text(in);auto count=word(in);gate(count<=1024,"Proxy query children");
    for(unsigned i=0;i<count;++i)n.children.push_back(proxyInput(in,depth+1));return n;
}
static std::string proxyField(const ProxyNode&n,const std::string&key){auto*p=sourceMaterialPatchFindConstCandidate(n,key);return p?p->text:std::string();}
static std::string boundLine(unsigned id,const std::string&requested,const std::optional<MaterialTypedDispatchCandidate>&value){
    std::ostringstream o;o<<"B "<<id<<' '<<std::quoted(requested)<<' '<<bool(value);
    if(value){const auto&v=*value;o<<' '<<std::quoted(v.name)<<' '<<v.value.numeric.typeAndMetadata<<' '<<v.value.numeric.integer;for(auto x:v.value.numeric.vector)o<<' '<<x;o<<' '<<std::quoted(v.value.text)<<' '<<bool(v.matrix)<<' '<<v.matrixIdentity;if(v.matrix)for(auto x:*v.matrix)o<<' '<<x;}
    return o.str();
}
static ProxyNode proxyDeclaration(std::string key,std::string var="$alpha",std::string tag="tag"){
    ProxyNode p;p.key=std::move(key);p.children.push_back({"var",1,std::move(var)});p.children.push_back({"tag",1,std::move(tag)});return p;
}
static ProxyNode proxyRoot(unsigned count=2){
    ProxyNode n;n.key="UnlitTwoTexture";n.children.push_back({"$alpha",1,".375"});n.children.push_back({"$temp",1,"[.25 .5]"});
    ProxyNode p;p.key="Proxies";for(unsigned i=0;i<count;++i)p.children.push_back(proxyDeclaration("Bind",i%2?"$TEMP":"$alpha",std::to_string(i)));n.children.push_back(std::move(p));return n;
}
#ifdef SFM64_NATIVE_OBJECT_PARAMETERS
struct ProxyNativeFixture:UnlitTwoTextureNativeFixture {
    using UnlitTwoTextureNativeFixture::UnlitTwoTextureNativeFixture;
    struct Instance {uintptr_t table=0;unsigned id=0;bool live=false,initialized=false;};
    struct Factory {uintptr_t table=0;unsigned identity=0;};
    std::array<uintptr_t,2>factoryMethods{};std::array<uintptr_t,1>proxyMethods{};
    Factory factoryA,factoryB;std::array<Instance,1024>instances{};
    std::vector<std::string>trace;uint8_t*material=nullptr;unsigned next=0,proxyWarnings=0;bool available=true;Factory*currentFactory=nullptr;
    static ProxyNativeFixture&self(){return *static_cast<ProxyNativeFixture*>(active);}
    static void* __fastcall getFactory(void*object,void*){auto&f=self();gate(object==f.system(),"Actual system slot60 receiver");return f.available?f.currentFactory:nullptr;}
    static void* __fastcall createProxy(void*object,void*,const char*name){
        auto&f=self();gate(object==f.currentFactory&&f.next<f.instances.size(),"Factory receiver/capacity");auto id=++f.next;f.trace.push_back("C "+std::to_string(id)+" "+name);
        if(std::string(name)=="Missing")return nullptr;
        auto&p=f.instances[id-1];p={reinterpret_cast<uintptr_t>(f.proxyMethods.data()),id,true,false};return &p;
    }
    static bool __fastcall initializeProxy(void*object,void*,void*forward,void*declaration){
        auto&f=self();auto&p=*static_cast<Instance*>(object);gate(p.live&&forward==f.material+0x80&&f.read(forward,4)==reinterpret_cast<uintptr_t>(f.material),"Real forward/backpointer identity");
        const auto node=f.extract(declaration);using Name=const char*(__thiscall*)(void*);auto ft=*static_cast<uintptr_t**>(forward);gate(ft[0]==0x10007520&&ft[11]==0x10007610,"Real forward name/FindVar slots");
        const auto name=std::string(reinterpret_cast<Name>(ft[0])(forward));
        f.trace.push_back("I "+std::to_string(p.id)+" "+node.key+" "+std::to_string(node.kind)+" "+proxyField(node,"tag")+" "+name);
        const auto requested=proxyField(node,"var");bool found=false;using Find=void*(__thiscall*)(void*,const char*,bool*,bool);
        auto*variable=reinterpret_cast<Find>(ft[11])(forward,requested.c_str(),&found,false);std::optional<MaterialTypedDispatchCandidate>value;
        if(found){
            gate(variable&&f.read(variable,32)==reinterpret_cast<uintptr_t>(f.material),"Real bound parameter owner");bool inArray=false;auto array=reinterpret_cast<void**>(f.read(f.material,0x24));for(unsigned i=0;i<f.material[0x22];++i)if(array[i]==variable)inArray=true;gate(inArray,"Binding is actual published variable");
            auto*b=static_cast<uint8_t*>(variable);MaterialTypedDispatchCandidate v;v.created=true;v.name=reinterpret_cast<Name>(0x10017b80)(b);v.value.numeric.typeAndMetadata=f.read(b,28)&0xffff;v.value.numeric.integer=f.read(b,8);std::memcpy(v.value.numeric.vector.data(),b+12,16);
            if((v.value.numeric.typeAndMetadata&15)==1)v.value.text=reinterpret_cast<const char*>(f.read(b,4));
            if((v.value.numeric.typeAndMetadata&15)==7){auto*m=reinterpret_cast<uint8_t*>(f.read(b,36));v.matrix.emplace();std::memcpy(v.matrix->data(),m,64);v.matrixIdentity=m[64]!=0;}
            value=std::move(v);
        }
        f.trace.push_back(boundLine(p.id,requested,value));p.initialized=node.key!="Fail";return p.initialized;
    }
    static void __fastcall deleteProxy(void*object,void*,void*proxy){
        auto&f=self();auto&p=*static_cast<Instance*>(proxy);gate(p.live&&(object==&f.factoryA||object==&f.factoryB),"Live factory destroy receiver");
        f.trace.push_back("D "+std::to_string(p.id)+" "+std::to_string(f.material[0x22]!=0)+" "+std::to_string(static_cast<Factory*>(object)->identity));p.live=false;
    }
    static void __cdecl proxyWarning(const char*format,...){
        auto&f=self();unsigned kind=0;if(format==reinterpret_cast<const char*>(0x10095674))kind=1;else if(format==reinterpret_cast<const char*>(0x10095638))kind=2;else if(format==reinterpret_cast<const char*>(0x10095608))kind=3;else throw std::runtime_error("Unexpected proxy warning format");
        ++f.proxyWarnings;f.trace.push_back("W "+std::to_string(kind));
    }
    void beginProxy(){
        beginParameters();configure({}, {},92);configureLightmapped({});trace.clear();next=proxyWarnings=0;instances.fill({});available=true;
        factoryMethods={reinterpret_cast<uintptr_t>(&createProxy),reinterpret_cast<uintptr_t>(&deleteProxy)};proxyMethods={reinterpret_cast<uintptr_t>(&initializeProxy)};
        factoryA={reinterpret_cast<uintptr_t>(factoryMethods.data()),0};factoryB={reinterpret_cast<uintptr_t>(factoryMethods.data()),1};currentFactory=&factoryA;
        lifetimeMethods[60]=reinterpret_cast<uintptr_t>(&getFactory);word(0x951e4,reinterpret_cast<uintptr_t>(&proxyWarning));
    }
    void cleanupDummy(){auto p=read(image.base+0x10568c);if(p){using Destroy=void*(__thiscall*)(void*,unsigned);reinterpret_cast<Destroy>(0x10018e60)(reinterpret_cast<void*>(p),1);word(0x10568c,0);}}
    std::vector<unsigned>published(){
        std::vector<unsigned>ids;auto**array=reinterpret_cast<Instance**>(read(material,0x28));for(unsigned i=0;i<material[0x23];++i){gate(array&&array[i]->live&&array[i]->initialized,"Actual proxy array live initialized entries");ids.push_back(array[i]->id);}return ids;
    }
    void endProxy(){for(const auto&i:instances)gate(!i.live,"Every service instance released");cleanupDummy();gate(tokens.empty(),"Proxy native token balance");stop();verifyCode();}
};
static void nativeProxyBoundaries(ProxyNativeFixture&f,std::ostream&out){
    for(unsigned scenario=0;scenario<6;++scenario){
        f.beginProxy();auto seed=builtinErrorMaterialSeedCandidate();auto*error=f.createActual(seed.name.c_str(),f.build(seed.source),false);f.word(0x1059ec,reinterpret_cast<uintptr_t>(error));
        auto root=proxyRoot(scenario==4?256:2);
        if(scenario==0){auto&v=root.children.back().children;v.insert(v.begin()+1,proxyDeclaration("Missing"));v.insert(v.begin()+2,proxyDeclaration("Fail"));}
        if(scenario==5){root.children.back().children.front()={"Scalar",1,"literal"};}
        auto*raw=f.createActual("proxy/boundary",f.build(root));f.material=raw;if(scenario==1)f.available=false;
        gate(f.initializeActual(raw),"Native boundary initialization");
        if(scenario==0)gate(raw[0x23]==2&&f.proxyWarnings==2&&f.next==4,"Native skips missing and failed proxy, keeps successful order");
        if(scenario==1)gate(raw[0x23]==0&&f.next==0,"Native missing factory skips declarations");
        if(scenario==2)f.currentFactory=&f.factoryB;
        if(scenario==3)f.available=false;
        const auto array=f.read(raw,0x28);f.uncacheActual(raw);
        if(scenario==3){
            gate(raw[0x23]==2&&f.read(raw,0x28)==array&&f.instances[0].live&&f.instances[1].live,"Native null teardown factory retains list");out<<"BOUNDARY null_factory_retains_list=1\n";
            f.available=true;using Release=void(__thiscall*)(void*);reinterpret_cast<Release>(0x10002dd0)(raw);
        }
        if(scenario==4){
            gate(raw[0x23]==0&&array&&f.read(raw,0x28)==array&&f.next==256&&f.proxyWarnings==1&&f.instances[0].live&&f.instances[255].live,"Native256 count wraps and skips teardown");out<<"BOUNDARY count256_wraps_and_leaks=1\n";
            // Explicit host-fixture cleanup AFTER observing the unchanged native
            // leak. Never call this successful original cleanup or copy it into
            // the owned implementation. Only fixture-owned service instances
            // and its allocator-tracked private array are reclaimed here.
            for(unsigned i=256;i>0;--i)ProxyNativeFixture::deleteProxy(&f.factoryA,nullptr,&f.instances[i-1]);
            MaterialFallbackNativeFixture::heapFree(nullptr,nullptr,reinterpret_cast<void*>(array));const uint32_t zero=0;std::memcpy(raw+0x28,&zero,4);
        }
        if(scenario==2){gate(f.trace[f.trace.size()-1]=="D 1 0 1"&&f.trace[f.trace.size()-2]=="D 2 0 1","Native uses current factory, not originating factory");out<<"BOUNDARY switched_factory_used=1\n";}
        out<<"CASE "<<scenario<<'\n';for(const auto&line:f.trace)out<<line<<'\n';using Decrement=void(__thiscall*)(void*);reinterpret_cast<Decrement>(0x1002dd90)(raw);f.destroyActual(raw);f.destroyActual(error);f.endProxy();
    }
}
#else
struct ProxyOwnedFactory:MaterialProxyFactoryCandidate {
    struct Instance:MaterialProxyInstanceCandidate {
        ProxyOwnedFactory&factory;unsigned id;MaterialProxyContextCandidate context;bool initialized=false;
        Instance(ProxyOwnedFactory&f,unsigned n):factory(f),id(n){}
        bool initialize(const MaterialProxyContextCandidate&c,const ProxyNode&node)override{
            context=c;factory.trace.push_back("I "+std::to_string(id)+" "+node.key+" "+std::to_string(node.kind)+" "+proxyField(node,"tag")+" "+c.name());
            auto requested=proxyField(node,"var");auto binding=c.find(requested);factory.trace.push_back(boundLine(id,requested,binding.read()));factory.bindings.push_back(binding);gate(c.owner()!=0,"Owned stable material token");
            if(factory.onInit)factory.onInit();initialized=node.key!="Fail";return initialized;
        }
    };
    std::vector<std::string>trace;std::vector<MaterialProxyBindingCandidate>bindings;std::vector<Instance*>live;
    std::function<void()>onInit,onDestroy;unsigned next=0,released=0;bool throwCreate=false;
    MaterialProxyInstanceCandidate*create(const std::string&name)override{
        unsigned id=++next;trace.push_back("C "+std::to_string(id)+" "+name);if(throwCreate)throw std::runtime_error("Test factory create exception");if(name=="Missing"){trace.push_back("W 1");return nullptr;}
        auto p=std::make_unique<Instance>(*this,id);live.push_back(p.get());return p.release();
    }
    void destroy(MaterialProxyInstanceCandidate*proxy)noexcept override{
        auto*p=static_cast<Instance*>(proxy);try{trace.push_back("D "+std::to_string(p->id)+" "+std::to_string(p->context.owner()!=0)+" 0");auto i=std::find(live.begin(),live.end(),p);if(i==live.end())std::terminate();live.erase(i);++released;if(!p->initialized)trace.push_back("W 2");if(onDestroy)onDestroy();}catch(...){std::terminate();}delete p;
    }
    std::vector<unsigned>published()const{std::vector<unsigned>ids;for(auto*p:live)if(p->initialized)ids.push_back(p->id);return ids;}
};
static int proxyOwnedSafety();
#endif

int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_OBJECT_PARAMETERS
    static_assert(sizeof(void*)==8,"Owned proxy lifetime is x64");if(argc==2&&std::string(argv[1])=="--test-proxies")return proxyOwnedSafety();
#endif
    gate(argc==6,"Private-or-absent libraries disposable-mount query fresh-output");gate(!std::ifstream(argv[5]).good(),"Fresh proxy output required");std::ifstream in(argv[4],std::ios::binary);std::ofstream out(argv[5],std::ios::binary);gate((bool(in)||std::string(argv[4])=="--native-boundaries")&&bool(out),"Proxy query/output");
#ifdef SFM64_NATIVE_OBJECT_PARAMETERS
    ProxyNativeFixture fixture(argv[1],argv[2],argv[3]);auto observer=AddVectoredExceptionHandler(1,&lifetimeFault);
    if(std::string(argv[4])=="--native-boundaries"){
        nativeProxyBoundaries(fixture,out);fixture.finishLifetime();RemoveVectoredExceptionHandler(observer);out.flush();gate(bool(out),"Native boundary output");std::cout<<"PASS native_proxy_boundaries=6 allocations="<<fixture.allocations<<" releases="<<fixture.releases<<'\n';return 0;
    }
#else
    SourceMaterialSourceProviderCandidate files({{std::filesystem::path(argv[3]),"GAME"}});auto provider=files.materialSource();
#endif
    const auto count=word(in);gate(count&&count<10000,"Proxy cases");unsigned totalEvents=0;
    for(unsigned caseIndex=0;caseIndex<count;++caseIndex){
        const auto route=word(in),queued=word(in);gate(route<=2&&queued<=1,"Proxy routes");auto name=text(in);auto root=proxyInput(in);std::vector<std::string>*trace=nullptr;
#ifdef SFM64_NATIVE_OBJECT_PARAMETERS
        currentCase=caseIndex;fixture.beginProxy();auto seed=builtinErrorMaterialSeedCandidate();auto*error=fixture.createActual(seed.name.c_str(),fixture.build(seed.source),false);fixture.word(0x1059ec,reinterpret_cast<uintptr_t>(error));
        void*source=route==0?fixture.build(root):nullptr,*external=route==1?fixture.build(root):nullptr;auto*raw=fixture.createActual(name.c_str(),source);fixture.material=raw;trace=&fixture.trace;
        auto init=[&](){gate(fixture.initializeActual(raw,external)&&raw[0x22]!=0,"Original proxy wrapper initialization");};
        auto preserve=[&](){fixture.uncacheActual(raw,true);};auto uncache=[&](){fixture.uncacheActual(raw);};
        auto ids=[&](){return fixture.published();};
        auto proxyCount=[&](){return unsigned(raw[0x23]);};
#else
        MaterialObjectStoreCandidate store;uintptr_t id=0;MaterialObjectStoreCandidate::Source source;if(route==0)source=std::make_unique<ProxyNode>(root);gate(store.createMaterial(name,source,id),"Owned proxy material");
        auto factory=std::make_shared<ProxyOwnedFactory>();trace=&factory->trace;MaterialObjectParameterOptionsCandidate options;options.proxyFactory=factory;options.provider=provider;if(route==1)options.externalSource=&root;
        auto init=[&](){gate(store.initializeParameters(id,options),"Owned proxy initialization");};auto preserve=[&](){gate(store.uncacheParameters(id,true),"Owned preserve");};auto uncache=[&](){gate(store.uncacheParameters(id),"Owned uncache");};
        auto ids=[&](){return factory->published();};auto proxyCount=[&](){auto*p=store.get(id);return unsigned(p&&p->proxyState?p->proxyState->size():0);};
#endif
        auto state=[&](unsigned stage){const auto values=ids();gate(values.size()==proxyCount(),"Published proxy list/count");std::ostringstream line;line<<"S "<<stage<<' '<<values.size();for(auto n:values)line<<' '<<n;trace->push_back(line.str());};
        state(0);init();state(1);init();preserve();state(2);uncache();state(3);init();state(4);
#ifdef SFM64_NATIVE_OBJECT_PARAMETERS
        fixture.enabled=queued!=0;fixture.forward(fixture.id(raw),13);fixture.forward(fixture.id(raw),51);if(queued)state(5);fixture.drain();gate(fixture.enumerate().empty(),"Native queued proxy owner removed");
#else
        gate(store.forward(id,MaterialObjectStoreCandidate::Operation::decrement,queued!=0)&&store.forward(id,MaterialObjectStoreCandidate::Operation::deleteIfUnreferenced,queued!=0),"Owned proxy deferred deletion");if(queued)state(5);store.drain();gate(store.size()==0&&factory->live.empty(),"Owned queued proxy owner removed");for(const auto&b:factory->bindings)gate(!b.valid()&&!b.read(),"Bindings expire after owner lifetime");
#endif
        trace->push_back("S 6 0");out<<"CASE "<<caseIndex<<'\n';for(const auto&line:*trace)out<<line<<'\n';totalEvents+=unsigned(trace->size());
#ifdef SFM64_NATIVE_OBJECT_PARAMETERS
        if(external)fixture.destroyTree(external);fixture.destroyActual(error);fixture.endProxy();
#endif
    }
    gate(in.peek()==std::char_traits<char>::eof(),"Proxy input tail");out.flush();gate(bool(out),"Complete proxy trace");
#ifdef SFM64_NATIVE_OBJECT_PARAMETERS
    fixture.finishLifetime();RemoveVectoredExceptionHandler(observer);std::cout<<"native_allocations="<<fixture.allocations<<" releases="<<fixture.releases<<'\n';
#endif
    std::cout<<"PASS proxy_cases="<<count<<" events="<<totalEvents<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

#ifndef SFM64_NATIVE_OBJECT_PARAMETERS
static int proxyOwnedSafety(){
    using Store=MaterialObjectStoreCandidate;unsigned checks=0;auto check=[&](bool b){gate(b,"Owned proxy safety");++checks;};
    for(unsigned scenario=0;scenario<9;++scenario){
        Store store;auto factory=std::make_shared<ProxyOwnedFactory>();Store::Source source=std::make_unique<ProxyNode>(proxyRoot());auto*root=source.get();uintptr_t id=0;check(store.createMaterial("proxy/safety",source,id));MaterialObjectParameterOptionsCandidate options;options.proxyFactory=factory;auto sourceCopy=*root;
        if(scenario==0){check(!store.initializeParameters(id));check(!store.get(id)->parameterState&&!store.get(id)->proxyState);}
        if(scenario==1||scenario==2){root->children.back().children.insert(root->children.back().children.begin()+1,proxyDeclaration(scenario==1?"Missing":"Fail"));sourceCopy=*root;check(!store.initializeParameters(id,options));check(factory->next==3&&factory->live.empty());}
        if(scenario==3){factory->throwCreate=true;check(!store.initializeParameters(id,options));check(factory->live.empty());}
        if(scenario==4){factory->onInit=[](){throw std::runtime_error("Test initializer exception");};check(!store.initializeParameters(id,options));check(factory->live.empty());}
        if(scenario==5){check(!store.initializeParameters(id,options,[](Store::Allocation a,size_t){return a!=Store::Allocation::parameterCommit;}));check(factory->released==2&&factory->live.empty());}
        if(scenario==6){*root=proxyRoot(256);sourceCopy=*root;check(!store.initializeParameters(id,options)&&factory->next==0);}
        if(scenario==7){factory->onInit=[&](){store.removeAndDelete(id);};check(!store.initializeParameters(id,options)&&!store.get(id)&&factory->live.empty());continue;}
        if(scenario==8){factory->onInit=[&](){root->children.front().text=".5";};check(!store.initializeParameters(id,options)&&factory->live.empty());sourceCopy=*root;}
        auto*p=store.get(id);check(p&&!(p->flags&4)&&!p->parameterState&&!p->proxyState&&!p->initializationPending&&materialObjectParameterTreeEqualCandidate(*root,sourceCopy));for(const auto&b:factory->bindings)check(!b.valid()&&!b.read());store.removeAndDelete(id);
    }
    // Published binding expiry, reinitialization, origin factory retention and
    // deletion/reentry during Release. Every callback sees expired parameters.
    for(unsigned scenario=0;scenario<5;++scenario){
        Store store;auto f=std::make_shared<ProxyOwnedFactory>();std::weak_ptr<ProxyOwnedFactory>weak=f;Store::Source source=std::make_unique<ProxyNode>(proxyRoot());uintptr_t id=0;check(store.createMaterial("proxy/release",source,id));MaterialObjectParameterOptionsCandidate o;o.proxyFactory=f;check(store.initializeParameters(id,o));auto bindings=f->bindings;auto old=bindings.front();check(old.valid()&&old.read()->value.numeric.vector[0]==0x3ec00000);check(!store.get(id)->releasePending&&store.uncacheParameters(id,true)&&old.valid());
        if(scenario==0){o.proxyFactory.reset();f.reset();check(!weak.expired());check(store.uncacheParameters(id)&&weak.expired()&&!old.valid());}
        if(scenario==1){f->onDestroy=[&](){check(!old.valid());check(!store.initializeParameters(id,o));check(!store.uncacheParameters(id));store.removeAndDelete(id);};check(store.uncacheParameters(id)&&!store.get(id)&&f->live.empty());}
        if(scenario==2){f->onDestroy=[&](){check(!store.removeAndDelete(id));};check(store.removeAndDelete(id)&&f->live.empty());}
        if(scenario==3){check(store.uncacheParameters(id)&&!old.valid());check(store.initializeParameters(id,o)&&!old.valid()&&f->bindings.back().valid());check(store.removeAndDelete(id));}
        if(scenario==4){auto other=std::make_shared<ProxyOwnedFactory>();o.proxyFactory=other;check(store.uncacheParameters(id)&&f->released==2&&other->released==0);check(store.initializeParameters(id,o)&&other->next==2);check(store.removeAndDelete(id));}
    }
    {
        auto owner=std::make_unique<Store>();auto*raw=owner.get();auto f=std::make_shared<ProxyOwnedFactory>();Store::Source source=std::make_unique<ProxyNode>(proxyRoot());uintptr_t id=0;check(owner->createMaterial("proxy/destructor",source,id));MaterialObjectParameterOptionsCandidate options;options.proxyFactory=f;check(owner->initializeParameters(id,options));auto binding=f->bindings.front();
        f->onDestroy=[&](){Store::Source empty;uintptr_t other=0;check(!binding.valid());check(!raw->createMaterial("forbidden-during-teardown",empty,other));check(!raw->initializeParameters(id,options));};owner.reset();check(f->live.empty()&&f->released==2&&!binding.read());
    }
    {
        Store store;auto f=std::make_shared<ProxyOwnedFactory>();Store::Source source=std::make_unique<ProxyNode>(proxyRoot());uintptr_t id=0;check(store.createMaterial("proxy/nonstandard-exception",source,id));MaterialObjectParameterOptionsCandidate options;options.proxyFactory=f;f->onInit=[](){throw 17;};check(!store.initializeParameters(id,options));check(f->live.empty()&&!store.get(id)->parameterState&&!store.get(id)->initializationPending);check(store.removeAndDelete(id));
    }
    // Sweep actual C++ heap failures through the production store/list. A
    // nonallocating factory keeps rollback callbacks safe under heap failure.
    struct AllocationFactory:MaterialProxyFactoryCandidate {
        struct Item:MaterialProxyInstanceCandidate{MaterialProxyBindingCandidate binding;bool initialize(const MaterialProxyContextCandidate&c,const ProxyNode&)override{binding=c.find("$alpha");return binding.valid();}};
        std::array<Item,4>items;unsigned created=0,released=0;
        MaterialProxyInstanceCandidate*create(const std::string&)override{return &items.at(created++);}
        void destroy(MaterialProxyInstanceCandidate*)noexcept override{++released;}
    };
    Store store;auto f=std::make_shared<AllocationFactory>();Store::Source source=std::make_unique<ProxyNode>(proxyRoot());auto*root=source.get();auto before=*root;uintptr_t id=0;check(store.createMaterial("proxy/allocation",source,id));MaterialObjectParameterOptionsCandidate options;options.proxyFactory=f;unsigned failures=0;bool success=false;
    for(int64_t n=0;n<4096;++n){f->created=f->released=0;const auto balance=heapNew-heapDelete;failAfter=n;bool ok=store.initializeParameters(id,options);failAfter=-1;if(ok){success=true;break;}++failures;for(auto&i:f->items){check(!i.binding.valid());i.binding={};}check(heapNew-heapDelete==balance&&f->created==f->released&&!store.get(id)->parameterState&&!store.get(id)->proxyState&&!store.get(id)->initializationPending&&materialObjectParameterTreeEqualCandidate(*root,before));}
    check(success&&failures>100&&f->created==2);check(store.removeAndDelete(id)&&f->released==2);
    std::cout<<"PASS proxy_safety_checks="<<checks<<" actual_allocation_failures="<<failures<<'\n';return 0;
}
#endif
