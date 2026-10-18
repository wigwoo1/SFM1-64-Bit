#pragma once
#include "dynamic_shader_commands_candidate.h"
#include "vcs6_shader_bytecode_candidate.h"
class NativeShaderProgramSelectionCandidate {
    RetainedVcs6ShaderCacheCandidate& vertexCache_;
    RetainedVcs6ShaderCacheCandidate& pixelCache_;
    uint32_t vertexStatic_,pixelStatic_;
    DynamicShaderSelectionCandidate state_{};
public:
    struct Programs {
        std::shared_ptr<const std::vector<uint8_t>> vertex,pixel;
    };
    NativeShaderProgramSelectionCandidate(RetainedVcs6ShaderCacheCandidate& vertex,
        uint32_t vertexStatic,RetainedVcs6ShaderCacheCandidate& pixel,uint32_t pixelStatic):
        vertexCache_(vertex),pixelCache_(pixel),vertexStatic_(vertexStatic),pixelStatic_(pixelStatic){}
    DynamicShaderSelectionCandidate state() const{return state_;}
    template<class Prepare> Programs executePrepared(const uint32_t* words,size_t count,Prepare prepare) {
        auto next=dynamicShaderCommandsCandidate(state_,words,count);
        Programs programs{vertexCache_.shader(vertexStatic_,next.vertex),pixelCache_.shader(pixelStatic_,next.pixel)};
        auto version=[](const std::vector<uint8_t>& data){if(data.size()<4)throw std::runtime_error("shader stage word absent");return uint16_t(data[2])|(uint16_t(data[3])<<8);};
        if(version(*programs.vertex)!=0xfffe||version(*programs.pixel)!=0xffff)
            throw std::runtime_error("selected cache shader stage mismatch");
        prepare(programs); // Shader/lighting preparation may fail before state commit.
        state_=next;return programs;
    }
    Programs execute(const uint32_t* words,size_t count) {
        return executePrepared(words,count,[](const Programs&){});
    }
};
