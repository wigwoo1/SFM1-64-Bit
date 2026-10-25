#include "ifm_setup_bones_candidate.h"
#include <iostream>
struct WrapperServices {
    unsigned prepares=0,begins=0,ends=0,clocks=0,warnings=0;bool result=true,clock=true;
    std::vector<RecoveredMatrix3x4>cache{RecoveredMatrix3x4{{{1,0,0,10},{0,1,0,20},{0,0,1,30}}}};
    void begin(){++begins;}void end(){++ends;}bool hasClock(){return clock;}int32_t clockTick(){++clocks;return 123;}
    bool prepare(int32_t,uint32_t,int32_t tick){if(tick!=INT32_MIN)throw std::runtime_error("sentinel contract");++prepares;cache[0][0][3]+=1;return result;}
    const auto&cachedPalette(){return cache;}void warnCapacity(int32_t,uint32_t){++warnings;}
};
int main(){try{
    unsigned checks=0,refusals=0;auto check=[&](bool v){if(!v)throw std::runtime_error("setup safety check");++checks;};auto reject=[&](auto f){try{f();}catch(const std::runtime_error&){++refusals;return;}throw std::runtime_error("setup refusal expected");};
    WrapperServices p;std::vector<RecoveredMatrix3x4>out(1);const auto initial=out;
    check(!ifmSetupBonesWrapperCandidate(0,0,0,&out,p)&&out==initial&&p.cache[0][0][3]==11&&p.warnings==1);
    check(ifmSetupBonesWrapperCandidate(INT32_MIN,0,0x7fc00001,nullptr,p)&&p.prepares==2);
    check(ifmSetupBonesWrapperCandidate(1,0xffffffff,0xff800000,&out,p)&&out[0][0][3]==13&&p.prepares==3);
    p.result=false;const auto before=out;check(!ifmSetupBonesWrapperCandidate(1,0,0,&out,p)&&out==before&&p.cache[0][0][3]==14);
    check(p.begins==4&&p.ends==4&&p.clocks==4);p.result=true;std::vector<RecoveredMatrix3x4>small;reject([&]{ifmSetupBonesWrapperCandidate(1,0,0,&small,p);});check(p.begins==p.ends);
    std::vector<float>flex{9,8,7};ifmSetupFlexPrefixCandidate({1,2},flex);check(flex==std::vector<float>{1,2,7});ifmSetupFlexPrefixCandidate({3,4,5,6},flex);check(flex==std::vector<float>{3,4,5,6});
    reject([&]{ifmSetupFlexPrefixCandidate(std::vector<float>(257),flex);});
    RecoveredMatrix3x4 world{{{1,0,0,10},{0,1,0,20},{0,0,1,30}}};const auto root=ifmSetupBoneRootCandidate(world,2);check(root[0][0]==2&&root[0][3]==10&&root[1][3]==20);
    reject([&]{ifmSetupBoneRootCandidate(world,std::numeric_limits<float>::infinity());});world[0][0]=std::numeric_limits<float>::quiet_NaN();reject([&]{ifmSetupBoneRootCandidate(world,1);});
    std::cout<<"PASS checks="<<checks<<" refusals="<<refusals<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
