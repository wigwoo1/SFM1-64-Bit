#pragma once
#include <unordered_map>
#include <memory>
#include <vector>
#include <stdexcept>
#include "reverse_reference_iterator.h"
#include "reference_record_transfer.h"
//Arena owns tail nodes independently of pending/live record location.
class RecoveredReferenceRecordListArenaCandidate{
 std::unordered_map<RecoveredReverseReferenceNodeCandidate*,std::unique_ptr<RecoveredReverseReferenceNodeCandidate>> nodes;
 RecoveredReverseReferenceNodeCandidate* checked(uintptr_t pointer)const{auto*p=reinterpret_cast<RecoveredReverseReferenceNodeCandidate*>(pointer);if(p&&!nodes.count(p))throw std::runtime_error("unowned reference node");return p;}
 void release(RecoveredReverseReferenceNodeCandidate*node){if(nodes.erase(node)!=1)throw std::runtime_error("reference node already freed");}
public:
 size_t allocationCount()const{return nodes.size();}
 void add(RecoveredTransferReferenceRecordCandidate&record,uint32_t attribute){
  if(attribute==0xffffffffu)throw std::invalid_argument("invalid attribute token");
  if(record.headAttribute!=0xffffffffu){auto node=std::make_unique<RecoveredReverseReferenceNodeCandidate>(RecoveredReverseReferenceNodeCandidate{record.headAttribute,checked(record.next)});auto*p=node.get();nodes.emplace(p,std::move(node));record.next=reinterpret_cast<uintptr_t>(p);}
  record.headAttribute=attribute;
 }
 void remove(RecoveredTransferReferenceRecordCandidate&record,uint32_t attribute){
  if(record.headAttribute==attribute){if(auto*node=checked(record.next)){record.headAttribute=node->attribute;record.next=reinterpret_cast<uintptr_t>(node->next);release(node);}else record.headAttribute=0xffffffffu;return;}
  auto*previous=static_cast<RecoveredReverseReferenceNodeCandidate*>(nullptr);auto*node=checked(record.next);
  while(node){if(node->attribute==attribute){if(previous)previous->next=node->next;else record.next=reinterpret_cast<uintptr_t>(node->next);release(node);return;}previous=node;node=checked(reinterpret_cast<uintptr_t>(node->next));}
 }
 std::vector<uint32_t>tokens(const RecoveredTransferReferenceRecordCandidate&record)const{
  std::vector<uint32_t>result;if(record.headAttribute==0xffffffffu)return result;result.push_back(record.headAttribute);
  for(auto*node=checked(record.next);node;node=checked(reinterpret_cast<uintptr_t>(node->next))){if(result.size()>nodes.size())throw std::runtime_error("reference list cycle");result.push_back(node->attribute);}return result;
 }
};
