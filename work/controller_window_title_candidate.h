#pragma once
// QString values/translation/encoding and lifetime remain supplied by native Qt.
template<class Adapter>
void updateControllerWindowTitleCandidate(Adapter& a){
    if(!a.window())return;
    auto* child=a.globalChild();
    if(!child){
        a.setWindowModified(a.window(),0);
        auto title=a.translate("Source Filmmaker [Beta]");
        a.setWindowTitle(a.window(),title);return;
    }
    a.setWindowModified(a.window(),a.childModifiedByte(child));
    auto name=a.fromAscii(a.childTitleSource(child));
    auto app=a.translate("Source Filmmaker [Beta]");
    auto format=a.translate("%1[*] - %2");
    auto first=a.arg(format,name,0,' ');
    auto title=a.arg(first,app,0,' ');
    a.setWindowTitle(a.window(),title);
}
