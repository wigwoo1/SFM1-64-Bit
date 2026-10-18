#pragma once
#include <algorithm>
#include <commctrl.h>
#include "session_track_group_rows_candidate.h"

struct TrackGroupVisibilityDialogCandidate {
 struct Edit { uint32_t element; bool visible; std::string name; };
 std::vector<SessionTrackGroupRowCandidate> rows;
 std::vector<Edit> edits;
 HWND list=nullptr;
};

inline INT_PTR CALLBACK trackGroupVisibilityDialogProcCandidate(HWND dialog,UINT message,WPARAM command,LPARAM parameter) {
 auto* context=reinterpret_cast<TrackGroupVisibilityDialogCandidate*>(GetWindowLongPtrA(dialog,DWLP_USER));
 if(message==WM_INITDIALOG) {
  context=reinterpret_cast<TrackGroupVisibilityDialogCandidate*>(parameter);SetWindowLongPtrA(dialog,DWLP_USER,parameter);
  SetWindowTextA(dialog,"Track Group Visibility");auto instance=GetModuleHandleA(nullptr);const auto scale=[](int value){return sfmEditorScaleMetricCandidate(value);};const auto font=sfmEditorShellCandidate.uiFont?sfmEditorShellCandidate.uiFont:GetStockObject(DEFAULT_GUI_FONT);
  auto note=CreateWindowExA(0,"STATIC","The first row may be a special clip-level track group.",WS_CHILD|WS_VISIBLE,scale(12),scale(10),scale(560),scale(20),dialog,nullptr,instance,nullptr);
  context->list=CreateWindowExA(WS_EX_CLIENTEDGE,WC_LISTVIEWA,"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|LVS_REPORT|LVS_EDITLABELS|LVS_SHOWSELALWAYS,scale(12),scale(34),scale(560),scale(350),dialog,reinterpret_cast<HMENU>(100),instance,nullptr);
  auto ok=CreateWindowExA(0,"BUTTON","OK",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,scale(372),scale(396),scale(92),scale(28),dialog,reinterpret_cast<HMENU>(IDOK),instance,nullptr);
  auto cancel=CreateWindowExA(0,"BUTTON","Cancel",WS_CHILD|WS_VISIBLE|WS_TABSTOP,scale(476),scale(396),scale(96),scale(28),dialog,reinterpret_cast<HMENU>(IDCANCEL),instance,nullptr);
  if(!note||!context->list||!ok||!cancel){EndDialog(dialog,-1);return TRUE;}
  for(auto hwnd:{note,context->list,ok,cancel})SendMessageA(hwnd,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);
  ListView_SetExtendedListViewStyle(context->list,LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER);
  LVCOLUMNA column{};column.mask=LVCF_TEXT|LVCF_WIDTH;column.pszText=const_cast<char*>("Track Group Name");column.cx=scale(525);ListView_InsertColumn(context->list,0,&column);
  for(size_t i=0;i<context->rows.size();++i){const auto&row=context->rows[i];LVITEMA item{};item.mask=LVIF_TEXT|LVIF_PARAM;item.iItem=int(i);item.pszText=const_cast<char*>(row.name.c_str());item.lParam=LPARAM(row.element);if(ListView_InsertItem(context->list,&item)<0){EndDialog(dialog,-1);return TRUE;}ListView_SetCheckState(context->list,int(i),row.visible);}
  return TRUE;
 }
 if(message==WM_NOTIFY&&context){auto*header=reinterpret_cast<NMHDR*>(parameter);if(header->hwndFrom==context->list&&header->code==LVN_ENDLABELEDITA){auto*edit=reinterpret_cast<NMLVDISPINFOA*>(parameter);if(edit->item.pszText&&edit->item.pszText[0])return TRUE;return FALSE;}}
 if(message==WM_CLOSE){EndDialog(dialog,IDCANCEL);return TRUE;}
 if(message==WM_COMMAND&&context){if(LOWORD(command)==IDCANCEL){EndDialog(dialog,IDCANCEL);return TRUE;}if(LOWORD(command)==IDOK){try{context->edits.clear();for(size_t i=0;i<context->rows.size();++i){char name[1024]{};ListView_GetItemText(context->list,int(i),0,name,sizeof(name));if(!name[0]){MessageBoxA(dialog,"Track group names cannot be empty.","Invalid name",MB_OK|MB_ICONERROR);return TRUE;}const auto&row=context->rows[i];const bool visible=ListView_GetCheckState(context->list,int(i))!=FALSE;if(name!=row.name||visible!=row.visible)context->edits.push_back({row.element,visible,name});}EndDialog(dialog,IDOK);return TRUE;}catch(...){EndDialog(dialog,-1);return TRUE;}}}
 return FALSE;
}

inline std::vector<TrackGroupVisibilityDialogCandidate::Edit> showTrackGroupVisibilityDialogCandidate(HWND parent,const Graph&graph){
 TrackGroupVisibilityDialogCandidate context;context.rows=sessionTrackGroupRowsCandidate(graph);
 if(context.rows.empty()){MessageBoxA(parent,"This session has no track groups.","Track Group Visibility",MB_OK);return {};}
 struct Template{DLGTEMPLATE dialog;WORD menu,windowClass,title;}layout{};layout.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_CENTER;const auto base=GetDialogBaseUnits();const int baseX=LOWORD(base),baseY=HIWORD(base);if(!baseX||!baseY)throw std::runtime_error("dialog base units unavailable");layout.dialog.cx=short(std::clamp(MulDiv(sfmEditorScaleMetricCandidate(585),4,baseX),1,32767));layout.dialog.cy=short(std::clamp(MulDiv(sfmEditorScaleMetricCandidate(437),8,baseY),1,32767));
 auto result=DialogBoxIndirectParamA(GetModuleHandleA(nullptr),&layout.dialog,parent,trackGroupVisibilityDialogProcCandidate,reinterpret_cast<LPARAM>(&context));if(result==-1)throw std::runtime_error("track group visibility dialog creation failed");return context.edits;
}
