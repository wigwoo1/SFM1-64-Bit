#ifdef SFM64_NATIVE_CLIENT_EQUALS
#define SFM64_NATIVE_OBJECT_PARAMETERS
#endif
#define SFM64_OBJECT_PARAMETER_MAIN textureEvaluationPriorParameterMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#include "material_sine_proxy_candidate.h"
#ifdef SFM64_NATIVE_CLIENT_EQUALS
#include "material_texture_transform_client_native_fixture.h"
#endif
static MaterialObjectParameterPlanCandidate preparedTexture(std::istream&in,const SourceMaterialPatchNodeCandidate&root){
    MaterialObjectParameterPlanCandidate p;p.finalRoot=root;p.state=std::make_unique<MaterialObjectParameterStateCandidate>();auto&m=p.state->material;m.shader=text(in);auto count=word(in);gate(count>=13&&count<=255,"Original prepared variable count");
    for(unsigned i=0;i<count;++i){std::array<uint32_t,28>q{};for(auto&x:q)x=word(in);MaterialTypedDispatchCandidate v;v.created=q[0]!=0;v.value.numeric.typeAndMetadata=q[1];v.value.numeric.integer=q[2];std::copy_n(q.begin()+3,4,v.value.numeric.vector.begin());gate(q[10]<4096&&q[11]<1024,"Prepared text lengths");v.value.text.resize(q[10]);v.name.resize(q[11]);in.read(v.value.text.data(),q[10]);in.read(v.name.data(),q[11]);if(q[8]){v.matrix.emplace();std::copy_n(q.begin()+12,16,v.matrix->begin());}v.matrixIdentity=q[9]!=0;m.parameters.values.push_back(std::move(v));}
    gate(bool(in),"Complete prepared state");return p;
}
static void textureWrites(std::ostream&out,const std::vector<MaterialProxyWriteCandidate>&writes){
    put(out,uint32_t(writes.size()));for(const auto&w:writes){put(out,w.index);put(out,w.operation);put(out,w.count);for(auto x:w.arguments)put(out,x);put(out,bool(w.matrix));const auto m=w.matrix?*w.matrix:MaterialMatrixMathCandidate{};for(auto x:m)put(out,x);}
}
#ifndef SFM64_NATIVE_CLIENT_EQUALS
static int textureSafety();
#endif
int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_CLIENT_EQUALS
    if(argc==2&&std::string(argv[1])=="--test-texture")return textureSafety();static_assert(sizeof(void*)==8,"Owned texture evaluation x64");
#endif
    gate((argc==5||(argc==6&&std::string(argv[5])=="--sse2"))&&!std::ifstream(argv[4]).good(),"client-or-absent query original-prepared fresh-output [--sse2]");std::ifstream in(argv[2],std::ios::binary),prepared(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);gate(bool(in)&&bool(prepared)&&bool(out),"Texture evaluation IO");
