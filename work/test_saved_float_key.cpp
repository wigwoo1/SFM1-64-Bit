#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "animation_keys.h"
int main(int argc,char**argv){try{
 if(argc!=6)throw std::runtime_error("INPUT SAVED LAYER INDEX VALUE required");
 auto before=loadGraph(argv[1]),after=loadGraph(argv[2]);validateGraph(after);
 auto layer=std::stoul(argv[3]),index=std::stoul(argv[4]);float expected=std::stof(argv[5]);
 KeyTrack keys(after,after.elements.at(layer));if(wireFloat(keys.values->values.at(index))!=expected)throw std::runtime_error("saved key mismatch");
 if(before.header!=after.header||before.strings!=after.strings||before.elements.size()!=after.elements.size())throw std::runtime_error("graph metadata changed");
 size_t targetAttribute=0;for(;targetAttribute<after.elements[layer].attributes.size();++targetAttribute)if(&after.elements[layer].attributes[targetAttribute]==keys.values)break;
 for(size_t e=0;e<before.elements.size();++e){const auto&b=before.elements[e];const auto&a=after.elements[e];if(b.type!=a.type||b.name!=a.name||b.id!=a.id||b.attributes.size()!=a.attributes.size())throw std::runtime_error("element changed");for(size_t j=0;j<b.attributes.size();++j){const auto&x=b.attributes[j];const auto&y=a.attributes[j];if(x.name!=y.name||x.type!=y.type||x.values.size()!=y.values.size())throw std::runtime_error("attribute changed");for(size_t k=0;k<x.values.size();++k){const auto&v=x.values[k];const auto&w=y.values[k];if(v.text!=w.text||v.reference!=w.reference||v.stringIndex!=w.stringIndex||(!(e==layer&&j==targetAttribute&&k==index)&&v.wire!=w.wire))throw std::runtime_error("unselected value changed");}}}
 std::cout<<"saved_key_verified full_graph_preserved=true\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
