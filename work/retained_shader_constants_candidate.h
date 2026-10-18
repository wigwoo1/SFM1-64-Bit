#pragma once
#include "native_shader_command_plan_candidate.h"
#include <algorithm>
class RetainedShaderConstantStageCandidate {
    bool pixel_;
    std::vector<uint32_t> words_;
    std::vector<uint8_t> known_;
public:
    RetainedShaderConstantStageCandidate(bool pixel,std::vector<uint32_t> initial,bool known):
        pixel_(pixel),words_(std::move(initial)),known_(words_.size()/4,known?1:0){
        if(words_.size()%4)throw std::runtime_error("constant cache dimensions");
    }
    const std::vector<uint32_t>& words() const{return words_;}
    void invalidate(){std::fill(known_.begin(),known_.end(),uint8_t(0));}
    template<class Upload> bool apply(uint32_t index,const std::vector<uint32_t>& values,Upload&& upload) {
        if(values.size()%4||index>known_.size()||values.size()/4>known_.size()-index)
            throw std::runtime_error("constant cache range");
        size_t count=values.size()/4,first=0;
        while(first<count&&known_[index+first]&&std::equal(values.begin()+first*4,values.begin()+first*4+4,words_.begin()+(index+first)*4))++first;
        if(first==count)return false;
        // Native pixel uploads skip matching leading registers; vertex uploads
        // send the complete range after detecting any changed word.
        if(!pixel_)first=0;
        NativeShaderConstantWordsCandidate command{pixel_,index+static_cast<uint32_t>(first),{values.begin()+first*4,values.end()}};
        upload(command); // Failed callbacks throw; no cached state is committed.
        std::copy(command.words.begin(),command.words.end(),words_.begin()+command.index*4);
        std::fill(known_.begin()+command.index,known_.begin()+index+count,uint8_t(1));
        return true;
    }
};

class RetainedShaderConstantPlanCandidate {
    uint32_t vertexLimit_,pixelLimit_;
    RetainedShaderConstantStageCandidate vertex_,pixel_;
    static std::vector<uint32_t> initial(uint32_t limit){
        if(limit>4096)throw std::runtime_error("constant profile capacity");return std::vector<uint32_t>(size_t(limit)*4);
    }
public:
    RetainedShaderConstantPlanCandidate(uint32_t vertexLimit,uint32_t pixelLimit):
        vertexLimit_(vertexLimit),pixelLimit_(pixelLimit),vertex_(false,initial(vertexLimit),false),pixel_(true,initial(pixelLimit),false){}
    void invalidate(){vertex_.invalidate();pixel_.invalidate();}
    template<class Upload> size_t apply(const NativeShaderCommandPlanCandidate& plan,Upload&& upload) {
        // Validate the whole plan before sending any request to the device.
        for(const auto& command:plan.uploads){auto limit=command.pixel?pixelLimit_:vertexLimit_;
            if(command.words.size()%4||command.index>limit||command.words.size()/4>limit-command.index)
                throw std::runtime_error("retained constant plan range");}
        size_t sent=0;
        for(const auto& command:plan.uploads){auto& cache=command.pixel?pixel_:vertex_;
            if(cache.apply(command.index,command.words,upload))++sent;}
        return sent;
    }
};
