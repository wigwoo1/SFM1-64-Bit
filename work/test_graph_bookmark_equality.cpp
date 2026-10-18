#include "graph_bookmark_equality_candidate.h"
#include <array>
#include <cassert>
#include <iostream>
struct Adapter{int count=3,reads=0;std::array<unsigned,3> times{0x80000000,0xffffffff,7},order{2,0,1},bookmarks{7,0x80000000,0xffffffff};
 int bookmarkCount105079b0(int c){assert(c==2);return count;}unsigned sourceMappedTime(int c,int i){assert(c==2);++reads;return times[order[i]];}unsigned bookmarkTime10517210(int i,int c){assert(c==2);return bookmarks[i];}
};
int main(){Adapter a;assert(graphBookmarkEqualityCandidate(3,2,a) && a.reads==3);a.count=2;a.reads=0;assert(!graphBookmarkEqualityCandidate(3,2,a) && !a.reads);a.count=3;a.bookmarks[1]=0;assert(!graphBookmarkEqualityCandidate(3,2,a) && a.reads==2);a.count=0;a.reads=0;assert(graphBookmarkEqualityCandidate(0,2,a) && !a.reads);std::cout<<"x64 bookmark equality: count gate, mapped raw times, first mismatch and empty array passed\n";}
