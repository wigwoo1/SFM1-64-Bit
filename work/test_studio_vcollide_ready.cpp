#include <iostream>
#include <vector>
#include "source_studio_vcollide_ready_query_candidate.h"
struct Services {
    unsigned groupCalls=0,lookups=0,retains=0;uint64_t retained=0;
    std::vector<uint16_t>groups;
    std::vector<uint64_t>ids;
    auto virtualModelGroups(uint16_t){++groupCalls;return groups;}
    uint64_t cachedGroupObject(uint16_t h){++lookups;return ids.at(h);}
    void retainCollisionObject(uint64_t id){++retains;retained=id;}
};
int main(){try{static_assert(sizeof(void*)==8,"owned x64");unsigned checks=0;auto require=[&](bool v){if(!v)throw std::runtime_error("ready-cache safety failed");++checks;};
    for(uint16_t flags:{uint16_t(0),uint16_t(1),uint16_t(0x80),uint16_t(0xfffd)}){
        SourceStudioVCollideRecordCandidate r{flags,0x123456789abcdef0ull};Services s;bool rejected=false;try{sourceStudioVCollideReadyQueryCandidate(r,0,false,s);}catch(const std::runtime_error&){rejected=true;}require(rejected&&r.flags==flags&&r.collisionObjectId==0x123456789abcdef0ull&&s.groupCalls==0&&s.retains==0);
        require(sourceStudioVCollideReadyQueryCandidate(r,0,true,s)==0&&r.flags==flags);require(sourceStudioVCollideReadyQueryCandidate(r,0xffff,false,s)==0&&r.flags==flags);require(!sourceStudioVCollideFailedCompletionCandidate(r)&&r.flags==uint16_t(flags|2)&&r.collisionObjectId==0x123456789abcdef0ull);
    }
    {Services s;SourceStudioVCollideRecordCandidate r{2,0};require(sourceStudioVCollideReadyQueryCandidate(r,0,false,s)==0&&r.flags==0x82&&s.groupCalls==1);sourceStudioVCollideReadyQueryCandidate(r,0,false,s);require(s.groupCalls==1);}
    {Services s;s.groups={0,1,2,3};s.ids={0xffffeeee12345678ull,0,0xaabbccdd12345678ull,0x1122334412345678ull};SourceStudioVCollideRecordCandidate r{0x4002,0};require(sourceStudioVCollideReadyQueryCandidate(r,0,false,s)==s.ids[2]&&r.flags==0x4082&&s.lookups==2&&s.retains==1&&s.retained==s.ids[2]);require(sourceStudioVCollideReadyQueryCandidate(r,0,false,s)==s.ids[2]&&s.lookups==2&&s.retains==1);}
    {Services s;s.groups={0,1};s.ids={1,2};SourceStudioVCollideRecordCandidate r{2,0x1234567887654321ull};require(sourceStudioVCollideReadyQueryCandidate(r,0,false,s)==r.collisionObjectId&&s.groupCalls==0&&s.retains==0&&r.flags==0x82);}
    {Services s;s.groups={0,1};s.ids={1,2};SourceStudioVCollideRecordCandidate r{0x82,0};require(sourceStudioVCollideReadyQueryCandidate(r,0,false,s)==0&&s.groupCalls==0&&s.retains==0);}
    std::cout<<"PASS ready-cache safety checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
