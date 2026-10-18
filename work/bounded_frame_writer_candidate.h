#pragma once
#include <filesystem>
#include <string>
#include <cstdint>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <exception>
#include <stdexcept>
// One owned frame may be outstanding. submit() supplies backpressure.
class BoundedFrameWriterCandidate {
 std::mutex mutex;std::condition_variable wake,done;
 bool stopping=false,busy=false,queued=false;std::filesystem::path path;std::vector<uint8_t>bytes;std::exception_ptr failure;std::thread thread;
 void run(){for(;;){std::unique_lock<std::mutex>lock(mutex);wake.wait(lock,[&]{return stopping||queued;});if(stopping&&!queued)return;auto target=std::move(path);auto data=std::move(bytes);queued=false;lock.unlock();std::exception_ptr error;try{write(target,data);}catch(...){error=std::current_exception();}lock.lock();if(error&&!failure)failure=error;busy=false;done.notify_all();}}
public:
 static void write(const std::filesystem::path&target,const std::vector<uint8_t>&data){std::ofstream file(target,std::ios::binary);file.write(reinterpret_cast<const char*>(data.data()),std::streamsize(data.size()));file.close();if(!file)throw std::runtime_error("frame output write failed: "+target.string());}
 BoundedFrameWriterCandidate():thread([this]{run();}){}
 ~BoundedFrameWriterCandidate(){ {std::unique_lock<std::mutex>lock(mutex);done.wait(lock,[&]{return !busy;});stopping=true;}wake.notify_one();if(thread.joinable())thread.join();}
 BoundedFrameWriterCandidate(const BoundedFrameWriterCandidate&)=delete;
 BoundedFrameWriterCandidate&operator=(const BoundedFrameWriterCandidate&)=delete;
 void submit(std::filesystem::path target,std::vector<uint8_t>data){std::unique_lock<std::mutex>lock(mutex);done.wait(lock,[&]{return !busy;});if(failure)std::rethrow_exception(failure);path=std::move(target);bytes=std::move(data);busy=queued=true;wake.notify_one();}
 void flush(){std::unique_lock<std::mutex>lock(mutex);done.wait(lock,[&]{return !busy;});if(failure)std::rethrow_exception(failure);}
};
