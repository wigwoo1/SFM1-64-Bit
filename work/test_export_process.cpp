#define NOMINMAX
#include "win32_export_process_candidate.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <filesystem>
int main(){try{
 auto suffix=std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count());
 std::wstring output=L"work/export-process-"+suffix+L".json";
 HANDLE child=nullptr;
 {
 ExportProcessCandidate process(L"C:\\Users\\wigwo\\.cache\\codex-runtimes\\codex-primary-runtime\\dependencies\\python\\python.exe",
  {L"work/export_process_fixture.py",output,L"spaces and \"quotes\"",L"trailing\\"});
 for(int i=0;i<100&&GetFileAttributesW(output.c_str())==INVALID_FILE_ATTRIBUTES;++i)Sleep(50);
 std::ifstream input{std::filesystem::path(output)};std::string line;std::getline(input,line);
 auto at=line.find("\"child\": ");if(at==std::string::npos)throw std::runtime_error("Missing live child receipt");
 auto pid=DWORD(std::stoul(line.substr(at+9)));child=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
 if(!child||WaitForSingleObject(child,0)!=WAIT_TIMEOUT||process.poll())throw std::runtime_error("Fixture exited early");
 }
 if(WaitForSingleObject(child,5000)!=WAIT_OBJECT_0){CloseHandle(child);throw std::runtime_error("Child survived owner shutdown");}CloseHandle(child);
 std::wcout<<L"PASS owned child terminated on native owner close; receipt="<<output<<L'\n';return 0;
 }catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
