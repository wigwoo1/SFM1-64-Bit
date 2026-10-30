#pragma once
#include "clip_hierarchy.h"
#include <limits>
#include <optional>
struct SessionMovieDimensionsCandidate {uint32_t width,height;size_t rgbaBytes;uint32_t settings;};
// Optional previews may retain their previous size when export settings are
// absent. Present but malformed settings always remain an error.
inline std::optional<SessionMovieDimensionsCandidate> readSessionMovieDimensionsCandidate(const Graph&graph,uint32_t root,bool required){
 const Element*owner=nullptr;for(const auto&element:graph.elements){auto active=hierarchyRefs(graph,element,"activeClip",1);if(active.size()==1&&active.front()==root){if(owner)throw std::runtime_error("ambiguous movie settings owner");owner=&element;}}
 if(!owner){if(required)throw std::runtime_error("movie settings owner missing");return std::nullopt;}auto settings=hierarchyRefs(graph,*owner,"settings",1);if(settings.size()!=1){if(required)throw std::runtime_error("session settings missing");return std::nullopt;}auto movie=hierarchyRefs(graph,graph.elements.at(settings.front()),"movieSettings",1);if(movie.size()!=1){if(required)throw std::runtime_error("session movie settings missing");return std::nullopt;}const auto&element=graph.elements.at(movie.front());
 auto dimension=[&](const char*key){const Attribute*found=nullptr;for(const auto&attribute:element.attributes)if(graph.strings.at(attribute.name)==key){if(found)throw std::runtime_error("duplicate movie dimension");found=&attribute;}if(!found||found->type!=2||found->values.size()!=1||found->values.front().wire.size()!=4)throw std::runtime_error("movie dimension schema");auto value=wireInt(found->values.front());if(value<=0)throw std::runtime_error("movie dimensions must be positive");return uint32_t(value);};
 auto width=dimension("width"),height=dimension("height");uint64_t pixels=uint64_t(width)*height;if(pixels>std::numeric_limits<size_t>::max()/4)throw std::runtime_error("movie capture byte overflow");return SessionMovieDimensionsCandidate{width,height,size_t(pixels*4),movie.front()};
}
inline SessionMovieDimensionsCandidate sessionMovieDimensionsCandidate(const Graph&graph,uint32_t root){return *readSessionMovieDimensionsCandidate(graph,root,true);}
