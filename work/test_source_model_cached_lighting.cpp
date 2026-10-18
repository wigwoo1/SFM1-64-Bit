#include "source_model_cached_lighting_candidate.h"
#include <iostream>
#include <limits>
struct Services {
    int32_t configured=4,first=4,second=4;unsigned calls=0,overflows=0;uintptr_t last=0;
    int32_t configuredLimit(){return configured;}
    int32_t hardwareLimit(){return calls++%2?second:first;}
    void overflow(uintptr_t id,const std::array<float,3>&,SourceModelLightingStateCandidate&s,bool special){if(special)throw std::runtime_error("overflow flag");++overflows;last=id;s.ambient[0]+=1;}
};
int main(){try{
    static_assert(sizeof(uintptr_t)==8,"native x64 safety");unsigned checks=0;auto require=[](bool b){if(!b)throw std::runtime_error("cached lighting safety assertion");};
    const uintptr_t high=0x123400000001ull;SourceModelLightingStateCandidate current;current.ambient.fill(2);current.lights={high,high,2};SourceModelInstanceLightingCandidate record;record.active=true;record.cached.ambient.fill(3);record.cached.lights={high};std::array<float,18>draw{};SourceModelLightingStateCandidate scratch;Services s;
    require(sourceModelCachedLightingCandidate(true,&record,true,current,{1,2,3},draw,scratch,s)&&draw[0]==5&&scratch.ambient[0]==0&&scratch.lights==std::vector<uintptr_t>{high,2}&&s.calls==1);++checks;
    record.cached.lights={1,2,3,4};current.lights={high};Services full;scratch={};sourceModelCachedLightingCandidate(true,&record,true,current,{0,0,0},draw,scratch,full);require(scratch.lights.size()==4&&full.calls==0&&full.overflows==0);++checks;
    record.cached.lights={};Services moving;moving.first=3;moving.second=0;scratch={};sourceModelCachedLightingCandidate(true,&record,true,current,{0,0,0},draw,scratch,moving);require(moving.calls==2&&moving.overflows==1&&moving.last==high&&scratch.lights.empty()&&scratch.ambient[0]==1&&draw[0]==5);++checks;
    Services zero;zero.configured=0;scratch={};sourceModelCachedLightingCandidate(true,&record,true,current,{0,0,0},draw,scratch,zero);require(zero.calls==1&&zero.overflows==1);++checks;
    const auto nan=std::numeric_limits<float>::quiet_NaN();current.ambient[0]=nan;
    for(unsigned gate=0;gate<4;++gate){Services unused;draw.fill(17);scratch={};scratch.lights={high};record.active=gate!=3;require(!sourceModelCachedLightingCandidate(gate!=0,gate==1?nullptr:&record,gate!=2,current,{nan,0,0},draw,scratch,unused)&&draw[0]==17&&scratch.lights==std::vector<uintptr_t>{high}&&unused.calls==0);++checks;}
    record.active=true;bool rejected=false;try{Services x;sourceModelCachedLightingCandidate(true,&record,true,current,{0,0,0},draw,scratch,x);}catch(const std::runtime_error&){rejected=true;}require(rejected);++checks;
    current.ambient.fill(std::numeric_limits<float>::max());record.cached.ambient=current.ambient;draw.fill(17);rejected=false;try{Services x;sourceModelCachedLightingCandidate(true,&record,true,current,{0,0,0},draw,scratch,x);}catch(const std::runtime_error&){rejected=true;}require(rejected&&draw[0]==17);++checks;
    current.ambient.fill(1);record.cached.ambient.fill(1);current.lights={1,2,3,4,5};rejected=false;try{Services x;sourceModelCachedLightingCandidate(true,&record,true,current,{0,0,0},draw,scratch,x);}catch(const std::runtime_error&){rejected=true;}require(rejected);++checks;
    std::vector<SourceModelInstanceLightingCandidate>records(2);require(sourceModelInstanceLightingCandidate(0xffff,records)==nullptr&&sourceModelInstanceLightingCandidate(1,records)==&records[1]);++checks;
    rejected=false;try{sourceModelInstanceLightingCandidate(2,records);}catch(const std::runtime_error&){rejected=true;}require(rejected);++checks;
    std::cout<<"PASS cached model lighting safety checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
