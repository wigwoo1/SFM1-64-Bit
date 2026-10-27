#pragma once
#include <array>
#include <atomic>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "material_dictionary_tree_candidate.h"
#include "source_material_patch_policy_candidate.h"
#include "studio_material_queue_owner_candidate.h"
#include "material_object_parameter_initialization_candidate.h"
#include "material_proxy_lifetime_candidate.h"
#include "material_proxy_evaluation_candidate.h"
#include "material_parameter_override_candidate.h"

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
    enum class Allocation {symbols,object,registry,tree,functor,append,parameters,parameterCommit,evaluation,evaluationCommit,sessionSnapshot,sessionApply,sessionRestore,sessionPublish,sessionProxyEvaluate,sessionDrawSnapshot};
    using Permit=std::function<bool(Allocation,size_t)>;
    // Caller supplies already-resolved, retained owned IDs. Nullopt is an
    // unavailable binding (refuse); engaged0 is an explicit skipped position.
    // These rows are typed value templates, not full DataModel element clones.
    struct SessionEntry {std::optional<Id>material;std::vector<MaterialSessionOverrideInputCandidate>rows;};
    struct SessionTargetView {Id material=0;const MaterialObjectParameterStateCandidate*parameters=nullptr;};
    struct SessionCollectionView {
        std::vector<SessionTargetView>targets; // unique IDs, first occurrence order
        std::vector<std::optional<size_t>>sourceTargets; // one per original position
        std::vector<std::vector<MaterialSessionOverrideInputCandidate>>backups; // independent per-position templates
    };
    // Explicit CPU bind sequence, not a frame/clock/selection scheduler. Caller
    // supplies retained actual IDs and verified per-bind inputs; repeated IDs
    // evaluate repeatedly. No time is inferred from session frames or wall time.
    struct SessionDrawStep {Id material=0;MaterialProxyEvaluationInputsCandidate inputs;};
    struct SessionDrawStepView {
        Id material=0;
        const MaterialObjectParameterStateCandidate*parameters=nullptr;
        const std::vector<MaterialProxyWriteCandidate>*writes=nullptr;
        std::array<uint32_t,7>forwardValues{};
    };
    struct SessionDrawView {
        const SessionCollectionView&collection;
        const std::vector<SessionDrawStepView>&steps;
    };
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
        bool sessionPending=false;
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
    size_t sessionScopes=0;
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
    // Filename-backed category0 cache object, initially unreferenced. Unlike
    // createOrReuse, this cannot accidentally reuse a supplied-source category1.
    bool createFileMaterial(const std::string&name,const std::string&group,Id&out,const Permit&p={}){Source absent;return create(name,group,absent,out,true,false,false,p);}
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
    bool increment(Id id){auto*p=mutableObject(id);if(!p||p->sessionPending)return false;p->references.fetch_add(1);return true;}
    bool decrement(Id id){auto*p=mutableObject(id);if(!p||p->sessionPending)return false;p->references.fetch_sub(1);return true;}
    bool removeOnly(Id id){auto*p=get(id);if(!p||p->sessionPending)return false;dictionary.eraseMaterial(reinterpret_cast<uintptr_t>(p));return true;}
    bool removeAndDelete(Id id){
        auto i=objects.find(id);if(i==objects.end()||i->second->removalPending||i->second->evaluationPending||i->second->sessionPending)return false;auto&p=*i->second;p.removalPending=true;
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
        if(shuttingDown||p->removalPending||p->releasePending||p->evaluationPending||p->sessionPending)return false;
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
                if(proxies){
                    plan.attempt.warnings+=proxies->warningCount();
                    if(!proxies->complete()){
                        plan.attempt.status=MaterialObjectParameterStatusCandidate::Refused;
                        if(attempt)*attempt=plan.attempt;return false;
                    }
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
        if(p->releasePending||p->evaluationPending||p->sessionPending)return false;
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
        auto*p=mutableObject(id);if(!p||shuttingDown||p->initializationPending||p->releasePending||p->removalPending||p->evaluationPending||p->sessionPending||!p->parameterState||!(p->flags&4))return false;
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
    // Owner-thread, single-material, non-rendering pilot. The observer borrows
    // the applied state only for the call; it must keep this store alive.
    // Failed preparation/observer restores the ORIGINAL pointer/forward values.
    // Success publishes NATIVE-TYPED restore setters, not exact original types.
    // No resource handles, proxy evaluation, document undo or multi-material
    // atomicity are implied. Arbitrary external source edits are not undone.
    bool withSessionOverrides(Id id,const std::vector<MaterialSessionOverrideInputCandidate>&rows,
            const std::function<bool(const MaterialObjectParameterStateCandidate&)>&observer,int phase=0,const Permit&permit={}){
        auto*p=mutableObject(id);
        if(!p||shuttingDown||!observer||!p->parameterState||!(p->flags&4)||p->initializationPending||p->releasePending||p->removalPending||p->evaluationPending||p->sessionPending)return false;
        auto*original=p;p->sessionPending=true;++sessionScopes;
        auto reset=[&](Object*)noexcept{auto*current=mutableObject(id);if(current==original)current->sessionPending=false;--sessionScopes;};
        std::unique_ptr<Object,decltype(reset)>pending(original,reset);
        try{
            const auto*before=p->parameterState.get();auto*root=p->source.get();
            const auto flags=p->flags;const auto count=p->parameterCount;const auto refs=p->references.load();
            const auto oldForward=p->forwarding.values;const auto oldInitialized=p->forwarding.valuesInitialized;
            SourceMaterialPatchNodeCandidate source;if(root)source=*root;
            const auto input=rows;const auto use=observer;const auto policy=permit;
            auto valid=[&](const MaterialObjectParameterStateCandidate*expected){return mutableObject(id)==original&&p->parameterState.get()==expected&&p->source.get()==root&&p->flags==flags&&p->parameterCount==count&&p->references.load()==refs&&(!root||materialObjectParameterTreeEqualCandidate(*root,source));};
            if(!permitted(policy,Allocation::sessionSnapshot,0)||!valid(before))return false;
            auto backup=input;if(!captureMaterialParameterBackupCandidate(backup,before->material.parameters,phase))return false;
            if(!permitted(policy,Allocation::sessionApply,0)||!valid(before))return false;
            auto applied=std::make_unique<MaterialObjectParameterStateCandidate>(*before);
            if(!applyMaterialParameterOverridesCandidate(applied->material.parameters,input))return false;
            if(!permitted(policy,Allocation::sessionRestore,0)||!valid(before))return false;
            auto restored=std::make_unique<MaterialObjectParameterStateCandidate>(*applied);
            if(!applyMaterialParameterOverridesCandidate(restored->material.parameters,backup))return false;
            std::array<uint32_t,7>appliedForward{},restoredForward{};
            if(!materialObjectParameterForwardCandidate(appliedForward,*applied)||!materialObjectParameterForwardCandidate(restoredForward,*restored))return false;
            if(!permitted(policy,Allocation::sessionPublish,0)||!valid(before))return false;
            auto originalState=std::move(p->parameterState);bool rollback=true;
            auto restore=[&](Object*)noexcept{if(rollback){p->parameterState.swap(originalState);if(p->proxyState)p->proxyState->retarget(p->parameterState.get());p->forwarding.values=oldForward;p->forwarding.valuesInitialized=oldInitialized;}};
            std::unique_ptr<Object,decltype(restore)>publication(original,restore);
            p->parameterState=std::move(applied);if(p->proxyState)p->proxyState->retarget(p->parameterState.get());p->forwarding.values=appliedForward;p->forwarding.valuesInitialized=true;
            const auto*appliedState=p->parameterState.get();
            if(!use(*appliedState)||!valid(appliedState))return false;
            auto transient=std::move(p->parameterState);p->parameterState=std::move(restored);if(p->proxyState)p->proxyState->retarget(p->parameterState.get());p->forwarding.values=restoredForward;p->forwarding.valuesInitialized=true;
            rollback=false;return true;
        }catch(...){return false;}
    }
    // Owner-thread, borrowed observation only. Source entry order is NEVER
    // deduplicated: aliases capture after preceding applies. Only ownership and
    // failure snapshots are unique per ID. Success replays backups in that same
    // order, unlike exact failure rollback. External resource acquisition and
    // arbitrary shared backup-element aliases are not implemented by this API.
    bool withSessionCollection(const std::vector<SessionEntry>&entries,
            const std::function<bool(const SessionCollectionView&)>&observer,int phase=0,const Permit&permit={}){
        return withSessionCollectionImpl(entries,observer,phase,permit,nullptr,nullptr);
    }
    // Prepare override -> ordered CPU proxy binds -> per-bind immutable value
    // snapshots -> ordered typed backup replay. The observer must consume each
    // step snapshot, not read the live store for earlier binds. All potentially
    // failing preparation precedes publication. Failure restores original owned
    // parameter pointers/forwarding; proxy-service external side effects are not
    // a global transaction. Not renderer admission, GPU rollback or queue lifetime.
    bool withSessionDrawSteps(const std::vector<SessionEntry>&entries,const std::vector<SessionDrawStep>&steps,
            const std::function<bool(const SessionDrawView&)>&observer,int phase=0,const Permit&permit={}){
        return withSessionCollectionImpl(entries,{},phase,permit,&steps,&observer);
    }
private:
    bool withSessionCollectionImpl(const std::vector<SessionEntry>&entries,
            const std::function<bool(const SessionCollectionView&)>&observer,int phase,const Permit&permit,
            const std::vector<SessionDrawStep>*steps,const std::function<bool(const SessionDrawView&)>*drawObserver){
        if(shuttingDown||(steps?(!drawObserver||!*drawObserver):!observer)||entries.size()>4096||(steps&&steps->size()>4096))return false;
        struct Plan {
            Id id=0;Object*object=nullptr;const MaterialObjectParameterStateCandidate*before=nullptr,*appliedPointer=nullptr;
            SourceMaterialPatchNodeCandidate*root=nullptr;SourceMaterialPatchNodeCandidate source;
            uint16_t flags=0;size_t count=0;int32_t refs=0;bool initialized=false,published=false;
            std::array<uint32_t,7>oldForward{},appliedForward{},restoredForward{};
            std::unique_ptr<MaterialObjectParameterStateCandidate>applied,restored;
            std::unique_ptr<const MaterialObjectParameterStateCandidate>original,transient;
            const MaterialProxyListCandidate*list=nullptr;
        };
        try{
            const auto drawInput=steps?*steps:std::vector<SessionDrawStep>{};
            std::vector<Plan>plans;plans.reserve(entries.size()+drawInput.size());std::map<Id,size_t>indices;SessionCollectionView view;
            view.sourceTargets.resize(entries.size());view.backups.resize(entries.size());
            for(size_t i=0;i<entries.size();++i){
                if(!entries[i].material)return false;const auto id=*entries[i].material;if(!id)continue;
                auto found=indices.find(id);
                if(found!=indices.end()){view.sourceTargets[i]=found->second;continue;}
                auto*p=mutableObject(id);if(!p||!p->parameterState||!(p->flags&4)||p->initializationPending||p->releasePending||p->removalPending||p->evaluationPending||p->sessionPending)return false;
                Plan plan;plan.id=id;plan.object=p;plan.before=p->parameterState.get();plan.root=p->source.get();plan.flags=p->flags;plan.count=p->parameterCount;plan.refs=p->references.load();plan.initialized=p->forwarding.valuesInitialized;plan.oldForward=p->forwarding.values;plan.list=p->proxyState.get();
                const auto index=plans.size();plans.push_back(std::move(plan));indices.emplace(id,index);view.sourceTargets[i]=index;
            }
            for(const auto&step:drawInput){
                const auto id=step.material;if(!id)return false;if(indices.count(id))continue;
                auto*p=mutableObject(id);if(!p||!p->parameterState||!(p->flags&4)||p->initializationPending||p->releasePending||p->removalPending||p->evaluationPending||p->sessionPending)return false;
                Plan plan;plan.id=id;plan.object=p;plan.before=p->parameterState.get();plan.root=p->source.get();plan.flags=p->flags;plan.count=p->parameterCount;plan.refs=p->references.load();plan.initialized=p->forwarding.valuesInitialized;plan.oldForward=p->forwarding.values;plan.list=p->proxyState.get();
                const auto index=plans.size();plans.push_back(std::move(plan));indices.emplace(id,index);
            }
            for(auto&plan:plans)plan.object->sessionPending=true;++sessionScopes;
            auto release=[&](MaterialObjectStoreCandidate*)noexcept{for(auto&plan:plans)if(mutableObject(plan.id)==plan.object)plan.object->sessionPending=false;--sessionScopes;};
            std::unique_ptr<MaterialObjectStoreCandidate,decltype(release)>busy(this,release);
            for(auto&plan:plans)if(plan.root)plan.source=*plan.root;
            const auto input=entries;const auto use=observer;const auto policy=permit;
            std::function<bool(const SessionDrawView&)>drawUse;if(steps)drawUse=*drawObserver;
            auto valid=[&](bool published){for(const auto&plan:plans){auto*p=plan.object;if(mutableObject(plan.id)!=p||p->parameterState.get()!=(published?plan.appliedPointer:plan.before)||p->source.get()!=plan.root||p->flags!=plan.flags||p->parameterCount!=plan.count||p->references.load()!=plan.refs||(steps&&p->proxyState.get()!=plan.list)||(plan.root&&!materialObjectParameterTreeEqualCandidate(*plan.root,plan.source)))return false;}return true;};
            if(!permitted(policy,Allocation::sessionSnapshot,plans.size())||!valid(false))return false;
            for(auto&plan:plans)plan.applied=std::make_unique<MaterialObjectParameterStateCandidate>(*plan.before);
            for(size_t i=0;i<input.size();++i){
                if(!view.sourceTargets[i])continue;auto&plan=plans[*view.sourceTargets[i]];
                if(!permitted(policy,Allocation::sessionApply,i)||!valid(false))return false;
                view.backups[i]=input[i].rows;
                if(!captureMaterialParameterBackupCandidate(view.backups[i],plan.applied->material.parameters,phase)||!applyMaterialParameterOverridesCandidate(plan.applied->material.parameters,input[i].rows))return false;
            }
            std::vector<std::unique_ptr<const MaterialObjectParameterStateCandidate>>drawStates;
            std::vector<std::vector<MaterialProxyWriteCandidate>>drawWrites;
            std::vector<SessionDrawStepView>drawViews;
            drawStates.reserve(drawInput.size());drawWrites.reserve(drawInput.size());drawViews.reserve(drawInput.size());
            for(size_t i=0;i<drawInput.size();++i){
                auto&plan=plans[indices.at(drawInput[i].material)];
                if(!permitted(policy,Allocation::sessionProxyEvaluate,i)||!valid(false))return false;
                std::vector<MaterialProxyWriteCandidate>journal;
                if(plan.list){MaterialProxyEvaluationCandidate frame(*plan.list,*plan.applied,drawInput[i].inputs);if(!plan.list->evaluate(frame))return false;journal.swap(frame.journal);}
                std::array<uint32_t,7>forward{};
                if(!materialObjectParameterForwardCandidate(forward,*plan.applied)||!permitted(policy,Allocation::sessionDrawSnapshot,i)||!valid(false))return false;
                drawStates.push_back(std::make_unique<MaterialObjectParameterStateCandidate>(*plan.applied));drawWrites.push_back(std::move(journal));
                drawViews.push_back({plan.id,drawStates.back().get(),&drawWrites.back(),forward});
            }
            // Replay only captured typed rows onto POST-PROXY state. Unrelated
            // proxy writes survive success, while failure still uses originals.
            for(auto&plan:plans)plan.restored=std::make_unique<MaterialObjectParameterStateCandidate>(*plan.applied);
            for(size_t i=0;i<input.size();++i){
                if(!view.sourceTargets[i])continue;
                if(!permitted(policy,Allocation::sessionRestore,i)||!valid(false))return false;
                if(!applyMaterialParameterOverridesCandidate(plans[*view.sourceTargets[i]].restored->material.parameters,view.backups[i]))return false;
            }
            view.targets.reserve(plans.size());
            for(auto&plan:plans){if(!materialObjectParameterForwardCandidate(plan.appliedForward,*plan.applied)||!materialObjectParameterForwardCandidate(plan.restoredForward,*plan.restored))return false;plan.appliedPointer=plan.applied.get();view.targets.push_back({plan.id,plan.appliedPointer});}
            if(!permitted(policy,Allocation::sessionPublish,plans.size())||!valid(false))return false;
            auto rollback=[&](MaterialObjectStoreCandidate*)noexcept{for(auto&plan:plans)if(plan.published){auto*p=plan.object;p->parameterState.swap(plan.original);if(p->proxyState)p->proxyState->retarget(p->parameterState.get());p->forwarding.values=plan.oldForward;p->forwarding.valuesInitialized=plan.initialized;}};
            std::unique_ptr<MaterialObjectStoreCandidate,decltype(rollback)>publication(this,rollback);
            for(auto&plan:plans){auto*p=plan.object;plan.original=std::move(p->parameterState);p->parameterState=std::move(plan.applied);if(p->proxyState)p->proxyState->retarget(p->parameterState.get());p->forwarding.values=plan.appliedForward;p->forwarding.valuesInitialized=true;plan.published=true;}
            if(!(steps?drawUse(SessionDrawView{view,drawViews}):use(view))||!valid(true))return false;
            for(auto&plan:plans){auto*p=plan.object;plan.transient=std::move(p->parameterState);p->parameterState=std::move(plan.restored);if(p->proxyState)p->proxyState->retarget(p->parameterState.get());p->forwarding.values=plan.restoredForward;p->forwarding.valuesInitialized=true;plan.published=false;}
            return true;
        }catch(...){return false;}
    }
public:
    // Null-queue flag applies immediately. Safe allocation/append failure is an
    // owned policy (the native null-allocation enqueue was unsafe).
    bool forward(Id id,Operation op,bool queued,const Permit&permit={}){
        if(!get(id)||get(id)->sessionPending)return false;
        if(!queued){if(op==Operation::decrement)decrement(id);else deleteIfUnreferenced(id);return true;}
        try{
            if(!permitted(permit,Allocation::functor,sizeof(StudioDecalQueuedCallCandidate)))return false;
            auto call=std::make_unique<StudioDecalQueuedCallCandidate>();call->owner=reinterpret_cast<uintptr_t>(this);call->argument=id;
            call->method=op==Operation::decrement?&deferredDecrement:&deferredDelete;
            if(!permitted(permit,Allocation::append,0))return false;
            queue.enqueue(call.get());call.release();return true;
        }catch(const std::bad_alloc&){return false;}
    }
    void drain(){if(!sessionScopes)queue.drain();}
    size_t queuedCount()const{return queue.queuedCount();}
    const StudioMaterialQueueCandidate&queueState()const{return queue.queueState();}
    size_t size()const{return objects.size();}
    size_t symbolCount()const{return symbols.size();}
    const Counts&statistics()const{return counts;}
    bool valid()const{return dictionary.valid();}
};
