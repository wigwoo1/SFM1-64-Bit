#include <iostream>
#include <stdexcept>
#include "runtime_undo_journal.h"
int main(){try{static_assert(sizeof(void*)==8,"x64 required");RuntimeUndoJournal<int>j;
 j.beginBoundary();j.append(1);j.append(2);j.beginBoundary();j.append(3);j.append(4);
 std::vector<int>events;auto replay=[&](int v,bool reverse){if(j.flags&8)throw std::runtime_error("recording enabled during replay");events.push_back(reverse?-v:v);};
 if(j.reverse(replay)!=2||events!=std::vector<int>{-4,-3}||j.undo.size()!=2||j.redo.size()!=2)throw std::runtime_error("first reverse boundary failed");
 if(j.reverse(replay)!=2||events!=std::vector<int>{-4,-3,-2,-1})throw std::runtime_error("second reverse boundary failed");
 events.clear();if(j.forward(replay)!=2||events!=std::vector<int>{1,2}||j.redo.size()!=2)throw std::runtime_error("first forward boundary failed");
 if(j.forward(replay)!=2||events!=std::vector<int>{1,2,3,4}||!j.redo.empty()||j.flags!=8)throw std::runtime_error("second forward boundary failed");
 j.reverse(replay);j.append(5);if(!j.redo.empty())throw std::runtime_error("new edit retained redo");
 RuntimeUndoJournal<int>f;f.beginBoundary();f.append(1);f.append(2);bool caught=false;try{f.reverse([](int value,bool){if(value==1)throw std::runtime_error("injected replay failure");});}catch(const std::runtime_error&){caught=true;}if(!caught||f.flags!=8||f.undo.size()!=1||f.undo.back().record!=1||!f.undo.back().boundary||f.redo.size()!=1||f.redo.back().record!=2)throw std::runtime_error("reverse failure lost history");f.reverse([](int,bool){});caught=false;try{f.forward([](int,bool){throw std::runtime_error("injected forward failure");});}catch(const std::runtime_error&){caught=true;}if(!caught||f.flags!=8||!f.undo.empty()||f.redo.size()!=2||f.redo.back().record!=1||!f.redo.back().boundary)throw std::runtime_error("forward failure lost history");if(f.forward([](int,bool){})!=2||f.undo.size()!=2||!f.redo.empty())throw std::runtime_error("retry failed");
 std::cout<<"undo_journal_order_checks=5 replay_failure_checks=3\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
