#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "create_runtime_float_log.h"
#include "create_runtime_float_curveinfo.h"
#include "initialize_float_layer_keys.h"
#include "write_float_log_curveinfo.h"
#include "scalar_reference_undo_record.h"
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;RuntimeFloatLayerState state(r);std::array<char,16>id{},lid{},cid{};id.fill(0x76);lid.fill(0x77);cid.fill(0x78);auto count=r.graph.elements.size();auto log=createRuntimeFloatLog(r,m,id,lid,4,&state);auto layer=r.elements.graphHandles.at(count+1);initializeEmptyFloatLayerKeys(r,layer,{{0,1.0f},{10,2.0f}});auto target=createRuntimeFloatCurveInfo(r,m,cid,"scalar undo test",4);
 RuntimeUndoRetention retention(r);retention.records[target]={0,0,1};uint32_t dirty=0;auto write=[&](uint32_t h,const std::string&name,uint32_t next){if(name!="curveinfo")throw std::runtime_error("unexpected replay name");return writeFloatLogCurveInfo(r,m,state,h,next,false,[](){return false;},false,&dirty);};
 {auto record=captureScalarReferenceRecord(r,retention,log,"curveinfo",target);if(!write(log,"curveinfo",target))throw std::runtime_error("initial reference write failed");
 auto curves=[&](){return r.attributes.resolve(r.graph,r.elements,r.attributes.find(layer,"curvetypes"))->values.size();};
 if(curves()!=2||!replayScalarReferenceRecord(r,m,record,true,write)||curves()!=0||runtimeLayerDefaultCurveType(state,layer))throw std::runtime_error("scalar reverse replay failed");
 if(!replayScalarReferenceRecord(r,m,record,false,write)||curves()!=2||runtimeLayerDefaultCurveType(state,layer)!=0)throw std::runtime_error("scalar forward replay failed");
 if((retention.records[target].packed>>20)!=1)throw std::runtime_error("record lost target retention");auto h=r.attributes.find(log,"curveinfo");m.owners.at(log).attributeFlags.at(r.attributes.identity(r.graph,r.elements,h))|=0x20;if(replayScalarReferenceRecord(r,m,record,true,write))throw std::runtime_error("read only replay accepted");}
 if(retention.records[target].packed!=0)throw std::runtime_error("record destruction leaked retention");std::cout<<"scalar_reference_replay_checks=4\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
