#include <cstdlib>
#include <new>
#include <iostream>
#include "runtime_undo_journal.h"
static bool failAllocation=false;
void* operator new(size_t bytes){if(failAllocation)throw std::bad_alloc();if(void*p=std::malloc(bytes?bytes:1))return p;throw std::bad_alloc();}
void operator delete(void*p)noexcept{std::free(p);}
void operator delete(void*p,size_t)noexcept{std::free(p);}
int main(){RuntimeUndoJournal<int>journal;journal.redo.push_back({42,true});journal.beginBoundary();bool rejected=false;failAllocation=true;try{journal.append(7);}catch(const std::bad_alloc&){rejected=true;}failAllocation=false;
 if(!rejected||!journal.undo.empty()||journal.redo.size()!=1||journal.redo[0].record!=42||!journal.pendingBoundary)return 1;
 journal.append(7);if(!journal.redo.empty()||journal.undo.size()!=1||!journal.undo[0].boundary||journal.pendingBoundary)return 2;
 for(bool reverse:{true,false}){RuntimeUndoJournal<int>transfer;transfer.flags=0xa8;if(reverse)transfer.undo.push_back({17,true});else transfer.redo.push_back({17,true});unsigned callbacks=0;rejected=false;failAllocation=true;try{auto replay=[&](int,bool){++callbacks;};if(reverse)transfer.reverse(replay);else transfer.forward(replay);}catch(const std::bad_alloc&){rejected=true;}failAllocation=false;
 if(!rejected||callbacks||transfer.flags!=0xa8||(reverse?transfer.undo.size():transfer.redo.size())!=1||(reverse?transfer.redo.size():transfer.undo.size())!=0)return 3;
 auto replay=[&](int value,bool direction){if(value!=17||direction!=reverse||(transfer.flags&8))throw std::runtime_error("transfer retry mismatch");++callbacks;};if((reverse?transfer.reverse(replay):transfer.forward(replay))!=1||callbacks!=1||transfer.flags!=0xa8)return 4;
 }
 std::cout<<"undo append and both replay transfer allocation failures preserve history; retries commit\n";
}
