#define SFM64_UNLIT_TWO_TEXTURE_PARAMETERS
#define SFM64_LIGHTMAPPED_PARAMETERS
#define SFM64_OBJECT_PARAMETER_MAIN unlitTwoTextureRecorderMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#ifndef SFM64_NATIVE_OBJECT_PARAMETERS
static int twoTextureSafety(){
    using Store=MaterialObjectStoreCandidate;Store store;Store::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>();source->key="UnlitTwoTexture";
    for(auto name:{"$basetexture","$texture2","$dynamic_extra"})source->children.push_back({name,1,std::string("fixture/")+name});source->children.push_back({"$cloakpassenabled",1,"1"});
    auto*root=source.get();auto*child=&root->children.front();uintptr_t id=0;gate(store.createMaterial("two/safety",source,id),"Two safety construction");
    auto snapshot=[&](){std::ostringstream out;auto*p=store.get(id);put(out,p->flags);put(out,uint32_t(p->parameterCount));putTree(out,*p->source);if(p->parameterState)putValues(out,p->parameterState->material);return out.str();};const auto before=snapshot();unsigned checks=0,failures=0;
    auto check=[&](bool value){if(!value){std::cerr<<"Two safety assertion="<<checks<<" failures="<<failures<<'\n';throw std::runtime_error("Two texture safety");}++checks;};
    for(auto stage:{Store::Allocation::parameters,Store::Allocation::parameterCommit}){auto balance=heapNew-heapDelete;check(!store.initializeParameters(id,{},[&](Store::Allocation k,size_t){return k!=stage;}));const bool unchanged=snapshot()==before;check(unchanged&&heapNew-heapDelete==balance&&store.get(id)->source.get()==root&&&root->children.front()==child);}
    bool succeeded=false;for(int64_t n=0;n<4096;++n){auto balance=heapNew-heapDelete;failAfter=n;bool ok=store.initializeParameters(id);failAfter=-1;if(ok){succeeded=true;break;}++failures;const bool unchanged=snapshot()==before;check(unchanged&&heapNew-heapDelete==balance&&store.get(id)->source.get()==root&&&root->children.front()==child);}
    check(succeeded&&failures>100&&store.get(id)->parameterCount==21&&store.get(id)->parameterState->material.shader=="UnlitTwoTexture_DX9");const auto&v=store.get(id)->parameterState->material.parameters.values;
    check((v[0].value.numeric.integer&0x800)&&v[2].value.numeric.integer==0x40040&&v[18].value.numeric.vector==std::array<uint32_t,4>{0x3f800000,0x3f800000,0x3f800000,0x3f800000});
    check(store.uncacheParameters(id));SourceMaterialPatchNodeCandidate proxies;proxies.key="Proxies";proxies.children.push_back({"Sine",0});root->children.push_back(proxies);const auto priorProxy=snapshot();check(!store.initializeParameters(id)&&snapshot()==priorProxy&&!store.get(id)->initializationPending);root->children.pop_back();check(store.initializeParameters(id));check(store.forward(id,Store::Operation::decrement,true)&&store.forward(id,Store::Operation::deleteIfUnreferenced,true));store.drain();check(!store.get(id)&&store.size()==0);
    std::cout<<"PASS unlit_two_texture_safety_checks="<<checks<<" actual_allocation_failures="<<failures<<'\n';return 0;
}
static int pilotProxyRefusal(const char*path){
    const auto name=std::filesystem::path(path).stem().string();unsigned reads=0;MaterialObjectParameterOptionsCandidate options;
    options.provider=[&](const std::string&request)->std::optional<std::string>{if(request!="materials/"+name+".vmt")return {};++reads;std::ifstream in(path,std::ios::binary);gate(bool(in),"Disposable pilot input exists");std::string source((std::istreambuf_iterator<char>(in)),{});gate(source.find("\"Proxies\"")!=std::string::npos,"Actual pilot proxy declaration required");return source;};
    MaterialObjectStoreCandidate store;MaterialObjectStoreCandidate::Source empty;uintptr_t id=0;gate(store.createMaterial(name,empty,id),"Pilot refusal construction");auto*p=store.get(id);auto before=p;const auto refs=p->references.load();const auto flags=p->flags;MaterialObjectParameterAttemptCandidate attempt;
    gate(!store.initializeParameters(id,options,{},&attempt)&&reads==1&&attempt.status==MaterialObjectParameterStatusCandidate::Refused&&store.get(id)==before&&p->references.load()==refs&&p->flags==flags&&!p->parameterState&&p->parameterCount==0&&!p->source&&!p->forwarding.valuesInitialized&&!p->initializationPending&&store.cached().size()==1&&store.valid(),"Complete pilot declaration must refuse before parameter publication");
    store.decrement(id);gate(store.deleteIfUnreferenced(id),"Refused pilot lifetime");std::cout<<"PASS untouched_pilot_proxy_refusal=1\n";return 0;
}
#endif
int main(int argc,char**argv){
#ifndef SFM64_NATIVE_OBJECT_PARAMETERS
    try{if(argc==2&&std::string(argv[1])=="--test-unlit-two-texture")return twoTextureSafety();if(argc==3&&std::string(argv[1])=="--test-pilot-refusal")return pilotProxyRefusal(argv[2]);}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
#endif
    return unlitTwoTextureRecorderMain(argc,argv);
}
