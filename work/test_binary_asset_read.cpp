#include "read_binary_asset_candidate.h"
#include <chrono>
#include <iostream>
#include <string>
#include <algorithm>
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("actual asset required");
 auto start=std::chrono::steady_clock::now();auto bytes=readBinaryAssetCandidate(argv[1]);auto bulkEnd=std::chrono::steady_clock::now();
 std::ifstream file(argv[1],std::ios::binary);if(!file)throw std::runtime_error("reference open failed");
 std::vector<uint8_t>reference((std::istreambuf_iterator<char>(file)),{});auto oldEnd=std::chrono::steady_clock::now();
 if(bytes!=reference)throw std::runtime_error("asset bytes changed");
 std::vector<double>bulkTimes,iteratorTimes;
 for(int iteration=0;iteration<8;++iteration){
  auto measure=[&](bool bulk){auto begin=std::chrono::steady_clock::now();std::vector<uint8_t>value;
   if(bulk)value=readBinaryAssetCandidate(argv[1]);else{std::ifstream input(argv[1],std::ios::binary);value.assign(std::istreambuf_iterator<char>(input),{});}
   auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
   if(value!=bytes)throw std::runtime_error("paired read changed bytes");return elapsed;};
  double bulkMs,iteratorMs;if(iteration%2){iteratorMs=measure(false);bulkMs=measure(true);}else{bulkMs=measure(true);iteratorMs=measure(false);}
  if(iteration>=2){bulkTimes.push_back(bulkMs);iteratorTimes.push_back(iteratorMs);}
 }
 auto median=[](std::vector<double>values){std::sort(values.begin(),values.end());return (values[2]+values[3])*.5;};
 auto directory=std::filesystem::path("work")/("binary-read-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 if(!std::filesystem::create_directory(directory))throw std::runtime_error("test folder already exists");
 auto path=directory/"fixture.bin";
 {std::ofstream out(path,std::ios::binary);}
 if(!readBinaryAssetCandidate(path).empty())throw std::runtime_error("empty file changed");
 std::vector<uint8_t>pattern(1024);for(size_t n=0;n<pattern.size();++n)pattern[n]=uint8_t(n);
 {std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(pattern.data()),std::streamsize(pattern.size()));}
 if(readBinaryAssetCandidate(path)!=pattern)throw std::runtime_error("binary pattern changed");
 bool rejected=false;try{readBinaryAssetCandidate(directory/"missing.bin");}catch(const std::runtime_error&){rejected=true;}
 if(!rejected)throw std::runtime_error("missing asset accepted");
 std::filesystem::remove(path);std::filesystem::remove(directory);
 std::cout<<"PASS asset bytes="<<bytes.size()<<" bulk_read_ms="<<std::chrono::duration<double,std::milli>(bulkEnd-start).count()<<" iterator_read_ms="<<std::chrono::duration<double,std::milli>(oldEnd-bulkEnd).count()<<" empty_binary_missing_cases=passed timing_scope=single_order_observation\n";
 std::cout<<"paired_warm_bulk_median_ms="<<median(bulkTimes)<<" paired_warm_iterator_median_ms="<<median(iteratorTimes)<<" timed_pairs=6 alternating_order=1\n";
 return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
