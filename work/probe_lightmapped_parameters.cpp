// Use the same complete normally constructed material lifecycle recorder.
// Only this new corpus extends its input with six explicit service settings.
#define SFM64_LIGHTMAPPED_PARAMETERS
#define SFM64_OBJECT_PARAMETER_MAIN materialObjectParameterMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#ifndef SFM64_NATIVE_OBJECT_PARAMETERS
static int lightmappedSafety(){
    using Store=MaterialObjectStoreCandidate;Store store;Store::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>();
    source->key="LightmappedGeneric";for(auto name:{"$basetexture","$albedo","$bumpmap","$envmap","$lightwarptexture","$blendmodulatetexture"})source->children.push_back({name,1,std::string("fixture/")+name});
    auto*root=source.get();auto*first=&root->children.front();uintptr_t id=0;gate(store.createMaterial("lightmapped/safety",source,id),"Lightmapped safety construction");
    auto snapshot=[&](){std::ostringstream out;auto*p=store.get(id);put(out,p->flags);put(out,uint32_t(p->parameterCount));putTree(out,*p->source);if(p->parameterState)putValues(out,p->parameterState->material);return out.str();};const auto before=snapshot();
    MaterialObjectParameterOptionsCandidate options;options.lightmappedSettings.paintEnabled=true;options.lightmappedSettings.disableLightwarp=1;options.lightmappedSettings.forceVertexFog=1;
    unsigned checks=0,failures=0;auto check=[&](bool value){if(!value){std::cerr<<"Lightmapped safety assertion="<<checks<<" failures="<<failures<<'\n';throw std::runtime_error("Lightmapped transaction safety");}++checks;};
    for(auto stage:{Store::Allocation::parameters,Store::Allocation::parameterCommit}){auto balance=heapNew-heapDelete;check(!store.initializeParameters(id,options,[&](Store::Allocation k,size_t){return k!=stage;}));const bool unchanged=snapshot()==before;check(unchanged&&heapNew-heapDelete==balance&&store.get(id)->source.get()==root&&&root->children.front()==first);}
    bool succeeded=false;for(int64_t n=0;n<4096;++n){auto balance=heapNew-heapDelete;failAfter=n;bool ok=store.initializeParameters(id,options);failAfter=-1;if(ok){succeeded=true;break;}++failures;const bool unchanged=snapshot()==before;check(unchanged&&heapNew-heapDelete==balance&&store.get(id)->source.get()==root&&&root->children.front()==first);}
    check(succeeded&&failures>100&&store.get(id)->parameterState->material.shader=="LightmappedGeneric"&&store.get(id)->parameterCount==68);
    const auto&v=store.get(id)->parameterState->material.parameters.values;check(v[6].value.text=="fixture/$albedo"&&v[66].value.text=="paint/splatnormal_default"&&v[67].value.text=="fixture/$envmap");
    check(store.uncacheParameters(id));auto*env=sourceMaterialPatchFindCandidate(*root,"$envmap",false);env->kind=2;env->bits=1;env->text.clear();options.lightmappedSettings.shaderInitSlot5=1;
    const auto unsupported=snapshot();check(!store.initializeParameters(id,options)&&snapshot()==unsupported&&!store.get(id)->initializationPending);
    env->kind=1;env->bits=0;env->text="fixture/env";check(store.initializeParameters(id,options));check(store.forward(id,Store::Operation::decrement,true)&&store.forward(id,Store::Operation::deleteIfUnreferenced,true));store.drain();check(!store.get(id)&&store.size()==0);
    std::cout<<"PASS lightmapped_safety_checks="<<checks<<" actual_allocation_failures="<<failures<<'\n';return 0;
}
#endif
int main(int argc,char**argv){
#ifndef SFM64_NATIVE_OBJECT_PARAMETERS
    if(argc==2&&std::string(argv[1])=="--test-lightmapped")try{return lightmappedSafety();}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
#endif
    return materialObjectParameterMain(argc,argv);
}
