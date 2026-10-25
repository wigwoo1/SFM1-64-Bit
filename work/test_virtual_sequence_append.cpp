#include "source_mdl_virtual_sequences_candidate.h"
#include <iostream>
int main(){try{
    unsigned checks=0,refusals=0;auto check=[&](bool v){if(!v)throw std::runtime_error("sequence check");++checks;};auto reject=[&](auto f){try{f();}catch(const std::runtime_error&){++refusals;return;}throw std::runtime_error("expected sequence refusal");};
    SourceMdlSequenceMetadataCandidate root,child;root.sequences={{"REF",0x800,3,{}},{"fixed",1,4,{}}};child.sequences={{"ref",2,7,{}},{"FIXED",0x800,8,{}},{"new",0,9,{}},{"NEW",4,10,{}}};
    const auto before=root.sequences;const auto r=sourceVirtualSequencesCandidate({root,child});
    check(r.entries.size()==4&&r.localToMaster==std::vector<std::vector<uint32_t>>{{0,1},{0,1,2,3}});
    check(r.entries[0].group==1&&r.entries[0].index==0&&r.entries[0].flags==2&&r.entries[0].activity==7);
    check(r.entries[1].group==0&&r.entries[1].index==1&&r.entries[1].flags==1);
    check(sourceVirtualSequenceCandidate(r,-1).group==1&&sourceVirtualSequenceCandidate(r,INT32_MAX).index==0);
    check(root.sequences[0].label==before[0].label&&root.sequences[0].flags==before[0].flags);
    SourceMdlSequenceMetadataCandidate duplicate,replace;duplicate.sequences={{"a",0x800,1,{}},{"A",0x800,2,{}},{"a",0x800,3,{}}};replace.sequences={{"A",4,7,{}}};
    const auto cached=sourceVirtualSequencesCandidate({duplicate,replace},SourceSequenceNameCacheModeCandidate::Enabled);
    const auto uncached=sourceVirtualSequencesCandidate({duplicate,replace});
    check(cached.localToMaster[1]==std::vector<uint32_t>{1}&&cached.entries[1].group==1&&cached.entries[1].activity==7);
    check(uncached.localToMaster[1]==std::vector<uint32_t>{0}&&uncached.entries[0].group==1);
    reject([&]{sourceVirtualSequencesCandidate({});});reject([&]{sourceVirtualSequenceCandidate({},0);});
    child.sequences[0].label="";reject([&]{sourceVirtualSequencesCandidate({root,child});});
    child.sequences[0].label=std::string(1,char(-1));reject([&]{sourceVirtualSequencesCandidate({child});});
    child.sequences[0].label=std::string(1025,'a');reject([&]{sourceVirtualSequencesCandidate({child});});
    child.sequences.resize(4097);reject([&]{sourceVirtualSequencesCandidate({child});});
    reject([&]{readSourceMdlSequenceMetadataCandidate(std::vector<uint8_t>(0x158));});
    std::cout<<"PASS checks="<<checks<<" refusals="<<refusals<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
