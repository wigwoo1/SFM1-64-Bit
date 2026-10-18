#pragma once
#include <cstdint>
#include <cstddef>
#include <stdexcept>
struct DynamicShaderSelectionCandidate {uint32_t vertex,pixel;};
inline DynamicShaderSelectionCandidate dynamicShaderCommandsCandidate(
    DynamicShaderSelectionCandidate state,const uint32_t* words,std::size_t count) {
    if(!words||!count)throw std::runtime_error("dynamic command buffer missing");
    std::size_t at=0;
    while(at<count){auto opcode=words[at++];
        if(opcode==0){if(at!=count)throw std::runtime_error("dynamic command trailing words");return state;}
        if((opcode!=11&&opcode!=12)||at==count)throw std::runtime_error("unsupported/truncated dynamic command");
        if(opcode==12)state.vertex=words[at++];else state.pixel=words[at++];
    }
    throw std::runtime_error("dynamic command terminator missing");
}
