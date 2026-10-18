#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "render_settings_session_candidate.h"
#include "atomic_session_save_candidate.h"
#include <chrono>
int main(int argc,char**argv){try{
 if(argc==3&&std::string(argv[1])=="--inspect"){auto settings=readRenderSettingsCandidate(loadGraph(argv[2]));if(!settings)throw std::runtime_error("Missing saved render settings");std::cout<<renderSettingsJsonCandidate(*settings)<<'\n';return 0;}
 auto original=loadGraph(argc>1?argv[1]:"work/roundtrip-11.dmx");if(readRenderSettingsCandidate(original))throw std::runtime_error("Unexpected original settings");
 unsigned count=0;PlaybackRenderSettingsCandidate settings;
 for(int renderer=0;renderer<3;++renderer)for(int resolution=0;resolution<3;++resolution)for(int flags=0;flags<4;++flags){settings={renderer,resolution,256,.025,bool(flags&1),bool(flags&2)};auto bytes=encodeRenderSettingsCandidate(settings);if(renderSettingsJsonCandidate(decodeRenderSettingsCandidate(bytes))!=renderSettingsJsonCandidate(settings))throw std::runtime_error("Codec roundtrip");++count;}
 auto graph=original;embedRenderSettingsCandidate(graph,settings);settings.samples=512;embedRenderSettingsCandidate(graph,settings);
 auto path="work/render-settings-session-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dmx";
 atomicSessionSaveAsCandidate(graph,path.c_str());auto reloaded=readRenderSettingsCandidate(loadGraph(path.c_str()));if(!reloaded||renderSettingsJsonCandidate(*reloaded)!=renderSettingsJsonCandidate(settings)||readRenderSettingsCandidate(original))throw std::runtime_error("Saved graph roundtrip");
 auto bytes=encodeRenderSettingsCandidate(settings);unsigned rejected=0;
 for(int index=0;index<9;++index){auto invalid=bytes;if(index==0)invalid[0]='X';if(index==1)invalid.pop_back();if(index==2)invalid.push_back(0);if(index==3)invalid[8]=3;if(index==4)invalid[12]=3;if(index==5){for(int i=16;i<20;++i)invalid[i]=0;}if(index==6)invalid[19]=128;if(index==7){for(int i=20;i<28;++i)invalid[i]=255;}if(index==8)invalid[28]=4;try{decodeRenderSettingsCandidate(invalid);}catch(const std::runtime_error&){++rejected;}}
 if(rejected!=9)throw std::runtime_error("Malformed settings accepted");
 std::cout<<"PASS "<<count<<" codec cases, nine malformed rejections, attribute replacement, actual DMX save/reopen, original absent unchanged output="<<path<<"\n";return 0;
 }catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
