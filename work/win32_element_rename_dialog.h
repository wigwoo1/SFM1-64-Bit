#pragma once
#include <functional>
struct ElementRenameDialogCandidate{uint32_t element=0;std::string original;std::string title;HWND edit=nullptr;std::function<bool(uint32_t,std::string)>submit;bool queued=false;};
inline INT_PTR CALLBACK elementRenameDialogProcCandidate(HWND dialog,UINT message,WPARAM command,LPARAM parameter){
 auto*context=reinterpret_cast<ElementRenameDialogCandidate*>(GetWindowLongPtrA(dialog,DWLP_USER));
 if(message==WM_INITDIALOG){context=reinterpret_cast<ElementRenameDialogCandidate*>(parameter);SetWindowLongPtrA(dialog,DWLP_USER,parameter);SetWindowTextA(dialog,context->title.c_str());auto instance=GetModuleHandleA(nullptr);
  auto label=CreateWindowExA(0,"STATIC","New Name:",WS_CHILD|WS_VISIBLE,12,12,340,20,dialog,nullptr,instance,nullptr);
  context->edit=CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT",context->original.c_str(),WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,12,36,340,24,dialog,reinterpret_cast<HMENU>(100),instance,nullptr);
  auto apply=CreateWindowExA(0,"BUTTON","OK",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,152,72,92,26,dialog,reinterpret_cast<HMENU>(IDOK),instance,nullptr);
  auto cancel=CreateWindowExA(0,"BUTTON","Cancel",WS_CHILD|WS_VISIBLE|WS_TABSTOP,260,72,92,26,dialog,reinterpret_cast<HMENU>(IDCANCEL),instance,nullptr);
  if(!label||!context->edit||!apply||!cancel){EndDialog(dialog,-1);return TRUE;}
  for(auto hwnd:{label,context->edit,apply,cancel})SendMessageA(hwnd,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);
  SendMessageA(context->edit,EM_SETLIMITTEXT,255,0);SetFocus(context->edit);SendMessageA(context->edit,EM_SETSEL,0,-1);
  if(GetEnvironmentVariableA("SFM64_DIAGNOSTIC_ELEMENT_RENAME_TEST",nullptr,0)){char value[256]{};auto n=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_ELEMENT_RENAME_VALUE",value,sizeof(value));if(!n||n>=sizeof(value))strcpy_s(value,context->original.c_str()),strcat_s(value,"_renamed");SetWindowTextA(context->edit,value);PostMessageA(dialog,WM_COMMAND,IDOK,0);}return FALSE;
 }
 if(message==WM_CLOSE){EndDialog(dialog,IDCANCEL);return TRUE;}
 if(message==WM_COMMAND&&context){if(LOWORD(command)==IDCANCEL){EndDialog(dialog,IDCANCEL);return TRUE;}if(LOWORD(command)==IDOK){char value[256]{};GetWindowTextA(context->edit,value,sizeof(value));if(!value[0]){MessageBoxA(dialog,"The name cannot be empty.","Invalid name",MB_OK|MB_ICONERROR);return TRUE;}try{context->queued=context->submit&&context->submit(context->element,value);}catch(const std::exception&e){MessageBoxA(dialog,e.what(),"Rename failed",MB_OK|MB_ICONERROR);return TRUE;}if(!context->queued){MessageBoxA(dialog,"The rename could not be queued.","Rename unavailable",MB_OK|MB_ICONERROR);return TRUE;}EndDialog(dialog,IDOK);return TRUE;}}
 return FALSE;
}
inline bool showElementRenameDialogCandidate(HWND parent,const Graph&graph,uint32_t element,const std::function<bool(uint32_t,std::string)>&submit){
 if(element>=graph.elements.size())throw std::runtime_error("rename element outside session");const auto&type=graph.strings.at(graph.elements[element].type);const bool isDag=type=="DmeDag"||type=="DmeGameModel"||type=="DmeCamera"||type=="DmeLight"||type=="DmeParticleSystem";if(!isDag&&type!="DmeAnimationSet")throw std::runtime_error("this element type cannot be renamed from the Animation Set Editor");
 const std::string title=type=="DmeAnimationSet"?"Rename AnimationSet":"Rename Dag";ElementRenameDialogCandidate context{element,graph.strings.at(graph.elements[element].name),title,nullptr,submit,false};struct Template{DLGTEMPLATE dialog;WORD menu,windowClass,title;}layout{};layout.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;layout.dialog.cx=275;layout.dialog.cy=78;
 auto result=DialogBoxIndirectParamA(GetModuleHandleA(nullptr),&layout.dialog,parent,elementRenameDialogProcCandidate,reinterpret_cast<LPARAM>(&context));if(result==-1)throw std::runtime_error("rename dialog creation failed");return context.queued;
}
