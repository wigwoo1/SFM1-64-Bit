#include "gameinfo_mounts_candidate.h"
#include <fstream>
#include <iostream>
int main(){
 try{
  std::filesystem::path game=R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game)";
  std::ifstream f(game/"usermod/gameinfo.txt",std::ios::binary);
  if(!f)throw std::runtime_error("installed GameInfo missing");
  std::string source((std::istreambuf_iterator<char>(f)),{});
  auto plan=gameinfoMountPlanCandidate(source,game/"usermod",game);
  auto&mounts=plan.gamePaths;
  std::vector<std::string>expected{"usermod","tf_movies","tf","left4dead2_movies","hl2","workshop","fluidscriptstudios_tools"};
  if(mounts.size()!=expected.size()*2)throw std::runtime_error("localized mount count");
  for(size_t n=0;n<expected.size();++n){
   auto localized=game/(expected[n]+"_english");auto base=game/expected[n];
   if(mounts[n*2]!=localized||mounts[n*2+1]!=base)throw std::runtime_error("localized/base mount order got="+mounts[n*2].string()+" then="+mounts[n*2+1].string()+" expected="+localized.string()+" then="+base.string());
   if(!std::filesystem::is_directory(base))throw std::runtime_error("declared mount missing: "+base.string());
   std::cout<<n<<" "<<localized.string()<<" -> "<<base.string()<<"\n";
  }
  if(plan.gameBinPaths.size()!=expected.size())throw std::runtime_error("GameBin mount count");
  for(size_t n=0;n<expected.size();++n)if(plan.gameBinPaths[n]!=game/expected[n]/"bin")throw std::runtime_error("GameBin mount order");
  if(plan.modPath!=game/"usermod"||plan.defaultWritePath!=game/"usermod")throw std::runtime_error("first Game mount aliases");
  auto noLanguage=gameinfoMountsCandidate(source,game/"usermod",game,"");
  if(noLanguage.size()!=expected.size())throw std::runtime_error("no-language mount count");
  for(size_t n=0;n<expected.size();++n)if(noLanguage[n]!=game/expected[n])throw std::runtime_error("no-language mount order");
  size_t rejected=0;
  for(auto value:{"../outside","|unknown|foo","C:/absolute","*"}){
   bool failed=false;
   try{gameinfoMountsCandidate(std::string("GameInfo { FileSystem { SearchPaths { Game \"")+value+"\" } } }",game/"usermod",game);}catch(std::runtime_error&){failed=true;}
   if(!failed)throw std::runtime_error("unsupported path accepted");++rejected;
  }
  bool badLanguage=false;try{gameinfoMountsCandidate(source,game/"usermod",game,"../english");}catch(std::runtime_error&){badLanguage=true;}
  if(!badLanguage)throw std::runtime_error("unsafe language suffix accepted");
  const std::string explicitBins="GameInfo { FileSystem { SearchPaths { GameBin \"bin_custom\" Game \"mod\" GameBin \"|gameinfo_path|dlls\" } } }";
  auto explicitPlan=gameinfoMountPlanCandidate(explicitBins,game/"usermod",game,"");
  if(explicitPlan.gameBinPaths.size()!=3||explicitPlan.gameBinPaths[0]!=game/"bin_custom"||explicitPlan.gameBinPaths[1]!=game/"mod"/"bin"||explicitPlan.gameBinPaths[2]!=game/"usermod"/"dlls")throw std::runtime_error("explicit/derived GameBin order");
  if(explicitPlan.gamePaths.size()!=1||explicitPlan.gamePaths[0]!=game/"mod")throw std::runtime_error("Game paths contaminated by GameBin");
  std::cout<<"declared mount order verified paths="<<expected.size()<<" localized_overlays="<<expected.size()<<" rejected_paths="<<rejected<<" unsafe_language_rejected=true gamebin_paths="<<plan.gameBinPaths.size()<<" explicit_gamebin_order=true mod_alias=true default_write_alias=true native_full_mount_parity=unverified\n";
  return 0;
 }catch(std::exception&e){std::cerr<<e.what()<<"\n";return 1;}
}
