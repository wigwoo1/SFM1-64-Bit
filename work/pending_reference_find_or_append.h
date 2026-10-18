#pragma once
#include <cstdint>
//1035ACA0: caller provides the table's hash/equality and allocation policy.
template<class Key,class Find,class Count,class Append>
uint32_t recoveredPendingFindOrAppendCandidate(const Key&key,Find find,Count count,Append append){
 uint32_t bucket=0,index=0;
 if(find(key,bucket,index))return (bucket<<16)|index;
 index=append(bucket,count(bucket),key);
 return (bucket<<16)|index;
}
