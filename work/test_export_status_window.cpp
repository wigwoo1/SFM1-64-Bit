#define NOMINMAX
#include <stdexcept>
#include "win32_export_status_candidate.h"
#include <iostream>
int main(){try{
 WNDCLASSW cls{};cls.hInstance=GetModuleHandleW(nullptr);cls.lpfnWndProc=exportStatusWindowProcCandidate;cls.lpszClassName=L"ExportStatusTest";if(!RegisterClassW(&cls))throw std::runtime_error("Test class registration");
 exportStatusTextCandidate=[](){return std::string("Converting Source session\r\nAnimation baking");};
 HWND window=CreateWindowExW(0,cls.lpszClassName,L"test",WS_OVERLAPPED,0,0,520,240,nullptr,nullptr,cls.hInstance,nullptr);
 if(!window)throw std::runtime_error("Test window creation");
 wchar_t text[512]{};GetWindowTextW(GetDlgItem(window,1),text,512);
 if(std::wstring(text)!=L"Converting Source session\r\nAnimation baking")throw std::runtime_error("Initial status text");
 exportStatusTextCandidate=[](){return std::string("Rendering with Cycles");};SendMessageW(window,WM_TIMER,1,0);GetWindowTextW(GetDlgItem(window,1),text,512);
 if(std::wstring(text)!=L"Rendering with Cycles")throw std::runtime_error("Timer status refresh");
 exportStatusTextCandidate=[]()->std::string{throw std::runtime_error("Status read failed");};SendMessageW(window,WM_TIMER,1,0);GetWindowTextW(GetDlgItem(window,1),text,512);
 if(std::wstring(text)!=L"Status read failed")throw std::runtime_error("Status error handling");
 SendMessageW(window,WM_CLOSE,0,0);if(IsWindow(window))throw std::runtime_error("Window close");
 std::cout<<"PASS native status initial text, timer updates, read error and isolated window closure\n";return 0;
 }catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
