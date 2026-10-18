#include "native_owner_reset_cleanup_candidate.h"
#include <cassert>
#include <vector>
static std::vector<unsigned> trace;
struct Resource{unsigned id;ULONG Release(){trace.push_back(id);return 0;}};
struct Query:Resource{HRESULT GetData(void*,DWORD bytes,DWORD flags){assert(bytes==4&&flags==1);trace.push_back(100+id);return S_OK;}};
struct Lifecycle{
 DWORD thread=42;bool main=true;unsigned deferred=0,nested=0;unsigned char* flags;unsigned char* queueActive;
 DWORD currentThreadId(){return thread;}bool inMainThread(){return main;}void deferCleanup(bool v){assert(v);++deferred;assert((*flags&0x20)==0);}
 void nestedWarning(uint32_t v){nested=v;}void nestedDiagnostic(){}bool notificationAvailable(){return true;}void beginNotification(uint32_t v){assert(v==0xfff5940f);trace.push_back(1);}void endNotification(){trace.push_back(2);}
 bool diagnosticEnabled(){return false;}void diagnostic(){}double time(){return 0;}double limit(){return 2;}
 void setCleanupMode(unsigned v){assert(v==2);assert(*queueActive==0);trace.push_back(3);}void finishCommandState(){trace.push_back(4);}void publishCleanupEnabled(bool v){assert(v);assert((*flags&0x20)==0);trace.push_back(5);}
};
// Instantiate the composed bridge against actual SDK interfaces without running GPU calls.
void compileSdkBridge(NativeOwnerResetCleanupViewCandidateT<>& owner,Lifecycle& lifecycle){NativeOwnerResetCleanupServicesCandidateT<Lifecycle> services{lifecycle};studioOwnerResetCleanupCandidate(owner,services,true);}
int main(){
 unsigned char flags=0,queueActive=1;uint32_t depth=0;DWORD thread=42;Resource resourceObject{10},queued{20},fixed{30};Query query{{40}};Resource* resource=&resourceObject;std::array<Query*,2> queries{&query,nullptr};std::array<unsigned char,2> active{1,1};std::array<Resource*,20> queuedResources{};queuedResources[19]=&queued;std::array<Resource*,5> fixedResources{};fixedResources[4]=&fixed;
 NativeOwnerResetCleanupViewCandidateT<Resource,Query> owner{flags,depth,thread,resource,queries,active,queueActive,queuedResources,fixedResources};Lifecycle lifecycle; lifecycle.flags=&flags;lifecycle.queueActive=&queueActive;NativeOwnerResetCleanupServicesCandidateT<Lifecycle> services{lifecycle};studioOwnerResetCleanupCandidate(owner,services,true);
 assert(trace==std::vector<unsigned>({1,2,10,140,40,20,30,3,4,5}));assert(flags==0x20&&depth==1&&queueActive==0&&!resource&&!queries[0]&&active[0]==0&&active[1]==1&&!queuedResources[19]&&!fixedResources[4]);
 trace.clear();studioOwnerResetCleanupCandidate(owner,services,true);assert(trace.empty()&&depth==2&&lifecycle.nested==2);
 lifecycle.thread=99;studioOwnerResetCleanupCandidate(owner,services,true);assert(trace.empty()&&depth==2&&flags==0&&lifecycle.deferred==1);
 lifecycle.thread=42;lifecycle.main=false;studioOwnerResetCleanupCandidate(owner,services,true);assert(trace.empty()&&depth==2&&lifecycle.deferred==2);
}
