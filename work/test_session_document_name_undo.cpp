#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_timeline_document_candidate.h"
#include <iostream>
int main(){try{
 SessionTimelineDocumentCandidate doc(loadGraph("work/roundtrip-11.dmx"));uint32_t target=UINT32_MAX;
 for(uint32_t i=0;i<doc.graph().elements.size();++i)if(doc.graph().strings.at(doc.graph().elements[i].type)=="DmeGameModel"){target=i;break;}
 if(target==UINT32_MAX)throw std::runtime_error("DmeGameModel fixture missing");
 const auto original=doc.graph().strings.at(doc.graph().elements[target].name);bool failed=false;
 try{doc.renameElement(target,"");}catch(const std::runtime_error&){failed=true;}if(!failed||doc.dirty()||doc.undo())throw std::runtime_error("invalid rename mutated document");
 if(!doc.renameElement(target,original+" renamed" )||!doc.dirty()||doc.graph().strings.at(doc.graph().elements[target].name)!=original+" renamed")throw std::runtime_error("rename write");
 if(doc.renameElement(target,original+" renamed"))throw std::runtime_error("equal rename was not a no-op");
 if(doc.undo()!=1||doc.dirty()||doc.graph().strings.at(doc.graph().elements[target].name)!=original)throw std::runtime_error("rename undo");
 if(doc.redo()!=1||!doc.dirty()||doc.graph().strings.at(doc.graph().elements[target].name)!=original+" renamed")throw std::runtime_error("rename redo");
 const auto path="work/name-undo-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64())+".dmx";doc.saveAs(path.c_str());if(doc.dirty())throw std::runtime_error("saved rename remains dirty");
 auto reopened=loadGraph(path.c_str());if(reopened.elements.at(target).name>=reopened.strings.size()||reopened.strings.at(reopened.elements.at(target).name)!=original+" renamed")throw std::runtime_error("renamed DMX reopen");
 if(doc.undo()!=1||!doc.dirty()||doc.graph().strings.at(doc.graph().elements[target].name)!=original)throw std::runtime_error("undo after save");
 if(doc.redo()!=1||doc.dirty())throw std::runtime_error("redo to saved name should be clean");
 SessionTimelineDocumentCandidate publicationDoc(loadGraph("work/roundtrip-11.dmx"));uint32_t publishTarget=target;const auto before=publicationDoc.graph().strings.at(publicationDoc.graph().elements[publishTarget].name);failed=false;try{publicationDoc.transact([&](auto&staged){staged.renameElement(publishTarget,"rejected publication");return true;},[](const Graph&){throw std::runtime_error("publication failed");});}catch(const std::runtime_error&){failed=true;}
 if(!failed||publicationDoc.dirty()||publicationDoc.graph().strings.at(publicationDoc.graph().elements[publishTarget].name)!=before||publicationDoc.undo()!=0)throw std::runtime_error("failed publication committed rename or history");
 std::filesystem::remove(path);
 std::cout<<"document rename passed target="<<target<<" invalid_no_mutation no_op undo_redo dirty_baseline save_reopen undo_after_save redo_to_saved_state failed_publish_atomicity\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\\n';return 1;}}
