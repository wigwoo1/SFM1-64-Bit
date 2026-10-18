#include "material_animation_frame_candidate.h"
#include <iostream>
uint32_t word(float v){uint32_t w;std::memcpy(&w,&v,4);return w;}
int main(){try{unsigned cases=0;auto check=[&](float now,float duration,float start,float rate,int count,bool wrap,int frame,int previous,bool detected,bool callback){const auto v=materialAnimationFrameCandidate(word(now),word(duration),word(start),word(rate),count,wrap);if(v.frame!=frame||v.previousFrame!=previous||v.wrapDetected!=detected||v.invokeWrapCallback!=callback)throw std::runtime_error("animation boundary mismatch");++cases;};
 check(0,0,0,15,16,true,0,0,false,false);
 check(-1,0.25f,0,15,16,true,0,0,false,false);
 check(1,0.25f,2,15,16,true,0,0,false,false);
 check(0.5f,0.25f,0,4,4,true,2,1,false,false);
 check(1,0.25f,0,4,4,true,0,3,true,true);
 check(1,0.25f,0,4,4,false,3,3,true,true);
 check(2,0.25f,0,4,4,false,3,3,true,false);
 check(2.25f,0.25f,0,4,4,false,1,0,false,false); // Preserve original branch, not idealized no-wrap behavior.
 check(1,0.25f,0,0,4,true,0,0,false,false);
 check(0.5f,0.25f,0,-4,4,true,-2,-1,true,true);
 const auto exceptional=materialAnimationFrameCandidate(0x7fc00000,0,0,word(15),3,true);if(exceptional.frame!=-2||exceptional.previousFrame!=-2)throw std::runtime_error("CVTT exceptional remainder");++cases;
 for(int count:{0,-1}){bool rejected=false;try{materialAnimationFrameCandidate(0,0,0,0,count,true);}catch(const std::invalid_argument&){rejected=true;}if(!rejected)throw std::runtime_error("invalid frame count");}
 std::cout<<"material animation arithmetic boundary checks passed cases="<<cases<<" invalid_counts_rejected=2 native_execution_parity=unverified\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
