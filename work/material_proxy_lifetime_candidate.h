#pragma once
#include "material_object_parameter_initialization_candidate.h"

class MaterialObjectStoreCandidate;
class MaterialProxyListCandidate;
class MaterialProxyContextCandidate;
class MaterialProxyEvaluationCandidate;
// Owner-thread only. A handle never exposes a pointer into a material's vector.
// Its control block is retired before parameters are freed or a plan rolls back.
class MaterialProxyBindingCandidate {
    struct State {
        const MaterialObjectParameterStateCandidate*parameters=nullptr;
        uintptr_t owner=0;
        std::string name;
    };
    std::weak_ptr<State>state;
    size_t index=0;
    friend class MaterialProxyContextCandidate;
    friend class MaterialProxyListCandidate;
    friend class MaterialProxyEvaluationCandidate;
public:
    std::optional<MaterialTypedDispatchCandidate>read()const{
        auto s=state.lock();if(!s||!s->parameters)return {};
        const auto&v=s->parameters->material.parameters.values;
        if(index>=v.size())return {};return v[index];
    }
    bool valid()const noexcept{auto s=state.lock();return s&&s->parameters&&index<s->parameters->material.parameters.values.size();}
};
class MaterialProxyContextCandidate {
    std::weak_ptr<MaterialProxyBindingCandidate::State>state;
    explicit MaterialProxyContextCandidate(const std::shared_ptr<MaterialProxyBindingCandidate::State>&s):state(s){}
    friend class MaterialProxyListCandidate;
public:
    MaterialProxyContextCandidate()=default;
    uintptr_t owner()const noexcept{auto s=state.lock();return s&&s->parameters?s->owner:0;}
    std::string name()const{auto s=state.lock();return s&&s->parameters?s->name:std::string();}
    MaterialProxyBindingCandidate find(const std::string&name)const{
        MaterialProxyBindingCandidate result;auto s=state.lock();if(!s||!s->parameters)return result;
        const auto key=sourceMaterialPatchLowerCandidate(name);
        const auto&v=s->parameters->material.parameters.values;
        // Measured FindVar searches backwards; no trimming, '$' insertion,
        // component suffix parsing, or dummy-variable creation is performed.
        for(size_t i=v.size();i>0;--i)if(sourceMaterialPatchLowerCandidate(v[i-1].name)==key){result.state=s;result.index=i-1;break;}
        return result;
    }
};
struct MaterialProxyInstanceCandidate {
    virtual bool initialize(const MaterialProxyContextCandidate&,const SourceMaterialPatchNodeCandidate&)=0;
    virtual bool evaluate(MaterialProxyEvaluationCandidate&){return false;}
    virtual ~MaterialProxyInstanceCandidate()=default;
};
// Explicit opt-in service boundary, not a registry of implemented SFM proxies.
// Destroy must be noexcept and use the same factory that created the instance.
struct MaterialProxyFactoryCandidate {
    virtual MaterialProxyInstanceCandidate*create(const std::string&)=0;
    virtual void destroy(MaterialProxyInstanceCandidate*)noexcept=0;
    virtual ~MaterialProxyFactoryCandidate()=default;
};
class MaterialProxyListCandidate {
    std::shared_ptr<MaterialProxyFactoryCandidate>factory;
    std::shared_ptr<MaterialProxyBindingCandidate::State>state;
    std::vector<MaterialProxyInstanceCandidate*>instances;
    uint32_t failures=0;
    bool overflow=false;
    friend class MaterialObjectStoreCandidate;
    friend class MaterialProxyEvaluationCandidate;
    void retire()const noexcept{if(state)state->parameters=nullptr;}
    void retarget(const MaterialObjectParameterStateCandidate*p)const noexcept{if(state)state->parameters=p;}
    bool evaluate(MaterialProxyEvaluationCandidate&frame)const{for(auto*p:instances)if(!p->evaluate(frame))return false;return true;}
public:
    MaterialProxyListCandidate()=default;
    MaterialProxyListCandidate(const MaterialProxyListCandidate&)=delete;
    MaterialProxyListCandidate&operator=(const MaterialProxyListCandidate&)=delete;
    ~MaterialProxyListCandidate(){retire();for(auto i=instances.rbegin();i!=instances.rend();++i)factory->destroy(*i);}
    size_t size()const noexcept{return instances.size();}
    uint32_t warningCount()const noexcept{return failures;}
    bool complete()const noexcept{return !failures&&!overflow;}
private:
    static std::unique_ptr<MaterialProxyListCandidate>prepare(
            const MaterialObjectParameterStateCandidate&parameters,uintptr_t owner,const std::string&name,
            const SourceMaterialPatchNodeCandidate&selected,const SourceMaterialPatchNodeCandidate&root,
            std::shared_ptr<MaterialProxyFactoryCandidate>factory){
        // Original selected override has a KeyValues chain to the final root.
        // A missing local key falls back; an empty/scalar local key hides root.
        const auto*proxies=sourceMaterialPatchFindConstCandidate(selected,"Proxies");
        if(!proxies&&&selected!=&root)proxies=sourceMaterialPatchFindConstCandidate(root,"Proxies");
        if(!proxies||proxies->children.empty())return {};
        auto result=std::make_unique<MaterialProxyListCandidate>();result->factory=std::move(factory);
        // Native256 successes wrap its byte count to0 and leak its array.
        // The owned contract refuses >255 declarations before factory calls.
        if(!result->factory||proxies->children.size()>255){result->overflow=true;return result;}
        result->instances.reserve(proxies->children.size());
        result->state=std::make_shared<MaterialProxyBindingCandidate::State>();
        result->state->parameters=&parameters;result->state->owner=owner;result->state->name=name;
        const MaterialProxyContextCandidate context(result->state);
        for(const auto&declaration:proxies->children){
            auto*p=result->factory->create(declaration.key);
            if(!p){++result->failures;continue;}
            // Capacity was reserved before callbacks. An exception from Init
            // leaves this instance owned by the reverse-order rollback list.
            result->instances.push_back(p);
            if(!p->initialize(context,declaration)){
                result->instances.pop_back();result->factory->destroy(p);++result->failures;
            }
        }
        return result;
    }
};
inline const SourceMaterialPatchNodeCandidate*materialProxySelectedNodeCandidate(
        const SourceMaterialPatchNodeCandidate&root,uint32_t index){
    uint32_t next=0;const SourceMaterialPatchNodeCandidate*found=nullptr;
    auto visit=[&](auto&&self,const SourceMaterialPatchNodeCandidate&n)->void{
        if(next++==index)found=&n;for(const auto&c:n.children)self(self,c);
    };visit(visit,root);return found;
}
