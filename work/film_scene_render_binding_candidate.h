#pragma once
#include "clip_hierarchy.h"
struct FilmSceneRenderBindingCandidate {
 uint32_t clip,camera,scene;
 std::vector<uint32_t>models;
 std::vector<uint32_t>lights;
 std::vector<uint32_t>cameras;
};
inline FilmSceneRenderBindingCandidate readFilmSceneRenderBindingCandidate(const Graph&graph,uint32_t clip){
 const auto&film=graph.elements.at(clip);
 if(graph.strings.at(film.type)!="DmeFilmClip")throw std::runtime_error("render film clip required");
 auto cameras=hierarchyRefs(graph,film,"camera",1),scenes=hierarchyRefs(graph,film,"scene",1);
 if(cameras.size()!=1||scenes.size()!=1||graph.strings.at(graph.elements[cameras[0]].type)!="DmeCamera")
  throw std::runtime_error("film camera/scene binding");
 FilmSceneRenderBindingCandidate result{clip,cameras[0],scenes[0],{},{},{}};
 std::set<uint32_t>visiting,done;
 auto visit=[&](auto&&self,uint32_t id)->void{
  if(visiting.count(id))throw std::runtime_error("cyclic scene DAG");
  if(done.count(id))return;
  visiting.insert(id);
  const auto&e=graph.elements.at(id);const auto&type=graph.strings.at(e.type);
  if(type=="DmeGameModel")result.models.push_back(id);
  if(type=="DmeProjectedLight")result.lights.push_back(id);
  if(type=="DmeCamera")result.cameras.push_back(id);
  for(auto child:hierarchyRefs(graph,e,"children",15))self(self,child);
  visiting.erase(id);done.insert(id);
 };
 visit(visit,result.scene);
 return result;
}
