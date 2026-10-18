#include "operator_root_selection.h"
#include <iostream>
int main(){try{
 std::vector<OperatorRootAttribute> attrs{{0x4080,false,{2,0,2}},{0xffff,true,{3}},{0x20,false,{3}},{0x4001,false,{1,3}}};
 auto roots=selectOperatorRoots({true,true,false,false},attrs);
 if(roots!=std::vector<size_t>{0,1,2,3}||attrs[0].flags!=0x80||attrs[1].flags!=0xbfff||attrs[2].flags!=0x20||attrs[3].flags!=1)throw std::runtime_error("root selection mismatch");
 if(!selectOperatorRoots({false,false,false,false},attrs).empty())throw std::runtime_error("dirty consumed twice");
 for(unsigned raw=0;raw<65536;++raw){std::vector<OperatorRootAttribute> a{{uint16_t(raw),false,{0}}};auto r=selectOperatorRoots({false},a);if(r.size()!=unsigned((raw&0x4000)!=0)||a[0].flags!=(raw&0xbfff))throw std::runtime_error("dirty flag preservation mismatch");}
 std::cout<<"root_selection_checks=2 dirty_flag_checks=65536\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
