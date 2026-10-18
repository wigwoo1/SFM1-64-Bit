#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "create_runtime_float_curveinfo.h"
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=3)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;std::array<char,16>id{};id.fill(0x54);auto index=r.graph.elements.size();auto h=createRuntimeFloatCurveInfo(r,m,id,"test float curveinfo",4);size_t checks=0;
 for(const auto&schema:floatCurveInfoSchemaRecovered){auto a=r.attributes.resolve(r.graph,r.elements,r.attributes.find(h,schema.name));if(!a||a->type!=schema.type||a->values.size()!=1)throw std::runtime_error("curveinfo schema mismatch");auto expected=std::vector<char>(schema.type==4?1:4,0);if(std::string(schema.name)=="maxvalue"){Value v;v.wire=expected;setWireU32(v,0x3f800000);expected=v.wire;}if(a->values[0].wire!=expected)throw std::runtime_error("curveinfo default mismatch");++checks;}
 saveGraph(r.graph,argv[2]);SessionRuntime copy(loadGraph(argv[2]));auto e=copy.elements.resolve(copy.graph,copy.elements.graphHandles.at(index));if(e->id!=id||copy.graph.strings.at(e->type)!="DmeFloatCurveInfo"||e->attributes.size()!=13)throw std::runtime_error("curveinfo reload mismatch");
 std::cout<<"curveinfo_default_checks="<<checks<<" reload=1\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
