#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "document_time_candidate.h"
#include "session_document_root_candidate.h"
#include "session_timeline_document_candidate.h"
#include "document_engine_gate_candidate.h"
static uint32_t word(float value){uint32_t bits;std::memcpy(&bits,&value,4);return bits;}
static void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}
int main(){try{
 auto graph=loadGraph("work/movie-640x480-session.dmx");
 const auto root=sessionDocumentRootCandidate(graph);
 require(hierarchyRefs(graph,graph.elements[root],"activeClip",1).size()==1,"fixture document root schema");
 AnchoredDocumentTimeState state(10000);state.mode47c=1;
 DocumentTimeCandidate clock(root,state);uint32_t retained=word(0);
 require(clock.sample(graph,true,word(2),retained)==10000,"forward sample");
 require(clock.sample(graph,true,word(1),retained)==10000&&retained==word(2),"backward retention");
 require(clock.sample(graph,true,word(3),retained)==20000,"next forward sample");
 Graph unavailable;
 clock.state().anchor430=0x80000000u;clock.state().saved684=201;
 require(clock.sample(unavailable,true,word(4),retained)==201&&retained==word(3),"sentinel lazy clock");
 clock.state().flag6c0=false;
 require(clock.sample(unavailable,false,word(5),retained)==201&&retained==word(3),"saved lazy settings");
 clock.state().flag6c0=true;
 const auto fps=documentFrameRateLookupCandidate(graph,root);require(bool(fps),"fixture rate absent");
 require(clock.sample(graph,false,word(6),retained)==recoveredTimeSnapCandidate(201,recoveredFloatRateCandidate(*fps)),"fixture snap");
 auto original=graph.elements[root].attributes;graph.elements[root].attributes.clear();
 require(clock.sample(graph,false,word(7),retained)==0&&retained==word(3),"settings change default");
 graph.elements[root].attributes=original;
 // Shared engine state survives sampling from a second document.
 DocumentTimeCandidate second(root,state);
 require(second.sample(graph,true,word(2),retained)==20000&&retained==word(3),"shared clock retention");
 require(clock.restoredRoot()==root,"root binding");
 // Another activeClip-bearing element must not change loader root identity.
 graph.elements.push_back(graph.elements[root]);
 require(sessionDocumentRootCandidate(graph)==root,"root inferred from activeClip uniqueness");
 bool rejected=false;try{Graph empty;empty.header=graph.header;sessionDocumentRootCandidate(empty);}catch(const std::runtime_error&){rejected=true;}
 require(rejected,"empty document accepted");
 SessionTimelineDocumentCandidate document(loadGraph("work/movie-640x480-session.dmx"));
 AnchoredDocumentTimeState savedState(0);savedState.saved684=201;savedState.flag6c0=true;
 const auto settings=hierarchySingleReference(document.graph(),document.graph().elements.at(document.documentRoot()),"settings");
 const auto renderSettings=hierarchySingleReference(document.graph(),document.graph().elements.at(settings),"renderSettings");
 const auto before=document.documentTime(savedState,false,word(8),retained);
 require(document.setScalarFloat(renderSettings,"frameRate",25),"rate edit missing");
 require(document.documentTime(savedState,false,word(8),retained)==400,"document edited rate");
 require(document.undo()==1&&document.documentTime(savedState,false,word(8),retained)==before,"document rate undo");
 require(document.redo()==1&&document.documentTime(savedState,false,word(8),retained)==400,"document rate redo");
 require(retained==word(3),"document settings sample changed engine clock");
 unsigned gateChecks=0;
 for(unsigned flag=0;flag<256;++flag)for(int32_t client:{INT32_MIN,-1,0,1,2,3,INT32_MAX}){
  const bool expected=flag!=0&&client>=2;
  require(documentEngineGateCandidate(uint8_t(flag),client)==expected,"engine gate signed threshold");++gateChecks;
 }
 AnchoredDocumentTimeState liveState(10000);liveState.mode47c=1;liveState.saved684=77;
 require(document.documentTime(liveState,documentEngineGateCandidate(1,2),word(4),retained)==30000,"recovered gate anchored dispatch");
 require(document.documentTime(liveState,documentEngineGateCandidate(1,1),word(5),retained)==77&&retained==word(4),"recovered gate fallback dispatch");
 std::cout<<"engine_gate_boundary_checks="<<gateChecks<<" integrated_gate_dispatch_checks=2\n";
 std::cout<<"document_time_component_sequence_checks=16 real_session_settings=passed shared_engine_retention=passed binary_document_root=first_element document_rate_edit_undo_redo=passed\n";
 return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