#ifdef SFM64_NATIVE_CLIENT_EQUALS
    SetErrorMode(SEM_NOGPFAULTERRORBOX|SEM_FAILCRITICALERRORS);MaterialTextureTransformClientNativeFixture client(argv[1]);client.word(0x854fc4,argc==6?1:0);auto handler=AddVectoredExceptionHandler(1,&lifetimeFault);unsigned cw=0,control=0;_controlfp_s(&cw,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
#endif
    const auto mx=_mm_getcsr();_mm_setcsr(0x1f80);auto count=word(in);gate(count&&count<10000&&word(prepared)==count,"Case count");put(out,count);unsigned frames=0,refused=0;
    for(unsigned caseIndex=0;caseIndex<count;++caseIndex){
        std::array<uint32_t,3>times{};for(auto&x:times)x=word(in);std::array<std::array<uint32_t,4>,3>missing{};for(auto&frame:missing)for(auto&x:frame)x=word(in);auto root=node(in);auto plan=preparedTexture(prepared,root);auto*selected=materialProxySelectedNodeCandidate(root,plan.state->material.selectedNode);gate(selected!=nullptr,"Selected declaration node");auto*declarations=sourceMaterialPatchFindConstCandidate(*selected,"Proxies");if(!declarations)declarations=sourceMaterialPatchFindConstCandidate(root,"Proxies");gate(declarations!=nullptr,"Declarations required");
#ifdef SFM64_NATIVE_CLIENT_EQUALS
        currentCase=caseIndex;client.prepareTexture(*plan.state);std::vector<void*>proxies;bool initialized=true;
        for(const auto&d:declarations->children){auto*p=client.createTexture(d.key);if(!client.initializeFunction(p,d)){client.destroy(p);initialized=false;break;}gate(client.materialIdentity(p),"Actual GetMaterial identity");proxies.push_back(p);}
#else
        MaterialObjectParameterOptionsCandidate options;options.proxyFactory=std::make_shared<MaterialFunctionProxyFactoryCandidate>();MaterialObjectParameterPlanCandidate own;gate(materialObjectParameterPrepareCandidate(own,"equals/case",0x10,&root,options),"Owned CPU state");std::ostringstream actual,expected;putValues(actual,own.state->material);putValues(expected,plan.state->material);gate(actual.str()==expected.str(),"Owned initial state equals separately executed original CMaterial CPU state");
        MaterialObjectStoreCandidate store;MaterialObjectStoreCandidate::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>(root);uintptr_t id=0;gate(store.createMaterial("equals/case",source,id),"Owned material");const bool initialized=store.initializeParameters(id,options);
#endif
        put(out,caseIndex);put(out,initialized);if(!initialized)++refused;
        if(initialized)for(unsigned frame=0;frame<3;++frame){
#ifdef SFM64_NATIVE_CLIENT_EQUALS
            client.writes.clear();client.missingCache(missing[frame]);for(auto*p:proxies)client.evaluateFunction(p,times[frame]);textureWrites(out,client.writes);auto material=plan.state->material;material.parameters.values.clear();for(size_t i=0;i<client.parameterCount;++i)material.parameters.values.push_back(client.variables[i].value);putValues(out,material);
            const auto&v=material.parameters.values;put(out,v[5].value.numeric.vector[0]);for(size_t i=0;i<3;++i)put(out,v[4].value.numeric.vector[i]);for(size_t i=0;i<3;++i)put(out,materialMatrixBinaryCandidate(v[4].value.numeric.vector[i],v[11].value.numeric.vector[i],true));
#else
            MaterialProxyEvaluationInputsCandidate context;context.currentTimeBits=times[frame];context.sharedMissingVariableCacheBits=missing[frame];MaterialProxyEvaluationAttemptCandidate attempt;gate(store.evaluateProxies(id,{},&attempt,context)&&attempt.evaluated,"Owned full frame");textureWrites(out,attempt.writes);putValues(out,store.get(id)->parameterState->material);for(auto x:store.get(id)->forwarding.values)put(out,x);
#endif
            ++frames;
        }
#ifdef SFM64_NATIVE_CLIENT_EQUALS
        for(auto i=proxies.rbegin();i!=proxies.rend();++i)client.destroy(*i);gate(client.blocks.empty(),"Native proxy allocation balance");client.image.verify();
#else
        gate(store.removeAndDelete(id),"Owned material cleanup");
#endif
    }
    gate(in.peek()==std::char_traits<char>::eof()&&prepared.peek()==std::char_traits<char>::eof(),"Input tails");out.flush();gate(bool(out),"Complete output");_mm_setcsr(mx);
#ifdef SFM64_NATIVE_CLIENT_EQUALS
    _controlfp_s(&control,cw,_MCW_RC|_MCW_PC);RemoveVectoredExceptionHandler(handler);std::cout<<"native_proxy_allocations="<<client.allocations<<" releases="<<client.releases<<'\n';
#endif
    std::cout<<"PASS texture_evaluation_cases="<<count<<" frames="<<frames<<" init_refused="<<refused<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
#ifndef SFM64_NATIVE_CLIENT_EQUALS
static int textureSafety(){
    using Store=MaterialObjectStoreCandidate;unsigned checks=0;auto check=[&](bool ok){if(!ok)throw std::runtime_error("Texture safety check="+std::to_string(checks));++checks;};
    auto root=[](bool missing){SourceMaterialPatchNodeCandidate r;r.key="UnlitTwoTexture";r.children={{"$out",1,"[1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1]"},{"$scale",1,"[1.5 .5]"},{"$driver",3,"",0x3e000000}};SourceMaterialPatchNodeCandidate e;e.key="Equals";e.children={{"resultVar",1,"$driver"},{"srcVar1",1,"$scale"}};SourceMaterialPatchNodeCandidate d;d.key="TextureTransform";d.children={{"resultVar",1,"$out[999]"},{"scaleVar",1,missing?"$not-present":"$scale"}};SourceMaterialPatchNodeCandidate p;p.key="Proxies";p.children={e,d};r.children.push_back(p);return r;};
    MaterialObjectParameterOptionsCandidate options;options.proxyFactory=std::make_shared<MaterialFunctionProxyFactoryCandidate>();Store store;Store::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>(root(true));auto*borrowed=source.get();auto oldRoot=*borrowed;uintptr_t id=0;check(store.createMaterial("texture/safety",source,id)&&store.initializeParameters(id,options));
    auto snapshot=[&](){std::ostringstream s;putValues(s,store.get(id)->parameterState->material);for(auto x:store.get(id)->forwarding.values)put(s,x);return s.str();};const auto before=snapshot();const auto*state=store.get(id)->parameterState.get();MaterialProxyEvaluationAttemptCandidate attempt;check(!store.evaluateProxies(id,{},&attempt)&&snapshot()==before&&store.get(id)->parameterState.get()==state&&!attempt.evaluated&&attempt.writes.empty());
    MaterialProxyEvaluationInputsCandidate context;context.sharedMissingVariableCacheBits=std::array<uint32_t,4>{0x3f800000,0x3f800000,0,0};check(!store.evaluateProxies(id,[](Store::Allocation a,size_t){return a!=Store::Allocation::evaluationCommit;},&attempt,context)&&snapshot()==before&&attempt.writes.empty());
    unsigned failures=0;bool success=false;for(int64_t n=0;n<2048;++n){const auto balance=heapNew-heapDelete;failAfter=n;const bool ok=store.evaluateProxies(id,{},&attempt,context);failAfter=-1;if(ok){success=true;break;}++failures;check(heapNew-heapDelete==balance&&snapshot()==before&&store.get(id)->parameterState.get()==state&&attempt.writes.empty()&&!attempt.evaluated&&!store.get(id)->evaluationPending&&materialObjectParameterTreeEqualCandidate(*borrowed,oldRoot));}
    check(success&&failures&&attempt.writes.size()==2&&attempt.writes[0].operation==1&&attempt.writes[1].operation==9&&attempt.writes[1].count==16&&attempt.writes[1].matrix);const auto first=attempt.writes[1].matrix;context.sharedMissingVariableCacheBits=std::array<uint32_t,4>{0,0,0,0};check(store.evaluateProxies(id,{},&attempt,context)&&attempt.writes[1].matrix!=first);check(store.uncacheParameters(id)&&!store.evaluateProxies(id,{},&attempt,context)&&store.initializeParameters(id,options)&&store.removeAndDelete(id));
    Store second;Store::Source r=std::make_unique<SourceMaterialPatchNodeCandidate>(root(false));uintptr_t other=0;check(second.createMaterial("texture/bound",r,other)&&second.initializeParameters(other,options)&&second.evaluateProxies(other)&&second.removeAndDelete(other));
    for(auto name:{"$alpha","$missing"}){Store a;auto tree=root(false);tree.children.back().children.back().children.front().text=name;Store::Source p=std::make_unique<SourceMaterialPatchNodeCandidate>(tree);uintptr_t x=0;check(a.createMaterial("texture/refusal",p,x)&&!a.initializeParameters(x,options)&&!a.get(x)->parameterState);}
    {Store a;auto tree=root(false);tree.children.back().children.back().children.front().text="$basetexture";Store::Source p=std::make_unique<SourceMaterialPatchNodeCandidate>(tree);uintptr_t x=0;check(a.createMaterial("texture/undefined-result",p,x)&&a.initializeParameters(x,options)&&a.evaluateProxies(x)&&a.removeAndDelete(x));}
    struct ObserverFactory:MaterialFunctionProxyFactoryCandidate {
        std::function<bool(MaterialProxyEvaluationCandidate&)>hook;
        struct Observer:MaterialProxyInstanceCandidate{ObserverFactory&f;explicit Observer(ObserverFactory&v):f(v){}bool initialize(const MaterialProxyContextCandidate&,const SourceMaterialPatchNodeCandidate&)override{return true;}bool evaluate(MaterialProxyEvaluationCandidate&frame)override{return !f.hook||f.hook(frame);}};
        MaterialProxyInstanceCandidate*create(const std::string&name)override{return name=="ObserveContext"?new Observer(*this):MaterialFunctionProxyFactoryCandidate::create(name);}
    };
    Store observed;auto factory=std::make_shared<ObserverFactory>();auto tree=root(true);SourceMaterialPatchNodeCandidate observe;observe.key="ObserveContext";tree.children.back().children.insert(tree.children.back().children.begin(),observe);Store::Source sr=std::make_unique<SourceMaterialPatchNodeCandidate>(tree);uintptr_t observedId=0;MaterialObjectParameterOptionsCandidate oo;oo.proxyFactory=factory;check(observed.createMaterial("texture/context",sr,observedId)&&observed.initializeParameters(observedId,oo));context.sharedMissingVariableCacheBits=std::array<uint32_t,4>{0x3f800000,0x3f800000,0,0};
    factory->hook=[&](auto&frame){check(frame.sharedMissingVariableCache()==context.sharedMissingVariableCacheBits);context.sharedMissingVariableCacheBits=std::array<uint32_t,4>{0,0,0,0};check(frame.sharedMissingVariableCache()->at(0)==0x3f800000);check(!observed.removeAndDelete(observedId)&&!observed.uncacheParameters(observedId)&&!observed.evaluateProxies(observedId));return true;};check(observed.evaluateProxies(observedId,{},&attempt,context)&&attempt.writes.back().matrix->at(0)==0x3f800000);factory->hook={};check(observed.evaluateProxies(observedId,{},&attempt,context)&&attempt.writes.back().matrix->at(0)==0);
    Store independent;Store::Source ir=std::make_unique<SourceMaterialPatchNodeCandidate>(root(true));uintptr_t independentId=0;check(independent.createMaterial("texture/shared",ir,independentId)&&independent.initializeParameters(independentId,options)&&independent.evaluateProxies(independentId,{},&attempt,context));const auto otherMatrix=attempt.writes.back().matrix;check(observed.evaluateProxies(observedId,{},&attempt,context)&&attempt.writes.back().matrix==otherMatrix&&observed.removeAndDelete(observedId)&&independent.removeAndDelete(independentId));
    std::cout<<"PASS texture_safety_checks="<<checks<<" actual_allocation_failures="<<failures<<'\n';return 0;
}
#endif
