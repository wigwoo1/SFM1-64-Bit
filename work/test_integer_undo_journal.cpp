#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "create_runtime_float_layer.h"
#include "integer_undo_journal.h"
#include "undo_recording_gate.h"
#include "recorded_integer_mutation.h"
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;std::array<char,16>id{};id.fill(0x75);auto layer=createRuntimeFloatLayer(r,m,id,4);uint32_t dirty=0;auto noop=[](unsigned,uint32_t,int32_t,int32_t){};auto suppressed=[](){return false;};
 for(int32_t v:{4,8})appendRuntimeIntegerArray(r,m,layer,"curvetypes",v,dirty,false,noop,suppressed);
 RuntimeIntegerUndoCapture capture;capture.fileIds[layer]=1;auto&journal=capture.journal;journal.beginBoundary();clearRecordedIntegerArray(r,m,capture,layer,"curvetypes",dirty,false,noop,suppressed);
 for(int32_t i=0;i<3;++i){appendRecordedIntegerArray(r,m,capture,layer,"curvetypes",9,dirty,false,noop,suppressed);}
 auto verify=[&](const std::vector<int32_t>&values){auto a=r.attributes.resolve(r.graph,r.elements,r.attributes.find(layer,"curvetypes"));if(a->values.size()!=values.size())throw std::runtime_error("journal array size mismatch");for(size_t i=0;i<values.size();++i)if(wireInt(a->values[i])!=values[i])throw std::runtime_error("journal value mismatch");};verify({9,9,9});
 size_t notices=0;auto notify=[&](unsigned,uint32_t,int32_t,int32_t){++notices;if(recoveredUndoRecordingGate(journal.flags,true,1))throw std::runtime_error("journal replay recording enabled");};
 auto h=r.attributes.find(layer,"curvetypes");m.owners.at(layer).attributeFlags.at(r.attributes.identity(r.graph,r.elements,h))|=0x100;
 if(replayIntegerJournal(journal,r,m,true,dirty,false,notify,suppressed)!=4||journal.undo.size()!=0||journal.redo.size()!=4)throw std::runtime_error("journal reverse count mismatch");verify({4,8});
 if(replayIntegerJournal(journal,r,m,false,dirty,false,notify,suppressed)!=4||journal.undo.size()!=4||!journal.redo.empty())throw std::runtime_error("journal forward count mismatch");verify({9,9,9});
 if(notices!=20||journal.flags!=8||!(dirty&0x4000))throw std::runtime_error("journal side effects mismatch");
 auto identity=r.attributes.identity(r.graph,r.elements,h);m.owners.at(layer).attributeFlags.at(identity)|=0x20;appendRecordedIntegerArray(r,m,capture,layer,"curvetypes",11,dirty,false,noop,suppressed);if(journal.undo.size()!=4)throw std::runtime_error("rejected mutation captured");
 std::cout<<"integer_journal_records=4 reverse_forward=verified notifications="<<notices<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}


