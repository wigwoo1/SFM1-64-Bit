#include "element_handle_release.h"
#include <iostream>
int main(){ElementHandleReleaseTable t;t.liveCount=3;t.slots={{0,123},{510,456},{509,789}};
 if(t.resolve(0)!=123||t.resolve(1)!=0||t.resolve(0xffffffff)!=0)return 5;
 if(!t.releaseIndex(0)||t.liveCount!=2||t.slots[0].metadata!=0x80000001||t.slots[0].object||t.reusable!=std::vector<uint32_t>{0}||t.resolve(0))return 1;
 if(!t.releaseIndex(1)||t.liveCount!=1||t.slots[1].metadata!=0x800001ff||t.reusable.size()!=1)return 2;
 if(!t.releaseIndex(2)||t.liveCount||t.reusable!=std::vector<uint32_t>({0,2}))return 3;
 if(t.releaseIndex(3)||t.liveCount)return 4;
 t.slots[0]={1,uintptr_t(0x123456789abcdef0ull)};
 if(t.resolve(0)||t.resolve(1u<<22)!=t.slots[0].object||t.resolve((1u<<22)|0x80000000u)!=t.slots[0].object)return 6;
 ElementHandleReleaseTable life;auto a=life.allocate(),b=life.allocate();
 if(a!=0||b!=1||life.liveCount!=2||life.resolve(a))return 7;
 if(!life.publish(a,123)||!life.publish(b,456)||life.resolve(a)!=123)return 8;
 life.releaseIndex(b);life.releaseIndex(a);auto c=life.allocate(),d=life.allocate();
 if(c!=((1u<<22)|1)||d!=(1u<<22)||life.resolve(a)||life.resolve(b)||life.liveCount!=2)return 9;
 if(life.publish(a,789)||!life.publish(c,uintptr_t(0x123456789abcdef0ull))||life.resolve(c)!=uintptr_t(0x123456789abcdef0ull))return 10;
 std::cout<<"element_handle_lifecycle_checks=10\n";}
