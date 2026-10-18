#pragma once
#include "clip_hierarchy.h"
#include <limits>
struct SessionMovieDimensionsCandidate {uint32_t width,height;size_t rgbaBytes;uint32_t settings;};
inline SessionMovieDimensionsCandidate sessionMovieDimensionsCandidate(const Graph&graph,uint32_t root){
 const Element*owner=nullptr;for(const auto&element:graph.elements){auto active=hierarchyRefs(graph,element,"activeClip",1);if(active.size()==1&&active.front()==root){if(owner)throw std::runtime_error("ambiguous movie settings owner");owner=&element;}}
 if(!owner)throw std::runtime_error("movie settings owner missing");auto settings=hierarchyRefs(graph,*owner,"settings",1);if(settings.size()!=1)throw std::runtime_error("session settings missing");auto movie=hierarchyRefs(graph,graph.elements.at(settings.front()),"movieSettings",1);if(movie.size()!=1)throw std::runtime_error("session movie settings missing");const auto&element=graph.elements.at(movie.front());
 auto dimension=[&](const char*key){const Attribute*found=nullptr;for(const auto&attribute:element.attributes)if(graph.strings.at(attribute.name)==key){if(found)throw std::runtime_error("duplicate movie dimension");found=&attribute;}if(!found||found->type!=2||found->values.size()!=1||found->values.front().wire.size()!=4)throw std::runtime_error("movie dimension schema");auto value=wireInt(found->values.front());if(value<=0)throw std::runtime_error("movie dimensions must be positive");return uint32_t(value);};
 auto width=dimension("width"),height=dimension("height");uint64_t pixels=uint64_t(width)*height;if(pixels>std::numeric_limits<size_t>::max()/4)throw std::runtime_error("movie capture byte overflow");return {width,height,size_t(pixels*4),movie.front()};
}
