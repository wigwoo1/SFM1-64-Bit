#pragma once
#include <array>
#include <atomic>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "material_dictionary_tree_candidate.h"
#include "source_material_patch_policy_candidate.h"
#include "studio_material_queue_owner_candidate.h"
#include "material_object_parameter_initialization_candidate.h"
#include "material_proxy_lifetime_candidate.h"
#include "material_proxy_evaluation_candidate.h"

// CPU material and opt-in proxy-service ownership. No IMaterial ABI, built-in
// proxy evaluation, GPU/resource initialization or rendering is claimed. Serialized owner
// thread API; atomic reference fields alone do not make the store thread safe.
// IDs never repeat in this store, so a delayed stale call cannot hit a recycled
// address. Queued calls intentionally do NOT retain their material target.
class MaterialObjectStoreCandidate {
public:
    using Id=uintptr_t;
    using Source=std::unique_ptr<SourceMaterialPatchNodeCandidate>;
    enum class Operation {decrement,deleteIfUnreferenced};
    enum class Allocation {symbols,object,registry,tree,functor,append,parameters,parameterCommit,evaluation,evaluationCommit};
    using Permit=std::function<bool(Allocation,size_t)>;
    struct Object;
    struct Forward {const Object*raw=nullptr;bool valuesInitialized=false;std::array<uint32_t,7>values{};};
    struct Object {
        Id id=0;uint16_t nameSymbol=0xffff,groupSymbol=0xffff,flags=0;
        std::string name,group;
        std::atomic<int32_t> references{0};
        Source source;Forward forwarding{this};
        std::array<float,3> initialValues{{0.2f,0.2f,0.2f}};
        // Published CPU parameters remain read-only. Proxy bindings are guarded
        // by a separate lifetime token, never raw pointers into this vector.
        std::unique_ptr<const MaterialObjectParameterStateCandidate>parameterState;
        std::unique_ptr<const MaterialProxyListCandidate>proxyState;
        size_t parameterCount=0;
        bool initializationPending=false;
        bool releasePending=false,removalPending=false;
        bool evaluationPending=false;
    };
    struct Counts {size_t created=0,destroyed=0,sourcesDestroyed=0,unusedSourcesDestroyed=0,referenceWarnings=0,staleCalls=0;};
private:
    MaterialDictionaryTreeCandidate dictionary;
    std::vector<std::string> symbols;
    std::map<Id,std::unique_ptr<Object>> objects;
    StudioMaterialQueueOwnerCandidate queue;
    Id nextId=1;
    Counts counts;
    bool shuttingDown=false;
    Object*mutableObject(Id id){auto i=objects.find(id);return i==objects.end()?nullptr:i->second.get();}
    static bool permitted(const Permit&p,Allocation kind,size_t bytes){return !p||p(kind,bytes);}
    static bool checkedText(const std::string&s){return s.size()<=4096&&std::all_of(s.begin(),s.end(),[](unsigned char c){return c>=32&&c<=126;});}
    static uint16_t intern(std::vector<std::string>&table,const std::string&s){
        const auto i=std::find(table.begin(),table.end(),s);if(i!=table.end())return uint16_t(i-table.begin());
        if(table.size()==0xffff)throw std::length_error("material symbol limit");table.push_back(s);return uint16_t(table.size()-1);
    }
    bool create(const std::string&name,const std::string&group,Source&source,Id&output,bool cached,bool publicReference,bool helper,const Permit&permit){
        if(shuttingDown||!checkedText(name)||!checkedText(group)||nextId==std::numeric_limits<Id>::max())return false;
        try{
            if(!permitted(permit,Allocation::symbols,0))return false;
            auto pendingSymbols=symbols;
            if(helper){
                auto lookup=sourceMaterialPatchLowerCandidate(name);std::replace(lookup.begin(),lookup.end(),'\\','/');
                auto key=intern(pendingSymbols,lookup);auto found=dictionary.find(key,true);
                if(found!=MaterialDictionaryTreeCandidate::none){
                    auto*existing=reinterpret_cast<Object*>(dictionary.slot(found)->entry.material);
                    // Native helper consumes the unused incoming source on a hit.
                    if(source){source.reset();++counts.unusedSourcesDestroyed;}
                    symbols.swap(pendingSymbols);output=existing->id;return true;
                }
            }
            auto normalized=normalizeName(name);auto nameSymbol=intern(pendingSymbols,normalized),groupSymbol=intern(pendingSymbols,group);
            if(!permitted(permit,Allocation::object,sizeof(Object)))return false;
            auto object=std::make_unique<Object>();object->id=nextId;object->nameSymbol=nameSymbol;object->groupSymbol=groupSymbol;
            object->name=std::move(normalized);object->group=group;object->references.store(publicReference?1:0);
            object->flags=uint16_t((source?0x10:0)|(!object->name.empty()&&object->name.front()=='{'?0x20:0));
            auto*raw=object.get();
            if(!permitted(permit,Allocation::registry,sizeof(Object*)))return false;
            objects.emplace(nextId,std::move(object));
            try{
                MaterialDictionaryTreeCandidate::Index index=MaterialDictionaryTreeCandidate::none;
                if(cached&&!dictionary.insert({reinterpret_cast<uintptr_t>(raw),nameSymbol,bool(source)},index,[&](size_t n){return permitted(permit,Allocation::tree,n);})){objects.erase(nextId);return false;}
            }catch(...){objects.erase(nextId);throw;}
            // All potentially failing work precedes this nonthrowing adoption.
            raw->source=std::move(source);symbols.swap(pendingSymbols);output=nextId++;++counts.created;return true;
        }catch(const std::bad_alloc&){return false;}catch(const std::length_error&){return false;}
    }
    static void deferredDecrement(uintptr_t owner,uintptr_t id){auto&s=*reinterpret_cast<MaterialObjectStoreCandidate*>(owner);if(!s.decrement(id))++s.counts.staleCalls;}
    static void deferredDelete(uintptr_t owner,uintptr_t id){auto&s=*reinterpret_cast<MaterialObjectStoreCandidate*>(owner);if(!s.get(id))++s.counts.staleCalls;else s.deleteIfUnreferenced(id);}
public:
    explicit MaterialObjectStoreCandidate(size_t reserve=0x800000,size_t commit=0x40000):queue(reserve,commit){}
    ~MaterialObjectStoreCandidate(){shuttingDown=true;queue.drain();while(!objects.empty())removeAndDelete(objects.begin()->first);}
    MaterialObjectStoreCandidate(const MaterialObjectStoreCandidate&)=delete;
    MaterialObjectStoreCandidate&operator=(const MaterialObjectStoreCandidate&)=delete;
    static std::string normalizeName(std::string s){
        const auto dot=s.find_last_of('.'),slash=s.find_last_of("/\\");
        // Original strip-extension skips a leading dot at index zero.
        if(dot!=std::string::npos&&dot>0&&(slash==std::string::npos||dot>slash))s.resize(dot);
        return sourceMaterialPatchLowerCandidate(std::move(s));
    }
    bool createRaw(const std::string&name,const std::string&group,Source&source,Id&out,const Permit&p={}){return create(name,group,source,out,false,false,false,p);}
    bool createMaterial(const std::string&name,Source&source,Id&out,const Permit&p={}){return create(name,"Other textures",source,out,true,true,false,p);}
    bool createOrReuse(const std::string&name,const std::string&group,Source&source,Id&out,const Permit&p={}){return create(name,group,source,out,true,false,true,p);}
    const Object*get(Id id)const{auto i=objects.find(id);return i==objects.end()?nullptr:i->second.get();}
    const Forward*forwarding(Id id)const{auto*p=get(id);return p?&p->forwarding:nullptr;}
    Id find(const std::string&name,bool suppliedSource){
        if(!checkedText(name))return 0;
        auto key=intern(symbols,name);auto i=dictionary.find(key,suppliedSource);
        return i==MaterialDictionaryTreeCandidate::none?0:reinterpret_cast<Object*>(dictionary.slot(i)->entry.material)->id;
    }
    std::vector<Id>cached()const{std::vector<Id>ids;for(auto i=dictionary.first();i!=MaterialDictionaryTreeCandidate::none;i=dictionary.next(i))ids.push_back(reinterpret_cast<Object*>(dictionary.slot(i)->entry.material)->id);return ids;}
    bool increment(Id id){auto*p=mutableObject(id);if(!p)return false;p->references.fetch_add(1);return true;}
    bool decrement(Id id){auto*p=mutableObject(id);if(!p)return false;p->references.fetch_sub(1);return true;}
    bool removeOnly(Id id){auto*p=get(id);if(!p)return false;dictionary.eraseMaterial(reinterpret_cast<uintptr_t>(p));return true;}
    bool removeAndDelete(Id id){
        auto i=objects.find(id);if(i==objects.end()||i->second->removalPending||i->second->evaluationPending)return false;auto&p=*i->second;p.removalPending=true;
        dictionary.eraseMaterial(reinterpret_cast<uintptr_t>(&p));
        if(p.references.load()!=0)++counts.referenceWarnings;
        uncacheParameters(id);
        if(p.source){p.source.reset();++counts.sourcesDestroyed;}
        objects.erase(i);++counts.destroyed;return true;
    }
    bool deleteIfUnreferenced(Id id){auto*p=get(id);return p&&p->references.load()<=0?removeAndDelete(id):false;}
    bool initializeParameters(Id id,const MaterialObjectParameterOptionsCandidate&options={},const Permit&permit={},MaterialObjectParameterAttemptCandidate*attempt=nullptr){
        if(attempt)*attempt={};
        auto*p=mutableObject(id);if(!p)return false;
        if(shuttingDown||p->removalPending||p->releasePending||p->evaluationPending)return false;
        if(p->flags&4){if(attempt)attempt->status=MaterialObjectParameterStatusCandidate::AlreadyInitialized;return true;} // Original guard.
        if(p->initializationPending)return false;
        try{
            if(!permitted(permit,Allocation::parameters,0))return false;
            if(mutableObject(id)!=p)return false;
            const auto name=p->name;const auto flags=p->flags;auto*root=p->source?p->source.get():options.externalSource;
            if(root){size_t nodes=0;sourceMaterialPatchFileTreeValidateCandidate(*root,0,nodes,1023);if(nodes>4096)return false;}
            SourceMaterialPatchNodeCandidate snapshot;if(root)snapshot=*root;
            auto*patch=(!p->source&&options.externalSource)?options.externalPatch:nullptr;
            SourceMaterialPatchNodeCandidate patchSnapshot;if(patch)patchSnapshot=*patch;
            auto input=options;if(input.externalSource)input.externalSource=&snapshot;
            input.externalPatch=patch?&patchSnapshot:nullptr;
            const bool stored=bool(p->source);auto*original=p;p->initializationPending=true;
            auto reset=[&](Object*){auto*current=mutableObject(id);if(current==original)current->initializationPending=false;};
            std::unique_ptr<Object,decltype(reset)>pendingGuard(original,reset);
            MaterialObjectParameterPlanCandidate plan;
            if(!materialObjectParameterPrepareCandidate(plan,name,flags,stored?&snapshot:nullptr,input)){if(attempt)*attempt=plan.attempt;return false;}
            std::unique_ptr<MaterialProxyListCandidate>proxies;
            if(input.proxyFactory){
                auto*selected=materialProxySelectedNodeCandidate(plan.finalRoot,plan.state->material.selectedNode);if(!selected)return false;
                try{proxies=MaterialProxyListCandidate::prepare(*plan.state,id,name,*selected,plan.finalRoot,input.proxyFactory);}
                catch(...){return false;} // A service exception rolls back its staged list.
                if(proxies&&!proxies->complete()){
                    plan.attempt.status=MaterialObjectParameterStatusCandidate::Refused;plan.attempt.warnings+=proxies->warningCount();
                    if(attempt)*attempt=plan.attempt;return false;
                }
            }
            if(!permitted(permit,Allocation::parameterCommit,0))return false;
            // Read services may reenter application code. Never commit through
            // an owner deleted/replaced during a callback or over a changed root.
            p=mutableObject(id);if(p!=original||!p||(p->flags&4)||(p->source?p->source.get():options.externalSource)!=root)return false;
            if(root&&!materialObjectParameterTreeEqualCandidate(*root,snapshot))return false;
            if(patch&&!materialObjectParameterTreeEqualCandidate(*patch,patchSnapshot))return false;
            static_assert(std::is_nothrow_move_assignable<SourceMaterialPatchNodeCandidate>::value,"Root publication must not throw");
            // Keep root identity. Replace child/value storage only if fallback
            // actually changed it; plain InitParams preserves borrowed children.
            if(root&&!materialObjectParameterTreeEqualCandidate(*root,plan.finalRoot))*root=std::move(plan.finalRoot);
            if(patch&&plan.updateExternalPatch)*patch=std::move(plan.finalExternalPatch);
            p->parameterCount=plan.state->material.parameters.values.size();p->parameterState=std::move(plan.state);p->proxyState=std::move(proxies);p->flags|=4;
            p->forwarding.values=plan.forwardValues;p->forwarding.valuesInitialized=true;if(attempt)*attempt=plan.attempt;return true;
        }catch(const std::bad_alloc&){return false;}catch(const std::runtime_error&){return false;}
    }
    bool uncacheParameters(Id id,bool preserveVariables=false){
        auto*p=mutableObject(id);if(!p)return false;
        if(p->releasePending||p->evaluationPending)return false;
        if(!preserveVariables){
            p->releasePending=true;auto*original=p;
            auto reset=[&](Object*){auto*current=mutableObject(id);if(current==original)current->releasePending=false;};
            std::unique_ptr<Object,decltype(reset)>releaseGuard(original,reset);
            auto proxies=std::move(p->proxyState);if(proxies)proxies->retire();
            p->parameterState.reset();p->parameterCount=0;p->flags&=uint16_t(0xfffb);
            // Release callbacks may delete this material. Do not dereference p
            // after releasing the detached list; the guard re-fetches by ID.
            proxies.reset();
        }
        // Original uncache leaves the forwarding value snapshot untouched.
        return true;
    }
    bool evaluateProxies(Id id,const Permit&permit={},MaterialProxyEvaluationAttemptCandidate*attempt=nullptr,const MaterialProxyEvaluationInputsCandidate&inputs={}){
        if(attempt){attempt->evaluated=false;attempt->writes.clear();}
        auto*p=mutableObject(id);if(!p||shuttingDown||p->initializationPending||p->releasePending||p->removalPending||p->evaluationPending||!p->parameterState||!(p->flags&4))return false;
        if(!p->proxyState){if(attempt)attempt->evaluated=true;return true;}
        auto*original=p;p->evaluationPending=true;
        auto reset=[&](Object*){auto*current=mutableObject(id);if(current==original)current->evaluationPending=false;};
        std::unique_ptr<Object,decltype(reset)>guard(original,reset);
        try{
            // Freeze per-draw inputs before any caller-owned policy/service
            // callback can mutate its source. The frame owns another copy.
            const auto inputSnapshot=inputs;
            if(!permitted(permit,Allocation::evaluation,0))return false;
            const auto*before=p->parameterState.get();auto*list=p->proxyState.get();auto*root=p->source.get();SourceMaterialPatchNodeCandidate source;if(root)source=*root;
            auto plan=std::make_unique<MaterialObjectParameterStateCandidate>(*before);MaterialProxyEvaluationCandidate frame(*list,*plan,inputSnapshot);
            if(!list->evaluate(frame))return false;
            std::array<uint32_t,7>values{};if(!materialObjectParameterForwardCandidate(values,*plan)||!permitted(permit,Allocation::evaluationCommit,0))return false;
            p=mutableObject(id);if(p!=original||p->parameterState.get()!=before||p->proxyState.get()!=list||p->source.get()!=root||(root&&!materialObjectParameterTreeEqualCandidate(*root,source)))return false;
            auto retired=std::move(p->parameterState);p->parameterState=std::move(plan);list->retarget(p->parameterState.get());p->forwarding.values=values;
            if(attempt){attempt->writes.swap(frame.journal);attempt->evaluated=true;}return true;
        }catch(...){return false;}
    }
    // Null-queue flag applies immediately. Safe allocation/append failure is an
    // owned policy (the native null-allocation enqueue was unsafe).
    bool forward(Id id,Operation op,bool queued,const Permit&permit={}){
        if(!get(id))return false;
        if(!queued){if(op==Operation::decrement)decrement(id);else deleteIfUnreferenced(id);return true;}
        try{
            if(!permitted(permit,Allocation::functor,sizeof(StudioDecalQueuedCallCandidate)))return false;
            auto call=std::make_unique<StudioDecalQueuedCallCandidate>();call->owner=reinterpret_cast<uintptr_t>(this);call->argument=id;
            call->method=op==Operation::decrement?&deferredDecrement:&deferredDelete;
            if(!permitted(permit,Allocation::append,0))return false;
            queue.enqueue(call.get());call.release();return true;
        }catch(const std::bad_alloc&){return false;}
    }
    void drain(){queue.drain();}
    size_t queuedCount()const{return queue.queuedCount();}
    const StudioMaterialQueueCandidate&queueState()const{return queue.queueState();}
    size_t size()const{return objects.size();}
    size_t symbolCount()const{return symbols.size();}
    const Counts&statistics()const{return counts;}
    bool valid()const{return dictionary.valid();}
};
