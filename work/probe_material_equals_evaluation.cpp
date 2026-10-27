#ifdef SFM64_NATIVE_CLIENT_EQUALS
#define SFM64_NATIVE_OBJECT_PARAMETERS
#endif
#define SFM64_OBJECT_PARAMETER_MAIN equalsPriorParameterRecorderMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#include "material_equals_proxy_candidate.h"
#ifdef SFM64_NATIVE_CLIENT_EQUALS
#include "material_equals_client_native_fixture.h"
#endif
static const SourceMaterialPatchNodeCandidate*equalsDeclarations(const MaterialObjectParameterPlanCandidate&plan){auto*p=materialProxySelectedNodeCandidate(plan.finalRoot,plan.state->material.selectedNode);gate(p!=nullptr,"Selected node");auto*r=sourceMaterialPatchFindConstCandidate(*p,"Proxies");return r?r:sourceMaterialPatchFindConstCandidate(plan.finalRoot,"Proxies");}
static void putWrites(std::ostream&out,const std::vector<MaterialProxyWriteCandidate>&writes){put(out,uint32_t(writes.size()));for(const auto&w:writes){put(out,w.index);put(out,w.operation);put(out,w.count);for(auto b:w.arguments)put(out,b);}}
static MaterialObjectParameterPlanCandidate readPrepared(std::istream&in,const SourceMaterialPatchNodeCandidate&root){
    MaterialObjectParameterPlanCandidate p;p.finalRoot=root;p.state=std::make_unique<MaterialObjectParameterStateCandidate>();auto&m=p.state->material;m.shader=text(in);auto count=word(in);gate(count>=13&&count<=255,"Prepared original variable count");
    for(unsigned i=0;i<count;++i){std::array<uint32_t,28>q{};for(auto&n:q)n=word(in);MaterialTypedDispatchCandidate v;v.created=q[0]!=0;v.value.numeric.typeAndMetadata=q[1];v.value.numeric.integer=q[2];std::copy_n(q.begin()+3,4,v.value.numeric.vector.begin());gate(q[10]<4096&&q[11]<1024,"Prepared variable text extent");v.value.text.resize(q[10]);v.name.resize(q[11]);in.read(v.value.text.data(),q[10]);in.read(v.name.data(),q[11]);if(q[8]){v.matrix.emplace();std::copy_n(q.begin()+12,16,v.matrix->begin());}v.matrixIdentity=q[9]!=0;m.parameters.values.push_back(std::move(v));}
    gate(bool(in),"Complete prepared original state");return p;
}
struct ScalarEqualsProbeFactory:MaterialProxyFactoryCandidate {
    struct Instance:MaterialProxyInstanceCandidate {
        MaterialEqualsProxyCandidate equals;MaterialProxyBindingCandidate result;int32_t component=-1;uint32_t bits;
        explicit Instance(uint32_t value):bits(value){}
        bool initialize(const MaterialProxyContextCandidate&c,const SourceMaterialPatchNodeCandidate&n)override{
            if(!equals.initialize(c,n))return false;auto*p=sourceMaterialPatchFindConstCandidate(n,"resultVar");if(!p)return false;auto name=p->text;auto at=name.find('[');if(at!=std::string::npos){component=int32_t(materialStringIntegerCandidate(name.substr(at+1)));name.resize(at);}result=c.find(name);return result.valid();
        }
        bool evaluate(MaterialProxyEvaluationCandidate&frame)override{return materialProxySetResultFloatCandidate(frame,result,component,bits);}
    };
    uint32_t bits;explicit ScalarEqualsProbeFactory(uint32_t value):bits(value){}
    MaterialProxyInstanceCandidate*create(const std::string&name)override{return name=="Equals"?new Instance(bits):nullptr;}
    void destroy(MaterialProxyInstanceCandidate*p)noexcept override{delete p;}
};
#ifndef SFM64_NATIVE_CLIENT_EQUALS
static int equalsSafety();
#endif
int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_CLIENT_EQUALS
    if(argc==2&&std::string(argv[1])=="--test-equals")return equalsSafety();
#endif
    gate(argc==5&&!std::ifstream(argv[4]).good(),"Private-client-or-absent query prepared-original-state fresh-output");std::ifstream in(argv[2],std::ios::binary),prepared(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);gate(bool(in)&&bool(prepared)&&bool(out),"Equals IO");
