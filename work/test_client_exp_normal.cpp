#include "client_exp_normal_candidate.h"
#include <fstream>
#include <iostream>
int main(){
 ClientExpNormalTablesCandidate tables{};
 std::ifstream tableInput("work/client-exp-tables.bin",std::ios::binary);
 if(!tableInput.read(reinterpret_cast<char*>(&tables),sizeof(tables)))return 1;
 std::ifstream input("work/client-exp-input.bin",std::ios::binary);
 std::ofstream output("outputs/sfm64-client-exp-normal-output.bin",std::ios::binary);
 float value;unsigned count=0;
 while(input.read(reinterpret_cast<char*>(&value),sizeof(value))){
  float result=clientExpNormalCandidate(value,tables);
  output.write(reinterpret_cast<char*>(&result),sizeof(result));++count;
 }
 if(!input.eof()||!output||count!=65536)return 2;
 try{clientExpNormalCandidate(0,tables);return 3;}catch(const std::runtime_error&){}
 std::cout<<"x64 recovered exponential normal path cases="<<count<<"\n";
}
