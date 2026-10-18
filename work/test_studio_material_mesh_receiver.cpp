#include <iostream>
#include "studio_material_mesh_receiver_candidate.h"
int main(){unsigned cases=0;
for(unsigned flags=0;flags<256;++flags)for(unsigned format=0;format<4;++format){
 uintptr_t receiver=uintptr_t(0x123456780000ull),selected=receiver+0x1000;unsigned calls[3]={};uint32_t low=(format&1)?0x80000000:0,high=(format&2)?0x80000000:0;
 auto validate=[&](uint32_t bit,uintptr_t a,uintptr_t b,uintptr_t c){if(bit!=(flags&1)||a!=0x1111||b!=0x2222||c!=0x3333||receiver==selected)throw 1;};
 studioMaterialMeshReceiverCandidate(receiver,low,high,flags,0x1111,0x2222,0x3333,
 [&]{if(receiver==selected)throw 1;++calls[0];return selected;},
 [&](uint32_t bit,uintptr_t a,uintptr_t b,uintptr_t c){validate(bit,a,b,c);++calls[1];return selected;},
 [&](uint32_t l,uint32_t h,uint32_t bit,uintptr_t a,uintptr_t b,uintptr_t c){if(l!=low||h!=high)throw 1;validate(bit,a,b,c);++calls[2];return selected;});
 unsigned branch=(flags&2)?0:(format?2:1);for(unsigned i=0;i<3;++i)if(calls[i]!=unsigned(i==branch))return 1;if(receiver!=selected)return 2;++cases;
}
std::cout<<"mesh_receiver_cases="<<cases<<" callback_selection=1 full_width_receiver=1 store_after_callback=1\n";
}
