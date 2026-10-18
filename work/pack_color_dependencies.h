#pragma once
#include "session_expression_chain_plan.h"
#include "session_pack_color_runtime.h"
inline AttributeOperation packColorOperationCandidate(const Graph&graph,uint32_t id){
 if(graph.strings.at(graph.elements.at(id).type)!="DmePackColorOperator")throw std::runtime_error("expected pack color dependency owner");
 AttributeOperation result{id,{},{{id,"color"}}};for(const char*name:{"red","green","blue","alpha"})result.inputs.emplace_back(id,name);return result;
}
