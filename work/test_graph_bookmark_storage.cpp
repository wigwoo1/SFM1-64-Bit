#include "graph_bookmark_storage_candidate.h"
#include <vector>
#include <cassert>
#include <iostream>
struct Adapter{std::vector<unsigned> times{0x80000000,7};unsigned assignments=0,countCalls=0;
 int componentCountSlot62(){++countCalls;return 1;}int storedCount90(int c){assert(!c);return int(times.size());}unsigned storedTime84(int c,int i){assert(!c);return times[i];}
 const unsigned* sourceBase(const std::vector<unsigned>& v){return v.data();}int sourceCount(const std::vector<unsigned>& v){return int(v.size());}
 void assignVector1038cd00(int c,const unsigned* p,int count){assert(!c);++assignments;if(count)times.assign(p,p+count);else times.clear();}
};
int main(){Adapter a;assert(graphBookmarkCountCandidate(0,a)==2 && graphBookmarkCountCandidate(1,a)==-1);assert(graphBookmarkTimeCandidate(0,0,a)==0x80000000 && graphBookmarkTimeCandidate(2,0,a)==0 && graphBookmarkTimeCandidate(0,1,a)==0);std::vector<unsigned> v{9,8,7};graphReplaceBookmarksCandidate(1,v,a);assert(!a.assignments);graphReplaceBookmarksCandidate(0,v,a);assert(a.times==v && a.assignments==1);v.clear();graphReplaceBookmarksCandidate(0,v,a);assert(a.times.empty());std::cout<<"x64 bookmark storage: signed upper bounds, raw time values, replacement and empty vectors passed\n";}
