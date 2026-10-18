#pragma once
#include <map>
#include <vector>
#include <cstdint>
struct RegisteredOperatorAttribute{uintptr_t identity=0;bool excluded=false;std::vector<size_t> associatedOperators;};
struct RegisteredOperator{uintptr_t identity=0;std::vector<size_t> attributeEdges;};
struct OperatorAttributeRegistration{
 std::vector<RegisteredOperatorAttribute> attributes;std::vector<RegisteredOperator> operators;std::map<uintptr_t,size_t> attributeIndices;
 size_t attribute(uintptr_t identity){auto found=attributeIndices.find(identity);if(found!=attributeIndices.end())return found->second;size_t index=attributes.size();attributes.push_back({identity,false,{}});attributeIndices.emplace(identity,index);return index;}
 // Native slot +0x10 appends associations; +0x14 appends edges and excludes.
 size_t add(uintptr_t identity,const std::vector<uintptr_t>&slot10,const std::vector<uintptr_t>&slot14){size_t index=operators.size();operators.push_back({identity,{}});for(auto a:slot10){size_t i=attribute(a);attributes[i].associatedOperators.push_back(index);}for(auto a:slot14){size_t i=attribute(a);attributes[i].excluded=true;operators[index].attributeEdges.push_back(i);}return index;}
};
