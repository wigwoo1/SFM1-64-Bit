from pathlib import Path
p=Path('work/test_native_owner_post_reset.cpp');s=p.read_text().replace('#include <cassert>','#include <cassert>\n#include "native_owner_reset_cleanup_candidate.h"')
s=s.replace('struct Texture{ULONG Release(){return 0;}};', 'struct Texture{unsigned releases=0;ULONG Release(){++releases;return 0;}};')
s=s.replace('struct Query{unsigned issues=0;', 'struct Query{unsigned issues=0,releases=0;ULONG Release(){++releases;return 0;}HRESULT GetData(void*,DWORD bytes,DWORD flags){assert(bytes==4&&flags==1);return S_OK;}')
s=s.replace('Texture texture;Query query;', 'Texture texture;std::array<Query,44> storage{};').replace('++queries;*output=&query;', '*output=&storage[queries++];')
s=s.replace('template<class D>struct Lifecycle{', 'template<class D>struct Lifecycle{void deferCleanup(bool){assert(false);}void nestedWarning(uint32_t){assert(false);}void nestedDiagnostic(){}void beginNotification(uint32_t v){assert(v==0xfff5940f);}void setCleanupMode(unsigned v){assert(v==2);}void finishCommandState(){}void publishCleanupEnabled(bool v){assert(v);}double time(){return 0;}double limit(){return 2;}')
s=s.replace('std::array<Query*,20> queued{};struct Owner', 'std::array<Query*,20> queued{};unsigned char flags=0;std::array<Query*,5> fixed{};struct Owner')
s=s.replace('std::array<Query*,20>& queuedResources;}owner{', 'std::array<Query*,20>& queuedResources;unsigned char& flags;std::array<Query*,5>& fixedResources;}owner{').replace('queueActive,queued};', 'queueActive,queued,flags,fixed};')
s=s.replace('device.query.issues==2', 'device.storage[0].issues==1&&device.storage[1].issues==1')
s=s.replace('for(auto* query:queued)assert(query==&device.query);}', '''for(unsigned i=0;i<20;++i)assert(queued[i]==&device.storage[i+2]);assert(queries[0]==&device.storage[0]&&queries[1]==&device.storage[1]);
 NativeOwnerResetCleanupServicesCandidateT<Lifecycle<Device>> cleanup{lifecycle};studioOwnerResetCleanupCandidate(owner,cleanup,true);
 assert(depth==1&&flags==0x20&&queueActive==0&&!texture&&!queries[0]&&!queries[1]&&active[0]==0&&active[1]==0&&device.texture.releases==1);for(auto* q:queued)assert(!q);for(unsigned i=0;i<22;++i)assert(device.storage[i].releases==1);
 studioOwnerPostResetCandidate(owner,services,true,false,nullptr);assert(depth==0&&device.textures==2&&device.queries==44&&queueActive==1&&texture==&device.texture&&queries[0]==&device.storage[22]&&queries[1]==&device.storage[23]);for(unsigned i=0;i<20;++i)assert(queued[i]==&device.storage[i+24]);
}''')
s=s.replace('textures==1','textures>=1')
p.write_text(s)
