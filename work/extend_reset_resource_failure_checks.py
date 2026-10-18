from pathlib import Path
p=Path('work/test_native_reset_resource_composition.cpp');s=p.read_text().replace('#include <cassert>','#include <cassert>\n#include <cstdio>\n#include <initializer_list>')
s=s.replace('unsigned textures=0,queries=0,resets=0;', 'unsigned textures=0,queries=0,resets=0;HRESULT resetResult=S_OK;').replace('++resets;return S_OK;', '++resets;return resetResult;')
s=s.replace('unsigned builds=0,managers=0;', 'unsigned builds=0,managers=0,failures=0,posts=0;').replace('void resetFailure(HRESULT){assert(false);}', 'void resetFailure(HRESULT result){assert(result<0);++failures;}').replace('void postReset(bool restore,bool force){', 'void postReset(bool restore,bool force){++posts;')
s=s.replace('int main(){Device device;', 'int main(){unsigned cases=0;for(HRESULT result:{S_OK,S_FALSE,static_cast<HRESULT>(0x7fffffffu),E_FAIL,static_cast<HRESULT>(0x80000000u)}){Device device;device.resetResult=result;')
s=s.replace('assert(studioDeviceResetCandidate(owner,descriptor,resetServices));', 'assert(studioDeviceResetCandidate(owner,descriptor,resetServices)==(result>=0));')
a=s.index(' assert(depth==0&&device.textures==2');s=s[:a]+''' if(result>=0){assert(resetServices.posts==1&&resetServices.failures==0);assert(depth==0&&device.textures==2&&device.queries==44&&queueActive==1&&texture==&device.texture&&queries[0]==&device.storage[22]&&queries[1]==&device.storage[23]);for(unsigned i=0;i<20;++i)assert(queued[i]==&device.storage[i+24]);}
 else{assert(resetServices.posts==0&&resetServices.failures==1&&depth==1&&device.textures==1&&device.queries==22&&queueActive==0&&!texture&&!queries[0]&&!queries[1]&&active[0]==0&&active[1]==0);for(auto* q:queued)assert(!q);}
 ++cases;}std::printf("reset_resource_composition_cases=%u passed\\n",cases);
}
'''
p.write_text(s)
