#pragma once
#include <functional>
#include <unordered_map>
#include <string>
#include <stdexcept>
struct OwnedClassResolveDispatchResult{size_t registered=0,unregistered=0;};
template<class Projection>class OwnedDmxClassResolveRegistryCandidate{
 using Callback=std::function<void(Projection&,uint32_t)>;
 std::unordered_map<std::string,Callback> callbacks_;
public:
 void bind(std::string type,Callback callback){if(!callback)throw std::invalid_argument("empty class callback");callbacks_.insert_or_assign(std::move(type),std::move(callback));}
 OwnedClassResolveDispatchResult dispatch(Projection&projection){std::unordered_map<uint32_t,uint32_t>indices;for(uint32_t i=0;i<projection.elementHandles.size();++i)indices.emplace(projection.elementHandles[i],i);OwnedClassResolveDispatchResult result;projection.scene.dispatchResolveQueue([&](auto*owner){auto index=indices.at(owner->record.handle);auto type=projection.graph.strings.at(projection.graph.elements.at(index).type);auto found=callbacks_.find(type);if(found==callbacks_.end())++result.unregistered;else{found->second(projection,index);++result.registered;}for(auto&flags:projection.typedAttributeFlags.at(index))flags&=0xff7f;});return result;}
};
