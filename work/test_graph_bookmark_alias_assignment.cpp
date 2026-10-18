#include "graph_bookmark_alias_assignment_candidate.h"
#include <cassert>
#include <iostream>
struct Adapter{unsigned data[8]={};int count=4,start=-1,amount=0;unsigned removed=0,inserted=0;unsigned* destinationBase(){return data;}int destinationCount(){return count;}
 void remove103883e0(int s,int n){++removed;start=s;amount=n;}void insert103771c0(int s,int n){++inserted;start=s;amount=n;}
};
int main(){Adapter a;assert(graphBookmarkAliasAssignmentCandidate(a.data,2,a) && a.removed==1 && a.start==2 && a.amount==2);Adapter b;assert(graphBookmarkAliasAssignmentCandidate(b.data,7,b) && b.inserted==1 && b.start==4 && b.amount==3);Adapter c;assert(graphBookmarkAliasAssignmentCandidate(c.data,4,c) && !c.inserted && !c.removed);unsigned other[1];assert(!graphBookmarkAliasAssignmentCandidate(other,1,c));assert(graphAliasDifference(INT32_MAX,-1)==INT32_MIN);std::cout<<"x64 bookmark self-assignment: shrink/grow/equal, distinct pointer and wrapped count difference passed\n";}
