#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_local_flex_controllers_candidate.h"
#include "mdl_direct_flex_evaluation_candidate.h"
#include <iostream>
void verifyBindingRejections(const Graph&original,const std::vector<MdlFlexControllerMetadata>&metadata){
 size_t operatorIndex=SIZE_MAX,sourceIndex=SIZE_MAX;
 for(size_t i=0;i<original.elements.size();++i){const auto&e=original.elements[i];if(original.strings.at(e.type)!="DmeGlobalFlexControllerOperator")continue;bool linked=false;for(const auto&a:e.attributes)if(original.strings.at(a.name)=="gameModel"&&a.values.at(0).reference==2552)linked=true;if(!linked)continue;operatorIndex=i;for(size_t j=0;j<e.attributes.size();++j)if(original.strings.at(e.attributes[j].name)=="flexWeight")sourceIndex=j;break;}
 if(operatorIndex==SIZE_MAX||sourceIndex==SIZE_MAX)throw std::runtime_error("test operator missing");
 for(int scenario=0;scenario<4;++scenario){auto graph=original;auto records=metadata;auto&e=graph.elements[operatorIndex];auto&source=e.attributes[sourceIndex];
  if(scenario==0)records[0].localIndex=999;
  if(scenario==1){uint32_t nan=0x7fc00000;std::memcpy(source.values[0].wire.data(),&nan,4);}
  if(scenario==2)e.attributes.push_back(source);
  if(scenario==3){auto duplicate=e;float value;std::memcpy(&value,source.values[0].wire.data(),4);value+=1;std::memcpy(duplicate.attributes[sourceIndex].values[0].wire.data(),&value,4);graph.elements.push_back(std::move(duplicate));}
  bool rejected=false;try{readSessionLocalFlexControllersCandidate(graph,2552,records);}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("invalid local binding accepted");
 }
 std::cout<<"local flex binding rejects order nonfinite duplicate_attribute conflicting_sources\n";
}
int main(){try{auto graph=loadGraph("work/roundtrip-11.dmx");std::ifstream file(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen.mdl)",std::ios::binary);if(!file)throw std::runtime_error("model missing");std::vector<uint8_t>bytes((std::istreambuf_iterator<char>(file)),{});auto metadata=readMdl49FlexMetadata(bytes);verifyBindingRejections(graph,metadata);auto controllers=readSessionLocalFlexControllersCandidate(graph,2552,metadata);uint32_t descriptorCount;std::memcpy(&descriptorCount,bytes.data()+260,4);auto weights=evaluateDirectMdlFlexRulesCandidate(readMdl49FlexRulesCandidate(bytes),descriptorCount,controllers);size_t nonzero=0;for(float value:weights)if(value!=0)++nonzero;std::cout<<"saved model named flex sources resolved controllers="<<controllers.size()<<" descriptors="<<weights.size()<<" nonzero="<<nonzero<<" historic_global_indices_unused native_parity=unverified\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
