// CPU resource/lifetime audit using real read-only loose VMT files. No GPU,
// texture resource creation, native IMaterial ABI or complete FindMaterial claim.
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_material_override_dispatch_candidate.h"
#include "session_material_overrides_candidate.h"
#include "material_session_store_bindings_candidate.h"
#include "material_find_file_request_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include <iostream>
static void requireBindings(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static std::string readBindings(const std::filesystem::path&p){std::ifstream in(p,std::ios::binary);requireBindings(bool(in),"read-only input open");std::string s((std::istreambuf_iterator<char>(in)),{});requireBindings(s.size()<=1u<<24,"read extent");return s;}
static uint64_t hashBindings(const std::string&s){uint64_t h=14695981039346656037ull;for(unsigned char c:s){h^=c;h*=1099511628211ull;}return h;}
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"owned x64 audit");requireBindings(argc==4&&!std::ifstream(argv[3]).good(),"session game-directory fresh-output");
 auto graph=loadGraph(argv[1]);const std::filesystem::path game=argv[2];std::ofstream out(argv[3],std::ios::binary);requireBindings(bool(out),"audit output");
 auto gameinfo=game/"usermod/gameinfo.txt";auto gameinfoText=readBindings(gameinfo);out<<"F\t"<<gameinfo.generic_string()<<'\t'<<gameinfoText.size()<<'\t'<<hashBindings(gameinfoText)<<'\n';
 auto mounts=gameinfoMountsCandidate(gameinfoText,game/"usermod",game);for(const auto&m:mounts)out<<"G\t"<<m.generic_string()<<'\n';
 std::map<std::string,std::optional<std::string>>reads;std::map<std::string,std::string>reasons;
 SourceMaterialKeyValuesSourceCandidate provider=[&](const std::string&request)->std::optional<std::string>{
  auto logical=sourceMaterialKeyValuesPathCandidate(request);requireBindings(logical.size()>=4&&logical.substr(logical.size()-4)==".vmt","VMT-only provider");auto found=reads.find(logical);if(found!=reads.end())return found->second;
  for(const auto&mount:mounts){auto path=mount/logical;if(!std::filesystem::is_regular_file(path))continue;auto data=readBindings(path);out<<"F\t"<<path.generic_string()<<'\t'<<data.size()<<'\t'<<hashBindings(data)<<'\n';reads.emplace(logical,data);return data;}
  reads.emplace(logical,std::nullopt);return {};
 };
 MaterialObjectStoreCandidate store;std::vector<uint32_t>ids;unsigned ready=0,unavailable=0,models=0,observed=0,missing=0,valueRefused=0;
 MaterialSessionStoreBindingsCandidate bindings(store,[&](std::string_view name)->std::optional<MaterialObjectStoreCandidate::Id>{
  std::string key;if(!materialFindCacheKeyCandidate(name,key))return {};
  try{
   auto id=store.find(key,false);if(id)return id;
   MaterialFindFileRequestCandidate request;if(!materialFindFileRequestCandidate(name,request)||request.pathID!="GAME"){reasons[key]="unsupported request";return {};}
   if(!provider(request.file)){reasons[key]="loose VMT unavailable; VPK/dynamic lookup not implemented";return {};}
   // Parse the real file/patch tree before declaring a filename-backed resource.
   // Its CPU parameters may still be unavailable (for example live proxies).
   auto file=sourceMaterialPatchFilesCandidate(key,provider,false,true);if(file.document.roots.size()!=1||file.status==SourceMaterialPatchFileLoadStatusCandidate::RootUnavailable||file.status==SourceMaterialPatchFileLoadStatusCandidate::RetainRootName){reasons[key]="file/patch tree unavailable";return {};}
   if(!id){MaterialObjectStoreCandidate::Source absent;requireBindings(store.createOrReuse(key,"Other textures",absent,id),"owned filename object creation");}
   MaterialObjectParameterOptionsCandidate options;options.provider=provider;
   if(!store.initializeParameters(id,options)){
    std::vector<std::string>proxyNames;auto visit=[&](auto&&self,const SourceMaterialPatchNodeCandidate&node)->void{if(sourceMaterialPatchLowerCandidate(node.key)=="proxies")for(const auto&child:node.children)proxyNames.push_back(child.key);for(const auto&child:node.children)self(self,child);};visit(visit,file.document.roots.front());
    out<<"P\t"<<id<<'\t'<<key<<'\t'<<file.document.roots.front().key<<'\t'<<proxyNames.size();for(const auto&proxy:proxyNames)out<<'\t'<<proxy;out<<'\n';
    reasons[key]=proxyNames.empty()?"resource exists; CPU shader/grammar initialization refused":"resource exists; CPU initialization refuses unavailable proxy factory";return id;
   }
   reasons[key]="owned CPU initialized; GPU/native parity unverified";return id;
  }catch(const std::exception&error){reasons[key]=std::string("unavailable: ")+error.what();return {};}
 });
 for(uint32_t id=0;id<graph.elements.size();++id){const auto&e=graph.elements[id];if(graph.strings.at(e.type)!="DmeMaterial")continue;
  auto*name=namedAttribute(graph,e,"mtlName");requireBindings(name&&name->type==5&&name->values.size()==1,"name schema");requireBindings(bindings.define(id)&&bindings.setName(id,name->values[0].text)&&bindings.clearDirty(id),"define actual session element");ids.push_back(id);
  const auto acquired=bindings.acquire(id);const auto key=normalizeSessionMaterialNameCandidate(name->values[0].text);if(acquired&&*acquired)++ready;else ++unavailable;
  out<<"M\t"<<id<<'\t'<<name->values[0].text<<'\t'<<key<<'\t'<<bool(acquired)<<'\t'<<(acquired?*acquired:0)<<'\t'<<reasons[key]<<'\n';
 }
 for(uint32_t id=0;id<graph.elements.size();++id){const auto&e=graph.elements[id];if(graph.strings.at(e.type)!="DmeGameModel")continue;auto*a=namedAttribute(graph,e,"materials");if(!a)continue;requireBindings(a->type==15,"material array schema");++models;
  std::vector<MaterialSessionStoreBindingsCandidate::Source>entries;for(size_t position=0;position<a->values.size();++position){auto material=a->values[position].reference;entries.push_back({material,readSessionMaterialOverrideDispatchCandidate(graph,material)});out<<"O\t"<<id<<'\t'<<position<<'\t'<<material<<'\n';}
  MaterialSessionStoreBindingsCandidate::Attempt attempt;unsigned calls=0;const bool ok=bindings.withCollection(entries,[&](const auto&){++calls;return true;},0,{},&attempt);
  if(ok)++observed;else if(attempt.status==MaterialSessionStoreBindingsCandidate::Status::UnavailableBinding)++missing;else ++valueRefused;
  requireBindings(calls==unsigned(ok),"no observed partial values");out<<"B\t"<<id<<'\t'<<entries.size()<<'\t'<<unsigned(attempt.status)<<'\t'<<calls<<'\t'<<attempt.acquiredPositions<<'\n';
 }
 auto cached=store.cached();for(auto id:cached){const auto*p=store.get(id);out<<"S\t"<<id<<'\t'<<p->name<<'\t'<<bool(p->parameterState)<<'\t'<<p->references.load()<<'\t'<<(p->parameterState?p->parameterState->material.shader:"")<<'\n';}
 for(auto id:ids)requireBindings(bindings.erase(id),"explicit session retirement");for(auto id:cached){const auto*p=store.get(id);requireBindings(p&&p->references.load()==0,"all actual retained refs balanced");out<<"R\t"<<id<<'\t'<<p->references.load()<<'\n';}
 const auto&stats=bindings.statistics();requireBindings(stats.retains==stats.releases&&!bindings.size()&&!stats.staleRetirements,"binding ownership balance");out.flush();requireBindings(bool(out),"audit complete");
 std::cout<<"PASS benchmark_owned_bindings elements="<<ids.size()<<" bound_resources="<<ready<<" unavailable="<<unavailable<<" cached_objects="<<cached.size()<<" model_arrays="<<models<<" observed="<<observed<<" unavailable_batches="<<missing<<" value_refused="<<valueRefused<<" retains="<<stats.retains<<" releases="<<stats.releases<<" file_reads="<<reads.size()<<" native_resource_parity=UNVERIFIED rendering=NOT_RUN\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
