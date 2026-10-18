#pragma once
#include "session_expression_runtime.h"
#include "pack_color_conversion.h"
#include "color_write_sequence.h"
inline bool executeSessionPackColorCandidate(Graph&graph,uint32_t id,SessionAttributeChanges*changes=nullptr){
 if(graph.strings.at(graph.elements.at(id).type)!="DmePackColorOperator")throw std::runtime_error("expected pack color operator");
 std::array<float,4>components{};const char*names[]={"red","green","blue","alpha"};
 for(unsigned i=0;i<4;++i){auto*a=runtimeAttribute(graph,id,names[i]);if(!a||a->type!=3||a->values.size()!=1||a->values[0].wire.size()!=4)throw std::runtime_error("invalid pack color component");components[i]=wireFloat(a->values[0]);}
 auto*output=runtimeAttribute(graph,id,"color");if(!output||output->type!=8||output->values.size()!=1||output->values[0].wire.size()!=4)throw std::runtime_error("invalid packed color output");
 uint32_t old;std::memcpy(&old,output->values[0].wire.data(),4);if(writeRecoveredColorCandidate(old,recoveredPackColorCandidate(components),[](){return true;},[](){},[](){},[](){})){std::memcpy(output->values[0].wire.data(),&old,4);if(changes)changes->attributes.emplace_back(id,"color");}return true;
}
