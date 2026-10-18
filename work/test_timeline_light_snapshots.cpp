#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "retained_shot_timeline_candidate.h"
#include "film_scene_render_binding_candidate.h"
#include "session_render_light_candidate.h"
#include "channel_clip_time.h"
#include <iostream>
int main(){try{
 static_assert(sizeof(void*)==8);auto baseline=loadGraph("work/roundtrip-11.dmx");
 auto paths=channelTimePaths(baseline,2237);auto found=paths.find(3882);if(found==paths.end()||found->second.size()!=1)throw std::runtime_error("light channel time path");
 const auto& frames=found->second.front();for(auto id:frames){AnimationTimeFrame f(baseline,baseline.elements.at(id));std::cout<<"channel_frame="<<id<<" start="<<f.start<<" duration="<<f.duration<<" offset="<<f.offset<<" scale="<<f.scale<<'\n';}
 std::vector<int32_t> probeTicks;
 for(int32_t key:{49999,50000}){int32_t low=0,high=2000000;while(low<high){auto mid=low+(high-low)/2;if(channelLocalTime(baseline,frames,mid,false)<key)low=mid+1;else high=mid;}
  std::cout<<"key_tick="<<key<<" root_tick="<<low<<" mapped_tick="<<channelLocalTime(baseline,frames,low,false)<<'\n';probeTicks.push_back(low);}
 if(probeTicks!=std::vector<int32_t>{870832,870833})throw std::runtime_error("fixture key mapping changed");
 auto beforeShot=activeFilmShotsCandidate(baseline,2237,probeTicks[0]);for(auto shot:beforeShot)if(shot.clip==2470)throw std::runtime_error("shot active before boundary");
 if(selectFilmShotCandidate(baseline,2237,probeTicks[1]).clip!=2470)throw std::runtime_error("shot inactive at boundary");
 std::cout<<"light_key_boundary=shot_activation prior_key_outside_selected_shot=yes\n";
 RetainedShotTimelineCandidate serial(baseline,2237),parallel(baseline,2237);
 size_t snapshots=0,changes=0;std::map<uint32_t,SessionRenderLightCandidate>previous;
 for(int32_t tick:{870833,875833,880833,984500}){
  auto a=serial.step(tick),b=parallel.step(tick,2,2);if(a.shot.clip!=b.shot.clip)throw std::runtime_error("shot mismatch");
  auto first=readFilmSceneRenderBindingCandidate(serial.graph(),a.shot.clip),second=readFilmSceneRenderBindingCandidate(parallel.graph(),b.shot.clip);
  if(first.lights!=second.lights)throw std::runtime_error("membership mismatch");
  GraphTransformHierarchy sh(serial.graph()),ph(parallel.graph());
  for(auto id:first.lights){auto s=readSessionRenderLightCandidate(serial.graph(),id,sh),p=readSessionRenderLightCandidate(parallel.graph(),id,ph);
   if(std::memcmp(s.world.data(),p.world.data(),sizeof(s.world))||s.fields.floatWords!=p.fields.floatWords||s.fields.colorWord!=p.fields.colorWord||s.fields.booleanBytes!=p.fields.booleanBytes||s.fields.textureName!=p.fields.textureName||s.fields.animationTimeWord!=p.fields.animationTimeWord)throw std::runtime_error("light snapshot worker mismatch");
   auto prior=previous.find(id);if(prior!=previous.end()&&(std::memcmp(prior->second.world.data(),s.world.data(),sizeof(s.world))||prior->second.fields.floatWords!=s.fields.floatWords||prior->second.fields.colorWord!=s.fields.colorWord))++changes;
   previous.insert_or_assign(id,s);++snapshots;
  }
  std::cout<<"tick="<<tick<<" shot="<<a.shot.clip<<" lights="<<first.lights.size()<<" executed="<<a.executed<<'\n';
 }
 if(snapshots!=12)throw std::runtime_error("expected fixture coverage");
 std::cout<<"light_snapshots="<<snapshots<<" changed_repeated_light_snapshots="<<changes<<" serial_parallel_bytes_equal=yes native_Source_animation_parity=unverified\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
