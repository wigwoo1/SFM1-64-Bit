#include "channel_source_type.h"
#include <iostream>
#include <stdexcept>
int main(){try{for(unsigned raw=0;raw<65536;++raw){unsigned low=raw&31,expected=low;if(low>=15&&low<=28)expected=low-14;if(channelSourceScalarType(uint16_t(raw))!=expected)throw std::runtime_error("source type normalization mismatch");}if(channelScalarType(29)!=29||channelScalarType(0xffffffff)!=0xffffffff)throw std::runtime_error("log type out-of-range altered");std::cout<<"source_type_checks=65536 raw_log_type_checks=2\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
