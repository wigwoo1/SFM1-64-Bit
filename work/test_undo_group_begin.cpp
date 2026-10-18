#include "undo_group_begin_candidate.h"
#include <cassert>
#include <iostream>
struct Adapter {
    unsigned char flags=8;std::uint32_t depth=0,counter=99;std::uintptr_t current=7,previous=0,first=0,second=0;int interns=0,logs=0;
    auto flags74(){return flags;}void setFlags74(unsigned char v){flags=v;}
    void logGroupBegin(const char*){++logs;}auto depth54(){return depth;}void setDepth54(std::uint32_t v){depth=v;}
    auto currentContext(){return current;}void setCurrentContext(std::uintptr_t v){current=v;}void setPreviousContext(std::uintptr_t v){previous=v;}
    std::uintptr_t internLabel(const char* p){++interns;return reinterpret_cast<std::uintptr_t>(p);}
    void setFirstLabel(std::uintptr_t v){first=v;}void setSecondLabel(std::uintptr_t v){second=v;}
    void setSubmissionCounter(std::uint32_t v){counter=v;}
};
int main(){char one[]="edit",two[]="edit";Adapter a;
    beginUndoGroupCandidate(one,one,0x100000009ull,a);assert(a.depth==1 && a.interns==1 && a.first==a.second && a.previous==7 && a.current==0x100000009ull && a.flags==9 && !a.counter);
    beginUndoGroupCandidate(two,two,8,a);assert(a.depth==2 && a.interns==1 && a.current==0x100000009ull);
    Adapter b;beginUndoGroupCandidate(one,two,1,b);assert(b.interns==2 && b.first!=b.second);
    Adapter c;c.flags=0;beginUndoGroupCandidate(one,two,1,c);assert(!c.logs && !c.depth && c.counter==99);
    Adapter d;d.depth=0xffffffff;beginUndoGroupCandidate(one,two,1,d);assert(!d.depth && !d.interns);
    std::cout<<"x64 undo group begin: outer/nested/disabled/pointer identity/raw depth wrap passed\n";
}
