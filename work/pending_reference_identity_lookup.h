#pragma once
#include <cstdint>
//10354D00: first match wins; index output is untouched on a miss.
//Caller must supply nonzero bucket count and valid mask-selected buckets.
template<class Key,class Hash,class Count,class Entry,class Equal>
bool recoveredPendingIdentityLookupCandidate(const Key&key,bool masked,uint32_t mask,uint32_t bucketCount,uint32_t&bucket,uint32_t&index,Hash hash,Count count,Entry entry,Equal equal){
 const uint32_t h=hash(key);
 bucket=masked?(h&mask):(h%bucketCount);
 const uint32_t size=count(bucket);
 for(uint32_t i=0;i<size;++i)if(equal(entry(bucket,i),key)){index=i;return true;}
 return false;
}
