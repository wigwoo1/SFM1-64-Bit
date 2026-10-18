#include "controller_container_search_candidate.h"
#include <cassert>
#include <iostream>
struct Node{const unsigned* data;Node* next;unsigned hash;};
struct Adapter{unsigned calls=0,visits=0,compares=0,selected=99,split=0;Node* buckets[8]={};
 unsigned hashKey(const unsigned*){return 13;}unsigned hashCalls(){return calls;}void setHashCalls(unsigned n){calls=n;}unsigned divisor18(){return 8;}unsigned split14(){return split;}unsigned divisor10(){return 4;}
 auto compareCallback(){return +[](const unsigned* a,const unsigned* b){return *a==*b?0:1;};}
 Node** bucketSlot(unsigned index){selected=index;return &buckets[index];}unsigned visited(){return visits;}void setVisited(unsigned n){visits=n;}unsigned comparisons(){return compares;}void setComparisons(unsigned n){compares=n;}
};
int main(){unsigned key=7,other=9,hash=0;Node third{&key,nullptr,13},second{&other,&third,13},first{&key,&second,12};Adapter a;a.buckets[5]=&first;assert(searchControllerContainerCandidate(&key,&hash,a)==&second.next && hash==13 && a.calls==1 && a.visits==3 && a.compares==2 && a.selected==5);
 Adapter b;b.split=6;b.buckets[1]=&second;assert(searchControllerContainerCandidate(&other,&hash,b)==&b.buckets[1] && b.selected==1 && b.visits==1);
 unsigned missing=42;Adapter c;c.buckets[5]=&first;assert(searchControllerContainerCandidate(&missing,&hash,c)==&third.next && !third.next && c.visits==3);
 Adapter d;assert(searchControllerContainerCandidate(&key,&hash,d)==&d.buckets[5] && !d.visits);
 std::cout<<"x64 bucket search: split selection, hash filtering, collision traversal and matching/final-null link passed\n";
}
