#pragma once
#include "material_string_value_candidate.h"
#include "material_decimal_scaling_candidate.h"
#include "material_decimal_double_candidate.h"
// Isolated constructor parser. Full acceptance is required before integration.
inline double materialConstructorDoubleCandidate(const std::string&s,std::array<uint8_t,12>*intermediate=nullptr,int*parserStatus=nullptr){
    if(intermediate)intermediate->fill(0);
    if(parserStatus)*parserStatus=0;
    size_t i=0;while(i<s.size()&&materialStringSpaceCandidate(static_cast<unsigned char>(s[i])))++i;
    bool negative=false;if(i<s.size()&&(s[i]=='+'||s[i]=='-'))negative=s[i++]=='-';
    std::array<unsigned,25> digits{};unsigned count=0;int power=0;bool seen=false;
    auto decimal=[](char c){return c>='0'&&c<='9';};
    while(i<s.size()&&decimal(s[i])){
        seen=true;unsigned d=unsigned(s[i++]-'0');if(!count&&!d)continue;
        if(count<25)digits[count++]=d;else ++power;
    }
    if(i<s.size()&&s[i]=='.'){
        ++i;while(i<s.size()&&decimal(s[i])){
            seen=true;unsigned d=unsigned(s[i++]-'0');if(!count&&!d){--power;continue;}
            if(count<25){digits[count++]=d;--power;}
        }
    }
    if(!seen){if(parserStatus)*parserStatus=4;return 0;}
    if(i<s.size()&&(s[i]=='e'||s[i]=='E'||s[i]=='d'||s[i]=='D')){
        ++i;bool minus=false;if(i<s.size()&&(s[i]=='+'||s[i]=='-'))minus=s[i++]=='-';
        int exponent=0;while(i<s.size()&&decimal(s[i])){if(exponent<=5200)exponent=exponent*10+(s[i]-'0');if(exponent>5200)exponent=5201;++i;}
        power+=minus?-exponent:exponent;
    }
    std::array<uint8_t,12> wide{};
    if(count){
        // Original tests/increments last retained digit23, not discarded digit24.
        // Numeric digit10 is intentional here and is handled by integer accumulation.
        if(count==25){if(digits[23]>=5)++digits[23];count=24;++power;}
        while(count&&digits[count-1]==0){--count;++power;}
        // Text-stage status is distinct from the final double conversion.
        // Scaling within this inclusive interval can itself produce exact
        // wide zero without reporting text-stage underflow.
        if(parserStatus)*parserStatus=power>5200?2:power< -5200?1:0;
        std::array<uint32_t,3> words{};
        for(unsigned j=0;j<count;++j){uint64_t carry=digits[j];for(auto&w:words){carry+=uint64_t(w)*10;w=uint32_t(carry);carry>>=32;}}
        uint16_t exponent=0x404e;
        while(!(words[2]&0x8000)){words[2]=(words[2]<<1)|(words[1]>>31);words[1]=(words[1]<<1)|(words[0]>>31);words[0]<<=1;--exponent;}
        for(unsigned j=0;j<10;++j)wide[j]=uint8_t(words[j/4]>>((j%4)*8));wide[10]=uint8_t(exponent);wide[11]=uint8_t(exponent>>8);
        materialDecimalScaleRawCandidate(wide,power,wide);
    }
    if(negative)wide[11]|=0x80;
    if(intermediate)*intermediate=wide;
    const auto value=materialDecimalDoubleCandidate(wide);double result=0;std::memcpy(&result,&value.bits,8);return result;
}
