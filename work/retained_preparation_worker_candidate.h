#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
// One retained thread; one queued task at most. Destruction drains accepted work.
class RetainedPreparationWorkerCandidate {
 std::mutex mutex_;
 std::condition_variable wake_;
 std::function<void()> pending_;
 bool stopping_=false;
 std::thread worker_;
 void run(){for(;;){std::function<void()> task;{std::unique_lock<std::mutex>lock(mutex_);wake_.wait(lock,[&]{return stopping_||bool(pending_);});if(!pending_){if(stopping_)return;continue;}task=std::move(pending_);pending_={};}task();}}
public:
 RetainedPreparationWorkerCandidate():worker_([this]{run();}){}
 RetainedPreparationWorkerCandidate(const RetainedPreparationWorkerCandidate&)=delete;
 RetainedPreparationWorkerCandidate&operator=(const RetainedPreparationWorkerCandidate&)=delete;
 ~RetainedPreparationWorkerCandidate(){{std::lock_guard<std::mutex>lock(mutex_);stopping_=true;}wake_.notify_one();worker_.join();}
 template<class F>auto submit(F&&function)->std::future<typename std::invoke_result<F>::type>{
  using Result=typename std::invoke_result<F>::type;
  auto task=std::make_shared<std::packaged_task<Result()>>(std::forward<F>(function));auto result=task->get_future();
  {std::lock_guard<std::mutex>lock(mutex_);if(stopping_)throw std::runtime_error("preparation worker stopping");if(pending_)throw std::runtime_error("preparation queue full");pending_=[task]{(*task)();};}wake_.notify_one();return result;
 }
};
