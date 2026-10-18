#pragma once
#include "mdl_flex_metadata.h"
struct MdlFlexOperationCandidate {int32_t opcode;uint32_t operandBits;};
struct MdlFlexRuleCandidate {uint32_t target;std::vector<MdlFlexOperationCandidate>operations;};
// Valve studio.h layout evidence: fixed-width MDL49 disk fields, not native
// SFM evaluator parity. Preserve operand union bits and unknown opcodes.
inline std::vector<MdlFlexRuleCandidate>readMdl49FlexRulesCandidate(const std::vector<uint8_t>&bytes){
 readMdl49FlexMetadata(bytes);
 auto word=[&](size_t offset){if(offset>bytes.size()||bytes.size()-offset<4)throw std::runtime_error("truncated flex rule field");uint32_t value;std::memcpy(&value,bytes.data()+offset,4);return value;};
 auto span=[&](uint64_t offset,uint64_t size){if(offset>bytes.size()||size>bytes.size()-offset)throw std::runtime_error("flex rule span");};
 auto nonnegative=[&](size_t offset){auto value=word(offset);if(value>INT32_MAX)throw std::runtime_error("negative flex rule field");return value;};
 auto descriptors=nonnegative(260);if(descriptors)span(nonnegative(264),uint64_t(descriptors)*4);
 auto count=nonnegative(276),offset=nonnegative(280);if(count)span(offset,uint64_t(count)*12);
 std::vector<MdlFlexRuleCandidate>rules;rules.reserve(count);
 for(uint32_t i=0;i<count;++i){size_t base=size_t(offset)+size_t(i)*12;auto target=nonnegative(base),operations=nonnegative(base+4);if(target>=descriptors)throw std::runtime_error("flex rule target");int32_t delta;auto bits=word(base+8);std::memcpy(&delta,&bits,4);int64_t start=int64_t(base)+delta;if(start<0)throw std::runtime_error("flex rule operation offset");if(operations)span(uint64_t(start),uint64_t(operations)*8);
  MdlFlexRuleCandidate rule{target,{}};rule.operations.reserve(operations);for(uint32_t n=0;n<operations;++n){size_t p=size_t(start)+size_t(n)*8;auto opcodeBits=word(p);int32_t opcode;std::memcpy(&opcode,&opcodeBits,4);rule.operations.push_back({opcode,word(p+4)});}rules.push_back(std::move(rule));
 }return rules;
}
