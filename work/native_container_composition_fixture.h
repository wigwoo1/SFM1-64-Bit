#pragma once
#include "controller_container_insertion_candidate.h"
#include "controller_container_resize_candidate.h"
#include "controller_container_search_candidate.h"
#include "controller_container_lookup_candidate.h"
#include "controller_registration_hash_comparison_candidate.h"
#include <cassert>
#include <cstdlib>
#include <vector>
#include <unordered_set>
#include <iostream>
struct Item{std::uint32_t code;};
struct Node{Item* data;Node* next;std::uint32_t hash;};
struct Adapter{
 Node** buckets=static_cast<Node**>(std::calloc(16,sizeof(Node*)));
 unsigned count=0,active=8,capacity=16,mod=8,split=0,error=0,inserted=0,replaced=0,splitCount=0,doubleCount=0,calls=0,visits=0,compares=0,hits=0,misses=0;
 bool failBuckets=false,failNode=false;
 Adapter(){assert(buckets);} Adapter(const Adapter&)=delete;
 ~Adapter(){for(unsigned i=0;i<capacity;++i){auto* n=buckets[i];while(n){auto* next=n->next;delete n;n=next;}}std::free(buckets);}
 unsigned itemCount(){return count;}void setItemCount(unsigned n){count=n;}
 unsigned activeBuckets(){return active;}void setActiveBuckets(unsigned n){active=n;}
 unsigned divisor10(){return capacity;}void setDivisor10(unsigned n){capacity=n;}
 unsigned divisor18(){return mod;}void setDivisor18(unsigned n){mod=n;}
 unsigned split14(){return split;}void setSplit14(unsigned n){split=n;}
 unsigned lookupError(){return error;}void setLookupError(unsigned n){error=n;}
 unsigned growThreshold(){return 512;}
 unsigned insertions(){return inserted;}void setInsertions(unsigned n){inserted=n;}
 unsigned replacements(){return replaced;}void setReplacements(unsigned n){replaced=n;}
 unsigned splits(){return splitCount;}void setSplits(unsigned n){splitCount=n;}
 unsigned doublings(){return doubleCount;}void setDoublings(unsigned n){doubleCount=n;}
 unsigned hashCalls(){return calls;}void setHashCalls(unsigned n){calls=n;}
 unsigned visited(){return visits;}void setVisited(unsigned n){visits=n;}
 unsigned comparisons(){return compares;}void setComparisons(unsigned n){compares=n;}
 unsigned hitCount(){return hits;}void setHitCount(unsigned n){hits=n;}
 unsigned missCount(){return misses;}void setMissCount(unsigned n){misses=n;}
 Node** reallocateBuckets(unsigned n){return failBuckets?nullptr:static_cast<Node**>(std::realloc(buckets,n*sizeof(Node*)));}
 void setBucketStorage(Node** p){buckets=p;}Node** bucketSlot(unsigned n){assert(n<capacity);return buckets+n;}
 void resizeContainer(){resizeControllerContainerCandidate(*this);}
 unsigned hashKey(const unsigned* p){return controllerRegistrationHashCandidate(*p);}
 auto compareCallback(){return +[](Item* a,const unsigned* b){return controllerRegistrationCompareCandidate(a->code,*b);};}
 Node** searchItemSlot(Item* p,unsigned* h){return searchControllerContainerCandidate(&p->code,h,*this);}
 Node** findNodeSlot(const unsigned* p,unsigned* h){return searchControllerContainerCandidate(p,h,*this);}
 Node* allocateNode(){return failNode?nullptr:new Node{};}Item* nodePayload(Node* n){return n->data;}Item* nullPayload(){return nullptr;}
 void validate(){std::unordered_set<Node*> seen;for(unsigned i=0;i<capacity;++i)for(auto* n=buckets[i];n;n=n->next){assert(seen.insert(n).second);auto bucket=n->hash%mod;if(bucket<split)bucket=n->hash%capacity;assert(bucket==i);}assert(seen.size()==count);}
};
