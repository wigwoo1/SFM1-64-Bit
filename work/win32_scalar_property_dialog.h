#pragma once
#include <algorithm>
#include <charconv>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <optional>
#include <utility>
#include "animation_keys.h"
// Included after Graph and the playback deferred edit queue declarations.
struct ScalarPropertyDialogCandidate{
 struct Property{uint32_t element;std::string name,label,value;size_t keyIndex=0;int32_t tick=0;std::optional<float>minimum,maximum;};
 std::vector<Property>properties;std::vector<size_t>visible;HWND choice=nullptr,value=nullptr,search=nullptr;bool queued=false;const char*title="Edit scalar property";std::function<bool(const Property&,float)>submit;
};
inline void selectScalarDialogValueCandidate(ScalarPropertyDialogCandidate&context){auto index=SendMessageA(context.choice,CB_GETCURSEL,0,0);if(index>=0&&size_t(index)<context.visible.size())SetWindowTextA(context.value,context.properties[context.visible[size_t(index)]].value.c_str());}
inline std::string foldScalarPropertyTextCandidate(std::string text){for(auto&c:text)c=char(std::tolower(static_cast<unsigned char>(c)));return text;}
struct NativeScalarRangeCandidate{const char*elementType;const char*name;float minimum,maximum,defaultValue;};
inline std::optional<NativeScalarRangeCandidate>nativeCameraLightScalarRangeCandidate(const std::string&elementType,const std::string&name)noexcept{
 static constexpr NativeScalarRangeCandidate ranges[]={
  {"DmeCamera","fieldOfView",10,120,30},{"DmeCamera","focalDistance",1,200,72},{"DmeCamera","aperture",0,10,.2f},{"DmeCamera","toneMapScale",.001f,10,.28f},{"DmeCamera","bloomScale",0,10,.28f},{"DmeCamera","SSAOBias",0,.01f,.0005f},{"DmeCamera","SSAOStrength",0,25,1},{"DmeCamera","SSAORadius",0,25,15},
  {"DmeProjectedLight","intensity",0,1000,500},{"DmeProjectedLight","ambientIntensity",0,1,.25f},{"DmeProjectedLight","horizontalFOV",10,120,45},{"DmeProjectedLight","verticalFOV",10,120,45},{"DmeProjectedLight","radius",0,50,0},{"DmeProjectedLight","shadowFilterSize",0,24,3},{"DmeProjectedLight","shadowAtten",0,1,1},{"DmeProjectedLight","minDistance",1,300,10},{"DmeProjectedLight","maxDistance",60,3000,600},{"DmeProjectedLight","constantAttenuation",0,1000,0},{"DmeProjectedLight","linearAttenuation",0,1000,0},{"DmeProjectedLight","quadraticAttenuation",0,3000,1500},{"DmeProjectedLight","shadowDepthBias",0,.001f,.00008f},{"DmeProjectedLight","shadowSlopeScaleDepthBias",0,10,2},{"DmeProjectedLight","farZAtten",0,3000,750},{"DmeProjectedLight","nearEdge",0,5,2},{"DmeProjectedLight","farEdge",0,200,100},{"DmeProjectedLight","cutOn",0,200,10},{"DmeProjectedLight","cutOff",0,1200,650},{"DmeProjectedLight","volumetricIntensity",0,10,1},{"DmeProjectedLight","noiseStrength",0,1,.8f},{"DmeProjectedLight","width",0,10,.3f},{"DmeProjectedLight","edgeWidth",0,10,.05f},{"DmeProjectedLight","height",0,10,.3f},{"DmeProjectedLight","edgeHeight",0,10,.05f}
 };
 for(const auto&range:ranges)if(elementType==range.elementType&&name==range.name)return range;
 return{};
}
inline bool scalarValueInNativeRangeCandidate(float value,const std::optional<float>&minimum,const std::optional<float>&maximum)noexcept{
 return std::isfinite(value)&&(!minimum||value>=*minimum)&&(!maximum||value<=*maximum);
}
inline bool filterScalarPropertiesCandidate(HWND dialog,ScalarPropertyDialogCandidate&context)noexcept{try{char search[256]{};GetWindowTextA(context.search,search,sizeof(search));auto needle=foldScalarPropertyTextCandidate(search);SendMessageA(context.choice,CB_RESETCONTENT,0,0);context.visible.clear();for(size_t i=0;i<context.properties.size();++i){const auto&property=context.properties[i];if(foldScalarPropertyTextCandidate(property.label).find(needle)==std::string::npos)continue;if(SendMessageA(context.choice,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(property.label.c_str()))<0)return false;context.visible.push_back(i);}EnableWindow(GetDlgItem(dialog,IDOK),!context.visible.empty());if(context.visible.empty())SetWindowTextA(context.value,"");else{SendMessageA(context.choice,CB_SETCURSEL,0,0);selectScalarDialogValueCandidate(context);}return true;}catch(...){return false;}}
inline INT_PTR CALLBACK scalarPropertyDialogProcCandidate(HWND dialog,UINT message,WPARAM command,LPARAM parameter){
 auto*context=reinterpret_cast<ScalarPropertyDialogCandidate*>(GetWindowLongPtrA(dialog,DWLP_USER));
 if(message==WM_INITDIALOG){context=reinterpret_cast<ScalarPropertyDialogCandidate*>(parameter);SetWindowLongPtrA(dialog,DWLP_USER,parameter);SetWindowTextA(dialog,context->title);auto instance=GetModuleHandleA(nullptr);
 context->search=CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,12,12,520,24,dialog,reinterpret_cast<HMENU>(102),instance,nullptr);
 context->choice=CreateWindowExA(0,"COMBOBOX","",WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,12,44,520,180,dialog,reinterpret_cast<HMENU>(100),instance,nullptr);
 context->value=CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,12,84,200,24,dialog,reinterpret_cast<HMENU>(101),instance,nullptr);
 auto apply=CreateWindowExA(0,"BUTTON","Apply",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,224,84,90,24,dialog,reinterpret_cast<HMENU>(IDOK),instance,nullptr);auto cancel=CreateWindowExA(0,"BUTTON","Cancel",WS_CHILD|WS_VISIBLE|WS_TABSTOP,326,84,90,24,dialog,reinterpret_cast<HMENU>(IDCANCEL),instance,nullptr);
 if(!context->search||!context->choice||!context->value||!apply||!cancel){EndDialog(dialog,-1);return TRUE;}
 for(auto hwnd:{context->search,context->choice,context->value,apply,cancel})SendMessageA(hwnd,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);
 SendMessageA(context->search,EM_SETLIMITTEXT,250,0);SendMessageW(context->search,EM_SETCUEBANNER,TRUE,reinterpret_cast<LPARAM>(L"Search properties"));if(!filterScalarPropertiesCandidate(dialog,*context)){EndDialog(dialog,-1);return TRUE;}
 SendMessageA(context->choice,CB_SETCURSEL,0,0);SendMessageA(context->value,EM_SETLIMITTEXT,100,0);selectScalarDialogValueCandidate(*context);if(context->submit&&GetEnvironmentVariableA("SFM64_DIAGNOSTIC_FLOAT_KEY_TEST",nullptr,0)){bool found=false;for(size_t i=0;i<context->properties.size();++i)if(context->properties[i].element==2814&&context->properties[i].keyIndex==0){SendMessageA(context->choice,CB_SETCURSEL,WPARAM(i),0);SetWindowTextA(context->value,"123");PostMessageA(dialog,WM_COMMAND,IDOK,0);found=true;break;}if(!found)EndDialog(dialog,-1);}if(GetEnvironmentVariableA("SFM64_DIAGNOSTIC_SCALAR_PROPERTY_TEST",nullptr,0)){bool found=false;for(size_t i=0;i<context->properties.size();++i)if(context->properties[i].element==3743&&context->properties[i].name=="value"){SendMessageA(context->choice,CB_SETCURSEL,WPARAM(i),0);SetWindowTextA(context->value,"0.25");PostMessageA(dialog,WM_COMMAND,IDOK,0);found=true;break;}if(!found)EndDialog(dialog,-1);}return TRUE;
 }
 if(message==WM_CLOSE){EndDialog(dialog,IDCANCEL);return TRUE;}
 if(message==WM_COMMAND&&context){if(LOWORD(command)==102&&HIWORD(command)==EN_CHANGE&&context->choice&&context->value){if(!filterScalarPropertiesCandidate(dialog,*context))EndDialog(dialog,-1);return TRUE;}if(LOWORD(command)==100&&HIWORD(command)==CBN_SELCHANGE){selectScalarDialogValueCandidate(*context);return TRUE;}if(LOWORD(command)==IDCANCEL){EndDialog(dialog,IDCANCEL);return TRUE;}if(LOWORD(command)==IDOK){auto index=SendMessageA(context->choice,CB_GETCURSEL,0,0);char text[128]{};GetWindowTextA(context->value,text,sizeof(text));float number{};auto end=text+std::strlen(text);auto parsed=std::from_chars(text,end,number);if(index<0||size_t(index)>=context->visible.size()||parsed.ec!=std::errc{}||parsed.ptr!=end||!std::isfinite(number)){MessageBoxA(dialog,"Enter a valid finite numeric value.","Invalid value",MB_OK|MB_ICONERROR);return TRUE;}const auto&property=context->properties[context->visible[size_t(index)]];if(!scalarValueInNativeRangeCandidate(number,property.minimum,property.maximum)){std::ostringstream warningText;warningText<<"Enter a value from "<<*property.minimum<<" to "<<*property.maximum<<'.';MessageBoxA(dialog,warningText.str().c_str(),"Value outside native range",MB_OK|MB_ICONWARNING);return TRUE;}try{context->queued=context->submit?context->submit(property,number):queuePlaybackScalarEditCandidate({property.element,property.name,number});}catch(...){EndDialog(dialog,-1);return TRUE;}if(!context->queued){MessageBoxA(dialog,"The edit could not be queued. Try again.","Edit unavailable",MB_OK|MB_ICONERROR);return TRUE;}EndDialog(dialog,IDOK);return TRUE;}}
 return FALSE;
}
inline bool showFloatKeyDialogCandidate(HWND parent,const Graph&graph,std::optional<std::pair<uint32_t,size_t>> target={},uint8_t vectorComponent=0xff){
 if(vectorComponent!=0xff&&vectorComponent>=3)throw std::runtime_error("vector key dialog component outside xyz");
 ScalarPropertyDialogCandidate context;context.title="Edit animation key";context.submit=[vectorComponent](const auto&property,float value){return queuePlaybackFloatKeyEditCandidate({property.element,property.keyIndex,value,property.tick,vectorComponent});};
 for(uint32_t i=0;i<graph.elements.size();++i){if(target&&target->first!=i)continue;const auto&e=graph.elements[i];const auto&type=graph.strings.at(e.type);if(vectorComponent==0xff?type!="DmeFloatLogLayer":type!="DmeVector3LogLayer")continue;try{KeyTrack track(graph,e);if(track.values->type!=(vectorComponent==0xff?17:24))continue;for(size_t k=0;k<track.values->values.size();++k){if(target&&target->second!=k)continue;const auto tick=wireInt(track.times->values[k]);const float number=vectorComponent==0xff?wireFloat(track.values->values[k]):wireFloat(track.values->values[k],vectorComponent);std::ostringstream value;value<<std::setprecision(9)<<number;const std::string axis=vectorComponent==0xff?std::string{}:std::string(" / ")+"XYZ"[vectorComponent];context.properties.push_back({i,"values",graph.strings.at(e.name)+axis+" / key "+std::to_string(k)+" / tick "+std::to_string(tick)+" ("+std::to_string(i)+")",value.str(),k,tick});}}catch(const std::runtime_error&){continue;}}
 if(context.properties.empty()){MessageBoxA(parent,vectorComponent==0xff?"This session has no supported float animation keys.":"This session has no supported vector animation keys.",context.title,MB_OK);return false;}
 struct Template{DLGTEMPLATE dialog;WORD menu,windowClass,title;}layout{};layout.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;layout.dialog.cx=370;layout.dialog.cy=110;
 auto result=DialogBoxIndirectParamA(GetModuleHandleA(nullptr),&layout.dialog,parent,scalarPropertyDialogProcCandidate,reinterpret_cast<LPARAM>(&context));if(result==-1)throw std::runtime_error("animation key dialog creation failed");return context.queued;
}
inline std::vector<ScalarPropertyDialogCandidate::Property> collectScalarPropertiesCandidate(
    const Graph& graph, const std::vector<uint32_t>& selectedElements = {}) {
 std::vector<ScalarPropertyDialogCandidate::Property> properties;
 for(uint32_t i=0;i<graph.elements.size();++i){
  if(!selectedElements.empty()&&std::find(selectedElements.begin(),selectedElements.end(),i)==selectedElements.end())continue;
  const auto&e=graph.elements[i];
  const auto&elementType=graph.strings.at(e.type);
  for(const auto&a:e.attributes)if(a.type==3&&a.values.size()==1&&a.values[0].wire.size()==4){
   std::ostringstream value;value<<std::setprecision(9)<<wireFloat(a.values[0]);const auto&name=graph.strings.at(a.name);const auto range=nativeCameraLightScalarRangeCandidate(elementType,name);std::ostringstream label;label<<graph.strings.at(e.name)<<" / "<<name;if(range)label<<" ["<<range->minimum<<"-"<<range->maximum<<", default "<<range->defaultValue<<"]";label<<" ("<<i<<")";
   ScalarPropertyDialogCandidate::Property property{i,name,label.str(),value.str()};if(range){property.minimum=range->minimum;property.maximum=range->maximum;}properties.push_back(std::move(property));
  }
 }
 return properties;
}
inline bool showScalarPropertyDialogCandidate(HWND parent,const Graph&graph,const std::vector<uint32_t>& selectedElements={}){
 ScalarPropertyDialogCandidate context;context.properties=collectScalarPropertiesCandidate(graph,selectedElements);
 if(context.properties.empty()){MessageBoxA(parent,"This session has no scalar properties.","Edit scalar property",MB_OK);return false;}
 struct Template{DLGTEMPLATE dialog;WORD menu,windowClass,title;}layout{};layout.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;layout.dialog.cx=370;layout.dialog.cy=110;
 auto result=DialogBoxIndirectParamA(GetModuleHandleA(nullptr),&layout.dialog,parent,scalarPropertyDialogProcCandidate,reinterpret_cast<LPARAM>(&context));if(result==-1)throw std::runtime_error("scalar property dialog creation failed");return context.queued;
}
