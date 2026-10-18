#include "gather_float_curve_key.h"
#include <iostream>
#include <limits>
int main(){try{static_assert(sizeof(void*)==8,"x64 required");std::vector<int32_t>t{10,20},k{0,0x606};std::vector<float>v{2,4};RecoveredFloatCurveInfo c;c.defaultKind=0x101;
 auto a=gatherRecoveredFloatCurveKey(t,v,k,c,0),b=gatherRecoveredFloatCurveKey(t,v,k,c,1);if(a.time!=10||a.value!=2||a.kind!=0x101||b.kind!=0x606)throw std::runtime_error("interior failed");
 a=gatherRecoveredFloatCurveKey(t,v,k,c,-1);b=gatherRecoveredFloatCurveKey(t,v,k,c,2);if(a.time!=0||a.value!=0||a.kind!=0x101||b.time!=21||b.value!=0)throw std::runtime_error("default edges failed");
 c.useEdgeInfo=true;c.defaultEdgeZero=3;c.leftActive=true;c.leftValue=5;c.leftKind=0x404;c.rightActive=true;c.rightValue=7;c.rightKind=0x606;c.rightEdgeTime=40;
 a=gatherRecoveredFloatCurveKey(t,v,k,c,-2);b=gatherRecoveredFloatCurveKey(t,v,k,c,99);if(a.value!=5||a.kind!=0x404||b.time!=40||b.value!=7||b.kind!=0x606)throw std::runtime_error("configured edges failed");
 c.rightActive=false;b=gatherRecoveredFloatCurveKey(t,v,k,c,2);if(b.value!=3||b.kind!=0x101)throw std::runtime_error("inactive edge failed");
 c.useEdgeInfo=false;a=gatherRecoveredFloatCurveKey(t,v,k,c,-1);if(a.value!=0||a.kind!=0x404)throw std::runtime_error("edge kind incorrectly gated");
 c.rightEdgeTime=0;t.back()=std::numeric_limits<int32_t>::max();if(gatherRecoveredFloatCurveKey(t,v,k,c,2).time!=std::numeric_limits<int32_t>::min())throw std::runtime_error("edge tick wrapping failed");
 if(gatherRecoveredFloatCurveKey({}, {}, {},c,0).time!=0||gatherRecoveredFloatCurveKey(t,v,k,c,0,false).kind!=0)throw std::runtime_error("empty/no-owner failed");
 std::cout<<"curve_key_gather_checks=7\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
