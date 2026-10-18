#pragma once
#include "model_render_inputs_candidate.h"
#include <memory>
#include <functional>
#include <utility>
// CPU inputs must remain immutable and outlive this cache. GPU resources must
// outlive all submissions. Uploads and draws use the same ordered queue.
template<class Inputs>auto sceneMaterialOwnerCandidate(const Inputs&inputs,size_t model,int)->decltype(inputs.ownerAt(model)){return inputs.ownerAt(model);}
template<class Inputs>std::shared_ptr<const ModelRenderInputsCandidate>sceneMaterialOwnerCandidate(const Inputs&,size_t,...){return {};}
template<class Images>
class RetainedSceneMaterialGpuCacheCandidate {
 enum class State {Prepared,Recorded,Submitted};
 struct Entry {std::unique_ptr<Images>images;State state=State::Prepared;std::shared_ptr<const ModelRenderInputsCandidate>owner;};
 std::map<const ModelRenderInputsCandidate*,Entry>entries_;
 std::function<std::unique_ptr<Images>(const ModelRenderInputsCandidate&)>create_;
public:
 explicit RetainedSceneMaterialGpuCacheCandidate(std::function<std::unique_ptr<Images>(const ModelRenderInputsCandidate&)>create):create_(std::move(create)){if(!create_)throw std::runtime_error("GPU material cache factory required");}
 Images&get(const ModelRenderInputsCandidate&input,std::shared_ptr<const ModelRenderInputsCandidate>owner={}){if(owner&&owner.get()!=&input)throw std::runtime_error("GPU material owner identity");auto found=entries_.find(&input);if(found!=entries_.end()){if(owner)found->second.owner=std::move(owner);return *found->second.images;}auto images=create_(input);if(!images||images->entries.size()!=input.materials.size())throw std::runtime_error("GPU material cache image coverage");return *entries_.emplace(&input,Entry{std::move(images),State::Prepared,std::move(owner)}).first->second.images;}
 template<class Scene,class Inputs>
 auto descriptors(const Scene&scene,const Inputs&inputs){using Set=decltype(std::declval<Images>().entries[0].set);std::vector<Set>sets;sets.reserve(scene.materials.size());for(const auto&address:scene.materials){const auto&input=inputs.at(address.model);if(address.material>=input.materials.size())throw std::runtime_error("GPU material cache address");sets.push_back(get(input,sceneMaterialOwnerCandidate(inputs,address.model,0)).entries.at(address.material).set);}return sets;}
 template<class Command>void recordUploads(Command command){for(auto&entry:entries_){if(entry.second.state==State::Recorded)throw std::runtime_error("GPU material uploads already pending");}for(auto&entry:entries_)if(entry.second.state==State::Prepared){entry.second.images->upload(command);entry.second.state=State::Recorded;}}
 void submitted(){for(auto&entry:entries_)if(entry.second.state==State::Recorded)entry.second.state=State::Submitted;}
 void discardRecording(){for(auto&entry:entries_)if(entry.second.state==State::Recorded)entry.second.state=State::Prepared;}
 size_t size()const{return entries_.size();}
 size_t preparedCount()const{size_t count=0;for(const auto&entry:entries_)if(entry.second.state==State::Prepared)++count;return count;}
};
