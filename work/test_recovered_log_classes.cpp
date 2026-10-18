#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "recovered_log_classes.h"
int main(int argc,char**argv){try{
 if(argc!=2)return 1;auto g=loadGraph(argv[1]);size_t logs=0;std::map<std::string,size_t>counts;
 for(const auto&e:g.elements){const auto&type=g.strings.at(e.type);if(recoveredLogClass(type)){++logs;++counts[type];}else if(type.size()>=3&&type.substr(type.size()-3)=="Log")throw std::runtime_error("uncovered log class: "+type);}
 if(recoveredLogClass("DmeChannel")||recoveredLogClass("DmeFloatLogLayer")||recoveredLogClass("unknown")||!logs)throw std::runtime_error("class coverage mismatch");
 std::cout<<"verified_log_instances="<<logs<<" distinct_classes="<<counts.size()<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
