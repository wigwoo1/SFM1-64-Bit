#pragma once
#include <charconv>
#include <cstddef>
#include <stdexcept>
#include <string>

// The installed loader reserves record0 for the world and uses the positive
// decimal number in '*n' directly, without decrementing. The BSP reader checks
// n against the actual record count. Keep malformed names strict rather than
// reproducing the original parser's permissive/fatal-error behavior.
inline size_t sourceBspInlineModelIndexCandidate(const std::string& name){
    if(name.size()<2||name[0]!='*')throw std::runtime_error("invalid BSP inline model name: "+name);
    size_t index=0;
    const auto parsed=std::from_chars(name.data()+1,name.data()+name.size(),index);
    if(parsed.ec!=std::errc{}||parsed.ptr!=name.data()+name.size()||!index)
        throw std::runtime_error("invalid BSP inline model reference: "+name);
    return index;
}
