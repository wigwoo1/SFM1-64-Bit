#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_timeline_document_candidate.h"
#include "retained_session_scene_candidate.h"
#include "gameinfo_mounts_candidate.h"
void compare(const PreparedSessionSceneCandidate&a,const PreparedSessionSceneCandidate&b){
 if(a.camera.rows!=b.camera.rows||a.upload.indices!=b.upload.indices||a.materialAlpha!=b.materialAlpha||a.upload.vertices.size()!=b.upload.vertices.size()||a.materialInputs.size()!=b.materialInputs.size())throw std::runtime_error("scene changed after transaction");
 for(size_t i=0;i<a.upload.vertices.size();++i)if(std::memcmp(&a.upload.vertices[i],&b.upload.vertices[i],sizeof(SceneGpuVertexCandidate)))throw std::runtime_error("vertex changed after transaction");
 for(size_t i=0;i<a.materialInputs.size();++i)if(&a.materialInputs[i]!=&b.materialInputs[i])throw std::runtime_error("material identity changed after transaction");
}
int main(){try{
 SessionTimelineDocumentCandidate doc(loadGraph("work/roundtrip-11.dmx"));
 std::filesystem::path content=R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod)";
 std::ifstream file(content/"gameinfo.txt",std::ios::binary);std::string text((std::istreambuf_iterator<char>(file)),{});
 RetainedSessionSceneCandidate renderer(Graph(doc.graph()),2237,gameinfoMountsCandidate(text,content,content.parent_path()),4,true,true);
 auto frameId=hierarchyRefs(doc.graph(),doc.graph().elements[2470],"timeFrame",1)[0];AnimationTimeFrame frame(doc.graph(),doc.graph().elements[frameId]);auto before=renderer.sample(frame.start+500);
 auto publish=[&](const Graph&graph){renderer.replaceSession(Graph(graph),2237);};
 auto trim=[&](SessionTimelineDocumentCandidate&staged){staged.trimClipStart(2470,frame.start+166);return true;};
 auto reject=[&](const Graph&graph){renderer.replaceSession(Graph(graph),UINT32_MAX);};
 bool failed=false;try{doc.transact(trim,reject);}catch(const std::exception&){failed=true;}
 if(!failed||doc.dirty()||doc.undo()!=0)throw std::runtime_error("rejected trim committed document/history");compare(before,renderer.sample(frame.start+500));
 if(!doc.transact(trim,publish)||!doc.dirty())throw std::runtime_error("trim retry failed");
 auto undo=[](SessionTimelineDocumentCandidate&staged){return staged.undo()!=0;};
 failed=false;try{doc.transact(undo,reject);}catch(const std::exception&){failed=true;}
 if(!failed||!doc.dirty())throw std::runtime_error("rejected undo changed document");compare(before,renderer.sample(frame.start+500));
 if(!doc.transact(undo,publish)||doc.dirty())throw std::runtime_error("undo retry failed");
 if(!doc.transact([](SessionTimelineDocumentCandidate&staged){return staged.redo()!=0;},publish)||!doc.dirty())throw std::runtime_error("redo retry failed");
 compare(before,renderer.sample(frame.start+500));doc.transact(undo,publish);
 auto trimEnd=[&](SessionTimelineDocumentCandidate&staged){return staged.trimClipEnd(2470,frame.start+1000);};failed=false;try{doc.transact(trimEnd,reject);}catch(const std::exception&){failed=true;}if(!failed||doc.dirty()||doc.undo()!=0)throw std::runtime_error("rejected end trim committed document/history");compare(before,renderer.sample(frame.start+500));
 if(!doc.transact(trimEnd,publish)||!doc.dirty())throw std::runtime_error("end trim publish retry failed");auto trimmedFrameRef=hierarchyRefs(doc.graph(),doc.graph().elements[2470],"timeFrame",1).at(0);AnimationTimeFrame trimmedFrame(doc.graph(),doc.graph().elements[trimmedFrameRef]);if(trimmedFrame.duration!=1000)throw std::runtime_error("end trim duration mismatch");compare(before,renderer.sample(frame.start+500));
 failed=false;try{doc.transact(undo,reject);}catch(const std::exception&){failed=true;}if(!failed||!doc.dirty())throw std::runtime_error("rejected end trim undo changed document");compare(before,renderer.sample(frame.start+500));
 if(!doc.transact(undo,publish)||doc.dirty())throw std::runtime_error("end trim undo failed");compare(before,renderer.sample(frame.start+500));if(!doc.transact([](SessionTimelineDocumentCandidate&staged){return staged.redo()!=0;},publish)||!doc.dirty())throw std::runtime_error("end trim redo failed");doc.transact(undo,publish);
 const float inputBefore=wireFloat(namedAttribute(doc.graph(),doc.graph().elements.at(3743),"value")->values.at(0));const float inputAfter=inputBefore==0.25f?0.75f:0.25f;auto scalarEdit=[&](auto&staged){return staged.setScalarFloat(3743,"value",inputAfter);};failed=false;try{doc.transact(scalarEdit,reject);}catch(const std::exception&){failed=true;}if(!failed||doc.dirty()||wireFloat(namedAttribute(doc.graph(),doc.graph().elements.at(3743),"value")->values.at(0))!=inputBefore)throw std::runtime_error("rejected scalar publish changed document");compare(before,renderer.sample(frame.start+500));
 if(!doc.transact(scalarEdit,publish)||!doc.dirty())throw std::runtime_error("scalar publish retry failed");renderer.sample(frame.start+500);if(!doc.transact(undo,publish)||doc.dirty())throw std::runtime_error("scalar publish undo failed");compare(before,renderer.sample(frame.start+500));
 std::cout<<"editor transaction passed rejected_trim rejected_undo preserved_document_history_scene retry_trim_undo_redo trim_end_atomic_undo_redo retained_material_identity scalar_publish_rejection_retry_undo\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
