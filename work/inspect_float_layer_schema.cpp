#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "float_layer_schema.h"
int main(int argc,char**argv){try{if(argc!=2)return 1;auto g=loadGraph(argv[1]);unsigned layers=0,present=0,missing=0;for(const auto&e:g.elements)if(g.strings.at(e.type)=="DmeFloatLogLayer"){++layers;for(const auto&field:floatLayerSchemaRecovered){auto a=namedAttribute(g,e,field.name);if(!a){++missing;continue;}if(a->type!=field.type)throw std::runtime_error("layer schema type mismatch");++present;}}std::cout<<"float_layers="<<layers<<" present_schema_fields="<<present<<" omitted_schema_fields="<<missing<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
