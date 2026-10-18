#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <optional>
#include <stdexcept>
inline std::wstring exportArgumentQuoteCandidate(const std::wstring& value){
 std::wstring result=L"\"";size_t slashes=0;
 for(wchar_t character:value){
  if(character==L'\\'){++slashes;continue;}
  if(character==L'\"')result.append(slashes*2+1,L'\\');else result.append(slashes,L'\\');
  result.push_back(character);slashes=0;
 }
 result.append(slashes*2,L'\\');result.push_back(L'\"');return result;
}
class ExportProcessCandidate {
 HANDLE job_=nullptr,process_=nullptr;
public:
 ExportProcessCandidate(const std::wstring& application,const std::vector<std::wstring>& arguments){
  job_=CreateJobObjectW(nullptr,nullptr);if(!job_)throw std::runtime_error("Export job creation failed");
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  if(!SetInformationJobObject(job_,JobObjectExtendedLimitInformation,&limits,sizeof(limits))){CloseHandle(job_);job_=nullptr;throw std::runtime_error("Export job containment unavailable");}
  std::wstring command=exportArgumentQuoteCandidate(application);for(const auto&argument:arguments){command+=L' ';command+=exportArgumentQuoteCandidate(argument);}
  STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;PROCESS_INFORMATION process{};
  if(!CreateProcessW(application.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_SUSPENDED|CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process)){CloseHandle(job_);job_=nullptr;throw std::runtime_error("Export process creation failed");}
  if(!AssignProcessToJobObject(job_,process.hProcess)||ResumeThread(process.hThread)==DWORD(-1)){
   TerminateProcess(process.hProcess,1);WaitForSingleObject(process.hProcess,5000);CloseHandle(process.hThread);CloseHandle(process.hProcess);CloseHandle(job_);job_=nullptr;throw std::runtime_error("Export process containment/start failed");
  }
  CloseHandle(process.hThread);process_=process.hProcess;
 }
 ExportProcessCandidate(const ExportProcessCandidate&)=delete;
 ExportProcessCandidate&operator=(const ExportProcessCandidate&)=delete;
 ~ExportProcessCandidate(){if(job_)CloseHandle(job_);if(process_)CloseHandle(process_);}
 std::optional<DWORD> poll()const{
  auto result=WaitForSingleObject(process_,0);if(result==WAIT_TIMEOUT)return {};
  if(result!=WAIT_OBJECT_0)throw std::runtime_error("Export process wait failed");
  DWORD code{};if(!GetExitCodeProcess(process_,&code))throw std::runtime_error("Export process exit query failed");return code;
 }
};
