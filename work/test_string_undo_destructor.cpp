#include "string_undo_destructor_candidate.h"
#include <cassert>
#include <iostream>
struct Record { const char* oldValue; const char* newValue; bool base=false; };
struct Adapter { int resets=0,frees=0; void setBaseRecordVtable(Record* r){r->base=true;++resets;} void freeRecord(Record* r){assert(r->base);++frees;} };
int main(){
    const char oldText[]="old",newText[]="new";
    for(std::uint32_t f=0;f<65536;++f){Record r{oldText,newText};Adapter a;
        assert(destroyStringUndoRecordCandidate(&r,f,a)==&r);
        assert(a.resets==1 && a.frees==static_cast<int>(f&1));
        assert(r.oldValue==oldText && r.newValue==newText);
    }
    std::cout<<"x64 string undo destructor: reset before free/low-bit flags/unchanged symbols 65536 cases passed\n";
}
