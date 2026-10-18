#pragma once
#include <string>
inline std::string flexRegistryFixtureName(unsigned i){if(i==0)return "Th";if(i==1)return "TH";if(i==2)return "th";if(i==3)return "";if(i==4)return std::string("\xc0",1);if(i==5)return std::string("\xe0",1);if(i==6)return "A B";if(i==7)return "a b";if(i==8)return "AB";if(i>=600&&i<1100)return "controller_"+std::to_string(i-600);return "controller_"+std::to_string(i);}
