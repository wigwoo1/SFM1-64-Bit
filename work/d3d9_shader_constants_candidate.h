#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
struct D3d9ShaderConstantCandidate {
    std::string name;
    uint16_t registerSet,index,count,parameterClass,type,rows,columns,elements;
};
// Observed leading CTAB comment only. This describes compiled metadata, not
// Source's ownership or update policy for register values.
inline std::vector<D3d9ShaderConstantCandidate> d3d9ShaderConstantsCandidate(const std::vector<uint8_t>& shader) {
    auto fail=[](){throw std::runtime_error("invalid D3D9 CTAB");};
    auto word=[&](size_t at)->uint32_t{if(at>shader.size()||shader.size()-at<4)fail();return uint32_t(shader[at])|(uint32_t(shader[at+1])<<8)|(uint32_t(shader[at+2])<<16)|(uint32_t(shader[at+3])<<24);};
    if(shader.size()<12||(word(4)&0xffff)!=0xfffe||word(8)!=0x42415443)fail();
    size_t length=((word(4)>>16)&0x7fff)*4;
    if(length<32||length>shader.size()-8)fail();
    length-=4;constexpr size_t base=12;
    auto bounded=[&](size_t offset,size_t size){if(offset>length||size>length-offset)fail();};
    auto value=[&](size_t offset){bounded(offset,4);return word(base+offset);};
    auto shortValue=[&](size_t offset)->uint16_t{bounded(offset,2);return uint16_t(shader[base+offset])|uint16_t(uint16_t(shader[base+offset+1])<<8);};
    auto stringValue=[&](size_t offset){bounded(offset,1);std::string name;while(offset<length&&shader[base+offset])name.push_back(char(shader[base+offset++]));if(offset==length)fail();return name;};
    if(value(0)!=28||value(8)!=word(0))fail();
    auto count=value(12),table=value(16);bounded(table,size_t(count)*20);
    std::vector<D3d9ShaderConstantCandidate> result;
    for(uint32_t i=0;i<count;++i){size_t at=table+size_t(i)*20;auto typeOffset=value(at+12);bounded(typeOffset,16);
        D3d9ShaderConstantCandidate entry{stringValue(value(at)),shortValue(at+4),shortValue(at+6),shortValue(at+8),
            shortValue(typeOffset),shortValue(typeOffset+2),shortValue(typeOffset+4),shortValue(typeOffset+6),shortValue(typeOffset+8)};
        if(entry.name.empty()||entry.registerSet>3||!entry.count)fail();
        for(const auto& previous:result)if(previous.name==entry.name)fail();
        result.push_back(std::move(entry));
    }
    return result;
}
