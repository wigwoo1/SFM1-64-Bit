#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_timeline_document_candidate.h"
#include "session_track_group_rows_candidate.h"
#include <iostream>

int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("usage: test_session_track_group_rows session.dmx");
 auto graph=loadGraph(argv[1]);auto rows=sessionTrackGroupRowsCandidate(graph);
 const auto root=sessionRenderRootCandidate(graph);const auto&clip=graph.elements.at(root);
 auto special=hierarchyRefs(graph,clip,"subClipTrackGroup",1);auto regular=hierarchyRefs(graph,clip,"trackGroups",15);
 size_t expected=special.size()+regular.size();if(rows.size()!=expected)throw std::runtime_error("track group row count mismatch");
 size_t cursor=0;if(!special.empty()){if(rows[cursor].element!=special[0]||!rows[cursor].special)throw std::runtime_error("special sub-clip row is not first");++cursor;}
 for(auto id:regular)if(rows.at(cursor++).element!=id)throw std::runtime_error("regular track group row order mismatch");
 if(rows.empty())throw std::runtime_error("benchmark unexpectedly has no track groups");
 SessionTimelineDocumentCandidate doc(graph);const auto row=rows.front();
 if(doc.setBooleanAttribute(row.element,"visible",!row.visible)==false)throw std::runtime_error("visibility edit was a no-op");
 if(doc.renameElement(row.element,row.name+" test") == false)throw std::runtime_error("name edit was a no-op");
 if(!doc.dirty()||doc.undo()!=1||doc.undo()!=1||doc.dirty())throw std::runtime_error("group edits did not undo as separate transactions");
 if(doc.redo()!=1||doc.redo()!=1||!doc.dirty())throw std::runtime_error("group edits did not redo");
 std::cout<<"track_group_rows="<<rows.size()<<" special_first="<<(!special.empty())<<" visibility_name_edit_undo_redo=passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
