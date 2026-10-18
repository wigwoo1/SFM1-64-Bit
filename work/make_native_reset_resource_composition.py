from pathlib import Path
s=Path('work/test_native_owner_post_reset.cpp').read_text().replace('#include <cassert>','#include <cassert>\n#include "studio_device_reset_candidate.h"')
s=s.replace('unsigned textures=0,queries=0;', 'unsigned textures=0,queries=0,resets=0;HRESULT Reset(D3DPRESENT_PARAMETERS*){assert(texture.releases==1);for(unsigned i=0;i<22;++i)assert(storage[i].releases==1);++resets;return S_OK;}')
pos=s.index('int main()')
adapter='''template<class Owner,class Life,class D>struct ResetServices{
 Owner& owner;Life& lifecycle;D*& current;unsigned builds=0,managers=0;
 unsigned descriptorFlags(unsigned value){return value;}
 void rebuild(HWND,UINT,unsigned&,bool first){assert(first==(builds==0));++builds;}
 void managerCleanup(){++managers;}
 void ownerHook(bool enabled){assert(builds==1&&managers==1);NativeOwnerResetCleanupServicesCandidateT<Life> services{lifecycle};studioOwnerResetCleanupCandidate(owner,services,enabled);}
 D*& commandState(){assert(builds==2);return current;}void flush(D*&){++lifecycle.flushes;}D* device(D*& state){return state;}
 HRESULT reset(D* device,D3DPRESENT_PARAMETERS& parameters){return device->Reset(&parameters);}void resetFailure(HRESULT){assert(false);}
 void postReset(bool restore,bool force){NativeOwnerPostResetServicesCandidateT<Life> services{lifecycle};studioOwnerPostResetCandidate(owner,services,restore,force,nullptr);}
};
'''
s=s[:pos]+adapter+s[pos:]
s=s.replace('std::array<Query*,5> fixed{};struct Owner','std::array<Query*,5> fixed{};HWND window=nullptr;UINT adapter=0;D3DPRESENT_PARAMETERS parameters{};struct Owner')
s=s.replace('std::array<Query*,5>& fixedResources;}owner{','std::array<Query*,5>& fixedResources;HWND& window;UINT& adapter;D3DPRESENT_PARAMETERS& parameters;}owner{').replace('queued,flags,fixed};','queued,flags,fixed,window,adapter,parameters};')
a=s.index(' NativeOwnerResetCleanupServicesCandidateT<Lifecycle<Device>> cleanup');b=s.index('assert(depth==0&&device.textures==2',a)
s=s[:a]+''' unsigned descriptor=0;ResetServices<decltype(owner),decltype(lifecycle),Device> resetServices{owner,lifecycle,pointer};assert(studioDeviceResetCandidate(owner,descriptor,resetServices));assert(device.resets==1&&device.texture.releases==1);for(unsigned i=0;i<22;++i)assert(device.storage[i].releases==1);
 '''+s[b:]
Path('work/test_native_reset_resource_composition.cpp').write_text(s)
s=Path('work/build_native_owner_post_reset.cmd').read_text().replace('test_native_owner_post_reset','test_native_reset_resource_composition');Path('work/build_native_reset_resource_composition.cmd').write_text(s)
