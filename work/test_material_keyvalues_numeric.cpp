#include "source_material_keyvalues_files_candidate.h"
#include <iostream>
#include <map>
int main(){try{
    static_assert(sizeof(void*)==8,"Owned x64 material numeric reader");
    unsigned checks=0;
    auto check=[&](bool b){if(!b)throw std::runtime_error("numeric check "+std::to_string(checks));++checks;};
    auto parse=[](const std::string&s,bool range=false){return sourceMaterialKeyValuesParseCandidate("S{v \""+s+"\" max 2147483647}",range);};
    for(const char*s:{"1d3","1D+3","+1d003","1000.0"}){
        const auto d=parse(s);const auto&n=d.roots[0].children;
        check(n[0].kind==3&&n[0].bits==0x447a0000&&n[1].kind==2&&!d.numericRangeError);
    }
    for(const char*s:{"1d","1D+","1d-","1e","1e+","nan","inf","0x1p2"}){
        const auto d=parse(s);check(d.roots[0].children[0].kind==1&&d.roots[0].children[0].text==s&&!d.numericRangeError);
    }
    for(const char*s:{"1d9999","-1D309","1d-9999","-1d-9999"}){
        const auto d=parse(s);check(d.roots[0].children[0].kind==3&&d.roots[0].children[1].kind==1&&d.numericRangeError);
    }
    for(const char*s:{"1d-310","-1d-310","0d-9999","-0d9999"}){
        const auto d=parse(s);check(d.roots[0].children[0].kind==3&&d.roots[0].children[1].kind==2&&!d.numericRangeError);
    }
    check(parse("1d3",true).numericRangeError&&parse("1d3",true).roots[0].children[1].kind==1);
    const auto edge=parse("-1d-5200");check(edge.roots[0].children[0].kind==3&&edge.roots[0].children[0].bits==0x80000000&&!edge.numericRangeError&&edge.roots[0].children[1].kind==2);
    const int before=errno;errno=EDOM;const auto parsed=parse("1d-9999");check(errno==EDOM&&parsed.numericRangeError);errno=before;
    const std::map<std::string,std::string>files={{"root.vmt","S{v 1d-9999} #include child.vmt"},{"child.vmt","S{v 2147483647}"}};
    const auto result=sourceMaterialKeyValuesFilesCandidate("root.vmt",[&](const std::string&p)->std::optional<std::string>{const auto i=files.find(p);return i==files.end()?std::nullopt:std::optional<std::string>(i->second);});
    check(result.document.numericRangeError&&result.document.roots.size()==2&&result.document.roots[1].children[0].kind==1);
    std::cout<<"PASS numeric safety checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
