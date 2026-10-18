#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "atomic_session_save_candidate.h"
#include <iostream>
#include <iterator>
static std::string bytes(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);return std::string(std::istreambuf_iterator<char>(f),{});}
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("usage: test_atomic_session_save TEST_DIRECTORY");
 const auto dir=std::filesystem::absolute(argv[1]);std::filesystem::create_directories(dir);
 const auto source=dir/"source-copy.dmx",fresh=dir/"fresh-save.dmx",existing=dir/"existing.dmx",race=dir/"race-target.dmx",stage=dir/"staged-session.dmx";
 for(const auto&p:{source,fresh,existing,race,stage})std::filesystem::remove(p);
 const auto g=loadGraph("work/roundtrip-11.dmx");saveGraph(g,source.string().c_str());const auto sourceBefore=bytes(source);
 auto invalid=g;invalid.elements[0].name=UINT32_MAX;bool failed=false;
 try{atomicSessionSaveAsCandidate(invalid,fresh.string().c_str());}catch(const std::runtime_error&){failed=true;}
 if(!failed||std::filesystem::exists(fresh))throw std::runtime_error("failed save left a destination");
 atomicSessionSaveAsCandidate(g,fresh.string().c_str());auto loaded=loadGraph(fresh.string().c_str());
 if(loaded.elements.size()!=g.elements.size())throw std::runtime_error("fresh save reload mismatch");
 const std::string sentinel="preserve this existing destination";{std::ofstream f(existing,std::ios::binary);f<<sentinel;}
 const auto existingBefore=bytes(existing);failed=false;
 try{atomicSessionSaveAsCandidate(g,existing.string().c_str());}catch(const std::runtime_error&){failed=true;}
 if(!failed||bytes(existing)!=existingBefore)throw std::runtime_error("existing destination changed");
 failed=false;try{atomicSessionSaveAsCandidate(g,source.string().c_str());}catch(const std::runtime_error&){failed=true;}
 if(!failed||bytes(source)!=sourceBefore)throw std::runtime_error("source destination not rejected");
 // Model the race: staging has completed, then another writer creates the
 // destination immediately before the same final commit helper used above.
 {std::ofstream f(stage,std::ios::binary);f<<"staged DMX bytes";}
 {std::ofstream f(race,std::ios::binary);f<<sentinel;}
 const auto raceBefore=bytes(race);failed=false;
 try{commitSessionSaveWithoutReplacementCandidate(stage,race);}catch(const std::runtime_error&){failed=true;}
 if(!failed||bytes(race)!=raceBefore||!std::filesystem::exists(stage))throw std::runtime_error("final commit replaced raced destination");
 std::filesystem::remove(stage);
 std::cout<<"copy_only_save_checks=fresh_reload existing_target_unchanged source_as_target final_commit_race_refusal\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
