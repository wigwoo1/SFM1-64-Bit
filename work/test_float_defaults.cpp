#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "float_log_default.h"
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("usage: test_float_defaults session.dmx");auto g=loadGraph(argv[1]);uint64_t logs=0,enabled=0,empty=0,nonzeroEmpty=0;
 for(auto&e:g.elements)if(g.strings[e.type]=="DmeFloatLog"){FloatLogDefault d(g,e);++logs;enabled+=d.enabled;for(auto ref:hierarchyRefs(g,e,"layers",15)){auto*t=namedAttribute(g,g.elements.at(ref),"times");if(!t||t->type!=21)throw std::runtime_error("invalid layer times");if(t->values.empty()){++empty;nonzeroEmpty+=d.emptyLayerValue()!=0;}}}
 // Verify both enabled and disabled paths with a nonzero default.
 Graph s;s.strings={"DmeFloatLog","usedefaultvalue","defaultvalue"};Element e;e.type=0;Attribute a;a.name=1;a.type=4;Value b;b.wire={1};a.values={b};Attribute v;v.name=2;v.type=3;Value f;f.wire.resize(4);setWireU32(f,0x40200000);v.values={f};e.attributes={a,v};
 if(FloatLogDefault(s,e).emptyLayerValue()!=2.5f)throw std::runtime_error("enabled default mismatch");e.attributes[0].values[0].wire[0]=0;if(FloatLogDefault(s,e).emptyLayerValue()!=0)throw std::runtime_error("disabled default mismatch");
 std::cout<<"logs="<<logs<<" enabled="<<enabled<<" empty_layers="<<empty<<" nonzero_empty_values="<<nonzeroEmpty<<" synthetic_cases=2\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
