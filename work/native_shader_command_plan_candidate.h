#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
struct NativeShaderConstantWordsCandidate {bool pixel;uint32_t index;std::vector<uint32_t> words;};
struct NativeShaderCommandPlanCandidate {
    std::vector<uint32_t> selections;
    std::vector<NativeShaderConstantWordsCandidate> uploads;
};
// Audited flat commands only. Limits come from the selected shader/device profile.
inline NativeShaderCommandPlanCandidate nativeShaderCommandPlanCandidate(
    const std::vector<uint32_t>& stream,uint32_t vertexLimit,uint32_t pixelLimit) {
    NativeShaderCommandPlanCandidate plan;size_t at=0;
    if(stream.size()>64u*1024u*1024u)throw std::runtime_error("shader command size limit");
    while(at<stream.size()) {
        auto opcode=stream[at++];
        if(opcode==0){if(at!=stream.size())throw std::runtime_error("shader command trailing words");plan.selections.push_back(0);return plan;}
        if(opcode==11||opcode==12){if(at==stream.size())throw std::runtime_error("shader selector truncated");plan.selections.push_back(opcode);plan.selections.push_back(stream[at++]);continue;}
        if((opcode!=3&&opcode!=4)||stream.size()-at<2)throw std::runtime_error("shader command unsupported/truncated");
        auto index=stream[at++],count=stream[at++];auto limit=opcode==3?pixelLimit:vertexLimit;
        if(index>limit||count>limit-index||count>(stream.size()-at)/4)throw std::runtime_error("shader constant range/payload");
        size_t words=size_t(count)*4;
        plan.uploads.push_back({opcode==3,index,{stream.begin()+at,stream.begin()+at+words}});at+=words;
    }
    throw std::runtime_error("shader command terminator absent");
}
