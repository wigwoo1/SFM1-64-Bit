#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_timeline_document_candidate.h"
#include "render_settings_candidate.h"
#include <chrono>
#include <fstream>
int main(){try{
 SessionTimelineDocumentCandidate doc(loadGraph("work/roundtrip-11.dmx"));auto original=doc.frameCount();
 doc.setDuration(10000);if(!doc.dirty()||doc.frameCount()!=60)throw std::runtime_error("Edit setup failed");
 auto path="work/export-snapshot-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dmx";
 doc.captureExportSnapshot(path.c_str(),[](Graph&copy){SessionTimelineDocumentCandidate staged(std::move(copy));staged.setDuration(20000);copy=staged.graph();});
 SessionTimelineDocumentCandidate exported(loadGraph(path.c_str()));
 if(exported.frameCount()!=120||doc.frameCount()!=60||!doc.dirty())throw std::runtime_error("Export altered live document or missed decoration");
 bool rejected=false;try{doc.captureExportSnapshot(path.c_str());}catch(const std::runtime_error&){rejected=true;}
 if(!rejected||!doc.dirty()||doc.frameCount()!=60)throw std::runtime_error("Failed capture changed live state");
 if(doc.undo()!=1||doc.frameCount()!=original||doc.dirty()||doc.redo()!=1||doc.frameCount()!=60||!doc.dirty())throw std::runtime_error("Export disturbed undo/redo or savepoint");
 std::ofstream settings("outputs/cycles-bridge/native-render-settings.jsonl");
 for(int resolution=0;resolution<3;++resolution){PlaybackRenderSettingsCandidate value;value.renderer=2;value.resolution=resolution;value.samples=256;value.noiseThreshold=.025;settings<<renderSettingsJsonCandidate(value)<<'\n';}
 std::cout<<"PASS edited snapshot copy decoration, unchanged live dirty/savepoint/undo/redo, capture failure isolation, native settings serialization\n";
 return 0;
 }catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
