#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "create_runtime_float_layer.h"
#include "integer_clear_undo_record.h"
#include "undo_recording_gate.h"
#include "undo_recording_suppression.h"
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;std::array<char,16>id{};id.fill(0x72);auto layer=createRuntimeFloatLayer(r,m,id,4);uint32_t dirty=0;auto notify=[](unsigned,uint32_t,int32_t,int32_t){};auto suppressed=[](){return false;};
 appendRuntimeIntegerArray(r,m,layer,"curvetypes",-7,dirty,false,notify,suppressed);appendRuntimeIntegerArray(r,m,layer,"curvetypes",123,dirty,false,notify,suppressed);auto record=captureIntegerClearRecord(r,layer,"curvetypes");clearRuntimeIntegerArray(r,m,layer,"curvetypes",dirty,false,notify,suppressed);
 uint8_t recordingFlags=8;{RuntimeUndoRecordingSuppression guard(recordingFlags);if(recoveredUndoRecordingGate(recordingFlags,true,1))throw std::runtime_error("replay recording enabled");recordingFlags|=0x20;
 if(!replayIntegerClearRecord(r,m,record,true,dirty,false,notify,suppressed))throw std::runtime_error("reverse replay failed");auto h=r.attributes.find(layer,"curvetypes");auto a=r.attributes.resolve(r.graph,r.elements,h);if(a->values.size()!=2||wireInt(a->values[0])!=-7||wireInt(a->values[1])!=123)throw std::runtime_error("snapshot restore mismatch");

 if(!replayIntegerClearRecord(r,m,record,false,dirty,false,notify,suppressed)||!a->values.empty())throw std::runtime_error("forward replay failed");}
 if(recordingFlags!=0x28||!recoveredUndoRecordingGate(recordingFlags,true,1))throw std::runtime_error("recording state restore failed");
 auto a=r.attributes.resolve(r.graph,r.elements,r.attributes.find(layer,"curvetypes"));a->type=17;if(replayIntegerClearRecord(r,m,record,true,dirty,false,notify,suppressed))throw std::runtime_error("type mismatch accepted");a->type=16;r.elements.retire(layer);if(replayIntegerClearRecord(r,m,record,true,dirty,false,notify,suppressed))throw std::runtime_error("retired owner accepted");
 std::cout<<"clear_record_replay_checks=6\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}


