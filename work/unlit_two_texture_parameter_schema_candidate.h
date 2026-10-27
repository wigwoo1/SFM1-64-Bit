#pragma once
#include <array>
#include <string_view>
#include <cstdint>
#include <vector>
// Functional registered names/types; no original pointers, help text or code.
inline const std::array<std::string_view,20>&unlitTwoTextureParameterNamesCandidate(){
    static constexpr std::array<std::string_view,20>names{
        "$flags",
        "$flags_defined",
        "$flags2",
        "$flags_defined2",
        "$color",
        "$alpha",
        "$basetexture",
        "$frame",
        "$basetexturetransform",
        "$flashlighttexture",
        "$flashlighttextureframe",
        "$color2",
        "$srgbtint",
        "$TEXTURE2",
        "$FRAME2",
        "$TEXTURE2TRANSFORM",
        "$CLOAKPASSENABLED",
        "$CLOAKFACTOR",
        "$CLOAKCOLORTINT",
        "$REFRACTAMOUNT",
    };return names;
}
inline std::vector<uint32_t>unlitTwoTextureParameterTypesCandidate(){return {1,1,1,1,2,7,0,1,10,0,1,2,2,0,1,10,8,7,2,7};}
