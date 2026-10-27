#define SFM64_OBJECT_PARAMETER_MAIN matrixWritePriorParameterMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#include "material_matrix_value_write_candidate.h"
static int matrixStoreSafety(){
    using Store=MaterialObjectStoreCandidate;unsigned checks=0;auto check=[&](bool value){if(!value)throw std::runtime_error("Matrix store safety check="+std::to_string(checks));++checks;};
    struct Factory:MaterialProxyFactoryCandidate {
        struct Instance:MaterialProxyInstanceCandidate {
            Factory&f;MaterialProxyBindingCandidate target;
            explicit Instance(Factory&value):f(value){}
            bool initialize(const MaterialProxyContextCandidate&context,const SourceMaterialPatchNodeCandidate&)override{target=context.find("$m");f.binding=target;return target.valid();}
            bool evaluate(MaterialProxyEvaluationCandidate&frame)override{
                if(!frame.setMatrix(target,f.matrix))return false;
                if(f.mode==1&&!frame.setFloat(target,0x3f400000))return false;
                if(f.mode==2){auto copy=frame.readMatrix(target);if(!copy||!frame.setMatrix(target,*copy))return false;}
                return !f.hook||f.hook(frame);
            }
        };
        MaterialMatrixMathCandidate matrix=materialMatrixTranslateCandidate(0x3e800000,0xbf000000,0x3f400000);
        unsigned mode=0;MaterialProxyBindingCandidate binding;std::function<bool(MaterialProxyEvaluationCandidate&)>hook;
        MaterialProxyInstanceCandidate*create(const std::string&name)override{return name=="MatrixWriteProbe"?new Instance(*this):nullptr;}
        void destroy(MaterialProxyInstanceCandidate*p)noexcept override{delete p;}
    };
    auto root=[](){SourceMaterialPatchNodeCandidate r;r.key="UnlitTwoTexture";r.children={{"$m",1,"[1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1]"},{"$out",1,"long preexisting scalar string"}};SourceMaterialPatchNodeCandidate declaration;declaration.key="MatrixWriteProbe";SourceMaterialPatchNodeCandidate proxies;proxies.key="Proxies";proxies.children.push_back(declaration);r.children.push_back(proxies);return r;};
    Store store;auto f=std::make_shared<Factory>();Store::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>(root());auto*storedRoot=source.get();auto beforeRoot=*storedRoot;uintptr_t id=0;check(store.createMaterial("matrix/safety",source,id));MaterialObjectParameterOptionsCandidate options;options.proxyFactory=f;check(store.initializeParameters(id,options));
    auto snapshot=[&](){std::ostringstream out;putValues(out,store.get(id)->parameterState->material);for(auto v:store.get(id)->forwarding.values)put(out,v);return out.str();};const auto before=snapshot();auto*state=store.get(id)->parameterState.get();MaterialProxyEvaluationAttemptCandidate attempt;
    f->hook=[&](auto&frame){auto staged=frame.readMatrix(f->binding);auto committed=f->binding.read();check(staged&&*staged==f->matrix&&committed&&committed->matrix&&*committed->matrix!=f->matrix);check(!store.removeAndDelete(id)&&!store.uncacheParameters(id)&&!store.evaluateProxies(id));return false;};check(!store.evaluateProxies(id,{},&attempt)&&snapshot()==before&&attempt.writes.empty()&&!attempt.evaluated);f->hook={};
    check(!store.evaluateProxies(id,[](Store::Allocation a,size_t){return a!=Store::Allocation::evaluationCommit;},&attempt)&&snapshot()==before&&store.get(id)->parameterState.get()==state);
    unsigned failures=0;bool success=false;
    for(int64_t n=0;n<2048;++n){const auto balance=heapNew-heapDelete;failAfter=n;const bool ok=store.evaluateProxies(id,{},&attempt);failAfter=-1;if(ok){success=true;break;}++failures;check(heapNew-heapDelete==balance&&snapshot()==before&&store.get(id)->parameterState.get()==state&&attempt.writes.empty()&&!attempt.evaluated&&!store.get(id)->evaluationPending&&materialObjectParameterTreeEqualCandidate(*storedRoot,beforeRoot));}
    check(success&&failures>0&&attempt.evaluated&&attempt.writes.size()==1);check(attempt.writes[0].operation==9&&attempt.writes[0].count==16&&attempt.writes[0].matrix&&*attempt.writes[0].matrix==f->matrix&&attempt.writes[0].arguments==std::array<uint32_t,4>{});auto expected=f->matrix;f->matrix[0]^=1;check(f->binding.read()->matrix==expected&&attempt.writes[0].matrix==expected);f->matrix=expected;
    f->mode=1;check(store.evaluateProxies(id,{},&attempt)&&attempt.writes.size()==2&&attempt.writes[0].matrix&&!attempt.writes[1].matrix&&attempt.writes[1].operation==1);auto numeric=f->binding.read();check(numeric&&!numeric->matrix&&!numeric->matrixIdentity&&(numeric->value.numeric.typeAndMetadata&15u)==0&&numeric->value.numeric.vector[0]==0x3f400000);
    f->mode=2;check(store.evaluateProxies(id,{},&attempt)&&attempt.writes.size()==2&&attempt.writes[0].matrix==expected&&attempt.writes[1].matrix==expected);auto binding=f->binding;check(store.uncacheParameters(id)&&!binding.valid()&&store.initializeParameters(id,options));f->hook=[&](auto&frame){check(!frame.readMatrix(binding)&&!frame.setMatrix(binding,expected));return true;};check(store.evaluateProxies(id));f->hook={};check(store.removeAndDelete(id)&&!f->binding.valid());
    MaterialTypedDispatchCandidate alias;alias.created=true;alias.name="$alias";alias.value.numeric.typeAndMetadata=7;alias.matrix=expected;check(materialTypedMatrixSetCandidate(alias,*alias.matrix)&&alias.matrix==expected);const auto saved=alias;check(!materialTypedNumericWriteCandidate(alias,99)&&alias.matrix==saved.matrix&&alias.value.numeric.typeAndMetadata==saved.value.numeric.typeAndMetadata);alias.value.numeric.typeAndMetadata=3;check(!materialTypedMatrixSetCandidate(alias,expected));alias.value.numeric.typeAndMetadata=7;alias.matrix.reset();check(!materialTypedMatrixSetCandidate(alias,expected)&&!materialTypedMatrixReadCandidate(alias));
    std::cout<<"PASS matrix_store_safety_checks="<<checks<<" actual_allocation_failures="<<failures<<'\n';return 0;
}
int main(){try{return matrixStoreSafety();}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
