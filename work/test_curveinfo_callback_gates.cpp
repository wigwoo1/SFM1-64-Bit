#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "create_runtime_float_log.h"
#include "create_runtime_float_curveinfo.h"
#include "initialize_float_layer_keys.h"
#include "write_float_log_curveinfo.h"
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 1;
 SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;RuntimeFloatLayerState state(r);
 std::array<char,16>id{},lid{},secondId{},cid{};id.fill(0x61);lid.fill(0x62);secondId.fill(0x63);cid.fill(0x64);
 auto count=r.graph.elements.size();auto log=createRuntimeFloatLog(r,m,id,lid,4,&state);auto first=r.elements.graphHandles.at(count+1);
 auto second=createRuntimeFloatLayer(r,m,secondId,4);state.construct(second);
 if(!attachMutatingFloatLayer(r,m,log,second,false,[](unsigned,uint32_t,size_t,size_t){},[](){return false;},&state))throw std::runtime_error("second layer attachment failed");
 initializeEmptyFloatLayerKeys(r,first,{{0,1.0f},{10,2.0f}});initializeEmptyFloatLayerKeys(r,second,{{0,3.0f},{5,4.0f},{10,5.0f}});
 auto target=createRuntimeFloatCurveInfo(r,m,cid,"multi layer curve info",4);
 auto field=r.attributes.resolve(r.graph,r.elements,r.attributes.find(target,"defaultCurveType"));setWireU32(field->values[0],9);
 auto curves=[&](uint32_t h)->Attribute*{return r.attributes.resolve(r.graph,r.elements,r.attributes.find(h,"curvetypes"));};
 auto verify=[&](uint32_t h,size_t n,int32_t expected){auto a=curves(h);if(!a||a->values.size()!=n)throw std::runtime_error("curve count mismatch");for(auto&v:a->values)if(wireInt(v)!=expected)throw std::runtime_error("curve type mismatch");};
 RuntimeIntegerUndoCapture capture;capture.fileIds[first]=1;capture.fileIds[second]=1;capture.journal.beginBoundary();uint32_t frameworkDirty=0;auto write=[&](uint32_t h,bool suppressed=false){return writeFloatLogCurveInfo(r,m,state,log,h,false,[](){return false;},suppressed,&frameworkDirty,&capture);};
 if(!write(target))throw std::runtime_error("initial write failed");verify(first,2,9);verify(second,3,9);
 if(capture.journal.undo.size()!=7)throw std::runtime_error("curve rebuild capture count mismatch");
 auto replayNotice=[](unsigned,uint32_t,int32_t,int32_t){};
 if(replayIntegerJournal(capture.journal,r,m,true,frameworkDirty,false,replayNotice,[](){return false;})!=7)throw std::runtime_error("curve undo failed");verify(first,0,0);verify(second,0,0);
 if(replayIntegerJournal(capture.journal,r,m,false,frameworkDirty,false,replayNotice,[](){return false;})!=7)throw std::runtime_error("curve redo failed");verify(first,2,9);verify(second,3,9);
 if(dispatchFloatLogCurveInfoChanged(state,log,r.attributes.find(log,"layers"))!=0)throw std::runtime_error("unrelated callback dispatched");
 if(!write(0xffffffff,true))throw std::runtime_error("suppressed write failed");verify(first,2,9);verify(second,3,9);
 if(!write(target))throw std::runtime_error("replacement failed");
 m.owners.at(log).state&=static_cast<uint8_t>(~4u);
 if(!write(0xffffffff))throw std::runtime_error("disabled owner write failed");verify(first,2,9);verify(second,3,9);
 m.owners.at(log).state|=4;if(!write(target)||!write(0xffffffff))throw std::runtime_error("enabled callback write failed");verify(first,0,0);verify(second,0,0);
 auto identity=r.attributes.identity(r.graph,r.elements,r.attributes.find(log,"curveinfo"));auto&flags=m.owners.at(log).attributeFlags.at(identity);flags&=static_cast<uint16_t>(~0x100u);
 if(!write(target))throw std::runtime_error("disabled attribute write failed");verify(first,0,0);verify(second,0,0);
 flags|=0x100;if(dispatchFloatLogCurveInfoChanged(state,log,r.attributes.find(log,"curveinfo"))!=2)throw std::runtime_error("multi callback count mismatch");verify(first,2,9);verify(second,3,9);
 std::cout<<"curve_callback_layers=2 key_counts=2,3 gate_checks=4\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}



