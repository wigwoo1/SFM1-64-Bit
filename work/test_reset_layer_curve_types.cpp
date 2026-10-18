#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "reset_layer_curve_types.h"
int main(int argc,char**argv){try{if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));RuntimeFloatLayerState state(r);size_t layers=0,keys=0;
 for(size_t i=0;i<r.graph.elements.size();++i)if(r.graph.strings.at(r.graph.elements[i].type)=="DmeFloatLogLayer"){
 auto h=r.elements.graphHandles[i];state.construct(h);auto curves=r.attributes.resolve(r.graph,r.elements,r.attributes.find(h,"curvetypes"));auto old=curves->values.size();
 if(!resetRuntimeFloatLayerCurveTypes(state,h,true,7)||curves->values.size()!=old)throw std::runtime_error("suppression changed curve count");
 if(!resetRuntimeFloatLayerCurveTypes(state,h,false,7))throw std::runtime_error("default curve reset failed");auto times=r.attributes.resolve(r.graph,r.elements,r.attributes.find(h,"times"));if(curves->values.size()!=times->values.size())throw std::runtime_error("curve count mismatch");for(const auto&v:curves->values)if(wireInt(v)!=7)throw std::runtime_error("curve default mismatch");keys+=curves->values.size();
 if(!resetRuntimeFloatLayerCurveTypes(state,h,false,std::nullopt)||!curves->values.empty())throw std::runtime_error("curve clear failed");++layers;
 }if(!layers)throw std::runtime_error("no layers");std::cout<<"curve_reset_layers="<<layers<<" filled_curve_values="<<keys<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
