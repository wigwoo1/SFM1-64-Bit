#include "source_world_light_visibility_candidate.h"
#include <iostream>
#include <limits>
struct Services {
    unsigned styles=0,thresholds=0,traces=0,debugs=0;int32_t styleValue=264,mode=0;float threshold=0;uintptr_t excluded=0;uint32_t filter=0;
    SourceWorldLightTraceCandidate hit;SourceWorldLightRayCandidate ray;
    std::array<std::array<float,3>,6>basis{{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}}};
    const auto&directions(){return basis;}
    int32_t directionCount(){return 6;}
    int32_t style(uint32_t){++styles;return styleValue;}
    float intensityThreshold(){++thresholds;return threshold;}
    int32_t debugMode(){return mode;}
    void debugLine(const std::array<float,3>&,const std::array<float,3>&){++debugs;}
    SourceWorldLightTraceCandidate trace(const SourceWorldLightRayCandidate&r,uint32_t mask,const SourceWorldLightFilterCandidate&f){if(mask!=0x4481)throw std::runtime_error("visibility mask");++traces;ray=r;excluded=f.excludedEntity;filter=uint32_t(f.mode);return hit;}
};
int main(){try{
    unsigned checks=0;auto require=[](bool b){if(!b)throw std::runtime_error("visibility safety assertion");};SourceWorldLightQueryCandidate l;l.angular.intensity={1,2,3};l.position={0,0,20};
    require(sourceWorldLightDistanceCandidate(l,{0,0,10},false)==1);++checks;
    l.radius=5;require(sourceWorldLightDistanceCandidate(l,{0,0,10},false)==0&&sourceWorldLightDistanceCandidate(l,{0,0,10},true)==1);++checks;
    l.radius=0;l.constant=-2;require(sourceWorldLightDistanceCandidate(l,{0,0,10},false)==-.5f);++checks;
    l.angular.type=0;require(sourceWorldLightDistanceCandidate(l,{0,0,.5f},false)==1&&sourceWorldLightDistanceCandidate(l,{0,0,2},false)==.25f);++checks;
    l.radius=1;require(sourceWorldLightDistanceCandidate(l,{0,0,2},true)==0);++checks;
    l.angular.type=4;l.linear=12;require(sourceWorldLightDistanceCandidate(l,{0,0,10},false)==2&&sourceWorldLightDistanceCandidate(l,{0,0,20},false)==0);++checks;
    l.angular.type=1;l.radius=0;l.constant=1;l.linear=0;Services visible;visible.hit.fraction=.6f;auto q=sourceWorldLightVisibilityQueryCandidate(l,{0,0,0},0,0,visible);require(q.attenuation==1&&visible.traces==1&&visible.styles==1&&visible.filter==1&&q.incoming[2]==1);++checks;
    Services blocked;blocked.hit.fraction=.5f;blocked.mode=2;q=sourceWorldLightVisibilityQueryCandidate(l,{0,0,0},0,0,blocked);require(q.attenuation==0&&blocked.traces==1&&blocked.debugs==1);++checks;
    Services skip;q=sourceWorldLightVisibilityQueryCandidate(l,{0,0,0},9,0,skip);require(q.attenuation==1&&skip.styles==0&&skip.traces==0);++checks;
    Services wide;sourceWorldLightVisibilityQueryCandidate(l,{0,0,0},4,uintptr_t(0x123400001234ull),wide);require(wide.excluded==0x123400001234ull&&wide.filter==3);++checks;
    Services culled;culled.threshold=4;q=sourceWorldLightVisibilityQueryCandidate(l,{0,0,0},0,0,culled);require(q.attenuation==0&&q.incoming[2]==20&&culled.traces==0);++checks;
    l.angular.type=0;Services surface;surface.threshold=100;q=sourceWorldLightVisibilityQueryCandidate(l,{0,0,0},1,0,surface);require(surface.thresholds==0&&q.attenuation>0);++checks;
    l.angular.type=3;l.angular.normal={0,0,-1};Services sky;sky.hit.surfaceFlags=4;q=sourceWorldLightVisibilityQueryCandidate(l,{0,0,0},9,0,sky);require(q.attenuation==1&&sky.traces==1&&sky.styles==0&&sky.thresholds==0&&q.incoming[2]==1);++checks;
    l.angular.type=5;Services ambient;q=sourceWorldLightVisibilityQueryCandidate(l,{0,0,0},0,0,ambient,{11,22,33});require(q.attenuation==0&&q.incoming==std::array<float,3>{11,22,33}&&ambient.traces==0&&ambient.styles==0);++checks;
    l.angular.type=1;l.constant=0;bool rejected=false;try{sourceWorldLightDistanceCandidate(l,{0,0,1},true);}catch(const std::runtime_error&){rejected=true;}require(rejected);++checks;
    l.constant=1;Services bad;bad.hit.fraction=std::numeric_limits<float>::quiet_NaN();rejected=false;try{sourceWorldLightVisibilityQueryCandidate(l,{0,0,0},0,0,bad);}catch(const std::runtime_error&){rejected=true;}require(rejected);++checks;
    rejected=false;try{sourceWorldLightVisibilityQueryCandidate(l,{0,0,0},16,0,bad);}catch(const std::runtime_error&){rejected=true;}require(rejected);++checks;
    l.angular.type=1;Services complete;complete.hit.fraction=1;SourceModelLightingStateCandidate state;state.lights={uintptr_t(0x123400001234ull)};sourceWorldLightCompleteOverflowCandidate(l,{0,0,0},state,false,complete);require(complete.traces==1&&state.ambient[12]==1&&state.ambient[13]==2&&state.ambient[14]==3&&state.lights[0]==0x123400001234ull);++checks;
    Services blockedComplete;blockedComplete.hit.fraction=0;const auto before=state.ambient;sourceWorldLightCompleteOverflowCandidate(l,{0,0,0},state,false,blockedComplete);require(blockedComplete.traces==1&&state.ambient==before);++checks;
    std::cout<<"PASS world light visibility safety checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
