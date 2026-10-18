#include <cstdio>
#include <stdexcept>
#include "native_manager_callback_registry_candidate.h"
using Callback=void(*)(uint8_t*,unsigned*);
using Registry=NativeManagerCallbackRegistryCandidateT<NativeManagerCallbackAllocatorCandidate,Callback>;
static Registry* active;static bool appendOnce;
static void callback(uint8_t* flags,unsigned* count){++*count;*flags|=0x40;if(appendOnce){appendOnce=false;active->append(callback);}}
static void require(bool p){if(!p)throw std::runtime_error("restore clear mismatch");}
int main(){try{NativeManagerCallbackAllocatorCandidate allocator;Registry registry{allocator};active=&registry;uint8_t flags=0x82;unsigned count=0;
 require(!registry.dispatchRestoreWith(false,flags,&flags,&count)&&flags==0x82&&count==0);
 for(unsigned i=0;i<8;++i)registry.append(callback);appendOnce=true;
 require(!registry.dispatchRestoreWith(true,flags,&flags,&count)&&flags==0x82&&registry.size()==8&&count==0&&appendOnce);
 require(registry.dispatchRestoreWith(false,flags,&flags,&count)&&flags==0&&registry.size()==0&&registry.capacity()==16&&count==8);
 flags=2;registry.append(callback);require(registry.dispatchRestoreWith(false,flags,&flags,&count)&&flags==0&&registry.size()==0&&count==9);
 std::puts("restore_callback_clear_empty_suppressed_growth_reschedule=passed");
 }catch(const std::exception& e){std::puts(e.what());return 1;}}
