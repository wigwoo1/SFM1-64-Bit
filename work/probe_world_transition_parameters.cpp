#define SFM64_WORLD_TRANSITION_PARAMETERS
#define SFM64_LIGHTMAPPED_PARAMETERS
#define SFM64_OBJECT_PARAMETER_MAIN worldTransitionRecorderMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#ifndef SFM64_NATIVE_OBJECT_PARAMETERS
static int transitionSafety(){
    using Store=MaterialObjectStoreCandidate;Store store;Store::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>();
    source->key="WorldVertexTransition";for(auto name:{"$basetexture","$basetexture2","$bumpmap","$envmap","$paintsplatenvmap","$alphatestreference"})source->children.push_back({name,1,std::string("fixture/")+name});
    auto*root=source.get();auto*first=&root->children.front();uintptr_t id=0;gate(store.createMaterial("world/safety",source,id),"World safety construction");
    auto snapshot=[&](){std::ostringstream out;auto*p=store.get(id);put(out,p->flags);put(out,uint32_t(p->parameterCount));putTree(out,*p->source);if(p->parameterState)putValues(out,p->parameterState->material);return out.str();};const auto before=snapshot();
    MaterialObjectParameterOptionsCandidate options;options.lightmappedSettings.paintEnabled=true;options.lightmappedSettings.forceVertexFog=1;unsigned checks=0,failures=0;
    auto check=[&](bool value){if(!value){std::cerr<<"World safety assertion="<<checks<<" failures="<<failures<<'\n';throw std::runtime_error("World transition safety");}++checks;};
    for(auto stage:{Store::Allocation::parameters,Store::Allocation::parameterCommit}){auto balance=heapNew-heapDelete;check(!store.initializeParameters(id,options,[&](Store::Allocation k,size_t){return k!=stage;}));bool unchanged=snapshot()==before;check(unchanged&&heapNew-heapDelete==balance&&store.get(id)->source.get()==root&&&root->children.front()==first);}
    bool succeeded=false;for(int64_t n=0;n<4096;++n){auto balance=heapNew-heapDelete;failAfter=n;bool ok=store.initializeParameters(id,options);failAfter=-1;if(ok){succeeded=true;break;}++failures;bool unchanged=snapshot()==before;check(unchanged&&heapNew-heapDelete==balance&&store.get(id)->source.get()==root&&&root->children.front()==first);}
    check(succeeded&&failures>100&&store.get(id)->parameterState->material.shader=="WorldVertexTransition_DX9"&&store.get(id)->parameterCount==52);
    const auto&v=store.get(id)->parameterState->material.parameters.values;check(v[50].name=="$paintsplatenvmap"&&v[50].value.text=="fixture/$paintsplatenvmap"&&v[51].name=="$alphatestreference"&&v[51].value.text=="fixture/$alphatestreference");
    check(store.uncacheParameters(id));auto*env=sourceMaterialPatchFindCandidate(*root,"$envmap",false);env->kind=2;env->bits=1;env->text.clear();options.lightmappedSettings.shaderInitSlot5=1;const auto unsupported=snapshot();check(!store.initializeParameters(id,options)&&snapshot()==unsupported&&!store.get(id)->initializationPending);
    env->kind=1;env->bits=0;env->text="fixture/env";check(store.initializeParameters(id,options));check(store.forward(id,Store::Operation::decrement,true)&&store.forward(id,Store::Operation::deleteIfUnreferenced,true));store.drain();check(!store.get(id)&&store.size()==0);
    std::cout<<"PASS world_transition_safety_checks="<<checks<<" actual_allocation_failures="<<failures<<'\n';return 0;
}
#endif
int main(int argc,char**argv){
#ifndef SFM64_NATIVE_OBJECT_PARAMETERS
    if(argc==2&&std::string(argv[1])=="--test-world-transition")try{return transitionSafety();}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
#endif
    return worldTransitionRecorderMain(argc,argv);
}
