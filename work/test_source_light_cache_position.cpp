#include "source_light_cache_position_candidate.h"
#include <iostream>
#include <limits>
#include <string>
#include <vector>
using Point=std::array<float,3>;
struct Script {
 int32_t leaf=7;uint32_t content=0;std::vector<SourceLightCacheTraceCandidate>traces;
 std::vector<std::string>events;std::vector<Point>targets;bool clipMoves=false,badClip=false;
 int32_t leafAt(const Point&){events.push_back("leaf");return leaf;}
 uint32_t contents(int32_t value){if(value!=leaf)throw std::runtime_error("wrong contents leaf");events.push_back("contents");return content;}
 void constrainToStartLeaf(const Point&,float tolerance,Point&target){if(tolerance!=0.5f)throw std::runtime_error("wrong tolerance");events.push_back("clip");if(clipMoves)target[0]+=0.25f;if(badClip)target[0]=std::numeric_limits<float>::quiet_NaN();}
 SourceLightCacheTraceCandidate trace(const Point&start,const Point&target,uint32_t mask){if(start!=Point{1,2,3}||mask!=0x4081)throw std::runtime_error("wrong trace inputs");events.push_back("trace");targets.push_back(target);if(targets.size()>traces.size())throw std::runtime_error("unexpected trace");return traces[targets.size()-1];}
};
static void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}
int main(){try{
 const Point original{1,2,3},center{16,16,64},horizontal{16,16,3};size_t cases=0;
 for(bool different:{false,true})for(uint32_t contents:{0u,1u,0x80u,0x4000u,0x4081u,2u})for(bool startsSolid:{false,true})for(float first:{0.0f,0.5f,1.0f})for(float second:{0.0f,1.0f}){
  Script s;s.leaf=different?8:7;s.content=contents;
  const bool skip=different&&(contents&0x4081u)!=0;
  s.traces=skip?std::vector<SourceLightCacheTraceCandidate>{{second,true}}:std::vector<SourceLightCacheTraceCandidate>{{first,startsSolid},{second,true}};
  const auto result=sourceLightCachePositionCandidate(original,7,s);
  const bool secondUsed=skip||(!startsSolid&&first<1.0f);
  Point expected=secondUsed?(second==1.0f?horizontal:original):(startsSolid?original:center);
  require(result==expected,"policy branch result differs");
  std::vector<std::string>events{"leaf"};if(different){events.push_back("contents");if(!skip)events.push_back("clip");}
  if(!skip)events.push_back("trace");if(secondUsed){events.push_back("clip");events.push_back("trace");}
  require(s.events==events,"policy callback order differs");
  require(s.targets.size()==size_t(skip?1:secondUsed?2:1),"trace count differs");
  require(s.targets.back()==(secondUsed?horizontal:center),"trace target differs");++cases;
 }
 Script moved;moved.leaf=8;moved.clipMoves=true;moved.traces={{1.0f,false}};
 require(sourceLightCachePositionCandidate(original,7,moved)==Point{16.25f,16,64},"constrained point ignored");++cases;
 size_t rejected=0;
 for(float fraction:{-0.1f,1.1f,std::numeric_limits<float>::quiet_NaN()}){Script bad;bad.traces={{fraction,false}};try{(void)sourceLightCachePositionCandidate(original,7,bad);}catch(const std::runtime_error&){++rejected;}}
 Script bad;bad.leaf=8;bad.badClip=true;try{(void)sourceLightCachePositionCandidate(original,7,bad);}catch(const std::runtime_error&){++rejected;}
 require(rejected==4,"invalid dependency results accepted");
 std::cout<<"PASS: "<<cases<<" scripted branch cases, callback order/mask/tolerance/targets, original/full/horizontal selection, second startSolid ignored, 4 rejected dependency results; real collision parity UNVERIFIED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
