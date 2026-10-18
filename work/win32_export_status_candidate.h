#pragma once
#include <windows.h>
#include <functional>
#include <string>
inline HWND exportStatusWindowCandidate=nullptr;
inline std::function<std::string()> exportStatusTextCandidate;
inline void refreshExportStatusCandidate(HWND window){
 std::string text;
 try{text=exportStatusTextCandidate?exportStatusTextCandidate():"No export request.";}catch(const std::exception&error){text=error.what();}
 if(text.empty())text="Waiting for export progress...";
 int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),nullptr,0);
 std::wstring wide(count>0?count:0,L'\0');
 if(count>0)MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),wide.data(),count);
 else wide=L"Export progress could not be decoded.";
 auto edit=GetDlgItem(window,1);int length=GetWindowTextLengthW(edit);
 std::wstring old(size_t(length)+1,L'\0');GetWindowTextW(edit,old.data(),length+1);old.resize(length);
 if(old!=wide)SetWindowTextW(edit,wide.c_str());
}
inline LRESULT CALLBACK exportStatusWindowProcCandidate(HWND window,UINT message,WPARAM key,LPARAM value){
 if(message==WM_CREATE){auto edit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL,12,12,480,180,window,reinterpret_cast<HMENU>(INT_PTR(1)),GetModuleHandleW(nullptr),nullptr);SendMessageW(edit,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);SetTimer(window,1,500,nullptr);refreshExportStatusCandidate(window);return 0;}
 if(message==WM_TIMER){refreshExportStatusCandidate(window);return 0;}
 if(message==WM_CLOSE){DestroyWindow(window);return 0;}
 if(message==WM_DESTROY){KillTimer(window,1);exportStatusWindowCandidate=nullptr;return 0;}
 return DefWindowProcW(window,message,key,value);
}
inline void showExportStatusCandidate(HWND owner){
 if(exportStatusWindowCandidate){ShowWindow(exportStatusWindowCandidate,SW_SHOW);SetForegroundWindow(exportStatusWindowCandidate);return;}
 WNDCLASSW cls{};cls.hInstance=GetModuleHandleW(nullptr);cls.lpfnWndProc=exportStatusWindowProcCandidate;cls.lpszClassName=L"Sfm64ExportStatusCandidate";cls.hbrBackground=GetSysColorBrush(COLOR_BTNFACE);cls.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));
 if(!RegisterClassW(&cls)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("Export status window registration failed");
 exportStatusWindowCandidate=CreateWindowExW(WS_EX_TOOLWINDOW,cls.lpszClassName,L"Export progress",WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,520,240,owner,nullptr,cls.hInstance,nullptr);
 if(!exportStatusWindowCandidate)throw std::runtime_error("Export status window creation failed");ShowWindow(exportStatusWindowCandidate,SW_SHOW);
}
