#include "retained_preparation_worker_candidate.h"
#include <iostream>
#include <atomic>
int main(){try{
 std::thread::id id;{RetainedPreparationWorkerCandidate worker;for(int n=0;n<100;++n){auto result=worker.submit([n]{return std::make_pair(std::this_thread::get_id(),n*n);}).get();if(!n)id=result.first;if(id!=result.first||result.second!=n*n||id==std::this_thread::get_id())throw std::runtime_error("worker thread reuse");}
 auto failed=worker.submit([]()->int{throw std::runtime_error("expected preparation failure");});bool caught=false;try{failed.get();}catch(const std::runtime_error&e){caught=std::string(e.what())=="expected preparation failure";}if(!caught||worker.submit([]{return 7;}).get()!=7)throw std::runtime_error("worker exception recovery");
 std::promise<void>entered,release;auto gate=release.get_future().share();auto active=worker.submit([&]{entered.set_value();gate.wait();return 1;});entered.get_future().wait();auto queued=worker.submit([]{return 2;});bool full=false;try{worker.submit([]{return 3;});}catch(const std::runtime_error&e){full=std::string(e.what())=="preparation queue full";}release.set_value();if(!full||active.get()!=1||queued.get()!=2)throw std::runtime_error("bounded preparation queue");}
 std::atomic<int>drained{0};std::future<int>last;{RetainedPreparationWorkerCandidate worker;last=worker.submit([&]{drained=1;return 9;});}if(drained!=1||last.get()!=9)throw std::runtime_error("destructor failed to drain");
 std::cout<<"retained preparation worker passed same_thread_tasks=100 exception_recovery=1 bounded_queue=1 destructor_drain=1\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
