#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include <iomanip>
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"owned x64 reader required");if(argc!=2)throw std::runtime_error("supply a session for read-only saved-value inspection");const auto g=loadGraph(argv[1]);size_t cameras=0,entries=0;
 for(size_t n=0;n<g.elements.size();++n){const auto&e=g.elements[n];if(g.strings.at(e.type)=="DmeCamera")++cameras;
  for(const auto&a:e.attributes)if(g.strings.at(a.name)=="toneMapScale"){
   if(a.type!=3||a.values.size()!=1||a.values[0].wire.size()!=4)throw std::runtime_error("toneMapScale saved scalar schema");
   std::cout<<"element="<<n<<" type="<<std::quoted(g.strings.at(e.type))<<" name="<<std::quoted(g.strings.at(e.name))<<" saved_scale="<<std::setprecision(9)<<wireFloat(a.values[0])<<" bits=0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<wireU32(a.values[0])<<std::dec<<std::setfill(' ')<<'\n';++entries;
  }
 }
 std::cout<<"PASS saved_tone_map_inspection=1 cameras="<<cameras<<" attributes="<<entries<<" evaluated_animation=NO original_runtime=ABSENT input_read_only=1\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
