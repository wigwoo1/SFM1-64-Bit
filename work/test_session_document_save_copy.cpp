#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_timeline_document_candidate.h"
#include <iostream>
#include <thread>
#include <chrono>
static std::string bytes(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);return std::string(std::istreambuf_iterator<char>(f),{});}
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("usage: test_session_document_save_copy TEST_DIRECTORY");
 const auto dir=std::filesystem::absolute(argv[1]);std::filesystem::create_directories(dir);
 const auto source=dir/"document-source-copy.dmx",fresh=dir/"document-fresh-copy.dmx",existing=dir/"document-existing.dmx",race=dir/"document-race-target.dmx";
 for(const auto&p:{source,fresh,existing,race})std::filesystem::remove(p);
 saveGraph(loadGraph("work/roundtrip-11.dmx"),source.string().c_str());const auto sourceBefore=bytes(source);
 SessionTimelineDocumentCandidate doc(loadGraph(source.string().c_str()));uint32_t target=UINT32_MAX;
 for(uint32_t i=0;i<doc.graph().elements.size();++i)if(doc.graph().strings.at(doc.graph().elements[i].type)=="DmeGameModel"){target=i;break;}
 if(target==UINT32_MAX)throw std::runtime_error("DmeGameModel fixture missing");
 const auto original=doc.graph().strings.at(doc.graph().elements[target].name);
 if(!doc.renameElement(target,original+" edited")||!doc.dirty())throw std::runtime_error("test edit did not become dirty");
 if(doc.saveCopy(nullptr)||!doc.dirty())throw std::runtime_error("cancel changed document state");
 if(doc.undo()!=1||doc.dirty()||doc.graph().strings.at(doc.graph().elements[target].name)!=original)throw std::runtime_error("history lost after cancel");
 if(doc.redo()!=1||!doc.dirty()||doc.graph().strings.at(doc.graph().elements[target].name)!=original+" edited")throw std::runtime_error("redo lost after cancel");
 const std::string sentinel="preserve existing destination";{std::ofstream f(existing,std::ios::binary);f<<sentinel;}
 const auto existingBefore=bytes(existing);bool failed=false;
 try{doc.saveCopy(existing.string().c_str());}catch(const std::runtime_error&){failed=true;}
 if(!failed||bytes(existing)!=existingBefore||!doc.dirty())throw std::runtime_error("failed save changed target or dirty state");
 if(doc.undo()!=1||doc.dirty()||doc.redo()!=1||!doc.dirty())throw std::runtime_error("history lost after failed save");
 const std::string raceSentinel="preserve destination created during save";std::atomic<bool>raceCreated{false};
 std::thread competingWriter([&]{const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);while(std::chrono::steady_clock::now()<deadline&&!raceCreated.load()){for(const auto&entry:std::filesystem::directory_iterator(dir)){if(entry.path().filename().string().find(".sfm64-save-")==0){std::ofstream f(race,std::ios::binary);f<<raceSentinel;f.close();raceCreated=true;break;}}std::this_thread::sleep_for(std::chrono::milliseconds(1));}});
 failed=false;try{doc.saveCopy(race.string().c_str());}catch(const std::runtime_error&){failed=true;}competingWriter.join();
 if(!failed||!raceCreated||bytes(race)!=raceSentinel||!doc.dirty())throw std::runtime_error("commit race changed destination or document state");
 if(doc.undo()!=1||doc.dirty()||doc.redo()!=1||!doc.dirty())throw std::runtime_error("history lost after commit race");
 failed=false;try{doc.saveCopy(source.string().c_str());}catch(const std::runtime_error&){failed=true;}
 if(!failed||bytes(source)!=sourceBefore||!doc.dirty())throw std::runtime_error("source destination rejection failed");
 if(!doc.saveCopy(fresh.string().c_str())||doc.dirty())throw std::runtime_error("fresh save did not mark document saved");
 const auto reopened=loadGraph(fresh.string().c_str());
 if(reopened.strings.at(reopened.elements.at(target).name)!=original+" edited")throw std::runtime_error("saved copy content mismatch");
 if(doc.undo()!=1||!doc.dirty()||doc.redo()!=1||doc.dirty())throw std::runtime_error("history/saved baseline incorrect after success");
 
 std::cout<<"document_copy_save_checks=cancel_dirty_history existing_rejection_dirty_history final_commit_race_dirty_history source_rejection fresh_save_reload clean_baseline\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
