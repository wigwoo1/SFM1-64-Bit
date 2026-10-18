from pathlib import Path
p=Path('work/test_graph_float_curve_sampler.cpp');s=p.read_text();needle='saveGraph(r.graph,argv[2]);';insert='''auto verifyModes=[&](Graph&g,const ChannelBinding&bound){
 const std::array<double,16>expected{2,2,1+2*std::sqrt(.5),1+2*(1-std::sqrt(.5)),2,99.0/48,2,2,1.91145,2.08855,2,1.8125,2,1.875,1+2*(1-std::sqrt(double(.001f))),1};
 auto*curves=runtimeAttribute(g,start+1,"curvetypes");for(uint32_t code=0;code<16;++code){for(auto&value:curves->values)setWireU32(value,(code<<8)|code);if(!executeSessionFloatPlay(g,bound,20000))throw std::runtime_error("mode channel rejected");float actual=wireFloat(runtimeAttribute(g,bound.destination.element,bound.destination.attribute)->values.at(bound.destination.index));if(std::abs(actual-expected[code])>3e-6)throw std::runtime_error("mode channel value mismatch code="+std::to_string(code)+" actual="+std::to_string(actual));}
 for(auto&value:curves->values)setWireU32(value,0);
 };verifyModes(r.graph,binding);'''
s=s.replace(needle,insert+needle).replace('std::cout<<"graph_curve_binding', 'verifyModes(reload,replay);std::cout<<"all_curve_codes=16 channel_mode_checks=32 graph_curve_binding');p.write_text(s)
