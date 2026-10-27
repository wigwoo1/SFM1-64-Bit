// Read-only mapping audit. No resource IDs are fabricated from material names.
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_material_overrides_candidate.h"
#include "session_material_override_dispatch_candidate.h"
#include "material_object_store_candidate.h"
#include <iostream>
static void requireBatch(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"owned x64 audit");
 requireBatch(argc==3&&!std::ifstream(argv[2]).good(),"session fresh-output required");
 auto graph=loadGraph(argv[1]);std::ofstream out(argv[2],std::ios::binary);requireBatch(bool(out),"audit output");
 MaterialObjectStoreCandidate store;unsigned models=0,positions=0,refused=0,empty=0,observers=0,numeric=0,textures=0,ignored=0;
 for(uint32_t id=0;id<graph.elements.size();++id){const auto&e=graph.elements[id];
  if(graph.strings.at(e.type)=="DmeMaterial"){
   auto*name=namedAttribute(graph,e,"mtlName");requireBatch(name&&name->type==5&&name->values.size()==1,"name schema");
   out<<"M\t"<<id<<'\t'<<name->values[0].text<<'\t'<<normalizeSessionMaterialNameCandidate(name->values[0].text)<<'\n';
  }
  if(graph.strings.at(e.type)!="DmeGameModel")continue;auto*a=namedAttribute(graph,e,"materials");if(!a)continue;
  requireBatch(a->type==15,"materials schema");auto records=readSessionMaterialOverridesCandidate(graph,id);requireBatch(records.size()==a->values.size(),"position count");++models;
  std::vector<MaterialObjectStoreCandidate::SessionEntry>entries;
  for(size_t i=0;i<records.size();++i){const auto&r=records[i];requireBatch(r.element==a->values[i].reference,"position order");
   auto rows=readSessionMaterialOverrideDispatchCandidate(graph,r.element);unsigned n=0,t=0,other=0;
   for(const auto&row:rows){if(row.name.empty()||row.name[0]!='$'){++other;continue;}switch(row.typeFlags){case 2:case 3:case 4:case 8:case 9:case 10:case 11:case 14:++n;break;case 5:++t;break;default:++other;break;}}
   numeric+=n;textures+=t;ignored+=other;++positions;
   out<<"O\t"<<id<<'\t'<<i<<'\t'<<r.element<<'\t'<<r.material<<'\t'<<n<<'\t'<<t<<'\t'<<other<<'\n';
   // Missing resource resolver is unavailable, not an intentionally skipped ID0.
   entries.push_back({std::nullopt,std::move(rows)});
  }
  unsigned calls=0;bool ok=store.withSessionCollection(entries,[&](const auto&v){++calls;return v.targets.empty()&&v.sourceTargets.empty()&&v.backups.empty();});observers+=calls;
  if(entries.empty()){requireBatch(ok&&calls==1,"empty batch");++empty;}
  else{requireBatch(!ok&&!calls,"unresolved batch must refuse before observation");++refused;}
  out<<"B\t"<<id<<'\t'<<entries.size()<<'\t'<<ok<<'\t'<<calls<<'\n';
 }
 out.flush();requireBatch(bool(out)&&store.size()==0,"audit complete; no dummy resources");
 std::cout<<"PASS benchmark_mapping arrays="<<models<<" positions="<<positions<<" unavailable_refused="<<refused<<" empty_observed="<<empty<<" observer_calls="<<observers<<" numeric_rows="<<numeric<<" texture_rows="<<textures<<" ignored_rows="<<ignored<<" resource_binding=UNVERIFIED rendering=NOT_RUN\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
