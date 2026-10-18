#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "create_runtime_float_log.h"
#include "create_runtime_float_curveinfo.h"
#include "initialize_float_layer_keys.h"
#include "write_float_log_curveinfo.h"
#include "scalar_reference_undo_record.h"
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;RuntimeFloatLayerState state(r);std::array<char,16>id{},lid{},cid{};id.fill(0x76);lid.fill(0x77);cid.fill(0x78);auto count=r.graph.elements.size();auto log=createRuntimeFloatLog(r,m,id,lid,4,&state);auto layer=r.elements.graphHandles.at(count+1);initializeEmptyFloatLayerKeys(r,layer,{{0,1.0f},{10,2.0f}});auto target=createRuntimeFloatCurveInfo(r,m,cid,"scalar undo test",4);
 RuntimeUndoRetention retention(r);retention.records[target]={0,0,1};uint32_t dirty=0;auto write=[&](uint32_t h,const std::string&name,uint32_t next){if(name!="curveinfo")throw std::runtime_error("unexpected replay name");return writeFloatLogCurveInfo(r,m,state,h,next,false,[](){return false;},false,&dirty);};
 {RuntimeIntegerUndoCapture capture;capture.retention=&retention;capture.fileIds[log]=1;capture.fileIds[layer]=1;capture.journal.beginBoundary();
 if(!writeFloatLogCurveInfo(r,m,state,log,target,false,[](){return false;},false,&dirty,&capture)||capture.journal.undo.size()!=4)throw std::runtime_error("automatic mixed capture failed");
 auto scalar=std::get_if<std::shared_ptr<RuntimeScalarReferenceUndoRecord>>(&capture.journal.undo[0].record);
 if(!scalar||!capture.journal.undo[0].boundary||(*scalar)->before.target!=0xffffffff||(*scalar)->after.target!=target||!std::holds_alternative<RuntimeIntegerClearRecord>(capture.journal.undo[1].record)||!std::holds_alternative<RuntimeIntegerAppendRecord>(capture.journal.undo[2].record))throw std::runtime_error("mixed record ordering failed");
 if(writeFloatLogCurveInfo(r,m,state,log,target,false,[](){return false;},false,&dirty,&capture)||capture.journal.undo.size()!=4)throw std::runtime_error("unchanged scalar captured");
 auto replayWrite=[&](uint32_t h,const std::string&name,uint32_t next){if(name!="curveinfo")throw std::runtime_error("unexpected mixed replay name");return writeFloatLogCurveInfo(r,m,state,h,next,false,[](){return false;},false,&dirty,&capture);};
 auto notify=[&](unsigned slot,uint32_t attr,int32_t first,int32_t last){notifyRecoveredFloatLayer(r,slot,attr,first,last);};
 auto curves=[&](){return r.attributes.resolve(r.graph,r.elements,r.attributes.find(layer,"curvetypes"))->values.size();};
 if(replayMixedJournal(capture.journal,r,m,true,dirty,false,notify,[](){return false;},replayWrite)!=4||curves()!=0||runtimeLayerDefaultCurveType(state,layer)||capture.journal.redo.size()!=4)throw std::runtime_error("mixed reverse failed");
 if(replayMixedJournal(capture.journal,r,m,false,dirty,false,notify,[](){return false;},replayWrite)!=4||curves()!=2||runtimeLayerDefaultCurveType(state,layer)!=0||capture.journal.undo.size()!=4||!capture.journal.redo.empty()||capture.journal.flags!=8)throw std::runtime_error("mixed forward failed");
 std::array<char,16>replacementId{};replacementId.fill(0x79);auto replacement=createRuntimeFloatCurveInfo(r,m,replacementId,"replacement curve info",4);retention.records[replacement]={0,0,1};
 auto defaultField=r.attributes.resolve(r.graph,r.elements,r.attributes.find(replacement,"defaultCurveType"));setWireU32(defaultField->values[0],7);
 auto verifyCurves=[&](int32_t value){auto a=r.attributes.resolve(r.graph,r.elements,r.attributes.find(layer,"curvetypes"));if(a->values.size()!=2||wireInt(a->values[0])!=value||wireInt(a->values[1])!=value||runtimeLayerDefaultCurveType(state,layer)!=value)throw std::runtime_error("mixed replacement curve values failed");};
 capture.journal.beginBoundary();if(!replayWrite(log,"curveinfo",replacement)||capture.journal.undo.size()!=8)throw std::runtime_error("replacement group capture failed");verifyCurves(7);
 if(replayMixedJournal(capture.journal,r,m,true,dirty,false,notify,[](){return false;},replayWrite)!=4||capture.journal.undo.size()!=4)throw std::runtime_error("replacement group reverse failed");verifyCurves(0);
 if(replayMixedJournal(capture.journal,r,m,false,dirty,false,notify,[](){return false;},replayWrite)!=4||capture.journal.undo.size()!=8)throw std::runtime_error("replacement group forward failed");verifyCurves(7);
 capture.journal.beginBoundary();if(!replayWrite(log,"curveinfo",0xffffffff)||capture.journal.undo.size()!=10||curves()!=0)throw std::runtime_error("null group capture failed");
 if(replayMixedJournal(capture.journal,r,m,true,dirty,false,notify,[](){return false;},replayWrite)!=2||capture.journal.undo.size()!=8)throw std::runtime_error("null group reverse failed");verifyCurves(7);
 if(replayMixedJournal(capture.journal,r,m,false,dirty,false,notify,[](){return false;},replayWrite)!=2||capture.journal.undo.size()!=10||curves()!=0||runtimeLayerDefaultCurveType(state,layer))throw std::runtime_error("null group forward failed");
 capture.journal.undo.clear();if(retention.records[target].packed||retention.records[replacement].packed)throw std::runtime_error("mixed history leaked retention");
 }write(log,"curveinfo",0xffffffff);
 {auto record=captureScalarReferenceRecord(r,retention,log,"curveinfo",target);if(!write(log,"curveinfo",target))throw std::runtime_error("initial reference write failed");
 auto curves=[&](){return r.attributes.resolve(r.graph,r.elements,r.attributes.find(layer,"curvetypes"))->values.size();};
 if(curves()!=2||!replayScalarReferenceRecord(r,m,record,true,write)||curves()!=0||runtimeLayerDefaultCurveType(state,layer))throw std::runtime_error("scalar reverse replay failed");
 if(!replayScalarReferenceRecord(r,m,record,false,write)||curves()!=2||runtimeLayerDefaultCurveType(state,layer)!=0)throw std::runtime_error("scalar forward replay failed");
 if((retention.records[target].packed>>20)!=1)throw std::runtime_error("record lost target retention");auto h=r.attributes.find(log,"curveinfo");m.owners.at(log).attributeFlags.at(r.attributes.identity(r.graph,r.elements,h))|=0x20;if(replayScalarReferenceRecord(r,m,record,true,write))throw std::runtime_error("read only replay accepted");}
 if(retention.records[target].packed!=0)throw std::runtime_error("record destruction leaked retention");std::cout<<"mixed_reference_groups=3 records=4,4,2 reverse_forward_values=verified retention=zero\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

