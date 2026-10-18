#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "create_runtime_float_log.h"
#include "initialize_float_layer_keys.h"
#include "runtime_curveinfo_default.h"
#include "create_runtime_float_curveinfo.h"
#include "write_float_log_curveinfo.h"
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=3)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;RuntimeFloatLayerState state(r);std::array<char,16>id{},layerId{};id.fill(0x31);layerId.fill(0x32);auto count=r.graph.elements.size();auto log=createRuntimeFloatLog(r,m,id,layerId,4,&state);auto layer=r.elements.graphHandles.at(count+1);
 if(!initializeEmptyFloatLayerKeys(r,layer,{{0,1.0f},{10,2.0f}}))throw std::runtime_error("key initialization failed");
 std::array<char,16>curveId{};curveId.fill(0x33);auto target=createRuntimeFloatCurveInfo(r,m,curveId,"constructed float curve info",4);
 if(!writeFloatLogCurveInfo(r,m,state,log,target,false,[](){return false;})||runtimeLayerDefaultCurveType(state,layer)!=0||!refreshRuntimeLayerCurveTypes(state,layer,false))throw std::runtime_error("initial curveinfo default failed");
 auto defaultField=r.attributes.resolve(r.graph,r.elements,r.attributes.find(target,"defaultCurveType"));setWireU32(defaultField->values[0],7);
 if(runtimeLayerDefaultCurveType(state,layer)!=7||dispatchFloatLogCurveInfoChanged(state,log,r.attributes.find(log,"curveinfo"))!=1)throw std::runtime_error("populated curve lookup failed");
 auto curves=r.attributes.resolve(r.graph,r.elements,r.attributes.find(layer,"curvetypes"));if(curves->values.size()!=2||wireInt(curves->values[0])!=7||wireInt(curves->values[1])!=7)throw std::runtime_error("curve rebuild mismatch");
 if(!writeFloatLogCurveInfo(r,m,state,log,0xffffffff,false,[](){return false;})||!curves->values.empty())throw std::runtime_error("null curveinfo callback failed");
 if(!writeFloatLogCurveInfo(r,m,state,log,target,false,[](){return false;})||curves->values.size()!=2||wireInt(curves->values[1])!=7)throw std::runtime_error("replacement curveinfo callback failed");
 if(writeFloatLogCurveInfo(r,m,state,log,target,false,[](){return false;}))throw std::runtime_error("unchanged curveinfo write accepted");
 auto typed=r.elements.resolve(r.graph,target);typed->type=internString(r.graph,"DmeIntCurveInfo");
 if(!writeFloatLogCurveInfo(r,m,state,log,0xffffffff,false,[](){return false;})||!writeFloatLogCurveInfo(r,m,state,log,target,false,[](){return false;})||runtimeLayerDefaultCurveType(state,layer)!=7)throw std::runtime_error("base schema acceptance failed");
 typed->type=internString(r.graph,"DmeFloatCurveInfo");
 saveGraph(r.graph,argv[2]);SessionRuntime reload(loadGraph(argv[2]));RuntimeFloatLayerState restored(reload);auto rl=reload.elements.graphHandles[count+1];restored.construct(rl);restored.attach(rl,reload.elements.graphHandles[count]);if(runtimeLayerDefaultCurveType(restored,rl)!=7||!refreshRuntimeLayerCurveTypes(restored,rl,false))throw std::runtime_error("curve restore lookup failed");
 auto field=r.attributes.resolve(r.graph,r.elements,r.attributes.find(target,"defaultCurveType"));field->type=3;if(!refreshRuntimeLayerCurveTypes(state,layer,true))throw std::runtime_error("suppression failed");bool rejected=false;try{refreshRuntimeLayerCurveTypes(state,layer,false);}catch(const std::runtime_error&){rejected=true;}if(!rejected||curves->values.size()!=2||wireInt(curves->values[0])!=7)throw std::runtime_error("malformed lookup changed arrays");
 std::cout<<"populated_curveinfo_checks=9 rebuilt_curves=2 reload=1\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}