#ifdef SFM64_NATIVE_CLIENT_EQUALS
    SetErrorMode(SEM_NOGPFAULTERRORBOX|SEM_FAILCRITICALERRORS);
    MaterialEqualsClientNativeFixture client(argv[1]);auto handler=AddVectoredExceptionHandler(1,&lifetimeFault);unsigned old=0,temp=0;_controlfp_s(&old,0,0);_controlfp_s(&temp,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
#else
    static_assert(sizeof(void*)==8,"Owned Equals runtime x64");
#endif
    const auto mx=_mm_getcsr();_mm_setcsr(0x1f80);auto count=word(in);gate(count&&count<10000&&word(prepared)==count,"Equals/prepared case count");put(out,count);unsigned frames=0,refused=0;
    for(unsigned caseIndex=0;caseIndex<count;++caseIndex){
        const auto mode=word(in),bits=word(in);auto root=node(in);gate(mode<=1,"Equals/scalar-helper mode");auto plan=readPrepared(prepared,root);auto*declarations=equalsDeclarations(plan);gate(declarations!=nullptr,"Proxy declarations required");
#ifdef SFM64_NATIVE_CLIENT_EQUALS
        currentCase=caseIndex;client.prepare(*plan.state);std::vector<void*>proxies;bool initialized=true;
        for(const auto&declaration:declarations->children){gate(declaration.key=="Equals","Pinned factory name");auto*p=client.create();bool ok=client.initialize(p,declaration);if(!ok){client.destroy(p);initialized=false;break;}gate(client.materialIdentity(p),"Original GetMaterial returns supplied material identity");proxies.push_back(p);}
#else
        MaterialObjectParameterOptionsCandidate options;options.proxyFactory=std::make_shared<MaterialEqualsProxyFactoryCandidate>();MaterialObjectParameterPlanCandidate own;gate(materialObjectParameterPrepareCandidate(own,"equals/case",0x10,&root,options),"Owned CPU state preparation");std::ostringstream actual,expected;putValues(actual,own.state->material);putValues(expected,plan.state->material);gate(actual.str()==expected.str(),"Owned initial state equals separately executed original materialsystem state");
        MaterialObjectStoreCandidate store;MaterialObjectStoreCandidate::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>(root);uintptr_t id=0;gate(store.createMaterial("equals/case",source,id),"Owned Equals material");if(mode)options.proxyFactory=std::make_shared<ScalarEqualsProbeFactory>(bits);const bool initialized=store.initializeParameters(id,options);
#endif
        put(out,caseIndex);put(out,initialized);if(!initialized)++refused;
        if(initialized)for(unsigned frame=0;frame<3;++frame){
#ifdef SFM64_NATIVE_CLIENT_EQUALS
            client.writes.clear();for(auto*p:proxies){if(mode)client.scalar(p,bits);else client.evaluate(p);}putWrites(out,client.writes);auto value=plan.state->material;value.parameters.values.clear();for(const auto&v:client.variables)value.parameters.values.push_back(v.value);putValues(out,value);
            const auto&values=value.parameters.values;put(out,values[5].value.numeric.vector[0]);for(size_t k=0;k<3;++k)put(out,values[4].value.numeric.vector[k]);for(size_t k=0;k<3;++k){float a,b;auto x=values[4].value.numeric.vector[k],y=values[11].value.numeric.vector[k];std::memcpy(&a,&x,4);std::memcpy(&b,&y,4);volatile float product=a*b;float z=product;uint32_t productBits;std::memcpy(&productBits,&z,4);put(out,productBits);}
#else
            MaterialProxyEvaluationAttemptCandidate attempt;gate(store.evaluateProxies(id,{},&attempt)&&attempt.evaluated,"Owned evaluated frame");putWrites(out,attempt.writes);putValues(out,store.get(id)->parameterState->material);for(auto n:store.get(id)->forwarding.values)put(out,n);
#endif
            ++frames;
        }
#ifdef SFM64_NATIVE_CLIENT_EQUALS
        for(auto i=proxies.rbegin();i!=proxies.rend();++i)client.destroy(*i);client.image.verify();gate(client.blocks.empty(),"Native per-case proxy cleanup");
#else
        gate(store.removeAndDelete(id),"Owned Equals cleanup");
#endif
    }
    gate(in.peek()==std::char_traits<char>::eof()&&prepared.peek()==std::char_traits<char>::eof(),"Equals input/prepared tail");out.flush();gate(bool(out),"Complete Equals trace");_mm_setcsr(mx);
#ifdef SFM64_NATIVE_CLIENT_EQUALS
    client.image.verify();RemoveVectoredExceptionHandler(handler);_controlfp_s(&temp,old,_MCW_RC|_MCW_PC);std::cout<<"native_proxy_allocations="<<client.allocations<<" releases="<<client.releases<<'\n';
#endif
    std::cout<<"PASS equals_cases="<<count<<" frames="<<frames<<" init_refused="<<refused<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
#ifndef SFM64_NATIVE_CLIENT_EQUALS
static SourceMaterialPatchNodeCandidate equalsSafetyRoot(){
    SourceMaterialPatchNodeCandidate root;root.key="UnlitTwoTexture";root.children={{"$out",3,"",0x3e800000},{"$src",3,"",0x3f400000}};SourceMaterialPatchNodeCandidate proxy;proxy.key="Equals";proxy.children={{"resultVar",1,"$out"},{"srcVar1",1,"$src"}};SourceMaterialPatchNodeCandidate block;block.key="Proxies";block.children.push_back(proxy);root.children.push_back(block);return root;
}
static int equalsSafety(){
    using Store=MaterialObjectStoreCandidate;unsigned checks=0;auto check=[&](bool v){if(!v)throw std::runtime_error("Equals transactional safety check="+std::to_string(checks));++checks;};
    struct Factory:MaterialEqualsProxyFactoryCandidate {
        struct Proxy:MaterialProxyInstanceCandidate {Factory&factory;MaterialEqualsProxyCandidate inner;MaterialProxyBindingCandidate out;explicit Proxy(Factory&f):factory(f){}bool initialize(const MaterialProxyContextCandidate&c,const SourceMaterialPatchNodeCandidate&n)override{out=c.find("$out");factory.binding=out;return inner.initialize(c,n);}bool evaluate(MaterialProxyEvaluationCandidate&frame)override{auto before=out.read();if(!inner.evaluate(frame))return false;if(factory.hook)return factory.hook(frame,before);return true;}};
        MaterialProxyBindingCandidate binding;std::function<bool(MaterialProxyEvaluationCandidate&,std::optional<MaterialTypedDispatchCandidate>)>hook;
        MaterialProxyInstanceCandidate*create(const std::string&name)override{return name=="Equals"?new Proxy(*this):nullptr;}
    };
    Store store;auto f=std::make_shared<Factory>();Store::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>(equalsSafetyRoot());auto*root=source.get();uintptr_t id=0;check(store.createMaterial("equals/safety",source,id));MaterialObjectParameterOptionsCandidate options;options.proxyFactory=f;check(store.initializeParameters(id,options));
    auto snapshot=[&](){std::ostringstream s;putValues(s,store.get(id)->parameterState->material);for(auto n:store.get(id)->forwarding.values)put(s,n);return s.str();};const auto before=snapshot();auto*state=store.get(id)->parameterState.get();auto rootBefore=*root;
    for(auto stage:{Store::Allocation::evaluation,Store::Allocation::evaluationCommit}){check(!store.evaluateProxies(id,[&](Store::Allocation a,size_t){return a!=stage;}));check(snapshot()==before&&store.get(id)->parameterState.get()==state&&f->binding.valid()&&!store.get(id)->evaluationPending);}
    f->hook=[&](auto&,auto){check(f->binding.read()->value.numeric.vector[0]==0x3e800000);check(!store.removeAndDelete(id)&&!store.uncacheParameters(id)&&!store.evaluateProxies(id));return false;};check(!store.evaluateProxies(id)&&snapshot()==before);
    f->hook=[](auto&,auto)->bool{throw 17;};check(!store.evaluateProxies(id)&&snapshot()==before&&!store.get(id)->evaluationPending);f->hook={};
    unsigned failures=0;bool success=false;
    for(int64_t n=0;n<2048;++n){const auto balance=heapNew-heapDelete;failAfter=n;const bool ok=store.evaluateProxies(id);failAfter=-1;if(ok){success=true;break;}++failures;check(heapNew-heapDelete==balance&&snapshot()==before&&store.get(id)->parameterState.get()==state&&materialObjectParameterTreeEqualCandidate(*root,rootBefore)&&!store.get(id)->evaluationPending&&f->binding.valid());}
    check(success&&failures>0&&f->binding.read()->value.numeric.vector[0]==0x3f400000&&materialObjectParameterTreeEqualCandidate(*root,rootBefore));auto committed=snapshot();check(store.evaluateProxies(id)&&snapshot()==committed);auto binding=f->binding;check(store.uncacheParameters(id)&&!binding.valid()&&!store.evaluateProxies(id));check(store.initializeParameters(id,options)&&!binding.valid()&&f->binding.valid());check(store.removeAndDelete(id)&&!f->binding.valid());
    for(auto name:{"$alpha","$out[4]","$out[999999999999]"}){Store a;auto r=equalsSafetyRoot();r.children.back().children.front().children.front().text=name;Store::Source s=std::make_unique<SourceMaterialPatchNodeCandidate>(r);uintptr_t x=0;check(a.createMaterial("equals/refusal",s,x));check(!a.initializeParameters(x,options)&&!a.get(x)->parameterState);}
    MaterialNumericValueCandidate v;v.typeAndMetadata=0x22;auto copy=v;check(!materialVectorComponentCandidate(v,4,1)&&std::memcmp(&v,&copy,24)==0);v.typeAndMetadata=0;copy=v;check(!materialVectorComponentCandidate(v,0,1)&&std::memcmp(&v,&copy,24)==0);
    std::cout<<"PASS equals_safety_checks="<<checks<<" actual_allocation_failures="<<failures<<'\n';return 0;
}
#endif
